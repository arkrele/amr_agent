"""Strategist Agent: reads solver results + knowledge base, proposes mesh adaptation strategies."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Optional

from mesh_agent.agents.base import StrongAgent
from mesh_agent.schemas import Strategy

STRATEGIST_PROMPT = """You are a **Mesh Adaptation Strategist** specialized in computational physics and numerical simulation.

Your job: analyze simulation results on the current mesh and propose 3-5 mesh refinement/coarsening strategies.

## Your Process

1. **Analyze the numerical fields** provided (velocity, pressure, vorticity, residuals, etc.)
2. **Identify problematic regions**:
   - High gradients that the current mesh cannot resolve
   - Areas with numerical oscillations or instability
   - Boundary layers with insufficient resolution (y+ too high)
   - Wake regions with vortex shedding that need refinement
   - Shock regions (for compressible flows) requiring high resolution
   - Regions that are over-resolved and can be coarsened
3. **Propose 3-5 strategies**, each with a clear rationale and mesh operations.
   - Each strategy should focus on 1-2 regions (single-dimension variation principle)
   - Prefer strategies that are physically motivated over purely numerical ones

## Region Description

Use the region type system to describe where to refine/coarsen:

- **Geometric**: `{"type": "box", "corner1": [x1,y1,z1], "corner2": [x2,y2,z2]}`, `{"type": "cylinder", "center": [x,y,z], "radius": r}`, `{"type": "sphere", "center": [x,y,z], "radius": r}`
- **Solution-driven**: `{"type": "gradient_threshold", "field": "pressure", "threshold": 1000.0}`, `{"type": "value_range", "field": "vorticity", "min": 50.0}`
- **Physics features**: `{"type": "boundary_layer", "on_surface": {"type": "tag", "tag": "wall_name"}, "first_layer_height": 5e-6, "growth_rate": 1.15, "num_layers": 15}`, `{"type": "wake", "from_body": {"type": "tag", "tag": "body"}, "direction": [1,0,0], "length": 20.0, "spread_angle": 5.0}`
- **Boolean combinations**: `{"type": "boolean", "operation": "union|intersection|difference", "a": {...}, "b": {...}}`

## Output Format

Respond with a JSON object:
```json
{
  "analysis": "Brief analysis of the current solution and where mesh adaptation is needed",
  "strategies": [
    {
      "strategy_id": "S1_descriptive_name",
      "rationale": "Why this strategy should improve the solution",
      "operations": [
        {
          "region": {"type": "...", ...},
          "action": "refine",
          "target_size": 0.01,
          "growth_rate": 1.1,
          "priority": 1
        }
      ],
      "expected_effect": {
        "cells_estimate": 15000,
        "affected_metrics": ["drag_coefficient", "lift_coefficient"],
        "risk": "low|medium|high",
        "risk_detail": "Specific risk description"
      }
    }
  ]
}
```
"""


class Strategist(StrongAgent):
    """Generates mesh adaptation strategies from solution analysis."""

    def __init__(self, knowledge_dir: Optional[Path] = None, client=None):
        super().__init__("Strategist", STRATEGIST_PROMPT, client=client)
        self.knowledge_dir = knowledge_dir

    def _load_knowledge(self) -> str:
        """Load preset strategy knowledge for RAG injection."""
        if not self.knowledge_dir or not self.knowledge_dir.exists():
            return ""
        parts = []
        for f in sorted(self.knowledge_dir.glob("*.md")):
            parts.append(f.read_text(encoding="utf-8"))
        return "\n\n---\n\n".join(parts)

    async def generate(
        self,
        problem_description: str,
        current_mesh_stats: dict[str, Any],
        solution_summary: dict[str, Any],
        previous_metrics: Optional[dict[str, float]] = None,
        previous_strategies: Optional[list[dict[str, Any]]] = None,
    ) -> dict[str, Any]:
        """Generate adaptation strategies based on current solution state."""

        knowledge = self._load_knowledge()
        knowledge_block = f"\n\n## Reference Knowledge Base\n{knowledge}" if knowledge else ""

        prev_block = ""
        if previous_metrics:
            prev_block += f"\n\n## Previous Round Metrics\n{json.dumps(previous_metrics, indent=2)}"
        if previous_strategies:
            prev_block += f"\n\n## Previously Attempted Strategies\n{json.dumps(previous_strategies, indent=2)}"

        user_message = f"""## Problem Description
{problem_description}

## Current Mesh Statistics
{json.dumps(current_mesh_stats, indent=2)}

## Current Solution Summary
{json.dumps(solution_summary, indent=2)}
{prev_block}{knowledge_block}

Analyze the current solution and propose 3-5 mesh adaptation strategies."""

        schema = {
            "type": "object",
            "properties": {
                "analysis": {"type": "string"},
                "strategies": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "strategy_id": {"type": "string"},
                            "rationale": {"type": "string"},
                            "operations": {"type": "array"},
                            "expected_effect": {
                                "type": "object",
                                "properties": {
                                    "cells_estimate": {"type": "integer"},
                                    "affected_metrics": {"type": "array", "items": {"type": "string"}},
                                    "risk": {"type": "string", "enum": ["low", "medium", "high"]},
                                    "risk_detail": {"type": "string"},
                                },
                                "required": ["cells_estimate", "affected_metrics", "risk", "risk_detail"],
                            },
                        },
                        "required": ["strategy_id", "rationale", "operations", "expected_effect"],
                    },
                },
            },
            "required": ["analysis", "strategies"],
        }

        return await self.run_structured(user_message, schema)
