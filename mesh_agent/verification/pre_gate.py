"""PRE-Gate: LLM-based experiment design logic check (~0.5s per check).

Validates that a proposed strategy has:
1. A measurable claim (not vague like "makes the solution better")
2. Testable outputs (matching metric keys in the output schema)
3. Logical consistency (correct control, variable isolation)
"""

from __future__ import annotations

import json
from typing import Any

from mesh_agent.agents.base import LightAgent

PRE_GATE_PROMPT = """You are a **Scientific Logic Validator** for mesh adaptation experiments.

Your job: check whether a proposed mesh adaptation strategy is logically sound BEFORE any code is executed.

## Check Dimensions

### 1. Claim Measurability
- Does the strategy make a specific, quantifiable prediction? ("Cd will decrease by at least 1%" vs "solution will improve")
- Are the target metrics clearly identified?

### 2. Testability
- Are the target metrics actually computable from the solver's output?
- Do the metric names match what the solver produces?

### 3. Logical Consistency
- Does the refinement region make physical sense? (e.g., refining in a uniform flow region is wasteful)
- Is the variable isolation correct? (changing only mesh, not physics)
- Could observed changes be attributed to the mesh change, or are there confounding factors?

## Output Format

```json
{
  "passed": true|false,
  "issues": [
    {
      "severity": "blocker|warning",
      "category": "measurability|testability|logic",
      "description": "Specific issue found"
    }
  ],
  "summary": "Overall assessment"
}
```

A "blocker" issue means the strategy should NOT proceed to execution until fixed.
"""


class PreGate:
    """PRE-Gate: validates experiment design before code execution."""

    def __init__(self, client=None):
        self.agent = LightAgent("PreGate", PRE_GATE_PROMPT, client=client)

    async def validate(
        self,
        strategy: dict[str, Any],
        problem_spec: dict[str, Any],
        previous_metrics: dict[str, float] | None = None,
    ) -> dict[str, Any]:
        user_message = f"""## Proposed Strategy
{json.dumps(strategy, indent=2)}

## Problem Specification
{json.dumps(problem_spec, indent=2)}

## Previous Metrics
{json.dumps(previous_metrics) if previous_metrics else "None (first round)"}

Validate the logical soundness of this strategy. Check measurability, testability, and logical consistency."""

        schema = {
            "type": "object",
            "properties": {
                "passed": {"type": "boolean"},
                "issues": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "severity": {"type": "string", "enum": ["blocker", "warning"]},
                            "category": {"type": "string", "enum": ["measurability", "testability", "logic"]},
                            "description": {"type": "string"},
                        },
                        "required": ["severity", "category", "description"],
                    },
                },
                "summary": {"type": "string"},
            },
            "required": ["passed", "issues", "summary"],
        }

        return await self.agent.run_structured(user_message, schema)
