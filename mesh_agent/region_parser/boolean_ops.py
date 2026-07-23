"""Boolean region operations: union, intersection, difference."""

from __future__ import annotations

from typing import Any

import numpy as np


def union(region_a: dict[str, Any], region_b: dict[str, Any]) -> dict[str, Any]:
    """Union of two parsed regions (points in either A or B)."""
    a_mask = _get_mask(region_a)
    b_mask = _get_mask(region_b)

    if a_mask is not None and b_mask is not None:
        mask = a_mask | b_mask
    elif a_mask is not None:
        mask = a_mask
    elif b_mask is not None:
        mask = b_mask
    else:
        mask = None

    return {
        "type": "boolean_union",
        "a": region_a,
        "b": region_b,
        "mask": mask,
    }


def intersection(region_a: dict[str, Any], region_b: dict[str, Any]) -> dict[str, Any]:
    """Intersection of two parsed regions (points in BOTH A and B)."""
    a_mask = _get_mask(region_a)
    b_mask = _get_mask(region_b)

    if a_mask is not None and b_mask is not None:
        mask = a_mask & b_mask
    else:
        mask = None

    return {
        "type": "boolean_intersection",
        "a": region_a,
        "b": region_b,
        "mask": mask,
    }


def difference(region_a: dict[str, Any], region_b: dict[str, Any]) -> dict[str, Any]:
    """Difference: points in A but NOT in B."""
    a_mask = _get_mask(region_a)
    b_mask = _get_mask(region_b)

    if a_mask is not None and b_mask is not None:
        mask = a_mask & ~b_mask
    elif a_mask is not None:
        mask = a_mask
    else:
        mask = None

    return {
        "type": "boolean_difference",
        "a": region_a,
        "b": region_b,
        "mask": mask,
    }


def _get_mask(region: dict[str, Any]) -> np.ndarray | None:
    """Extract boolean mask from a parsed region if available."""
    return region.get("mask") or region.get("indices")


BOOLEAN_OPS = {
    "union": union,
    "intersection": intersection,
    "difference": difference,
}
