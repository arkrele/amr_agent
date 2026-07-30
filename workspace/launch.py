#!/usr/bin/env python3
"""
Mesh Agent 一键启动脚本

用法:
  python launch.py                              # 交互式
  python launch.py --prompt "圆柱绕流 Re=100"    # 命令行
  python launch.py --config my_problem.yaml     # 直接用已有配置

首次使用会自动从模板创建 solver.py。
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parent
PROJECT_ROOT = WORKSPACE.parent
TEMPLATES = {
    "problem": WORKSPACE / "problem.yaml",
    "solver": WORKSPACE / "solver_template.py",
}
USER_SOLVER = WORKSPACE / "solver.py"
MESH_DIR = WORKSPACE / "mesh"


def ensure_env() -> bool:
    """检查环境是否就绪。"""
    env_file = PROJECT_ROOT / ".env"
    if not env_file.exists():
        print("[错误] 未找到 .env 文件")
        print(f"  请在 {PROJECT_ROOT} 创建 .env:")
        print("  OPENAI_API_KEY=your-key")
        print("  OPENAI_BASE_URL=https://api.deepseek.com")
        return False
    content = env_file.read_text(encoding="utf-8")
    if "your_api_key_here" in content or "your-key" in content:
        print("[警告] .env 中似乎还是示例 key，请先填入真实的 API key")
        return False
    return True


def ensure_solver() -> bool:
    """如果用户没写 solver.py，从模板复制。"""
    if USER_SOLVER.exists():
        return True

    template = TEMPLATES["solver"]
    if not template.exists():
        print(f"[错误] 求解器模板不存在: {template}")
        return False

    shutil.copy(str(template), str(USER_SOLVER))
    print(f"[提示] 已从模板创建 solver.py，请实现 read_mesh() 和 run_solver()")
    return True


def ensure_mesh() -> bool:
    """确保 mesh 目录存在且非空。"""
    MESH_DIR.mkdir(exist_ok=True)
    has_files = any(MESH_DIR.iterdir())
    if not has_files:
        print("[警告] mesh/ 目录为空，请放入初始网格文件")
    return True


def write_problem(description: str, pde_type: str = "user-defined",
                  solver_path: str = "./solver.py", max_cells: int = 100000,
                  max_runs: int = 5, max_time: int = 120,
                  metrics: str = "error") -> Path:
    """根据用户输入生成 problem.yaml。"""
    import yaml

    target_metrics = [m.strip() for m in metrics.split(",")]

    config = {
        "problem": {
            "description": description,
            "pde_type": pde_type,
            "geometry": "2d",
            "boundary_conditions": {},
        },
        "solver": {
            "type": "user_template" if solver_path else "agent_generated",
            "path": solver_path,
            "interface": "generic_cli",
            "params": {},
        },
        "mesh": {
            "initial": "./mesh",
            "format": "gmsh",
            "max_cells": max_cells,
        },
        "budget": {
            "max_solver_runs": max_runs,
            "max_wall_time_minutes": max_time,
        },
        "output": {
            "target_metrics": target_metrics,
            "convergence_tolerance": 0.01,
            "output_dir": "./output",
        },
    }

    config_path = WORKSPACE / "problem_当前任务.yaml"
    with open(config_path, "w", encoding="utf-8") as f:
        yaml.dump(config, f, allow_unicode=True, default_flow_style=False)

    print(f"[生成] 问题配置已写入: {config_path.name}")
    return config_path


def interactive() -> dict:
    """交互式问答。"""
    print("=" * 60)
    print("  Mesh Agent — 交互式启动")
    print("=" * 60)
    print()

    desc_lines = []
    print("请输入问题描述 (输入空行结束):")
    while True:
        line = input("  > ")
        if not line:
            break
        desc_lines.append(line)

    if not desc_lines:
        print("使用默认描述: 二维圆柱绕流 Re=100")
        desc_lines = ["二维圆柱绕流，Re=100，不可压缩层流。",
                      "预计圆柱后方形成卡门涡街，需要在尾流区和边界层加密。"]

    description = " ".join(desc_lines)

    pde = input("PDE 类型 [user-defined]: ").strip() or "user-defined"

    use_solver = input("已有求解器? (y/n) [y]: ").strip().lower()
    solver_path = "./solver.py" if use_solver != "n" else ""

    max_cells = input("网格单元数上限 [100000]: ").strip()
    max_cells = int(max_cells) if max_cells else 100000

    max_runs = input("求解器最多运行次数 [5]: ").strip()
    max_runs = int(max_runs) if max_runs else 5

    max_time = input("最长运行时间(分钟) [120]: ").strip()
    max_time = int(max_time) if max_time else 120

    metrics = input("目标指标 (逗号分隔) [drag_coefficient, lift_coefficient]: ").strip()
    metrics = metrics or "drag_coefficient, lift_coefficient"

    return {
        "description": description,
        "pde_type": pde,
        "solver_path": solver_path,
        "max_cells": max_cells,
        "max_runs": max_runs,
        "max_time": max_time,
        "metrics": metrics,
    }


def run_agent(config_path: str, memory: bool = True) -> int:
    """执行 mesh-agent。"""
    cmd = [
        sys.executable, "-m", "mesh_agent.cli", "run",
        "-p", config_path,
        "-o", str(WORKSPACE / "output"),
    ]
    if memory:
        cmd.extend(["-m", str(WORKSPACE / "memory")])

    print(f"\n[运行] {' '.join(cmd)}\n")
    return subprocess.run(cmd, cwd=str(PROJECT_ROOT)).returncode


def main():
    parser = argparse.ArgumentParser(description="Mesh Agent 一键启动")
    parser.add_argument("--prompt", "-p", type=str, help="问题描述")
    parser.add_argument("--config", "-c", type=str, help="直接使用已有的 problem.yaml")
    parser.add_argument("--pde", type=str, default="user-defined")
    parser.add_argument("--max-cells", type=int, default=100000)
    parser.add_argument("--max-runs", type=int, default=5)
    parser.add_argument("--max-time", type=int, default=120)
    parser.add_argument("--metrics", type=str, default="drag_coefficient,lift_coefficient")
    parser.add_argument("--no-solver", action="store_true", help="Agent 自己写求解器")
    parser.add_argument("--no-memory", action="store_true", help="不启用跨会话记忆")
    args = parser.parse_args()

    os.chdir(str(WORKSPACE))

    # 1. 检查环境
    if not ensure_env():
        sys.exit(1)

    # 2. 确定配置文件
    if args.config:
        config_path = args.config
        if not Path(config_path).exists():
            print(f"[错误] 配置文件不存在: {config_path}")
            sys.exit(1)
        print(f"[使用] 已有配置: {config_path}")
    else:
        if args.prompt:
            params = {
                "description": args.prompt,
                "pde_type": args.pde,
                "solver_path": "" if args.no_solver else "./solver.py",
                "max_cells": args.max_cells,
                "max_runs": args.max_runs,
                "max_time": args.max_time,
                "metrics": args.metrics,
            }
        else:
            params = interactive()

        config_path = str(write_problem(**params))

    # 3. 准备求解器和网格
    if not args.no_solver and not args.config:
        ensure_solver()
    ensure_mesh()

    # 4. 运行
    print()
    print("=" * 60)
    print("  开始运行...")
    print(f"  配置: {config_path}")
    print(f"  输出: {WORKSPACE / 'output'}")
    print("=" * 60)
    print()

    rc = run_agent(config_path, memory=not args.no_memory)

    print()
    print("=" * 60)
    if rc == 0:
        print("  运行完成！")
    else:
        print(f"  运行异常 (退出码: {rc})")
    print(f"  结果: {WORKSPACE / 'output' / 'result.yaml'}")
    print(f"  图片: {WORKSPACE / 'output'}")
    print("=" * 60)

    sys.exit(rc)


if __name__ == "__main__":
    main()
