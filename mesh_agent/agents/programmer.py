"""Programmer Agent: generates/modifies mesh generation code and solver parameters."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

from mesh_agent.agents.base import StrongAgent

PROGRAMMER_PROMPT = """You are a **Scientific Computing Programmer** specialized in mesh generation and CFD/FEM solver configuration.

Your job: given a mesh adaptation strategy, produce the code to implement it.

## Code Requirements

### Mesh Script
- Read the current mesh file(s) from --input-mesh directory
- Apply refinement/coarsening operations to the node coordinates
- Write new mesh file(s) to --output-dir directory
- The script MUST accept --input-mesh and --output-dir arguments
- Use only numpy for array operations. Keep it simple and correct.
- IMPORTANT: Avoid infinite loops. Use bounded for-loops. Always ensure termination.

### Solver Parameters (solver_params)
- JSON object with solver configuration changes (dt, max_iterations, etc.)
- Only include parameters that need to change for the new mesh

### Post-processing Script
- Read solver output and extract target metrics
- Write metrics.json to --output-dir

### Visualization Script
- Generate comparison plots saved as PNG files to --output-dir
- MUST include: (1) mesh node distribution before/after, (2) solution field before/after, (3) error distribution if analytical solution known
- Use matplotlib. Accept --output-dir, --mesh-before, --mesh-after, --metrics-before, --metrics-after arguments
- Save at minimum: `mesh_comparison.png`, `solution_comparison.png`

## Output Format
Always respond with JSON:
```json
{
  "mesh_script": "complete Python script content",
  "solver_params": {"key": value},
  "post_script": "complete Python script content",
  "viz_script": "complete Python viz script content",
  "changes_summary": "What was changed and why"
}
```

Put the FULL script content as strings. Do not use external file references.
"""


class Programmer(StrongAgent):
    """Generates and modifies mesh/solver/post-processing code."""

    def __init__(self, client=None):
        super().__init__("Programmer", PROGRAMMER_PROMPT, client=client)

    async def implement(
        self,
        selected_strategy: dict[str, Any],
        current_mesh_path: str,
        solver_path: str,
        output_dir: str,
        problem_spec: dict[str, Any],
        previous_metrics: Optional[dict[str, float]] = None,
        error_context: Optional[str] = None,
    ) -> dict[str, Any]:
        error_block = ""
        if error_context:
            error_block = f"\n\n## Previous Attempt Error\n{error_context}\nPlease fix this error in your new implementation."

        user_message = f"""## Selected Strategy
{json.dumps(selected_strategy, indent=2)}

## File Paths
- Current mesh: {current_mesh_path}
- Solver template: {solver_path}
- Output directory: {output_dir}

## Problem Specification
{json.dumps(problem_spec, indent=2)}

## Previous Metrics
{json.dumps(previous_metrics) if previous_metrics else "None (first round)"}
{error_block}

Generate the mesh modification script, solver parameters, and post-processing script to implement this strategy."""

        schema = {
            "type": "object",
            "properties": {
                "mesh_script": {"type": "string"},
                "solver_params": {"type": "object"},
                "post_script": {"type": "string"},
                "viz_script": {"type": "string"},
                "changes_summary": {"type": "string"},
            },
            "required": ["mesh_script", "solver_params", "post_script", "viz_script", "changes_summary"],
        }

        result = await self.run_structured(user_message, schema)
        return result

    async def generate_solver(
        self,
        problem_spec: dict[str, Any],
        mesh_path: str,
        output_dir: str,
        error_context: str | None = None,
    ) -> dict[str, Any]:
        """Generate a complete solver from scratch when no user template is provided."""
        error_block = ""
        if error_context:
            error_block = f"\n\n## Previous Attempt Error\n{error_context}\nPlease fix this error."

        user_message = f"""## Problem Specification
{json.dumps(problem_spec, indent=2)}

## Output Requirements
- Mesh path: {mesh_path}
- Output directory: {output_dir}

## Task
Generate a COMPLETE, self-contained Python solver script for this problem.
The script must:
1. Accept --mesh, --output-dir, --params arguments (argparse)
2. Read the mesh file(s) from --mesh directory
3. Solve the PDE described in the problem spec
4. Write a `metrics.json` file to --output-dir with at minimum:
   - "solver_convergence": {{"converged": bool, "residual_final": float, "residual_initial": float, "iterations": int}}
   - All target metrics from the problem spec
5. Write a `mesh_quality.json` with basic mesh statistics if applicable
6. Be executable: `python solver.py --mesh <dir> --output-dir <dir> --params '{{}}'`
{error_block}

Use numpy/scipy only. Keep it simple and self-contained."""

        schema = {
            "type": "object",
            "properties": {
                "solver_script": {"type": "string", "description": "Complete Python solver script"},
                "expected_metrics": {"type": "array", "items": {"type": "string"}},
                "explanation": {"type": "string"},
            },
            "required": ["solver_script", "expected_metrics", "explanation"],
        }

        return await self.run_structured(user_message, schema)

    @staticmethod
    def write_scripts(implementation: dict[str, Any], work_dir: Path) -> dict[str, Path]:
        """Write generated scripts to the work directory."""
        paths = {}
        for key in ["mesh_script", "post_script", "solver_script", "viz_script"]:
            if implementation.get(key):
                fname = f"{key.replace('_script', '')}.py"
                fpath = work_dir / fname
                fpath.write_text(implementation[key], encoding="utf-8")
                paths[key] = fpath
        return paths
