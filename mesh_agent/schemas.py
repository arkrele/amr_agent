"""All Pydantic models for the mesh agent system."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from enum import Enum
from pathlib import Path
from typing import Any, Optional

from pydantic import BaseModel, Field


# ── Enums ──────────────────────────────────────────────────────────

class RegionType(str, Enum):
    BOX = "box"
    CYLINDER = "cylinder"
    SPHERE = "sphere"
    CONE = "cone"
    TAG = "tag"
    TAG_RANGE = "tag_range"
    TOPOLOGY = "topology"
    GRADIENT_THRESHOLD = "gradient_threshold"
    VALUE_RANGE = "value_range"
    CURVATURE = "curvature"
    RESIDUAL = "residual"
    BOUNDARY_LAYER = "boundary_layer"
    WAKE = "wake"
    SHOCK = "shock"
    SEPARATION_BUBBLE = "separation_bubble"
    BOOLEAN = "boolean"
    DISTANCE = "distance"
    ALONG_CURVE = "along_curve"


class ActionType(str, Enum):
    REFINE = "refine"
    COARSEN = "coarsen"


class Priority(int, Enum):
    MUST = 1
    SHOULD = 2


class RiskLevel(str, Enum):
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"


class EvidenceLevel(int, Enum):
    EXECUTED = 1
    PRELIMINARY = 2
    ANALYZED = 3
    VALIDATED = 4
    AUDITED = 5


class ClaimStatus(str, Enum):
    PENDING = "pending"
    VALIDATED = "validated"
    REJECTED = "rejected"


class Verdict(str, Enum):
    ACCEPTED = "ACCEPTED"
    REJECTED = "REJECTED"


# ── Geometry / Region ──────────────────────────────────────────────

class GeometricRegion(BaseModel):
    type: str
    corner1: Optional[list[float]] = None
    corner2: Optional[list[float]] = None
    center: Optional[list[float]] = None
    radius: Optional[float] = None
    axis: str = "z"
    height: Optional[float] = None
    apex: Optional[list[float]] = None
    base_center: Optional[list[float]] = None
    base_radius: Optional[float] = None


class SurfaceRef(BaseModel):
    type: str  # tag, tag_range, topology
    tag: Optional[str] = None
    from_: Optional[str] = Field(None, alias="from")
    to: Optional[str] = None
    topology_type: Optional[str] = None


class FieldDrivenRegion(BaseModel):
    type: str
    field: Optional[str] = None
    threshold: Optional[float] = None
    operator: Optional[str] = "greater_than"
    min: Optional[float] = None
    max: Optional[float] = None
    padding_cells: int = 0


class BoundaryLayerParams(BaseModel):
    on_surface: dict[str, Any]
    first_layer_height: float
    growth_rate: float = 1.15
    num_layers: int = 15
    normal_direction: str = "wall_normal"


class WakeParams(BaseModel):
    from_body: dict[str, Any]
    direction: list[float]
    length: float
    spread_angle: float = 5.0


class ShockParams(BaseModel):
    from_field: str
    normal_thickness: float
    tangential_width: float


class SeparationBubbleParams(BaseModel):
    on_surface: dict[str, Any]
    estimated_length: float


class CurveRef(BaseModel):
    type: str
    tag: str
    range: Optional[dict[str, str]] = None


class GrowthSpec(BaseModel):
    start: float
    end: float
    growth: str = "linear"


class AlongCurveParams(BaseModel):
    type: str = "along_curve"
    curve: dict[str, Any]
    transverse_width: Optional[GrowthSpec] = None
    streamwise_size: Optional[GrowthSpec] = None


class BooleanOp(BaseModel):
    type: str = "boolean"
    operation: str  # union | intersection | difference
    a: dict[str, Any]
    b: dict[str, Any]


class RegionSpec(BaseModel):
    """Flexible region specification — exact shape depends on type."""
    type: str
    data: dict[str, Any] = Field(default_factory=dict)

    @classmethod
    def from_raw(cls, raw: dict[str, Any]) -> "RegionSpec":
        t = raw.pop("type", "box")
        return cls(type=t, data=raw)


# ── Strategy ───────────────────────────────────────────────────────

class MeshOperation(BaseModel):
    region: RegionSpec
    action: ActionType
    target_size: Optional[float] = None
    growth_rate: float = 1.1
    priority: Priority = Priority.SHOULD
    target_metric: Optional[str] = None
    target_value: Optional[float] = None


class ExpectedEffect(BaseModel):
    cells_estimate: int
    affected_metrics: list[str] = Field(default_factory=list)
    risk: RiskLevel = RiskLevel.MEDIUM
    risk_detail: str = ""


class Strategy(BaseModel):
    strategy_id: str
    rationale: str
    operations: list[MeshOperation]
    expected_effect: ExpectedEffect


class StrategyScore(BaseModel):
    strategy_id: str
    physical_reasonableness: int = Field(ge=1, le=5)
    numerical_necessity: int = Field(ge=1, le=5)
    expected_benefit: int = Field(ge=1, le=5)
    implementation_feasibility: int = Field(ge=1, le=5)
    overall: int = Field(ge=1, le=5)
    comment: str = ""


class CounterScore(BaseModel):
    strategy_id: str
    mesh_quality_risk: int = Field(ge=1, le=5)
    numerical_risk: int = Field(ge=1, le=5)
    cost_risk: int = Field(ge=1, le=5)
    physical_misjudgment_risk: int = Field(ge=1, le=5)
    overall: int = Field(ge=1, le=5)
    fatal_flaws: list[str] = Field(default_factory=list)
    alternative: str = ""
    comment: str = ""


# ── Gate ───────────────────────────────────────────────────────────

class GateDecision(BaseModel):
    action: str  # "select" | "reject_all"
    selected_strategy: Optional[str] = None
    reason: str
    strategy_scores: list[dict[str, Any]] = Field(default_factory=list)


# ── Problem Spec ───────────────────────────────────────────────────

class ProblemDescription(BaseModel):
    description: str
    pde_type: str = "user-defined"
    geometry: str = ""
    boundary_conditions: dict[str, Any] = Field(default_factory=dict)


class SolverSpec(BaseModel):
    type: str = "user_template"  # user_template | agent_generated
    path: str = ""
    interface: str = "generic_cli"  # fenicsx | openfoam | generic_cli
    params: dict[str, Any] = Field(default_factory=dict)


class MeshSpec(BaseModel):
    initial: str = ""
    format: str = "gmsh"
    max_cells: int = 500000


class BudgetSpec(BaseModel):
    max_solver_runs: int = 5
    max_wall_time_minutes: int = 120


class OutputSpec(BaseModel):
    target_metrics: list[str] = Field(default_factory=list)
    convergence_tolerance: float = 1e-4
    output_dir: str = "./output"


class ProblemSpec(BaseModel):
    problem: ProblemDescription
    solver: SolverSpec
    mesh: MeshSpec
    budget: BudgetSpec = Field(default_factory=BudgetSpec)
    output: OutputSpec = Field(default_factory=OutputSpec)

    @classmethod
    def from_yaml(cls, path: str | Path) -> "ProblemSpec":
        import yaml

        with open(path, encoding="utf-8") as f:
            data = yaml.safe_load(f)
        return cls(**data)


# ── Metrics ────────────────────────────────────────────────────────

class MeshQualityReport(BaseModel):
    min_skewness: float
    max_skewness: float
    max_aspect_ratio: float
    min_jacobian: float
    has_inverted_cells: bool
    total_cells: int
    passed: bool
    failures: list[str] = Field(default_factory=list)


class SolverConvergence(BaseModel):
    converged: bool
    residual_final: float
    residual_initial: float
    orders_dropped: float
    iterations: int
    warnings: list[str] = Field(default_factory=list)


class Metrics(BaseModel):
    mesh_quality: Optional[MeshQualityReport] = None
    solver_convergence: Optional[SolverConvergence] = None
    custom: dict[str, float] = Field(default_factory=dict)
    source_path: str = ""


# ── Claim Ledger ───────────────────────────────────────────────────

class ClaimEvidence(BaseModel):
    before: dict[str, float] = Field(default_factory=dict)
    after: dict[str, float] = Field(default_factory=dict)
    delta: dict[str, float] = Field(default_factory=dict)
    delta_pct: dict[str, float] = Field(default_factory=dict)


class ClaimValidation(BaseModel):
    method: str = "auto_extraction"
    match: bool = False
    significant: bool = False
    detail: str = ""


class Claim(BaseModel):
    claim_id: str
    round: int
    text: str
    strategy_id: str
    evidence: ClaimEvidence = Field(default_factory=ClaimEvidence)
    validation: ClaimValidation = Field(default_factory=ClaimValidation)
    evidence_level: EvidenceLevel = EvidenceLevel.EXECUTED
    status: ClaimStatus = ClaimStatus.PENDING
    timestamp: str = Field(default_factory=lambda: datetime.now(timezone.utc).isoformat())


# ── Round Record ───────────────────────────────────────────────────

class RoundRecord(BaseModel):
    round: int
    strategy_id: str
    strategy_summary: str
    mesh_change: dict[str, Any] = Field(default_factory=dict)
    before: dict[str, float] = Field(default_factory=dict)
    after: dict[str, float] = Field(default_factory=dict)
    claim: str = ""
    evidence_ref: str = ""
    verdict: str = "PENDING"
    images: list[str] = Field(default_factory=list)


class ResultSummary(BaseModel):
    rounds: int
    initial_mesh_cells: int
    final_mesh_cells: int
    improvements: dict[str, float] = Field(default_factory=dict)


class Result(BaseModel):
    summary: ResultSummary
    rounds: list[RoundRecord] = Field(default_factory=list)
    evidence_chain: list[Claim] = Field(default_factory=list)
    artifacts: dict[str, str] = Field(default_factory=dict)

    def to_yaml(self, path: str | Path) -> None:
        import yaml

        with open(path, "w", encoding="utf-8") as f:
            yaml.dump(self.model_dump(), f, allow_unicode=True, default_flow_style=False)


# ── Session Context ────────────────────────────────────────────────

class SessionContext(BaseModel):
    """Mutable state carried through the adaptive loop."""
    round_number: int = 0
    current_mesh_path: str = ""
    current_solution_path: str = ""
    previous_metrics: Metrics = Field(default_factory=Metrics)
    current_metrics: Metrics = Field(default_factory=Metrics)
    best_metrics: dict[str, float] = Field(default_factory=dict)
    claims: list[Claim] = Field(default_factory=list)
    round_records: list[RoundRecord] = Field(default_factory=list)
    consecutive_no_improvement: int = 0
    consecutive_gate_rejections: int = 0
    solver_runs: int = 0
    wall_time_start: Optional[float] = None
    total_llm_calls: int = 0
    stage_llm_calls: int = 0
