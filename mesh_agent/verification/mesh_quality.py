"""Mesh quality metrics computation (programmatic, no LLM)."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any


# Default quality thresholds
DEFAULTS = {
    "max_skewness": 0.9,
    "max_aspect_ratio": 100.0,
    "min_jacobian": 0.0,  # Must be positive (no inverted cells)
}


def check_mesh_quality(
    metrics_file: str | Path,
    thresholds: dict[str, float] | None = None,
) -> dict[str, Any]:
    """Check mesh quality from a metrics JSON file.

    Expects a JSON file with optional fields:
        - min_skewness, max_skewness
        - max_aspect_ratio
        - min_jacobian
        - has_inverted_cells (bool)
        - total_cells (int)

    Returns a MeshQualityReport as dict.
    """
    t = {**DEFAULTS, **(thresholds or {})}
    path = Path(metrics_file)

    if not path.exists():
        return {
            "min_skewness": 0.0,
            "max_skewness": 0.0,
            "max_aspect_ratio": 0.0,
            "min_jacobian": 0.0,
            "has_inverted_cells": False,
            "total_cells": 0,
            "passed": False,
            "failures": [f"Metrics file not found: {metrics_file}"],
        }

    data = json.loads(path.read_text(encoding="utf-8"))
    failures = []

    max_skew = data.get("max_skewness", 0.0)
    if max_skew > t["max_skewness"]:
        failures.append(f"Max skewness {max_skew:.3f} exceeds threshold {t['max_skewness']}")

    max_ar = data.get("max_aspect_ratio", 0.0)
    if max_ar > t["max_aspect_ratio"]:
        failures.append(f"Max aspect ratio {max_ar:.1f} exceeds threshold {t['max_aspect_ratio']}")

    min_jac = data.get("min_jacobian", 1.0)
    if min_jac <= t["min_jacobian"]:
        failures.append(f"Min Jacobian {min_jac:.6f} <= 0 (inverted or degenerate cells)")

    has_inverted = data.get("has_inverted_cells", False)
    if has_inverted:
        failures.append("Mesh contains inverted cells")

    return {
        "min_skewness": data.get("min_skewness", 0.0),
        "max_skewness": max_skew,
        "max_aspect_ratio": max_ar,
        "min_jacobian": min_jac,
        "has_inverted_cells": has_inverted,
        "total_cells": data.get("total_cells", 0),
        "passed": len(failures) == 0,
        "failures": failures,
    }


def compute_basic_quality(
    cell_vertices: list[list[list[float]]],
    total_cells: int,
) -> dict[str, Any]:
    """Compute basic mesh quality metrics from cell vertex arrays.

    This is a simplified computation for cases where the mesh tool
    doesn't provide quality metrics natively.

    Args:
        cell_vertices: List of cells, each cell is a list of vertex coordinates [[x,y,z], ...]
        total_cells: Total number of cells

    Returns quality dict suitable for check_mesh_quality input.
    """
    import numpy as np

    skewness_values = []
    ar_values = []
    jac_values = []
    inverted = False

    for verts in cell_vertices:
        pts = np.array(verts)
        if len(pts) < 3:
            continue

        # Aspect ratio: max edge / min edge
        edges = []
        for i in range(len(pts)):
            for j in range(i + 1, len(pts)):
                edges.append(np.linalg.norm(pts[i] - pts[j]))
        if edges:
            max_edge = max(edges)
            min_edge = min(e for e in edges if e > 1e-15)
            if min_edge > 1e-15:
                ar_values.append(max_edge / min_edge)

        # Simplified Jacobian for triangles: 2D area sign
        if len(pts) == 3 and pts.shape[1] >= 2:
            v1 = pts[1, :2] - pts[0, :2]
            v2 = pts[2, :2] - pts[0, :2]
            jac = v1[0] * v2[1] - v1[1] * v2[0]
            jac_values.append(jac)
            if jac <= 0:
                inverted = True

        # Skewness: 1 - (equilateral measure)
        if len(pts) == 3:
            a = np.linalg.norm(pts[1] - pts[0])
            b = np.linalg.norm(pts[2] - pts[1])
            c = np.linalg.norm(pts[0] - pts[2])
            s = (a + b + c) / 2.0
            area = max(0, s * (s - a) * (s - b) * (s - c)) ** 0.5
            optimal_area = (max([a, b, c]) ** 2) * (3 ** 0.5) / 4.0
            if optimal_area > 1e-15:
                skewness_values.append(1.0 - min(1.0, area / optimal_area))

    return {
        "min_skewness": min(skewness_values) if skewness_values else 0.0,
        "max_skewness": max(skewness_values) if skewness_values else 0.0,
        "max_aspect_ratio": max(ar_values) if ar_values else 0.0,
        "min_jacobian": min(jac_values) if jac_values else 1.0,
        "has_inverted_cells": inverted,
        "total_cells": total_cells,
    }
