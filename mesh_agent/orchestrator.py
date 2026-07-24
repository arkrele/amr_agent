"""Main orchestrator: ties all agents and states together into the adaptive loop."""

from __future__ import annotations

import asyncio
import json
import os
import time
from pathlib import Path
from typing import Any, Optional

from openai import AsyncOpenAI

from mesh_agent.agents.strategist import Strategist
from mesh_agent.agents.optimist import Optimist
from mesh_agent.agents.skeptic import Skeptic
from mesh_agent.agents.gatekeeper import Gatekeeper
from mesh_agent.agents.programmer import Programmer
from mesh_agent.agents.reviewer import Reviewer
from mesh_agent.budget import BudgetTracker
from mesh_agent.executor.cell_executor import CellExecutor
from mesh_agent.executor.worktree_manager import WorktreeManager
from mesh_agent.schemas import (
    MeshQualityReport,
    Metrics,
    ProblemSpec,
    Result,
    ResultSummary,
    RoundRecord,
    SessionContext,
)
from mesh_agent.solver_interface.base import UserTemplateSolver
from mesh_agent.state_machine import IllegalTransitionError, State, StateMachine
from mesh_agent.verification.claim_ledger import ClaimLedger
from mesh_agent.verification.mesh_quality import check_mesh_quality
from mesh_agent.verification.post_gate import run_post_gate
from mesh_agent.verification.pre_gate import PreGate


def _make_client() -> AsyncOpenAI:
    kwargs = {"api_key": os.environ.get("OPENAI_API_KEY", "")}
    base_url = os.environ.get("OPENAI_BASE_URL")
    if base_url:
        kwargs["base_url"] = base_url
    return AsyncOpenAI(**kwargs)


class Orchestrator:
    """Main orchestrator for the mesh adaptation agent.

    Drives the state machine through IDLE → ANALYZE → DEBATE → GATE →
    EXECUTE → VERIFY → DECIDE → (loop or SUMMARIZE → CLOSED).
    """

    def __init__(
        self,
        problem: ProblemSpec,
        output_dir: str | Path,
        knowledge_dir: Optional[str | Path] = None,
        client: Optional[AsyncOpenAI] = None,
    ):
        self.problem = problem
        self.output_dir = Path(output_dir).resolve()
        self.output_dir.mkdir(parents=True, exist_ok=True)

        # Shared OpenAI client
        self.client = client or _make_client()
        self.reviewer_client = _make_client()  # Different instance for independence

        # Subsystems
        self.sm = StateMachine(State.IDLE)
        self.budget = BudgetTracker(
            max_solver_runs=problem.budget.max_solver_runs,
            max_wall_time_seconds=problem.budget.max_wall_time_minutes * 60,
            max_mesh_cells=problem.mesh.max_cells,
        )
        self.ledger = ClaimLedger()
        self.worktree = WorktreeManager(self.output_dir)
        self.pre_gate = PreGate(client=self.client)

        # Agents (Programmer and Reviewer get separate client instances for independence)
        kd = Path(knowledge_dir) if knowledge_dir else None
        self.strategist = Strategist(knowledge_dir=kd, client=self.client)
        self.optimist = Optimist(client=self.client)
        self.skeptic = Skeptic(client=self.client)
        self.gatekeeper = Gatekeeper(client=self.client)
        self.programmer = Programmer(client=self.client)
        self.reviewer = Reviewer(client=self.reviewer_client)  # Different instance!

        # Solver
        solver_path = Path(problem.solver.path) if problem.solver.path else None
        if solver_path and solver_path.exists():
            self.solver = UserTemplateSolver(solver_path)
        else:
            self.solver = None

        # Cell executor
        self.executor = CellExecutor(self.programmer, self.reviewer, self.solver)

        # Context
        self.ctx = SessionContext()
        # Resolve mesh path to absolute
        mesh_initial = Path(problem.mesh.initial)
        if not mesh_initial.is_absolute():
            mesh_initial = self.output_dir.parent / mesh_initial
        self.ctx.current_mesh_path = str(mesh_initial.resolve())

    async def run(self) -> Result:
        """Main loop: drive the state machine until CLOSED."""
        self.ctx.wall_time_start = time.monotonic()
        self.budget.wall_time_start = time.monotonic()
        self._log(f"Starting mesh adaptation for: {self.problem.problem.description}")

        while not self.sm.is_terminal:
            state = self.sm.current
            self._log(f"State: {state.value}")

            try:
                if state == State.IDLE:
                    await self._handle_idle()
                elif state == State.INIT_SOLVE:
                    await self._handle_init_solve()
                elif state == State.ANALYZE:
                    await self._handle_analyze()
                elif state == State.DEBATE:
                    await self._handle_debate()
                elif state == State.GATE:
                    await self._handle_gate()
                elif state == State.EXECUTE:
                    await self._handle_execute()
                elif state == State.VERIFY:
                    await self._handle_verify()
                elif state == State.DECIDE:
                    await self._handle_decide()
                elif state == State.SUMMARIZE:
                    await self._handle_summarize()
            except Exception as e:
                self._log(f"Error in state {state.value}: {e}")
                # Try to gracefully degrade
                if state in (State.ANALYZE, State.DEBATE, State.GATE, State.EXECUTE, State.VERIFY):
                    self.sm.transition(State.DECIDE)
                else:
                    self.sm.transition(State.SUMMARIZE)

        return self._build_result()

    # ── State Handlers ──────────────────────────────────────────

    async def _handle_idle(self) -> None:
        self.sm.transition(State.INIT_SOLVE)

    async def _handle_init_solve(self) -> None:
        """Run initial solve on the starting mesh before the first analysis.

        If no solver is provided, generates one first.
        If a solution already exists, skips to ANALYZE.
        """
        existing_metrics = self.output_dir / "output" / "metrics.json"
        if existing_metrics.exists():
            self._log("Existing solution found, skipping initial solve")
            try:
                data = json.loads(existing_metrics.read_text(encoding="utf-8"))
                self.ctx.current_metrics = Metrics(
                    custom={k: v for k, v in data.items()
                            if isinstance(v, (int, float)) and not k.startswith("_")},
                    source_path=str(existing_metrics),
                )
            except Exception:
                pass
            self.sm.transition(State.ANALYZE)
            return

        # Generate or reuse solver
        solver = self.solver
        solver_path = self.output_dir / "solver.py"

        if solver is None and solver_path.exists():
            # Reuse previously generated solver
            solver = UserTemplateSolver(solver_path)
            self.solver = solver
            self._log("Reusing existing solver")

        if solver is None:
            # Generate solver with retry
            for attempt in range(1, 7):
                self._log(f"No solver template — Agent generating solver (attempt {attempt})")
                impl = await self.programmer.generate_solver(
                    self.problem.model_dump(),
                    self.ctx.current_mesh_path,
                    str(self.output_dir / "output"),
                )
                solver_code = impl.get("solver_script", "")
                if not solver_code:
                    continue
                solver_path.write_text(solver_code, encoding="utf-8")
                solver = UserTemplateSolver(solver_path)

                # Test the solver
                try:
                    test_output = self.output_dir / "output"
                    test_output.mkdir(parents=True, exist_ok=True)
                    metrics = await solver.run(
                        mesh_path=self.ctx.current_mesh_path,
                        output_dir=str(test_output),
                        params=self.problem.solver.params,
                        work_dir=str(self.output_dir),
                    )
                except Exception:
                    continue

                if metrics.get("_success"):
                    self.solver = solver
                    self._log("Solver generated and tested successfully")
                    # Got working solver, proceed to use these metrics
                    break
            else:
                self._log("Solver generation failed after 6 attempts")
                self.sm.transition(State.SUMMARIZE)
                return

            # Use metrics from test run
            self.ctx.current_metrics = Metrics(
                custom={k: v for k, v in metrics.items()
                        if isinstance(v, (int, float)) and not k.startswith("_")},
                source_path=str(test_output / "metrics.json"),
            )
            self.budget.record_solver_run()
            self.ctx.solver_runs += 1
            self._log(f"Initial solve complete: {self.ctx.current_metrics.custom}")
            self.sm.transition(State.ANALYZE)
            return

        # Solver exists (from template or reuse) — run it
        self._log("Running solver on initial mesh...")
        output_dir = self.output_dir / "output"
        output_dir.mkdir(parents=True, exist_ok=True)

        try:
            metrics = await solver.run(
                mesh_path=self.ctx.current_mesh_path,
                output_dir=str(output_dir),
                params=self.problem.solver.params,
                work_dir=str(self.output_dir),
            )
        except Exception as e:
            self._log(f"Initial solve failed: {e}")
            self.sm.transition(State.SUMMARIZE)
            return

        if not metrics.get("_success"):
            self._log(f"Initial solve failed: {metrics.get('_error', 'unknown')}")
            self.sm.transition(State.SUMMARIZE)
            return

        self.ctx.current_metrics = Metrics(
            custom={k: v for k, v in metrics.items()
                    if isinstance(v, (int, float)) and not k.startswith("_")},
            source_path=str(output_dir / "metrics.json"),
        )
        self.budget.record_solver_run()
        self.ctx.solver_runs += 1
        self._log(f"Initial solve complete: {self.ctx.current_metrics.custom}")
        self.ctx._mesh_before_path = self.ctx.current_mesh_path  # type: ignore[attr-defined]  # baseline for 1st round viz
        self.sm.transition(State.ANALYZE)

    async def _handle_analyze(self) -> None:
        self.budget.enter_stage("analyze")

        # Build solution summary from context
        mesh_stats = {
            "current_mesh": self.ctx.current_mesh_path,
            "total_cells": getattr(self.ctx.current_metrics.mesh_quality, "total_cells", 0) if self.ctx.current_metrics.mesh_quality else 0,
            "max_cells": self.budget.max_mesh_cells,
        }
        solution_summary = {
            "custom_metrics": self.ctx.current_metrics.custom if self.ctx.current_metrics else {},
            "previous_metrics": self.ctx.previous_metrics.custom if self.ctx.previous_metrics else {},
        }

        prev_strategies = [r.model_dump() for r in self.ctx.round_records] if self.ctx.round_records else None
        prev_metrics = self.ctx.previous_metrics.custom if self.ctx.previous_metrics else None

        result = await self.strategist.generate(
            problem_description=self.problem.problem.description,
            current_mesh_stats=mesh_stats,
            solution_summary=solution_summary,
            previous_metrics=prev_metrics,
            previous_strategies=prev_strategies,
        )
        self.budget.record_llm_call()
        self.ctx.total_llm_calls += 1

        strategies = result.get("strategies", [])
        analysis = result.get("analysis", "")

        if not strategies:
            self._log("Strategist found no viable strategies — transitioning to DECIDE")
            self.sm.transition(State.DECIDE)
            return

        self._log(f"Strategist proposed {len(strategies)} strategies:\n{analysis}")
        self.ctx._debate_strategies = strategies  # type: ignore[attr-defined]
        self.sm.transition(State.DEBATE)

    async def _handle_debate(self) -> None:
        self.budget.enter_stage("debate")
        strategies = getattr(self.ctx, "_debate_strategies", [])

        solution_summary = {
            "custom_metrics": self.ctx.current_metrics.custom if self.ctx.current_metrics else {},
        }
        mesh_stats = {
            "total_cells": getattr(self.ctx.current_metrics.mesh_quality, "total_cells", 0) if self.ctx.current_metrics and self.ctx.current_metrics.mesh_quality else 0,
        }
        budget_info = {
            "solver_runs_remaining": self.budget.solver_runs_remaining,
            "wall_time_remaining_min": self.budget.wall_time_remaining / 60,
            "max_mesh_cells": self.budget.max_mesh_cells,
        }

        # Optimist
        optimist_scores = await self.optimist.evaluate(
            strategies, solution_summary, self.problem.problem.description,
        )
        self.budget.record_llm_call()
        self.ctx.total_llm_calls += 1

        # Skeptic
        skeptic_scores = await self.skeptic.evaluate(
            strategies, solution_summary, mesh_stats,
            self.problem.problem.description, budget_info,
        )
        self.budget.record_llm_call()
        self.ctx.total_llm_calls += 1

        self._log(f"Debate complete: {len(optimist_scores)} optimist, {len(skeptic_scores)} skeptic evaluations")
        self.ctx._optimist_scores = optimist_scores  # type: ignore[attr-defined]
        self.ctx._skeptic_scores = skeptic_scores  # type: ignore[attr-defined]
        self.sm.transition(State.GATE)

    async def _handle_gate(self) -> None:
        self.budget.enter_stage("gate")
        strategies = getattr(self.ctx, "_debate_strategies", [])
        optimist_scores = getattr(self.ctx, "_optimist_scores", [])
        skeptic_scores = getattr(self.ctx, "_skeptic_scores", [])

        decision = await self.gatekeeper.decide(
            strategies, optimist_scores, skeptic_scores,
            is_first_attempt=(self.budget.consecutive_gate_rejections == 0),
        )
        self.budget.record_llm_call()
        self.ctx.total_llm_calls += 1

        self._log(f"Gatekeeper decision: {decision.get('action')} — {decision.get('reason')}")

        if decision.get("action") == "reject_all":
            self.budget.record_gate_rejection()
            self._log("All strategies rejected by Gatekeeper — terminating this round")
            self.sm.transition(State.DECIDE)
            return

        self.budget.reset_gate_rejections()
        selected_id = decision.get("selected_strategy")
        selected = next((s for s in strategies if s.get("strategy_id") == selected_id), None)

        if selected is None:
            self._log(f"Selected strategy {selected_id} not found — falling back to first strategy")
            selected = strategies[0]

        self.ctx._selected_strategy = selected  # type: ignore[attr-defined]
        self.sm.transition(State.EXECUTE)

    async def _handle_execute(self) -> None:
        self.budget.enter_stage("execute")
        strategy = getattr(self.ctx, "_selected_strategy", {})
        self.ctx.round_number += 1
        round_num = self.ctx.round_number

        self._log(f"Executing Round {round_num}: {strategy.get('strategy_id', 'unknown')}")

        # Create isolated worktree
        wt_name = f"round_{round_num}"
        wt_path = self.worktree.create(wt_name)

        # Copy inputs to worktree
        input_files = {}
        mesh_src = Path(self.ctx.current_mesh_path)
        if mesh_src.exists():
            if mesh_src.is_dir():
                input_files["input_mesh"] = self.ctx.current_mesh_path
                worktree_mesh = str(wt_path / "input_mesh")
            else:
                input_files["current_mesh.msh"] = self.ctx.current_mesh_path
                worktree_mesh = str(wt_path / "current_mesh.msh")
        else:
            worktree_mesh = self.ctx.current_mesh_path

        if self.problem.solver.path:
            input_files["solver.py"] = self.problem.solver.path
        self.worktree.copy_inputs(wt_path, input_files)

        # Prepare previous metrics
        prev_metrics = self.ctx.current_metrics.custom if self.ctx.current_metrics else None

        # Track mesh-before for visualization comparison
        mesh_before_path = self.ctx._mesh_before_path if hasattr(self.ctx, "_mesh_before_path") else worktree_mesh

        # Run cell-by-cell execution
        results = await self.executor.execute(
            strategy=strategy,
            work_dir=wt_path,
            current_mesh_path=worktree_mesh,
            solver_path=str(wt_path / "solver.py") if self.problem.solver.path else "",
            problem_spec=self.problem.model_dump(),
            previous_metrics=prev_metrics,
            mesh_before_path=mesh_before_path,
        )

        # Track LLM calls from executor
        self.budget.total_llm_calls += results.get("mesh_generation", type("", (), {"retries": 0})()).retries * 2  # rough estimate
        self.ctx.total_llm_calls = self.budget.total_llm_calls

        # Record solver run
        self.budget.record_solver_run()
        self.ctx.solver_runs += 1

        # Commit worktree
        all_ok = all(r.success for r in results.values())
        self.worktree.commit(wt_path, f"Round {round_num}: {strategy.get('strategy_id')} — {'PASS' if all_ok else 'FAIL'}")

        if not all_ok:
            for cell_name, r in results.items():
                if not r.success:
                    self._log(f"  Cell [{cell_name}] FAILED (retries={r.retries}): {r.error[:200]}")

        self.ctx._execution_results = results  # type: ignore[attr-defined]
        self.ctx._worktree_path = wt_path  # type: ignore[attr-defined]

        if all_ok:
            self.sm.transition(State.VERIFY)
        else:
            failed = [k for k, v in results.items() if not v.success]
            self._log(f"Execution partially failed: {failed}")
            self.sm.transition(State.DECIDE)

    async def _handle_verify(self) -> None:
        self.budget.enter_stage("verify")
        strategy = getattr(self.ctx, "_selected_strategy", {})
        results = getattr(self.ctx, "_execution_results", {})
        wt_path = getattr(self.ctx, "_worktree_path", self.output_dir)

        output_dir = wt_path / "output"
        metrics_file = output_dir / "metrics.json"

        # Load current metrics
        current_metrics_dict = {}
        if metrics_file.exists():
            try:
                current_metrics_dict = json.loads(metrics_file.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                pass

        # PRE-Gate check (validate experiment design)
        pre_gate_result = await self.pre_gate.validate(
            strategy,
            self.problem.model_dump(),
            self.ctx.current_metrics.custom if self.ctx.current_metrics else None,
        )
        self.budget.record_llm_call()
        self.ctx.total_llm_calls += 1

        if not pre_gate_result.get("passed", True):
            blockers = [i for i in pre_gate_result.get("issues", []) if i["severity"] == "blocker"]
            if blockers:
                self._log(f"PRE-Gate blocked: {blockers}")
                self.sm.transition(State.DECIDE)
                return

        # Check mesh quality if available
        mesh_quality_file = output_dir / "mesh_quality.json"
        mesh_qr = None
        if mesh_quality_file.exists():
            mesh_qr = check_mesh_quality(mesh_quality_file)

        # POST-Gate
        post_gate_result = run_post_gate(output_dir, mesh_qr)

        self._log(f"POST-Gate: {post_gate_result['summary']}")

        # Build metrics object
        current_metrics = Metrics(
            mesh_quality=MeshQualityReport(**mesh_qr) if mesh_qr else None,
            custom={k: v for k, v in current_metrics_dict.items()
                    if isinstance(v, (int, float)) and not k.startswith("_")},
            source_path=str(metrics_file),
        )

        # Store previous and current
        self.ctx.previous_metrics = self.ctx.current_metrics
        self.ctx.current_metrics = current_metrics

        # Create Claim
        before = self.ctx.previous_metrics.custom if self.ctx.previous_metrics else {}
        after = current_metrics.custom

        round_num = self.ctx.round_number
        strategy_id = strategy.get("strategy_id", "unknown")

        # Summarize mesh change
        mesh_change_summary = {
            "cells_before": getattr(self.ctx.previous_metrics.mesh_quality, "total_cells", 0) if self.ctx.previous_metrics and self.ctx.previous_metrics.mesh_quality else 0,
            "cells_after": getattr(current_metrics.mesh_quality, "total_cells", 0) if current_metrics.mesh_quality else 0,
        }

        # Auto-generate claim text
        if before and after:
            common_keys = set(before.keys()) & set(after.keys())
            changes = []
            for k in sorted(common_keys):
                delta = after[k] - before[k]
                denom = max(abs(before[k]), 1e-10)
                pct = delta / denom * 100
                direction = "increased" if delta > 0 else "decreased"
                changes.append(f"{k} {direction} by {abs(pct):.1f}% (from {before[k]:.4g} to {after[k]:.4g})")
            claim_text = f"Mesh adaptation ({strategy_id}): " + "; ".join(changes[:3])
        else:
            claim_text = f"Mesh adaptation ({strategy_id}) executed. Post-gate passed: {post_gate_result['passed']}"

        # Record in ledger
        claim = self.ledger.create_claim(
            round_number=round_num,
            text=claim_text,
            strategy_id=strategy_id,
            evidence_file=str(metrics_file),
            before_values=before,
            after_values=after,
        )

        # Extract visualization images
        viz_result = results.get("visualization")
        viz_images: list[str] = []
        if viz_result and viz_result.success:
            viz_images = viz_result.output.get("images", [])

        # Record round
        record = RoundRecord(
            round=round_num,
            strategy_id=strategy_id,
            strategy_summary=strategy.get("rationale", ""),
            mesh_change=mesh_change_summary,
            before=before,
            after=after,
            claim=claim_text,
            evidence_ref=claim.claim_id,
            verdict="ACCEPTED" if post_gate_result["passed"] and claim.status.value == "validated" else "REJECTED",
            images=viz_images,
        )
        self.ctx.round_records.append(record)

        # Update best metrics
        self.ctx.best_metrics = after if after else self.ctx.best_metrics

        # Update mesh path for next round. Save old path for viz comparison.
        old_mesh = self.ctx.current_mesh_path
        new_mesh = wt_path / "output" / "adapted_mesh.msh"
        if new_mesh.exists():
            self.ctx.current_mesh_path = str(new_mesh)
            self.ctx._mesh_before_path = old_mesh  # type: ignore[attr-defined]
        elif mesh_quality_file.exists():
            # Try to find any generated mesh
            candidates = list(wt_path.glob("*.msh")) + list((wt_path / "output").glob("*.msh"))
            if candidates:
                self.ctx.current_mesh_path = str(candidates[0])

        self.ctx._post_gate_result = post_gate_result  # type: ignore[attr-defined]
        self.sm.transition(State.DECIDE)

    async def _handle_decide(self) -> None:
        post_gate = getattr(self.ctx, "_post_gate_result", {})

        # Check early stop conditions
        should_stop, reason = self.budget.should_stop()
        if should_stop:
            self._log(f"Early stop triggered: {reason}")
            self.budget.stop_reason = reason
            self.sm.transition(State.SUMMARIZE)
            return

        # Rule-based: check numerical improvement
        before = self.ctx.previous_metrics.custom if self.ctx.previous_metrics else {}
        after = self.ctx.current_metrics.custom if self.ctx.current_metrics else {}
        has_improvement = self.budget.check_improvement(before, after) if before else False

        # Check Gatekeeper rejection chain
        if self.budget.consecutive_gate_rejections >= self.budget.max_gate_rejections:
            self._log("Consecutive gate rejections limit reached")
            self.sm.transition(State.SUMMARIZE)
            return

        # Check solver divergence
        if not post_gate.get("passed", True):
            if self.ctx.round_number > 1:
                self._log("POST-Gate failed — stopping adaptation")
                self.sm.transition(State.SUMMARIZE)
                return

        # Rule-based decision
        if has_improvement and self.budget.solver_runs_remaining > 0:
            self._log("Improvement detected — continuing to next round")
            self.sm.transition(State.ANALYZE)
            return

        if not has_improvement and self.budget.consecutive_no_improvement >= self.budget.max_no_improvement:
            self._log("No improvement for consecutive rounds — stopping")
            self.sm.transition(State.SUMMARIZE)
            return

        if not has_improvement:
            # LLM-based semantic check: is there a quality improvement?
            self._log("No significant numerical improvement — checking semantic quality")
            semantic_improvement = await self._check_semantic_improvement()
            self.budget.record_llm_call()
            self.ctx.total_llm_calls += 1

            if semantic_improvement and self.budget.solver_runs_remaining > 0:
                self._log("Semantic improvement found — one additional round granted")
                self.sm.transition(State.ANALYZE)
                return
            else:
                self.sm.transition(State.SUMMARIZE)
                return

        # Fallthrough
        self.sm.transition(State.SUMMARIZE)

    async def _handle_summarize(self) -> None:
        self._log("Summarizing results...")
        self.sm.transition(State.CLOSED)

    # ── Helpers ─────────────────────────────────────────────────

    async def _check_semantic_improvement(self) -> bool:
        """LLM-based semantic quality check when numerical metrics show no change."""
        user_message = f"""## Previous Round Metrics
{json.dumps(self.ctx.previous_metrics.custom if self.ctx.previous_metrics else {}, indent=2)}

## Current Round Metrics
{json.dumps(self.ctx.current_metrics.custom if self.ctx.current_metrics else {}, indent=2)}

## Problem Description
{self.problem.problem.description}

The numerical metrics show no significant change (less than {self.budget.improvement_threshold*100}%).
However, is there a semantic/qualitative improvement that justifies another round of adaptation?
For example:
- Is the flow field structure more physically reasonable?
- Has an oscillation or instability been resolved?
- Is the solver converging faster even if the final values are similar?

Respond with JSON: {{"has_improvement": true|false, "reason": "..."}}"""

        schema = {
            "type": "object",
            "properties": {
                "has_improvement": {"type": "boolean"},
                "reason": {"type": "string"},
            },
            "required": ["has_improvement", "reason"],
        }

        result = await self.strategist.run_structured(user_message, schema)
        return result.get("has_improvement", False)

    def _build_result(self) -> Result:
        summary = ResultSummary(
            rounds=self.ctx.round_number,
            initial_mesh_cells=getattr(self.ctx.previous_metrics.mesh_quality, "total_cells", 0) if self.ctx.round_records and self.ctx.previous_metrics and self.ctx.previous_metrics.mesh_quality else 0,
            final_mesh_cells=getattr(self.ctx.current_metrics.mesh_quality, "total_cells", 0) if self.ctx.current_metrics and self.ctx.current_metrics.mesh_quality else 0,
            improvements=self.ctx.best_metrics,
        )
        return Result(
            summary=summary,
            rounds=self.ctx.round_records,
            evidence_chain=self.ledger.claims,
            artifacts={
                "output_dir": str(self.output_dir),
                "final_mesh": self.ctx.current_mesh_path,
            },
        )

    def _log(self, message: str) -> None:
        print(f"[mesh-agent] {message}")
