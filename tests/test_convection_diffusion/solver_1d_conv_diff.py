#!/usr/bin/env python3
"""1D steady convection-diffusion: -u*dT/dx + kappa*d2T/dx2 = 0, T(0)=0, T(1)=1.
Pe = u/kappa. Upwind finite volume. Stable for Pe=50."""

import argparse, json, os, sys
import numpy as np


def read_mesh(mesh_dir):
    """Read mesh.txt: either single column or index<TAB>value format."""
    mesh_file = os.path.join(mesh_dir, "mesh.txt")
    raw = np.loadtxt(mesh_file)
    if raw.ndim == 0:
        raw = np.array([raw])
    if raw.ndim == 2 and raw.shape[1] >= 2:
        return raw[:, 1].astype(float)
    return raw.ravel().astype(float)


def solve(nodes, pe):
    n = len(nodes)
    x = nodes
    h = np.diff(x)
    A = np.zeros((n, n))
    b = np.zeros(n)
    A[0, 0] = 1.0; b[0] = 0.0
    A[n-1, n-1] = 1.0; b[n-1] = 1.0

    kappa = 1.0 / pe
    for i in range(1, n - 1):
        hm = h[i-1]; hp = h[i]
        Aw = (kappa + 1.0) / hm
        Ae = kappa / hp
        A[i, i] = Aw + Ae
        A[i, i-1] = -Aw
        A[i, i+1] = -Ae

    T = np.linalg.solve(A, b)

    # T(x) = (exp(Pe*x) - 1) / (exp(Pe) - 1) ≈ exp(Pe*(x-1)) for large Pe
    T_ana = np.zeros(n)
    for j in range(n):
        d = pe * (x[j] - 1.0)
        if d > 0:  # x > 1
            T_ana[j] = 1.0
        elif d < -50:
            T_ana[j] = 0.0
        else:
            # Exact: T = (exp(Pe*x) - 1) / (exp(Pe) - 1)
            ex = np.exp(pe * x[j])
            ed = np.exp(pe)
            T_ana[j] = (ex - 1.0) / (ed - 1.0)

    return {
        "max_error": float(np.max(np.abs(T - T_ana))),
        "l2_error": float(np.sqrt(np.trapz((T - T_ana)**2, x))),
        "n_nodes": n,
        "min_cell_size": float(np.min(h)),
        "max_cell_size": float(np.max(h)),
        "peclet_number": pe,
        "solver_convergence": {
            "converged": True,
            "residual_final": float(np.max(np.abs(A @ T - b))),
            "residual_initial": 1.0,
            "iterations": 1,
        },
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mesh", required=True)
    p.add_argument("--output-dir", required=True)
    p.add_argument("--params", default="{}")
    args = p.parse_args()

    params = json.loads(args.params)
    pe = float(params.get("peclet", 50))

    nodes = read_mesh(args.mesh)
    metrics = solve(nodes, pe)

    os.makedirs(args.output_dir, exist_ok=True)
    with open(os.path.join(args.output_dir, "metrics.json"), "w") as f:
        json.dump(metrics, f, indent=2)

    mq = {
        "total_cells": len(nodes) - 1,
        "total_nodes": len(nodes),
        "min_cell_size": metrics["min_cell_size"],
        "max_cell_size": metrics["max_cell_size"],
        "range": [float(nodes[0]), float(nodes[-1])],
        "max_skewness": 0.0, "min_skewness": 0.0,
        "max_aspect_ratio": metrics["max_cell_size"] / max(metrics["min_cell_size"], 1e-30),
        "min_jacobian": 1.0,
        "has_inverted_cells": False,
    }
    with open(os.path.join(args.output_dir, "mesh_quality.json"), "w") as f:
        json.dump(mq, f, indent=2)

    print(f"Solver: Pe={pe}, n={len(nodes)}, max_err={metrics['max_error']:.6e}, l2_err={metrics['l2_error']:.6e}")
    sys.exit(0)


if __name__ == "__main__":
    main()
