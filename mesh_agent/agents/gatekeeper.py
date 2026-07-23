"""Gatekeeper Agent: synthesizes debate, selects best strategy or rejects all."""

from __future__ import annotations

import json
from typing import Any

from mesh_agent.agents.base import LightAgent

GATEKEEPER_PROMPT = """You are the **Gatekeeper** in a mesh adaptation pipeline. Your job is to synthesize the debate between the Optimist (who argued FOR each strategy) and the Skeptic (who argued AGAINST each strategy), then make a final decision.

## Decision Rules

1. Compute a composite score for each strategy:
   `composite = (optimist_overall * 0.5) + ((5 - skeptic_overall) * 0.5)`
   This balances the positive and negative assessments equally.

2. Any strategy with a **fatal flaw** from the Skeptic is automatically disqualified.

3. If the composite score of the best remaining strategy is **below 2.5** (out of 5), all strategies are rejected. Return `action: "reject_all"`.

4. Otherwise, select the strategy with the highest composite score.

5. If all strategies are rejected and this is the first round of this type, you may lower the threshold to 2.0 as a one-time concession.

## Output Format

```json
{
  "action": "select|reject_all",
  "selected_strategy": "S1_..." or null,
  "reason": "Clear explanation of the decision",
  "composite_scores": [
    {"strategy_id": "S1_...", "optimist": 4, "skeptic": 2, "composite": 3.5, "fatal": false}
  ]
}
```
"""


class Gatekeeper(LightAgent):
    """Synthesizes debate and makes the final strategy selection."""

    def __init__(self, client=None):
        super().__init__("Gatekeeper", GATEKEEPER_PROMPT, client=client)

    async def decide(
        self,
        strategies: list[dict[str, Any]],
        optimist_scores: list[dict[str, Any]],
        skeptic_scores: list[dict[str, Any]],
        is_first_attempt: bool = True,
    ) -> dict[str, Any]:
        user_message = f"""## Proposed Strategies
{json.dumps(strategies, indent=2)}

## Optimist Scores
{json.dumps(optimist_scores, indent=2)}

## Skeptic Scores
{json.dumps(skeptic_scores, indent=2)}

## Context
This is the {"first" if is_first_attempt else "subsequent"} gate attempt for this round.

Synthesize the debate and make your decision."""

        schema = {
            "type": "object",
            "properties": {
                "action": {"type": "string", "enum": ["select", "reject_all"]},
                "selected_strategy": {"type": ["string", "null"]},
                "reason": {"type": "string"},
                "composite_scores": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "strategy_id": {"type": "string"},
                            "optimist": {"type": "number"},
                            "skeptic": {"type": "number"},
                            "composite": {"type": "number"},
                            "fatal": {"type": "boolean"},
                        },
                    },
                },
            },
            "required": ["action", "selected_strategy", "reason", "composite_scores"],
        }

        return await self.run_structured(user_message, schema)
