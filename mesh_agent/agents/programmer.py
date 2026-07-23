"""Programmer Agent: generates/modifies mesh generation code and solver parameters."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

from mesh_agent.agents.base import StrongAgent

PROGRAMMER_PROMPT = """You are a **Scientific Computing Programmer** specialized in mesh generation and CFD/FEM solver configuration.

Your job: given a mesh adaptation strategy, produce the code and configuration to implement it.

## Responsibilities

1. **Mesh Generation Code**: Write Python scripts using pygmsh/gmsh to:
   - Read the current mesh
   - Apply the specified refinement/coarsening operations
   - Output a new mesh file

2. **Solver Parameter Adjustment**: Modify solver configuration (time step, relaxation factors, CFL number, etc.) if needed to accommodate the new mesh.

3. **Post-processing Code**: Write scripts to extract target metrics from solver output.

## Code Standards

- All code must be self-contained Python scripts that can run from the command line
- Use explicit file paths (provided in the context)
- Include error handling for common failure modes
- Output a `metrics.json` file with the extracted metrics
- Each script must accept `--input-mesh`, `--output-dir` arguments

## Output Format

Always respond with a JSON object:
```json
{
  "mesh_script": "path/to/generated/mesh_script.py content here...",
  "solver_params": {
    "dt": 0.0005,
    "max_iterations": 10000
  },
  "post_script": "path/to/generated/post_script.py content here...",
  "changes_summary": "Summary of what was changed and why"
}
```

When writing code, put the FULL script content in the string fields. Do not use external file references.
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
                "changes_summary": {"type": "string"},
            },
            "required": ["mesh_script", "solver_params", "post_script", "changes_summary"],
        }

        result = await self.run_structured(user_message, schema)
        return result

    @staticmethod
    def write_scripts(implementation: dict[str, Any], work_dir: Path) -> dict[str, Path]:
        """Write generated scripts to the work directory."""
        paths = {}
        for key in ["mesh_script", "post_script"]:
            if implementation.get(key):
                ext = ".py"
                fname = f"{key.replace('_script', '')}.py"
                fpath = work_dir / fname
                fpath.write_text(implementation[key], encoding="utf-8")
                paths[key] = fpath
        return paths
