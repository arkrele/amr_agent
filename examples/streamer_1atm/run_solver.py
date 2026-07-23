#!/usr/bin/env python3
"""Python wrapper for the CUDA plasma streamer solver.

Implements the mesh-agent solver interface contract:
    python run_solver.py --mesh-dir <path> --output-dir <path> --params <json>

The solver is a 2D axisymmetric plasma streamer simulation (CUDA).
Mesh is stored as two 1D coordinate arrays:
    - mesh_z2.dat: axial coordinate (index<TAB>value_meters)
    - mesh_r2_0624.dat: radial coordinate (index<TAB>value_meters)

Physics landmark coordinates are in mesh_physics.cfg.

Usage:
    python run_solver.py --mesh-dir ./inputdata --output-dir ./output --params '{"max_timesteps": 5000}'
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Any


MESH_FILES = ["mesh_z2.dat", "mesh_r2_0624.dat"]
PHYSICS_CFG = "mesh_physics.cfg"
REQUIRED_INPUTS = [
    # Bolsig data
    "Bolsig_Data/O2_20p.dat",
    # Initial conditions
    "Initial/initial_N2_80p_300K.dat",
    # Reaction data
    "i_reaction_300K_modmod0516.dat",
    "e_reaction_mod1803.dat",
    "n_reaction_modmod.dat",
    # Voltage waveform
    "V_Ono_single_str.dat",
    # Cross-section data
    "ion_co.dat",
    "PowerEdep.dat",
    "c1.dat", "c2.dat", "c3.dat",
    "dEv_H2O.dat", "dEv_N2.dat", "dEv_O2.dat",
    "sum_dE(m-1)_0.dat", "sum_dE(m-1)_1.dat", "sum_dE(m-1)_3.dat",
    "V_Ono_single_str2.dat", "V_toda_20k.dat",
]


def read_mesh_coords(mesh_dir: Path, filename: str) -> tuple[list[int], list[float]]:
    """Read a mesh coordinate file (index<TAB>value format)."""
    indices = []
    values = []
    with open(mesh_dir / filename, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) >= 2:
                indices.append(int(parts[0]))
                values.append(float(parts[1]))
    return indices, values


def read_physics_cfg(mesh_dir: Path) -> dict[str, float]:
    """Read mesh_physics.cfg into a dict of {key: value}."""
    cfg = {}
    cfg_path = mesh_dir / PHYSICS_CFG
    if cfg_path.exists():
        with open(cfg_path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                parts = line.split("=")
                if len(parts) == 2:
                    cfg[parts[0].strip()] = float(parts[1].strip())
    return cfg


def get_mesh_quality(mesh_dir: Path) -> dict[str, Any]:
    """Compute mesh quality metrics for 1D structured grid."""
    quality = {
        "total_cells": 0,
        "total_nodes": 0,
        "min_cell_size_z": None,
        "max_cell_size_z": None,
        "min_cell_size_r": None,
        "max_cell_size_r": None,
        "expansion_ratio_max_z": None,
        "expansion_ratio_max_r": None,
        "regions": {},
        "warnings": [],
    }

    for dim, fname in [("z", "mesh_z2.dat"), ("r", "mesh_r2_0624.dat")]:
        try:
            _, values = read_mesh_coords(mesh_dir, fname)
        except Exception:
            quality["warnings"].append(f"Cannot read {fname}")
            continue

        if len(values) < 2:
            quality["warnings"].append(f"{fname} has < 2 points")
            continue

        cell_sizes = [values[i + 1] - values[i] for i in range(len(values) - 1)]

        quality[f"nodes_{dim}"] = len(values)
        quality[f"cells_{dim}"] = len(cell_sizes)
        quality[f"min_cell_size_{dim}"] = min(cell_sizes)
        quality[f"max_cell_size_{dim}"] = max(cell_sizes)

        # Expansion ratio: cell_size[i+1] / cell_size[i]
        ratios = []
        for i in range(len(cell_sizes) - 1):
            if cell_sizes[i] > 1e-30:
                ratios.append(cell_sizes[i + 1] / cell_sizes[i])
        if ratios:
            quality[f"expansion_ratio_max_{dim}"] = max(max(ratios), 1.0 / min(r for r in ratios if r > 1e-30) if any(r > 1e-30 for r in ratios) else 1.0)

        # First/last values
        quality[f"range_{dim}"] = [values[0], values[-1]]

    quality["total_cells"] = quality.get("cells_z", 0) * quality.get("cells_r", 0)
    quality["total_nodes"] = quality.get("nodes_z", 0) * quality.get("nodes_r", 0)

    # Read physics landmark regions
    cfg = read_physics_cfg(mesh_dir)
    if cfg:
        quality["regions"] = cfg

    return quality


def collect_metrics(output_dir: Path) -> dict[str, Any]:
    """Extract key metrics from solver output files."""
    metrics: dict[str, Any] = {
        "_success": True,
    }

    # Current data
    current_file = output_dir / "current" / "current.dat"
    if current_file.exists():
        try:
            currents = []
            with open(current_file, "r", encoding="utf-8") as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) >= 4:
                        currents.append(float(parts[3]))  # Total current column
            if currents:
                metrics["peak_current"] = max(abs(c) for c in currents)
                metrics["final_current"] = currents[-1]
                metrics["current_timesteps"] = len(currents)
        except Exception:
            pass

    # Power data
    power_file = output_dir / "current" / "Power.dat"
    if power_file.exists():
        try:
            with open(power_file, "r", encoding="utf-8") as f:
                for line in f:
                    parts = line.strip().split()
                    if parts:
                        metrics["peak_power"] = float(parts[0])
                        break
        except Exception:
            pass

    # Count output files
    for subdir in output_dir.iterdir():
        if subdir.is_dir():
            count = len(list(subdir.glob("*")))
            if count > 0:
                metrics[f"output_files_{subdir.name}"] = count

    # Check for convergence flags
    priflag = Path("Priflag.dat")
    priflag2 = Path("Priflag2.dat")
    for flag_name, flag_path in [("priflag", priflag), ("priflag2", priflag2)]:
        if flag_path.exists():
            try:
                val = int(flag_path.read_text(encoding="utf-8").strip())
                metrics[flag_name] = val
            except Exception:
                pass

    return metrics


def run_solver(
    solver_dir: Path,
    mesh_dir: Path,
    output_dir: Path,
    params: dict[str, Any],
) -> dict[str, Any]:
    """Run the CUDA streamer solver.

    Steps:
    1. Sync mesh files from mesh_dir to solver's inputdata/
    2. Optionally modify NR/NZ in streamer.cu if mesh density changed
    3. make && ./a.out
    4. Collect output metrics
    """
    output_dir.mkdir(parents=True, exist_ok=True)

    # Ensure outputdata subdirectories exist
    output_subdirs = [
        "2DCphi", "2DE", "2DEx", "2DEy", "2DLphi", "2DN2Bp", "2DN2C",
        "2DSph", "2DT", "2Ddens", "2Dne", "2Dp", "2Dphi", "2Dpht",
        "Diff", "Ey_ne", "Ir", "Particle", "Refile2",
        "Streak_Ey_e_cu", "Streak_particle", "Streak_particle_1D",
        "csv", "current", "velo",
    ]
    for sub in output_subdirs:
        (output_dir / sub).mkdir(parents=True, exist_ok=True)

    # Step 1: Sync mesh files into solver's inputdata/
    solver_input = solver_dir / "inputdata"
    solver_input.mkdir(parents=True, exist_ok=True)

    for fname in MESH_FILES:
        src = mesh_dir / fname
        dst = solver_input / fname
        if src.exists():
            shutil.copy2(str(src), str(dst))

    cfg_src = mesh_dir / PHYSICS_CFG
    cfg_dst = solver_input / PHYSICS_CFG
    if cfg_src.exists():
        shutil.copy2(str(cfg_src), str(cfg_dst))

    # Step 2: Update NR/NZ in streamer.cu if mesh size changed
    streamer_cu = solver_dir / "streamer.cu"
    z_idx, z_vals = read_mesh_coords(mesh_dir, "mesh_z2.dat")
    r_idx, r_vals = read_mesh_coords(mesh_dir, "mesh_r2_0624.dat")
    new_nz = len(z_vals)
    new_nr = len(r_vals)

    source = streamer_cu.read_text(encoding="utf-8")
    import re
    source = re.sub(r"#define NR\s+\d+", f"#define NR {new_nr}", source)
    source = re.sub(r"#define NZ\s+\d+", f"#define NZ {new_nz}", source)
    streamer_cu.write_text(source, encoding="utf-8")

    # Step 3: Build
    max_timesteps = params.get("max_timesteps", 5000)
    timeout = params.get("_timeout", 7200)  # 2 hour default for CUDA solver

    build_result = subprocess.run(
        ["make", "-C", str(solver_dir)],
        capture_output=True, text=True, timeout=120,
        env={**__import__("os").environ, "PATH": __import__("os").environ.get("PATH", "")},
    )
    if build_result.returncode != 0:
        return {
            "_success": False,
            "_error": f"Build failed (rc={build_result.returncode})",
            "_stdout": build_result.stdout[-2000:],
            "_stderr": build_result.stderr[-2000:],
        }

    # Step 4: Run
    try:
        run_result = subprocess.run(
            [str(solver_dir / "a.out")],
            cwd=str(solver_dir),
            capture_output=True, text=True, timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return {
            "_success": False,
            "_error": f"Solver timed out after {timeout}s",
        }

    if run_result.returncode != 0:
        return {
            "_success": False,
            "_error": f"Solver exited with code {run_result.returncode}",
            "_stdout": run_result.stdout[-2000:],
            "_stderr": run_result.stderr[-2000:],
        }

    # Step 5: Collect output
    solver_output_data = solver_dir / "outputdata"
    if solver_output_data.exists():
        for sub in output_subdirs:
            src_sub = solver_output_data / sub
            dst_sub = output_dir / sub
            if src_sub.exists():
                for f in src_sub.iterdir():
                    if f.is_file():
                        shutil.copy2(str(f), str(dst_sub / f.name))

    # Also copy root-level output files
    for pattern in ["Production_rate_*.dat", "Priflag*.dat"]:
        for f in solver_dir.glob(pattern):
            shutil.copy2(str(f), str(output_dir / f.name))

    metrics = collect_metrics(output_dir)
    metrics["_success"] = True
    metrics["_stdout"] = run_result.stdout[-2000:]
    metrics["_stderr"] = run_result.stderr[-2000:]
    metrics["solver"] = {
        "nr": new_nr,
        "nz": new_nz,
        "total_cells": new_nr * new_nz,
        "total_nodes": new_nr * new_nz,
    }

    return metrics


def main():
    parser = argparse.ArgumentParser(description="CUDA Plasma Streamer Solver Wrapper")
    parser.add_argument("--mesh-dir", required=True, help="Path to directory with mesh input files")
    parser.add_argument("--output-dir", required=True, help="Path to output directory")
    parser.add_argument("--params", default="{}", help="JSON string of solver parameters")
    args = parser.parse_args()

    try:
        params = json.loads(args.params)
    except json.JSONDecodeError:
        print("Error: --params must be valid JSON", file=sys.stderr)
        sys.exit(1)

    solver_dir = Path(__file__).resolve().parent
    mesh_dir = Path(args.mesh_dir).resolve()
    output_dir = Path(args.output_dir).resolve()

    metrics = run_solver(
        solver_dir=solver_dir,
        mesh_dir=mesh_dir,
        output_dir=output_dir,
        params=params,
    )

    # Write metrics.json
    output_dir.mkdir(parents=True, exist_ok=True)
    with open(output_dir / "metrics.json", "w", encoding="utf-8") as f:
        json.dump(metrics, f, indent=2, default=str)

    # Also write mesh quality report
    mesh_quality = get_mesh_quality(mesh_dir)
    with open(output_dir / "mesh_quality.json", "w", encoding="utf-8") as f:
        json.dump(mesh_quality, f, indent=2)

    if metrics.get("_success"):
        print(f"Solver completed successfully. NR={metrics['solver']['nr']}, NZ={metrics['solver']['nz']}")
        sys.exit(0)
    else:
        print(f"Solver failed: {metrics.get('_error', 'Unknown error')}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
