"""Cell-by-Cell executor with retry logic."""

from __future__ import annotations

import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional

from mesh_agent.agents.programmer import Programmer
from mesh_agent.agents.reviewer import Reviewer
from mesh_agent.solver_interface.base import SolverInterface, UserTemplateSolver


@dataclass
class CellResult:
    cell_name: str
    success: bool
    output: dict[str, Any]
    retries: int = 0
    error: str = ""


class CellExecutor:
    """Cell-by-Cell pipeline: mesh generation → solver → post-processing.

    Each cell independently retries up to N times (default 4).
    If solver=None, generates solver via Programmer→Reviewer before Cell 2.
    """

    def __init__(
        self,
        programmer: Programmer,
        reviewer: Reviewer,
        solver: SolverInterface | None,
        max_retries: int = 6,
    ):
        self.programmer = programmer
        self.reviewer = reviewer
        self.solver = solver
        self.max_retries = max_retries
        self._mesh_before = ""
        self._metrics_before: Optional[dict[str, float]] = None

    # ── Main pipeline ──────────────────────────────────────────

    async def execute(
        self,
        strategy: dict[str, Any],
        work_dir: Path,
        current_mesh_path: str,
        solver_path: str,
        problem_spec: dict[str, Any],
        previous_metrics: Optional[dict[str, float]] = None,
        mesh_before_path: str = "",
    ) -> dict[str, CellResult]:
        results: dict[str, CellResult] = {}
        self._mesh_before = mesh_before_path or current_mesh_path
        self._metrics_before = previous_metrics

        # Cell 1: mesh generation
        mesh_result = await self._cell_mesh(
            strategy, work_dir, current_mesh_path, solver_path,
            problem_spec, previous_metrics,
        )
        results["mesh_generation"] = mesh_result
        if not mesh_result.success:
            return results

        new_mesh_path = self._find_mesh(work_dir, current_mesh_path)

        # Cell 1.5: generate solver if none provided
        actual_solver = self.solver
        if actual_solver is None:
            gen_result = await self._cell_solver_gen(
                strategy, work_dir, str(new_mesh_path), problem_spec,
            )
            results["solver_generation"] = gen_result
            if not gen_result.success:
                return results
            actual_solver = UserTemplateSolver(work_dir / "solver.py")

        # Cell 2: run solver
        solver_result = await self._cell_solver_run(
            actual_solver, work_dir, new_mesh_path, problem_spec,
        )
        results["solver"] = solver_result
        if not solver_result.success:
            return results

        # Cell 3: post-processing
        post_result = await self._cell_post(work_dir)
        results["post_processing"] = post_result

        # Cell 4: visualization
        viz_result = await self._cell_viz(work_dir, current_mesh_path, str(new_mesh_path))
        results["visualization"] = viz_result

        return results

    # ── Cell 1: Mesh Generation ────────────────────────────────

    async def _cell_mesh(
        self,
        strategy: dict[str, Any],
        work_dir: Path,
        current_mesh_path: str,
        solver_path: str,
        problem_spec: dict[str, Any],
        previous_metrics: Optional[dict[str, float]],
    ) -> CellResult:
        output_dir = str(work_dir / "output")
        last_error: Optional[str] = None

        for attempt in range(1, self.max_retries + 1):
            implementation = await self.programmer.implement(
                strategy, current_mesh_path, solver_path, output_dir,
                problem_spec, previous_metrics, error_context=last_error,
            )
            Programmer.write_scripts(implementation, work_dir)

            review = await self.reviewer.review(
                strategy,
                implementation.get("mesh_script", ""),
                implementation.get("post_script", ""),
                implementation.get("solver_params", {}),
                implementation.get("changes_summary", ""),
            )
            if review.get("verdict") == "fail":
                critical = [i for i in review.get("issues", []) if i["severity"] == "critical"]
                if critical:
                    last_error = f"Review failed: {json.dumps(critical)}"
                    continue
                # Non-critical issues: log but proceed

            mesh_script = work_dir / "mesh.py"
            if mesh_script.exists():
                try:
                    proc = subprocess.run(
                        [sys.executable, str(mesh_script),
                         "--input-mesh", current_mesh_path,
                         "--output-dir", output_dir],
                        cwd=str(work_dir), capture_output=True, text=True, timeout=300,
                    )
                    if proc.returncode != 0:
                        last_error = f"Mesh script error (attempt {attempt}):\n{proc.stderr[-1000:]}"
                        continue
                except subprocess.TimeoutExpired:
                    last_error = f"Mesh script timed out (attempt {attempt})"
                    continue

            return CellResult(cell_name="mesh_generation", success=True, output=implementation, retries=attempt - 1)

        return CellResult(cell_name="mesh_generation", success=False, output={}, retries=self.max_retries, error=last_error or "Max retries")

    # ── Cell 1.5: Solver Generation ────────────────────────────

    async def _cell_solver_gen(
        self,
        strategy: dict[str, Any],
        work_dir: Path,
        mesh_path: str,
        problem_spec: dict[str, Any],
    ) -> CellResult:
        output_dir = str(work_dir / "output")
        last_error: Optional[str] = None

        for attempt in range(1, self.max_retries + 1):
            impl = await self.programmer.generate_solver(
                problem_spec, mesh_path, output_dir, error_context=last_error,
            )
            solver_code = impl.get("solver_script", "")
            if not solver_code:
                last_error = "Programmer produced empty solver"
                continue

            (work_dir / "solver.py").write_text(solver_code, encoding="utf-8")

            review = await self.reviewer.review_solver(problem_spec, solver_code)
            if review.get("verdict") == "fail":
                critical = [i for i in review.get("issues", []) if i["severity"] == "critical"]
                last_error = f"Reviewer rejected: {json.dumps(critical)}"
                continue

            # Syntax check
            try:
                proc = subprocess.run(
                    [sys.executable, "-c",
                     f"compile(open(r'{work_dir / 'solver.py'}').read(), 'solver.py', 'exec')"],
                    capture_output=True, text=True, timeout=10,
                )
                if proc.returncode != 0:
                    last_error = f"Syntax error: {proc.stderr[-500:]}"
                    continue
            except Exception as e:
                last_error = f"Smoke test failed: {e}"
                continue

            return CellResult(
                cell_name="solver_generation", success=True,
                output={"solver_path": str(work_dir / "solver.py"),
                        "expected_metrics": impl.get("expected_metrics", [])},
                retries=attempt - 1,
            )

        return CellResult(cell_name="solver_generation", success=False, output={}, retries=self.max_retries, error=last_error or "Generation failed")

    # ── Cell 2: Solver Run ─────────────────────────────────────

    async def _cell_solver_run(
        self,
        solver: SolverInterface,
        work_dir: Path,
        mesh_path: Path,
        problem_spec: dict[str, Any],
    ) -> CellResult:
        output_dir = work_dir / "output"
        params = dict(problem_spec.get("solver", {}).get("params", {}))
        last_error: Optional[str] = None

        for attempt in range(1, self.max_retries + 1):
            try:
                metrics = await solver.run(
                    mesh_path=str(mesh_path), output_dir=str(output_dir),
                    params=params, work_dir=str(work_dir),
                )
                if metrics.get("_success"):
                    return CellResult(cell_name="solver", success=True, output=metrics, retries=attempt - 1)
                last_error = metrics.get("_error", "Solver returned failure")
            except Exception as e:
                last_error = str(e)
            params = self._adjust_params(params, attempt, last_error)

        return CellResult(cell_name="solver", success=False, output={}, retries=self.max_retries, error=last_error or "Solver failed")

    # ── Cell 3: Post-Processing ─────────────────────────────────

    async def _cell_post(self, work_dir: Path) -> CellResult:
        output_dir = work_dir / "output"
        post_script = work_dir / "post_processing.py"

        if not post_script.exists():
            mf = output_dir / "metrics.json"
            if mf.exists():
                return CellResult(cell_name="post_processing", success=True, output={"metrics_file": str(mf)})
            return CellResult(cell_name="post_processing", success=True, output={})

        for attempt in range(1, self.max_retries + 1):
            try:
                proc = subprocess.run(
                    [sys.executable, str(post_script), "--output-dir", str(output_dir)],
                    cwd=str(work_dir), capture_output=True, text=True, timeout=300,
                )
                if proc.returncode != 0:
                    continue
                mf = output_dir / "metrics.json"
                if mf.exists():
                    metrics = json.loads(mf.read_text(encoding="utf-8"))
                    return CellResult(cell_name="post_processing", success=True, output=metrics, retries=attempt - 1)
                return CellResult(cell_name="post_processing", success=True, output={}, retries=attempt - 1)
            except subprocess.TimeoutExpired:
                continue

        return CellResult(cell_name="post_processing", success=False, output={}, retries=self.max_retries, error="Failed")

    # ── Cell 4: Visualization ────────────────────────────────────

    async def _cell_viz(
        self,
        work_dir: Path,
        mesh_before: str,
        mesh_after: str,
    ) -> CellResult:
        """Run visualization script to generate comparison plots."""
        output_dir = work_dir / "output"
        viz_script = work_dir / "viz.py"
        if not viz_script.exists():
            return CellResult(cell_name="visualization", success=True, output={"images": []})

        metrics_before = self._metrics_before or {}

        for attempt in range(1, 3):
            try:
                proc = subprocess.run(
                    [sys.executable, str(viz_script),
                     "--output-dir", str(output_dir),
                     "--mesh-before", mesh_before,
                     "--mesh-after", mesh_after,
                     "--metrics-before", json.dumps(metrics_before),
                     ],
                    cwd=str(work_dir), capture_output=True, text=True, timeout=120,
                )
                images = list(output_dir.glob("*.png"))
                return CellResult(
                    cell_name="visualization", success=True,
                    output={"images": [str(p) for p in images]},
                    retries=attempt - 1,
                )
            except subprocess.TimeoutExpired:
                continue

        return CellResult(cell_name="visualization", success=True, output={"images": []})

    # ── Helpers ─────────────────────────────────────────────────

    @staticmethod
    def _find_mesh(work_dir: Path, fallback: str) -> Path:
        candidates = list(work_dir.glob("**/*.msh"))
        if candidates:
            return candidates[0]
        return Path(fallback)

    @staticmethod
    def _adjust_params(params: dict[str, Any], attempt: int, _error: str) -> dict[str, Any]:
        p = dict(params)
        if "dt" in p:
            p["dt"] = p["dt"] * (0.5 ** attempt)
        if "max_iterations" in p:
            p["max_iterations"] = min(p["max_iterations"] * 2, 100000)
        if "relaxation" in p:
            p["relaxation"] = max(0.1, p["relaxation"] - 0.1 * attempt)
        return p
