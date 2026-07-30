#!/bin/bash
# ==============================================================
# 求解器包装脚本 — Shell 版
# 适用于 Fortran / C / CUDA / 商业软件等非 Python 求解器
#
# Agent 调用方式:
#   ./solver_wrapper.sh --mesh <mesh_dir> --output-dir <output_dir> --params '<json>'
#
# 你需要:
#   1. 从 --mesh 目录读取网格文件
#   2. 用 --params 中的参数运行你的求解器
#   3. 把结果写成 JSON 到 --output-dir
#
# 必须输出:
#   <output-dir>/metrics.json       — 指标值
#   <output-dir>/mesh_quality.json  — (推荐) 网格质量
# ==============================================================

set -e

MESH_DIR=""
OUTPUT_DIR=""
PARAMS="{}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --mesh)       MESH_DIR="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        --params)     PARAMS="$2"; shift 2 ;;
        *) shift ;;
    esac
done

if [ -z "$MESH_DIR" ] || [ -z "$OUTPUT_DIR" ]; then
    echo "Usage: $0 --mesh <dir> --output-dir <dir> --params '<json>'" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# ==============================================================
# TODO: 在这里调用你的求解器
#
# 示例:
#   ./your_solver.exe --input "$MESH_DIR/mesh.dat" --output "$OUTPUT_DIR/"
#
# 或:
#   mpirun -np 4 your_solver "$MESH_DIR" "$OUTPUT_DIR"
# ==============================================================

echo "请在 solver_wrapper.sh 中实现你的求解器调用逻辑" >&2
# exit 1  # 实现后取消这行注释

# ==============================================================
# 示例: 提取指标并写成 metrics.json
# 替换下面的值为你的实际输出
# ==============================================================

cat > "$OUTPUT_DIR/metrics.json" << 'METRICS'
{
  "drag_coefficient": 1.42,
  "lift_coefficient": 0.08,
  "solver_convergence": {
    "converged": true,
    "residual_final": 1e-6,
    "residual_initial": 1.0,
    "iterations": 5000
  }
}
METRICS

cat > "$OUTPUT_DIR/mesh_quality.json" << 'MQ'
{
  "total_cells": 15000,
  "max_skewness": 0.7,
  "max_aspect_ratio": 15.0,
  "min_jacobian": 0.2,
  "has_inverted_cells": false
}
MQ

echo "Solver wrapper completed."
