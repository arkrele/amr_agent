"""Field-driven region parser: uses solution data to identify regions."""

from __future__ import annotations

from typing import Any

import numpy as np


def parse_gradient_threshold(
    region: dict[str, Any],
    field_data: dict[str, np.ndarray],
    mesh_coords: np.ndarray,
) -> dict[str, Any]:
    """Find cells where the gradient of `field` exceeds `threshold`.

    Returns a mask over cells and the bounding region.
    """
    field_name = region.get("field", "")
    threshold = region.get("threshold", 0.0)
    operator = region.get("operator", "greater_than")
    padding = region.get("padding_cells", 0)

    if field_name not in field_data:
        return {"type": "gradient_threshold", "indices": np.array([], dtype=int), "bounding_box": None}

    data = field_data[field_name]
    grad = _compute_gradient(data, mesh_coords)

    if operator == "greater_than":
        mask = grad > threshold
    elif operator == "less_than":
        mask = grad < threshold
    else:
        mask = np.abs(grad) > threshold

    indices = np.where(mask)[0]
    bbox = _compute_bounding_box(mesh_coords, indices)

    return {
        "type": "gradient_threshold",
        "field": field_name,
        "indices": indices,
        "bounding_box": bbox,
        "num_cells": len(indices),
    }


def parse_value_range(
    region: dict[str, Any],
    field_data: dict[str, np.ndarray],
    mesh_coords: np.ndarray,
) -> dict[str, Any]:
    """Find cells where field value is within [min, max]."""
    field_name = region.get("field", "")
    min_val = region.get("min")
    max_val = region.get("max")
    padding = region.get("padding_cells", 0)

    if field_name not in field_data:
        return {"type": "value_range", "indices": np.array([], dtype=int), "bounding_box": None}

    data = field_data[field_name]
    mask = np.ones(len(data), dtype=bool)

    if min_val is not None:
        mask &= data >= min_val
    if max_val is not None:
        mask &= data <= max_val

    indices = np.where(mask)[0]
    bbox = _compute_bounding_box(mesh_coords, indices)

    return {
        "type": "value_range",
        "field": field_name,
        "min": min_val,
        "max": max_val,
        "indices": indices,
        "bounding_box": bbox,
        "num_cells": len(indices),
    }


def parse_residual(
    region: dict[str, Any],
    residual_data: np.ndarray,
    mesh_coords: np.ndarray,
) -> dict[str, Any]:
    """Find cells with high residual."""
    threshold = region.get("threshold", 1e-3)
    mask = residual_data > threshold
    indices = np.where(mask)[0]
    bbox = _compute_bounding_box(mesh_coords, indices)

    return {
        "type": "residual",
        "threshold": threshold,
        "indices": indices,
        "bounding_box": bbox,
        "num_cells": len(indices),
    }


def _compute_gradient(data: np.ndarray, coords: np.ndarray) -> np.ndarray:
    """Compute cell-wise gradient magnitude using finite differences."""
    grad = np.zeros(len(data))
    for i in range(len(data)):
        # Find neighbors based on shared faces (simplified: coordinate distance)
        ci = coords[i]
        dists = np.linalg.norm(coords - ci, axis=1)
        # Get closest neighbors (excluding self)
        neighbor_idx = np.argsort(dists)[1:min(5, len(dists))]
        if len(neighbor_idx) == 0:
            continue
        diffs = np.abs(data[neighbor_idx] - data[i])
        max_dist = max(dists[neighbor_idx].max(), 1e-15)
        grad[i] = diffs.max() / max_dist
    return grad


def _compute_bounding_box(
    coords: np.ndarray, indices: np.ndarray,
) -> dict[str, Any] | None:
    """Compute bounding box for a set of cell indices."""
    if len(indices) == 0:
        return None
    subset = coords[indices]
    return {
        "x_range": (float(subset[:, 0].min()), float(subset[:, 0].max())),
        "y_range": (float(subset[:, 1].min()), float(subset[:, 1].max())),
        "z_range": (float(subset[:, 2].min()), float(subset[:, 2].max())) if subset.shape[1] > 2 else (0.0, 0.0),
    }
