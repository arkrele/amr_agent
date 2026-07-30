#!/usr/bin/env python3
"""
求解器模板 — 复制此文件，实现你的求解逻辑。

Agent 调用方式:
    python solver.py --mesh <mesh_dir> --output-dir <output_dir> --params '<json>'

你必须:
    1. 从 --mesh 目录读取网格文件
    2. 用 --params 中的参数运行求解器
    3. 将结果写入 --output-dir

必须输出到 --output-dir 的文件:
    metrics.json       — 目标指标值
    mesh_quality.json  — (推荐) 网格质量报告
"""

import argparse
import json
import os
import sys


def read_mesh(mesh_dir: str) -> dict:
    """
    从 --mesh 目录读取网格。
    根据你的网格格式实现此函数。
    返回你的网格数据结构。
    """
    # TODO: 实现你的网格读取逻辑
    # mesh_file = os.path.join(mesh_dir, "your_mesh_file.msh")
    # ...读取...
    return {}


def run_solver(mesh_data: dict, params: dict, output_dir: str) -> dict:
    """
    运行求解器，返回指标字典。

    params 包含 problem.yaml 中 solver.params 的内容
    （Agent 可能在运行过程中修改它们）。

    返回的字典必须包含以下内容（Agent 用于后续分析）:
      - 目标指标（如 drag_coefficient, lift_coefficient）
      - solver_convergence 子对象
    """
    # TODO: 实现你的求解逻辑
    # ...求解...

    return {
        # 目标指标 — 替换为你的实际输出
        "drag_coefficient": 1.42,
        "lift_coefficient": 0.08,
        "strouhal_number": 0.16,

        # 收敛信息 — 必须包含
        "solver_convergence": {
            "converged": True,
            "residual_final": 1e-6,
            "residual_initial": 1.0,
            "iterations": 5000,
        },
    }


def mesh_quality(mesh_data: dict) -> dict:
    """
    (推荐) 计算网格质量指标。
    Agent 用它做 POST-Gate 验证。
    """
    # TODO: 实现网格质量计算
    return {
        "total_cells": 15000,
        "total_nodes": 16000,
        "max_skewness": 0.7,
        "max_aspect_ratio": 15.0,
        "min_jacobian": 0.2,
        "has_inverted_cells": False,
    }


# ==============================================================
# 以下内容通常不需要修改
# ==============================================================

def main():
    parser = argparse.ArgumentParser(description="Mesh Agent Solver")
    parser.add_argument("--mesh", required=True, help="Path to mesh directory")
    parser.add_argument("--output-dir", required=True, help="Path to output directory")
    parser.add_argument("--params", default="{}", help="JSON string of solver parameters")
    args = parser.parse_args()

    # Parse params
    try:
        params = json.loads(args.params)
    except json.JSONDecodeError:
        print(f"Error: --params must be valid JSON", file=sys.stderr)
        sys.exit(1)

    # Read mesh
    mesh_data = read_mesh(args.mesh)

    # Run solver
    metrics = run_solver(mesh_data, params, args.output_dir)

    # Write outputs
    os.makedirs(args.output_dir, exist_ok=True)

    with open(os.path.join(args.output_dir, "metrics.json"), "w", encoding="utf-8") as f:
        json.dump(metrics, f, indent=2)

    try:
        quality = mesh_quality(mesh_data)
        with open(os.path.join(args.output_dir, "mesh_quality.json"), "w", encoding="utf-8") as f:
            json.dump(quality, f, indent=2)
    except Exception:
        pass  # mesh_quality is optional

    print("Solver completed successfully.")
    sys.exit(0)


if __name__ == "__main__":
    main()
