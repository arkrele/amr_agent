"""POST-Gate: Programmatic numerical validation (~0.01s, no LLM).

Validates:
1. Artifact completeness (files exist, non-empty)
2. Solver convergence (residual drop sufficient)
3. Numerical range (metrics within physical bounds)
4. Mesh quality (from mesh quality module)

Pure Python, no LLM — zero hallucination risk.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any


def validate_artifacts(output_dir: str | Path, required_files: list[str] | None = None) -> dict[str, Any]:
    """Check that all expected output files exist and are non-empty."""
    output_dir = Path(output_dir)
    if required_files is None:
        required_files = ["metrics.json"]

    missing = []
    empty = []
    for fname in required_files:
        fpath = output_dir / fname
        if not fpath.exists():
            missing.append(fname)
        elif fpath.stat().st_size == 0:
            empty.append(fname)

    return {
        "all_present": len(missing) == 0 and len(empty) == 0,
        "missing": missing,
        "empty": empty,
    }


def validate_convergence(metrics: dict[str, Any]) -> dict[str, Any]:
    """Check solver convergence from metrics."""
    conv = metrics.get("solver_convergence", {})

    residual_final = conv.get("residual_final", 0.0)
    residual_initial = conv.get("residual_initial", 1.0)
    converged = conv.get("converged", False)
    iterations = conv.get("iterations", 0)

    issues = []

    if not converged:
        issues.append("Solver did not converge")

    if residual_initial > 1e-15:
        orders_dropped = abs(residual_final / residual_initial)
        # Check residual dropped at least 3 orders of magnitude
        if orders_dropped > 1e-3 and not converged:
            issues.append(f"Residual only dropped to {orders_dropped:.1e} of initial value")

    # Also check custom convergence info
    if "residual_final" in metrics and "residual_initial" in metrics:
        rf = metrics.get("residual_final", 0.0)
        ri = max(metrics.get("residual_initial", 1.0), 1e-15)
        if rf / ri > 1e-3:
            issues.append(f"Custom residual check: {rf/ri:.1e} > 1e-3 threshold")

    return {
        "converged": converged,
        "residual_final": residual_final or metrics.get("residual_final", 0.0),
        "iterations": iterations,
        "passed": len(issues) == 0,
        "issues": issues,
    }


def validate_numerical_range(
    metrics: dict[str, Any],
    bounds: dict[str, tuple[float, float]] | None = None,
) -> dict[str, Any]:
    """Check that metrics are within physically reasonable bounds."""
    if bounds is None:
        bounds = {}

    issues = []
    custom = metrics.get("custom", metrics)

    for key, value in custom.items():
        if not isinstance(value, (int, float)):
            continue
        if key.startswith("_"):
            continue

        if key in bounds:
            lo, hi = bounds[key]
            if value < lo:
                issues.append(f"{key}={value} below physical bound {lo}")
            if value > hi:
                issues.append(f"{key}={value} above physical bound {hi}")

    return {
        "checked": len(bounds),
        "out_of_range": len(issues),
        "passed": len(issues) == 0,
        "issues": issues,
    }


def run_post_gate(
    output_dir: str | Path,
    mesh_quality_report: dict[str, Any] | None = None,
    bounds: dict[str, tuple[float, float]] | None = None,
) -> dict[str, Any]:
    """Run full POST-Gate validation pipeline.

    Returns a dict with overall pass/fail and per-check details.
    """
    output_dir = Path(output_dir)
    checks = {}

    # 1. Artifact check
    checks["artifacts"] = validate_artifacts(output_dir)

    # 2. Read metrics
    metrics_file = output_dir / "metrics.json"
    metrics = {}
    if metrics_file.exists():
        try:
            metrics = json.loads(metrics_file.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            checks["parse_error"] = {"passed": False, "error": "Invalid metrics.json"}

    # 3. Convergence check
    checks["convergence"] = validate_convergence(metrics)

    # 4. Numerical range check
    checks["numerical_range"] = validate_numerical_range(metrics, bounds)

    # 5. Mesh quality check (if provided)
    if mesh_quality_report:
        checks["mesh_quality"] = mesh_quality_report

    all_passed = all(c.get("passed", False) for c in checks.values())

    return {
        "passed": all_passed,
        "checks": checks,
        "summary": _summarize(checks),
    }


def _summarize(checks: dict[str, Any]) -> str:
    parts = []
    for name, result in checks.items():
        if not isinstance(result, dict):
            continue
        status = "PASS" if result.get("passed", False) else "FAIL"
        parts.append(f"[{status}] {name}")
    return "; ".join(parts)
