"""Cell-by-Cell executor with retry logic."""

from __future__ import annotations

import asyncio
import json
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional

from mesh_agent.agents.programmer import Programmer
from mesh_agent.agents.reviewer import Reviewer
from mesh_agent.solver_interface.base import SolverInterface


@dataclass
class CellResult:
    cell_name: str
    success: bool
    output: dict[str, Any]
    retries: int = 0
    error: str = ""


class CellExecutor:
    """Executes the Cell-by-Cell pipeline: mesh generation → solver → post-processing.

    Each cell runs independently with up to N retries (default 4).
    Failures trigger Programmer → Reviewer → retry loop within each cell.
    """

    def __init__(
        self,
        programmer: Programmer,
        reviewer: Reviewer,
        solver: SolverInterface,
        max_retries: int = 4,
    ):
        self.programmer = programmer
        self.reviewer = reviewer
        self.solver = solver
        self.max_retries = max_retries

    async def execute(
        self,
        strategy: dict[str, Any],
        work_dir: Path,
        current_mesh_path: str,
        solver_path: str,
        problem_spec: dict[str, Any],
        previous_metrics: Optional[dict[str, float]] = None,
    ) -> dict[str, CellResult]:
        """Run all cells in sequence. Returns results keyed by cell name."""
        results: dict[str, CellResult] = {}

        # Cell 1: Generate mesh code + review + run
        mesh_result = await self._run_cell(
            "mesh_generation",
            strategy,
            work_dir,
            current_mesh_path,
            solver_path,
            problem_spec,
            previous_metrics,
        )
        results["mesh_generation"] = mesh_result
        if not mesh_result.success:
            return results

        # Get the new mesh path
        new_mesh_path = work_dir / "output" / "adapted_mesh.msh"
        if not new_mesh_path.exists():
            # Try to find any generated mesh file
            candidates = list(work_dir.glob("*.msh")) + list((work_dir / "output").glob("*.msh")) if (work_dir / "output").exists() else []
            if candidates:
                new_mesh_path = candidates[0]
            else:
                new_mesh_path = Path(current_mesh_path)

        # Cell 2: Run solver
        solver_result = await self._run_solver(
            strategy,
            work_dir,
            new_mesh_path,
            problem_spec,
        )
        results["solver"] = solver_result
        if not solver_result.success:
            return results

        # Cell 3: Post-processing
        post_result = await self._run_post(
            strategy,
            work_dir,
            problem_spec,
        )
        results["post_processing"] = post_result

        return results

    async def _run_cell(
        self,
        cell_name: str,
        strategy: dict[str, Any],
        work_dir: Path,
        current_mesh_path: str,
        solver_path: str,
        problem_spec: dict[str, Any],
        previous_metrics: Optional[dict[str, float]],
    ) -> CellResult:
        """Cell 1: Generate code, review, run mesh script. Retry on failure."""
        output_dir = str(work_dir / "output")
        last_error: Optional[str] = None

        for attempt in range(1, self.max_retries + 1):
            # Generate
            implementation = await self.programmer.implement(
                strategy, current_mesh_path, solver_path, output_dir,
                problem_spec, previous_metrics, error_context=last_error,
            )
            Programmer.write_scripts(implementation, work_dir)

            # Review
            review = await self.reviewer.review(
                strategy,
                implementation.get("mesh_script", ""),
                implementation.get("post_script", ""),
                implementation.get("solver_params", {}),
                implementation.get("changes_summary", ""),
            )

            if review.get("verdict") == "fail":
                critical_issues = [i for i in review.get("issues", []) if i["severity"] == "critical"]
                last_error = f"Review failed: {json.dumps(critical_issues)}"
                continue

            # Run mesh script
            mesh_script = work_dir / "mesh_generation.py"
            if mesh_script.exists():
                try:
                    proc = subprocess.run(
                        [sys.executable, str(mesh_script), "--output-dir", output_dir],
                        cwd=str(work_dir),
                        capture_output=True,
                        text=True,
                        timeout=300,
                    )
                    if proc.returncode != 0:
                        last_error = f"Mesh script error (attempt {attempt}):\nstdout: {proc.stdout[-1000:]}\nstderr: {proc.stderr[-1000:]}"
                        continue
                except subprocess.TimeoutExpired:
                    last_error = f"Mesh script timed out (attempt {attempt})"
                    continue

            return CellResult(cell_name=cell_name, success=True, output=implementation, retries=attempt - 1)

        return CellResult(cell_name=cell_name, success=False, output={}, retries=self.max_retries, error=last_error or "Max retries exceeded")

    async def _run_solver(
        self,
        strategy: dict[str, Any],
        work_dir: Path,
        mesh_path: Path,
        problem_spec: dict[str, Any],
    ) -> CellResult:
        """Cell 2: Run solver with retry on failure."""
        output_dir = work_dir / "output"
        solver_params = problem_spec.get("solver", {}).get("params", {})
        last_error: Optional[str] = None

        for attempt in range(1, self.max_retries + 1):
            try:
                metrics = await self.solver.run(
                    mesh_path=str(mesh_path),
                    output_dir=str(output_dir),
                    params=solver_params,
                    work_dir=str(work_dir),
                )

                if metrics.get("_success"):
                    return CellResult(
                        cell_name="solver",
                        success=True,
                        output=metrics,
                        retries=attempt - 1,
                    )

                last_error = metrics.get("_error", "Unknown solver error")
                solver_params = self._adjust_solver_params(solver_params, attempt, last_error)

            except Exception as e:
                last_error = str(e)
                solver_params = self._adjust_solver_params(solver_params, attempt, last_error)

        return CellResult(cell_name="solver", success=False, output={}, retries=self.max_retries, error=last_error or "Solver failed after max retries")

    async def _run_post(
        self,
        strategy: dict[str, Any],
        work_dir: Path,
        problem_spec: dict[str, Any],
    ) -> CellResult:
        """Cell 3: Run post-processing script."""
        output_dir = work_dir / "output"
        post_script = work_dir / "post_processing.py"

        if not post_script.exists():
            # No post script needed — check if solver already produced metrics
            metrics_file = output_dir / "metrics.json"
            if metrics_file.exists():
                return CellResult(cell_name="post_processing", success=True, output={"metrics_file": str(metrics_file)})
            return CellResult(cell_name="post_processing", success=True, output={}, error="No post-processing script, no metrics found")

        for attempt in range(1, self.max_retries + 1):
            try:
                proc = subprocess.run(
                    [sys.executable, str(post_script), "--output-dir", str(output_dir)],
                    cwd=str(work_dir),
                    capture_output=True,
                    text=True,
                    timeout=300,
                )
                if proc.returncode != 0:
                    last_error = f"Post-processing error (attempt {attempt}):\n{proc.stderr[-1000:]}"
                    continue

                metrics_file = output_dir / "metrics.json"
                if metrics_file.exists():
                    metrics = json.loads(metrics_file.read_text(encoding="utf-8"))
                    return CellResult(cell_name="post_processing", success=True, output=metrics, retries=attempt - 1)

                return CellResult(cell_name="post_processing", success=True, output={}, retries=attempt - 1)

            except subprocess.TimeoutExpired:
                continue

        return CellResult(cell_name="post_processing", success=False, output={}, retries=self.max_retries, error="Post-processing failed")

    @staticmethod
    def _adjust_solver_params(params: dict[str, Any], attempt: int, error: str) -> dict[str, Any]:
        """Adjust solver parameters on failure."""
        adjusted = dict(params)
        if "dt" in adjusted:
            adjusted["dt"] = adjusted["dt"] * (0.5 ** attempt)
        if "max_iterations" in adjusted:
            adjusted["max_iterations"] = min(adjusted["max_iterations"] * 2, 100000)
        if "relaxation" in adjusted:
            adjusted["relaxation"] = max(0.1, adjusted["relaxation"] - 0.1 * attempt)
        return adjusted
