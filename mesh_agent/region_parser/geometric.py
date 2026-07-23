"""Geometric region parser: box, cylinder, sphere, cone → coordinate bounds."""

from __future__ import annotations

from typing import Any


def parse_box(region: dict[str, Any]) -> dict[str, Any]:
    """Parse a box region into coordinate bounds."""
    c1 = region.get("corner1", [0, 0, 0])
    c2 = region.get("corner2", [1, 1, 1])
    return {
        "type": "box",
        "x_range": (min(c1[0], c2[0]), max(c1[0], c2[0])),
        "y_range": (min(c1[1], c2[1]), max(c1[1], c2[1])),
        "z_range": (min(c1[2], c2[2]) if len(c1) > 2 and len(c2) > 2 else (0, 0)),
        "corner1": c1,
        "corner2": c2,
    }


def parse_cylinder(region: dict[str, Any]) -> dict[str, Any]:
    """Parse a cylinder region."""
    return {
        "type": "cylinder",
        "center": region.get("center", [0, 0, 0]),
        "radius": region.get("radius", 1.0),
        "axis": region.get("axis", "z"),
        "height": region.get("height"),
    }


def parse_sphere(region: dict[str, Any]) -> dict[str, Any]:
    """Parse a sphere region."""
    return {
        "type": "sphere",
        "center": region.get("center", [0, 0, 0]),
        "radius": region.get("radius", 1.0),
    }


def parse_cone(region: dict[str, Any]) -> dict[str, Any]:
    """Parse a cone region."""
    return {
        "type": "cone",
        "apex": region.get("apex", [0, 0, 0]),
        "base_center": region.get("base_center", [0, 0, 1]),
        "base_radius": region.get("base_radius", 1.0),
        "height": region.get("height", 1.0),
    }


def point_in_box(point: list[float], box: dict[str, Any], padding: float = 0.0) -> bool:
    """Check if a 2D/3D point is inside a box region."""
    xr = box["x_range"]
    yr = box["y_range"]
    zr = box.get("z_range", (0, 0))
    return (
        xr[0] - padding <= point[0] <= xr[1] + padding
        and yr[0] - padding <= point[1] <= yr[1] + padding
        and (len(point) < 3 or zr[0] - padding <= point[2] <= zr[1] + padding)
    )


def point_in_cylinder(point: list[float], cyl: dict[str, Any]) -> bool:
    """Check if a point is inside a cylinder."""
    cx, cy = cyl["center"][0], cyl["center"][1]
    r = cyl["radius"]
    dx = point[0] - cx
    dy = point[1] - cy
    return (dx * dx + dy * dy) <= r * r


def point_in_sphere(point: list[float], sph: dict[str, Any]) -> bool:
    """Check if a point is inside a sphere."""
    cx, cy, cz = sph["center"][0], sph["center"][1], sph["center"][2] if len(sph["center"]) > 2 else 0
    r = sph["radius"]
    dx = point[0] - cx
    dy = point[1] - cy
    dz = (point[2] if len(point) > 2 else 0) - cz
    return (dx * dx + dy * dy + dz * dz) <= r * r


GEOMETRIC_PARSERS = {
    "box": parse_box,
    "cylinder": parse_cylinder,
    "sphere": parse_sphere,
    "cone": parse_cone,
}
