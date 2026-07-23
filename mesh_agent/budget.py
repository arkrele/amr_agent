"""Budget management: L1 hard limits + L2 per-stage quotas + L5 early stopping."""

from __future__ import annotations

import time
from dataclasses import dataclass, field
from enum import Enum


class StopReason(str, Enum):
    MAX_SOLVER_RUNS = "max_solver_runs_reached"
    MAX_WALL_TIME = "max_wall_time_exceeded"
    MAX_LLM_CALLS = "max_llm_calls_reached"
    NO_IMPROVEMENT = "consecutive_no_improvement"
    GATE_REJECTIONS = "consecutive_gate_rejections"
    MESH_CELLS_LIMIT = "mesh_cells_limit_exceeded"
    SOLVER_DIVERGED = "solver_diverged"
    USER_REQUESTED = "user_requested"


@dataclass
class StageQuota:
    max_llm_calls: int
    used: int = 0

    @property
    def remaining(self) -> int:
        return max(0, self.max_llm_calls - self.used)

    @property
    def exhausted(self) -> bool:
        return self.used >= self.max_llm_calls


@dataclass
class BudgetTracker:
    # L1: Hard limits
    max_solver_runs: int = 5
    max_wall_time_seconds: float = 7200.0  # 2 hours default
    max_llm_calls: int = 200
    max_mesh_cells: int = 500000

    # L1: Counters
    solver_runs: int = 0
    total_llm_calls: int = 0
    wall_time_start: float = field(default_factory=time.monotonic)

    # L2: Per-stage quotas
    stage_llm_calls: int = 0
    stage_quotas: dict[str, StageQuota] = field(default_factory=lambda: {
        "analyze": StageQuota(max_llm_calls=10),
        "debate": StageQuota(max_llm_calls=30),
        "gate": StageQuota(max_llm_calls=5),
        "execute": StageQuota(max_llm_calls=60),  # Programmer + Reviewer, up to 4 retries × 2 cells
        "verify": StageQuota(max_llm_calls=10),
    })

    # L5: Early stopping signals
    consecutive_no_improvement: int = 0
    max_no_improvement: int = 2
    consecutive_gate_rejections: int = 0
    max_gate_rejections: int = 2
    improvement_threshold: float = 0.01  # 1% minimum

    # State
    current_stage: str = ""
    stop_reason: str = ""

    # ── L1 checks ──────────────────────────────────────────────

    @property
    def wall_time_elapsed(self) -> float:
        return time.monotonic() - self.wall_time_start

    @property
    def wall_time_remaining(self) -> float:
        return max(0.0, self.max_wall_time_seconds - self.wall_time_elapsed)

    @property
    def solver_runs_remaining(self) -> int:
        return max(0, self.max_solver_runs - self.solver_runs)

    @property
    def llm_calls_remaining(self) -> int:
        return max(0, self.max_llm_calls - self.total_llm_calls)

    def record_solver_run(self) -> None:
        self.solver_runs += 1

    def record_llm_call(self) -> None:
        self.total_llm_calls += 1
        self.stage_llm_calls += 1

    # ── L2 stage management ────────────────────────────────────

    def enter_stage(self, stage: str) -> None:
        self.current_stage = stage
        self.stage_llm_calls = 0

    @property
    def stage_quota(self) -> StageQuota | None:
        return self.stage_quotas.get(self.current_stage)

    @property
    def stage_quota_exhausted(self) -> bool:
        q = self.stage_quota
        return q is not None and q.exhausted

    # ── L5 early stopping ──────────────────────────────────────

    def check_improvement(self, before: dict[str, float], after: dict[str, float]) -> bool:
        """Returns True if any metric improved beyond the threshold."""
        for key in before:
            if key in after:
                delta = abs(after[key] - before[key])
                denom = max(abs(before[key]), 1e-10)
                if delta / denom >= self.improvement_threshold:
                    self.consecutive_no_improvement = 0
                    return True
        self.consecutive_no_improvement += 1
        return False

    def record_gate_rejection(self) -> None:
        self.consecutive_gate_rejections += 1

    def reset_gate_rejections(self) -> None:
        self.consecutive_gate_rejections = 0

    # ── Composite check ────────────────────────────────────────

    def should_stop(self) -> tuple[bool, str]:
        """Check all stop conditions. Returns (should_stop, reason)."""
        if self.solver_runs >= self.max_solver_runs:
            return True, StopReason.MAX_SOLVER_RUNS.value
        if self.wall_time_elapsed >= self.max_wall_time_seconds:
            return True, StopReason.MAX_WALL_TIME.value
        if self.total_llm_calls >= self.max_llm_calls:
            return True, StopReason.MAX_LLM_CALLS.value
        if self.consecutive_no_improvement >= self.max_no_improvement:
            return True, StopReason.NO_IMPROVEMENT.value
        if self.consecutive_gate_rejections >= self.max_gate_rejections:
            return True, StopReason.GATE_REJECTIONS.value
        return False, ""
