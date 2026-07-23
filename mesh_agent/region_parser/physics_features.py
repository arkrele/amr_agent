"""Physics feature region parser: boundary layer, wake, shock, separation bubble."""

from __future__ import annotations

from typing import Any

import numpy as np


def parse_boundary_layer(
    region: dict[str, Any],
    surface_coords: np.ndarray | None = None,
    surface_normals: np.ndarray | None = None,
) -> dict[str, Any]:
    """Parse a boundary layer region specification.

    The LLM provides semantic parameters; this function computes
    the actual coordinate ranges for mesh generation.
    """
    on_surface = region.get("on_surface", {})
    first_layer = region.get("first_layer_height", 1e-5)
    growth_rate = region.get("growth_rate", 1.15)
    num_layers = region.get("num_layers", 15)
    normal_dir = region.get("normal_direction", "wall_normal")

    # Compute total boundary layer thickness
    total_height = first_layer * (1 - growth_rate ** num_layers) / (1 - growth_rate)

    return {
        "type": "boundary_layer",
        "on_surface": on_surface,
        "first_layer_height": first_layer,
        "growth_rate": growth_rate,
        "num_layers": num_layers,
        "total_height": total_height,
        "normal_direction": normal_dir,
        "surface_coords": surface_coords,
        "surface_normals": surface_normals,
    }


def parse_wake(
    region: dict[str, Any],
    body_center: np.ndarray | None = None,
) -> dict[str, Any]:
    """Parse a wake region specification."""
    from_body = region.get("from_body", {})
    direction = np.array(region.get("direction", [1.0, 0.0, 0.0]))
    length = region.get("length", 20.0)
    spread_angle = np.radians(region.get("spread_angle", 5.0))

    # Normalize direction
    norm = np.linalg.norm(direction)
    if norm > 1e-10:
        direction = direction / norm

    # Compute spread width at end
    end_width = length * np.tan(spread_angle) * 2

    return {
        "type": "wake",
        "from_body": from_body,
        "direction": direction.tolist(),
        "length": length,
        "spread_angle_rad": spread_angle,
        "end_width": end_width,
        "body_center": body_center.tolist() if body_center is not None else None,
    }


def parse_shock(
    region: dict[str, Any],
    field_data: dict[str, np.ndarray] | None = None,
) -> dict[str, Any]:
    """Parse a shock region specification.

    For compressible flows, the shock location can be estimated
    from the pressure gradient field.
    """
    from_field = region.get("from_field", "pressure")
    normal_thickness = region.get("normal_thickness", 0.001)
    tangential_width = region.get("tangential_width", 0.5)

    return {
        "type": "shock",
        "from_field": from_field,
        "normal_thickness": normal_thickness,
        "tangential_width": tangential_width,
        "field_data": field_data,
    }


def parse_separation_bubble(
    region: dict[str, Any],
    surface_coords: np.ndarray | None = None,
) -> dict[str, Any]:
    """Parse a separation bubble region specification."""
    on_surface = region.get("on_surface", {})
    estimated_length = region.get("estimated_length", 0.1)

    return {
        "type": "separation_bubble",
        "on_surface": on_surface,
        "estimated_length": estimated_length,
        "surface_coords": surface_coords,
    }


PHYSICS_PARSERS = {
    "boundary_layer": parse_boundary_layer,
    "wake": parse_wake,
    "shock": parse_shock,
    "separation_bubble": parse_separation_bubble,
}
