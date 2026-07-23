"""Skeptic Agent: challenges each strategy, finds risks and failure modes."""

from __future__ import annotations

import json
from typing import Any

from mesh_agent.agents.base import StrongAgent

SKEPTIC_PROMPT = """You are the **Skeptic** in a mesh adaptation debate. Your job is ruthless: find every reason a proposed strategy might FAIL.

## Scoring Dimensions (1-5, where 5 = HIGHEST RISK)

1. **Mesh Quality Risk** (1-5): Will this operation create poor-quality cells (high skewness, extreme aspect ratios, inverted cells)?
2. **Numerical Risk** (1-5): Could this introduce numerical problems (stiff matrices from high aspect ratios, CFL constraint violations, interpolation errors)?
3. **Cost Risk** (1-5): Is the cell count increase proportional to the expected benefit? Could it blow the budget?
4. **Physical Misjudgment Risk** (1-5): Could what looks like a physical feature actually be a numerical artifact? Is the "gradient" just grid-induced oscillation?

## Fatal Flaw Detection

A strategy has a **fatal flaw** if ANY of these are true:
- The refinement targets a region that is clearly NOT where the physics happens
- The proposed cell size is physically impossible (smaller than mean free path, below geometry tolerance)
- The operation would definitely create inverted cells
- The strategy contradicts a known principle (e.g., coarsening inside a boundary layer)
- The cost would clearly exceed any reasonable budget (>10x current cell count)

If you find a fatal flaw, set `fatal_flaws` with a clear description. The Gatekeeper may automatically reject strategies with fatal flaws.

## Output Format

```json
{
  "scores": [
    {
      "strategy_id": "S1_...",
      "mesh_quality_risk": 3,
      "numerical_risk": 2,
      "cost_risk": 1,
      "physical_misjudgment_risk": 2,
      "overall": 2,
      "fatal_flaws": [],
      "alternative": "A simpler approach that might work better...",
      "comment": "Detailed critical analysis"
    }
  ]
}
```
"""


class Skeptic(StrongAgent):
    """Evaluates strategies from a critical/skeptical perspective."""

    def __init__(self, client=None):
        super().__init__("Skeptic", SKEPTIC_PROMPT, client=client)

    async def evaluate(
        self,
        strategies: list[dict[str, Any]],
        solution_summary: dict[str, Any],
        mesh_stats: dict[str, Any],
        problem_description: str,
        budget_info: dict[str, Any],
    ) -> list[dict[str, Any]]:
        user_message = f"""## Problem Description
{problem_description}

## Current Solution Summary
{json.dumps(solution_summary, indent=2)}

## Current Mesh Statistics
{json.dumps(mesh_stats, indent=2)}

## Budget Constraints
{json.dumps(budget_info, indent=2)}

## Proposed Strategies
{json.dumps(strategies, indent=2)}

For each strategy, provide your Skeptic evaluation. Be thorough in finding potential failures."""

        schema = {
            "type": "object",
            "properties": {
                "scores": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "strategy_id": {"type": "string"},
                            "mesh_quality_risk": {"type": "integer", "minimum": 1, "maximum": 5},
                            "numerical_risk": {"type": "integer", "minimum": 1, "maximum": 5},
                            "cost_risk": {"type": "integer", "minimum": 1, "maximum": 5},
                            "physical_misjudgment_risk": {"type": "integer", "minimum": 1, "maximum": 5},
                            "overall": {"type": "integer", "minimum": 1, "maximum": 5},
                            "fatal_flaws": {"type": "array", "items": {"type": "string"}},
                            "alternative": {"type": "string"},
                            "comment": {"type": "string"},
                        },
                        "required": ["strategy_id", "mesh_quality_risk", "numerical_risk", "cost_risk",
                                     "physical_misjudgment_risk", "overall", "fatal_flaws", "alternative", "comment"],
                    },
                },
            },
            "required": ["scores"],
        }

        result = await self.run_structured(user_message, schema)
        return result.get("scores", [])
