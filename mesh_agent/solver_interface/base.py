"""Abstract solver interface and user template adapter."""

from __future__ import annotations

import json
import subprocess
import sys
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Any


class SolverInterface(ABC):
    """Abstract interface for solver execution."""

    @abstractmethod
    async def run(
        self,
        mesh_path: str | Path,
        output_dir: str | Path,
        params: dict[str, Any],
        work_dir: str | Path,
    ) -> dict[str, Any]:
        """Run solver on given mesh, return metrics dict."""
        ...


class UserTemplateSolver(SolverInterface):
    """Wraps a user-provided solver script.

    The user's script must accept:
        --mesh <path>      : Path to the mesh file
        --output-dir <path>: Directory for output files
        --params <json>    : JSON string of solver parameters

    It must write a metrics.json file to the output directory.
    """

    def __init__(self, solver_path: str | Path):
        self.solver_path = Path(solver_path)
        if not self.solver_path.exists():
            raise FileNotFoundError(f"Solver not found: {self.solver_path}")

    async def run(
        self,
        mesh_path: str | Path,
        output_dir: str | Path,
        params: dict[str, Any],
        work_dir: str | Path,
    ) -> dict[str, Any]:
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        mesh_path = Path(mesh_path)

        cmd = [
            sys.executable,
            str(self.solver_path.resolve()),
            "--mesh", str(mesh_path.resolve()),
            "--output-dir", str(output_dir.resolve()),
            "--params", json.dumps(params),
        ]

        result = subprocess.run(
            cmd,
            cwd=str(Path(work_dir).resolve()),
            capture_output=True,
            text=True,
            timeout=params.get("_timeout", 3600),
        )

        if result.returncode != 0:
            return {
                "_success": False,
                "_error": f"Solver exited with code {result.returncode}",
                "_stdout": result.stdout[-2000:],
                "_stderr": result.stderr[-2000:],
            }

        # Try to read the output metrics
        metrics_file = output_dir / "metrics.json"
        if metrics_file.exists():
            try:
                metrics = json.loads(metrics_file.read_text(encoding="utf-8"))
                metrics["_success"] = True
                metrics["_stdout"] = result.stdout[-2000:]
                metrics["_stderr"] = result.stderr[-2000:]
                return metrics
            except json.JSONDecodeError:
                pass

        return {
            "_success": True,
            "_stdout": result.stdout[-2000:],
            "_stderr": result.stderr[-2000:],
            "_warning": "No metrics.json found in output directory",
        }
