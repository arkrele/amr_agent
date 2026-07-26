"""Parallel Executor: multi-worktree dispatch + result merge (Phase 3).

Handles strategy-level parallelism (Phase 3A):
    Gatekeeper → Top N strategies → N parallel worktrees → merge results.

Also handles parameter-level parallelism (Phase 3B):
    When a strategy shows high promise, explore parameter variations
    (different growth rates, target sizes) in parallel.
"""

from __future__ import annotations

import asyncio
import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from mesh_agent.agents.programmer import Programmer
from mesh_agent.agents.reviewer import Reviewer
from mesh_agent.agents.gatekeeper import Gatekeeper
from mesh_agent.executor.cell_executor import CellExecutor, CellResult
from mesh_agent.executor.worktree_manager import WorktreeManager
from mesh_agent.solver_interface.base import SolverInterface, UserTemplateSolver


@dataclass
class ParallelResult:
    strategy_id: str
    worktree_name: str
    cell_results: dict[str, CellResult]
    metrics: dict[str, float]
    success: bool
    error: str = ""


@dataclass
class ParameterVariant:
    label: str
    params: dict[str, Any]


class ParallelExecutor:
    """Dispatches strategies to parallel worktrees and merges results.

    Two modes:
    - Strategy-level (A): Gatekeeper outputs N strategies → N parallel worktrees
    - Parameter-level (B): One promising strategy → explore parameter grid
    """

    def __init__(
        self,
        programmer: Programmer,
        reviewer: Reviewer,
        solver: SolverInterface | None,
        worktree: WorktreeManager,
        gatekeeper: Gatekeeper | None = None,
        max_parallel: int = 3,
    ):
        self.programmer = programmer
        self.reviewer = reviewer
        self.solver = solver
        self.worktree = worktree
        self.gatekeeper = gatekeeper
        self.max_parallel = max_parallel

    # ── Strategy-level parallel (A) ────────────────────────────

    async def execute_parallel(
        self,
        strategies: list[dict[str, Any]],
        work_dir_base: Path,
        current_mesh_path: str,
        solver_path: str,
        problem_spec: dict[str, Any],
        previous_metrics: dict[str, float] | None = None,
        mesh_before_path: str = "",
    ) -> list[ParallelResult]:
        """Execute multiple strategies in parallel worktrees."""
        limited = strategies[:self.max_parallel]
        tasks = []
        for i, strategy in enumerate(limited):
            tasks.append(self._run_one_worktree(
                strategy=strategy,
                work_dir_base=work_dir_base,
                index=i,
                current_mesh_path=current_mesh_path,
                solver_path=solver_path,
                problem_spec=problem_spec,
                previous_metrics=previous_metrics,
                mesh_before_path=mesh_before_path,
            ))

        results = await asyncio.gather(*tasks, return_exceptions=True)
        parsed: list[ParallelResult] = []
        for r in results:
            if isinstance(r, Exception):
                parsed.append(ParallelResult(
                    strategy_id="error", worktree_name="error",
                    cell_results={}, metrics={}, success=False, error=str(r),
                ))
            else:
                parsed.append(r)
        return parsed

    async def _run_one_worktree(
        self,
        strategy: dict[str, Any],
        work_dir_base: Path,
        index: int,
        current_mesh_path: str,
        solver_path: str,
        problem_spec: dict[str, Any],
        previous_metrics: dict[str, float] | None,
        mesh_before_path: str,
    ) -> ParallelResult:
        sid = strategy.get("strategy_id", f"unknown_{index}")
        wt_name = f"parallel_{index}_{sid[:20]}"
        wt_path = self.worktree.create(wt_name)

        # Copy inputs
        import shutil
        mesh_src = Path(current_mesh_path)
        if mesh_src.exists():
            dst = wt_path / "input_mesh"
            if mesh_src.is_dir():
                if dst.exists(): shutil.rmtree(str(dst))
                shutil.copytree(str(mesh_src), str(dst))
            else:
                shutil.copy2(str(mesh_src), str(wt_path / "mesh"))

        if solver_path and Path(solver_path).exists():
            shutil.copy2(solver_path, str(wt_path / "solver.py"))

        actual_solver = self.solver
        if actual_solver is None and (wt_path / "solver.py").exists():
            actual_solver = UserTemplateSolver(wt_path / "solver.py")

        executor = CellExecutor(self.programmer, self.reviewer, actual_solver)

        actual_mesh = str(wt_path / "input_mesh") if (wt_path / "input_mesh").exists() else current_mesh_path
        cell_results = await executor.execute(
            strategy=strategy,
            work_dir=wt_path,
            current_mesh_path=actual_mesh,
            solver_path=str(wt_path / "solver.py") if (wt_path / "solver.py").exists() else "",
            problem_spec=problem_spec,
            previous_metrics=previous_metrics,
            mesh_before_path=mesh_before_path,
        )

        all_ok = all(r.success for r in cell_results.values())
        metrics = {}
        solver_result = cell_results.get("solver")
        if solver_result and solver_result.success:
            solver_output = solver_result.output
            for k, v in solver_output.items():
                if isinstance(v, (int, float)) and not k.startswith("_"):
                    metrics[k] = float(v)

        self.worktree.commit(
            wt_path,
            f"[{'PASS' if all_ok else 'FAIL'}] {sid}",
        )

        return ParallelResult(
            strategy_id=sid,
            worktree_name=wt_name,
            cell_results=cell_results,
            metrics=metrics,
            success=all_ok,
            error="" if all_ok else "; ".join(
                f"{k}: {v.error[:100]}" for k, v in cell_results.items() if not v.success
            ),
        )

    # ── Parameter-level parallel (B) ───────────────────────────

    async def should_explore_params(
        self,
        strategy: dict[str, Any],
        composite_score: float,
        budget_remaining: dict[str, Any],
    ) -> bool:
        """LLM-based decision: should we explore parameter variations?"""
        if not self.gatekeeper:
            return False

        solver_runs_left = budget_remaining.get("solver_runs", 0)
        if solver_runs_left < 2:
            return False

        user_message = f"""## Strategy with High Promise
{json.dumps(strategy, indent=2)}
Composite score from debate: {composite_score}/5

## Budget
Solver runs remaining: {solver_runs_left}

This strategy scored very well in debate. Should we explore parameter
variations (different growth rates, target sizes, refinement region widths)
to find the optimal configuration? This would use {min(3, solver_runs_left)} solver runs.

Respond with JSON: {{"explore": true/false, "variants": [{{"label": "...", "params": {{}}}}], "reason": "..."}}"""

        schema = {
            "type": "object",
            "properties": {
                "explore": {"type": "boolean"},
                "variants": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "label": {"type": "string"},
                            "params": {"type": "object"},
                        },
                    },
                },
                "reason": {"type": "string"},
            },
            "required": ["explore", "variants", "reason"],
        }

        result = await self.gatekeeper.run_structured(user_message, schema)
        return result.get("explore", False)

    # ── Result merge ───────────────────────────────────────────

    def select_best(self, results: list[ParallelResult]) -> ParallelResult | None:
        """Select best result from parallel execution."""
        successful = [r for r in results if r.success]
        if not successful:
            # Pick the one with most cell results passing
            best = max(results, key=lambda r: sum(1 for v in r.cell_results.values() if v.success))
            return best if best else None

        # Pick best by metrics improvement
        best = successful[0]
        best_improvement = sum(best.metrics.values())
        for r in successful[1:]:
            improvement = sum(r.metrics.values())
            if improvement < best_improvement:  # Lower error = better
                best = r
                best_improvement = improvement
        return best

    def merge_metrics(self, results: list[ParallelResult]) -> dict[str, Any]:
        """Produce a summary of all parallel results."""
        return {
            "total_strategies": len(results),
            "successful": sum(1 for r in results if r.success),
            "failed": sum(1 for r in results if not r.success),
            "best_strategy": self.select_best(results).strategy_id if self.select_best(results) else None,
            "per_strategy": [
                {
                    "strategy_id": r.strategy_id,
                    "success": r.success,
                    "metrics": r.metrics,
                    "error": r.error,
                }
                for r in results
            ],
        }
