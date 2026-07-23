"""Region parser: dispatches region specifications to appropriate parsers.

For Phase 1, handles: box, cylinder, sphere, cone (geometric),
gradient_threshold, value_range, residual (field-driven),
boundary_layer, wake, shock, separation_bubble (physics features),
boolean operations (union, intersection, difference).
"""

from __future__ import annotations

from typing import Any

import numpy as np

from mesh_agent.region_parser.geometric import GEOMETRIC_PARSERS
from mesh_agent.region_parser.field_driven import (
    parse_gradient_threshold,
    parse_value_range,
    parse_residual,
)
from mesh_agent.region_parser.physics_features import PHYSICS_PARSERS
from mesh_agent.region_parser.boolean_ops import BOOLEAN_OPS


def parse_region(
    region_spec: dict[str, Any],
    field_data: dict[str, np.ndarray] | None = None,
    mesh_coords: np.ndarray | None = None,
    surface_coords: np.ndarray | None = None,
    surface_normals: np.ndarray | None = None,
    residual_data: np.ndarray | None = None,
) -> dict[str, Any]:
    """Parse a region specification into concrete coordinates/ranges.

    This is the main entry point for the region parser system.
    Takes a region spec dict (as output by the Strategist LLM) and
    resolves it into a concrete representation suitable for mesh generation.

    Args:
        region_spec: Region specification dict with at minimum a "type" key
        field_data: Dict of field name → numpy array of cell values
        mesh_coords: (N, 3) array of cell center coordinates
        surface_coords: Coordinates of tagged surfaces
        surface_normals: Surface normal vectors
        residual_data: Per-cell residual values

    Returns:
        Parsed region dict with concrete coordinates/masks
    """
    region_type = region_spec.get("type", "box")

    # Geometric regions
    if region_type in GEOMETRIC_PARSERS:
        return GEOMETRIC_PARSERS[region_type](region_spec)

    # Field-driven regions
    if region_type == "gradient_threshold" and field_data is not None and mesh_coords is not None:
        return parse_gradient_threshold(region_spec, field_data, mesh_coords)
    if region_type == "value_range" and field_data is not None and mesh_coords is not None:
        return parse_value_range(region_spec, field_data, mesh_coords)
    if region_type == "residual" and residual_data is not None and mesh_coords is not None:
        return parse_residual(region_spec, residual_data, mesh_coords)

    # Physics feature regions
    if region_type in PHYSICS_PARSERS:
        parser = PHYSICS_PARSERS[region_type]
        kwargs = {}
        if region_type == "boundary_layer":
            kwargs["surface_coords"] = surface_coords
            kwargs["surface_normals"] = surface_normals
        elif region_type == "separation_bubble":
            kwargs["surface_coords"] = surface_coords
        elif region_type == "shock":
            kwargs["field_data"] = field_data
        return parser(region_spec, **kwargs)

    # Boolean operations
    if region_type == "boolean":
        op = region_spec.get("operation", "union")
        if op in BOOLEAN_OPS:
            a = parse_region(region_spec["a"], field_data, mesh_coords, surface_coords, surface_normals, residual_data)
            b = parse_region(region_spec["b"], field_data, mesh_coords, surface_coords, surface_normals, residual_data)
            return BOOLEAN_OPS[op](a, b)

    # Tag references — pass through for mesh generator to resolve
    if region_type in ("tag", "tag_range", "topology"):
        return dict(region_spec)

    # Distance from region — simplified: pass through
    if region_type == "distance":
        inner = parse_region(
            region_spec.get("from_region", {}),
            field_data, mesh_coords, surface_coords, surface_normals, residual_data,
        )
        return {"type": "distance", "from_region": inner, "distance": region_spec.get("distance", 0.05)}

    # Unknown types: pass through
    return dict(region_spec)
