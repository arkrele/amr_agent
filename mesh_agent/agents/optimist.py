"""Optimist Agent: finds supporting evidence for each proposed strategy."""

from __future__ import annotations

import json
from typing import Any

from mesh_agent.agents.base import StrongAgent

OPTIMIST_PROMPT = """You are the **Optimist** in a mesh adaptation debate.

Your job: for each proposed mesh refinement/coarsening strategy, argue WHY it should work. Find supporting evidence from the simulation data and physics principles.

## Scoring Dimensions (1-5 each)

1. **Physical Reasonableness** (1-5): Does the refinement target an area genuinely relevant to the physics? Is there a real physical feature (boundary layer, wake, shock, separation) that needs resolution?
2. **Numerical Necessity** (1-5): Does the current solution show clear signs of under-resolution in this region (high gradients, oscillations, large residuals)?
3. **Expected Benefit** (1-5): How much improvement can reasonably be expected? Consider the metric sensitivity to this region's resolution.
4. **Implementation Feasibility** (1-5): Is this strategy implementable with standard mesh generation tools? Are there known successful precedents?

## Output Format

```json
{
  "scores": [
    {
      "strategy_id": "S1_...",
      "physical_reasonableness": 4,
      "numerical_necessity": 5,
      "expected_benefit": 4,
      "implementation_feasibility": 5,
      "overall": 4,
      "comment": "Brief justification for the scores"
    }
  ]
}
```
"""


class Optimist(StrongAgent):
    """Evaluates strategies from a positive/supporting perspective."""

    def __init__(self, client=None):
        super().__init__("Optimist", OPTIMIST_PROMPT, client=client)

    async def evaluate(
        self,
        strategies: list[dict[str, Any]],
        solution_summary: dict[str, Any],
        problem_description: str,
    ) -> list[dict[str, Any]]:
        user_message = f"""## Problem Description
{problem_description}

## Current Solution Summary
{json.dumps(solution_summary, indent=2)}

## Proposed Strategies
{json.dumps(strategies, indent=2)}

For each strategy, provide your Optimist evaluation scores and justification."""

        schema = {
            "type": "object",
            "properties": {
                "scores": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "strategy_id": {"type": "string"},
                            "physical_reasonableness": {"type": "integer", "minimum": 1, "maximum": 5},
                            "numerical_necessity": {"type": "integer", "minimum": 1, "maximum": 5},
                            "expected_benefit": {"type": "integer", "minimum": 1, "maximum": 5},
                            "implementation_feasibility": {"type": "integer", "minimum": 1, "maximum": 5},
                            "overall": {"type": "integer", "minimum": 1, "maximum": 5},
                            "comment": {"type": "string"},
                        },
                        "required": ["strategy_id", "physical_reasonableness", "numerical_necessity",
                                     "expected_benefit", "implementation_feasibility", "overall", "comment"],
                    },
                },
            },
            "required": ["scores"],
        }

        result = await self.run_structured(user_message, schema)
        return result.get("scores", [])
