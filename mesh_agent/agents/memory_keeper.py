"""Memory Keeper Agent: summarizes rounds and persists strategic knowledge.

Runs after each adaptation round and at session end.
"""

from __future__ import annotations

import json
from typing import Any

import numpy as np

from mesh_agent.agents.base import LightAgent
from mesh_agent.memory.store import MemoryStore

MEMORY_KEEPER_PROMPT = """You are the **Memory Keeper** for a mesh adaptation agent system.

Your job: after each mesh adaptation round, summarize what happened into a concise,
structured memory entry. These entries form a persistent knowledge base that
future adaptation sessions can query.

## What to capture

1. **Scene fingerprint**: PDE type, geometry, mesh stats (for future matching)
2. **Strategy summary**: What was attempted? (strategy_id, rationale, operations)
3. **Outcome**: Was it effective? Quantify the improvement (or degradation).
   - "effective": error reduced by > threshold
   - "ineffective": error unchanged or worse
   - "partial": mixed results
4. **Tags**: Key terms for retrieval (e.g., "boundary_layer", "geometric_refinement", "coarsening")
5. **Roles**: Which agent roles should see this? (strategist, programmer, reviewer, etc.)

## Output Format
```json
{
  "scene": {
    "pde_type": "...",
    "description": "concise problem description",
    "geometry": "...",
    "total_cells": 0
  },
  "strategy": {
    "strategy_id": "...",
    "rationale": "brief summary",
    "operations_summary": "what was changed"
  },
  "outcome": {
    "label": "effective|ineffective|partial",
    "improvement": {"metric_name": "delta_pct"},
    "error_summary": "if failed, why?",
    "summary": "one-line outcome"
  },
  "tags": ["tag1", "tag2"],
  "roles": ["strategist", "programmer"]
}
```
"""


class MemoryKeeper:
    """Summarizes rounds and persists structured memory."""

    def __init__(self, store: MemoryStore, client=None):
        self.store = store
        self.agent = LightAgent("MemoryKeeper", MEMORY_KEEPER_PROMPT, client=client)

    async def record_round(
        self,
        round_number: int,
        strategy: dict[str, Any],
        before_metrics: dict[str, float],
        after_metrics: dict[str, float],
        mesh_stats: dict[str, Any],
        problem_spec: dict[str, Any],
        claim_verdict: str,
        error_context: str = "",
    ) -> str:
        """Summarize a round and persist to memory."""

        improvement = {}
        for k in before_metrics:
            if k in after_metrics:
                denom = max(abs(before_metrics[k]), 1e-10)
                improvement[k] = round((after_metrics[k] - before_metrics[k]) / denom * 100, 1)

        user_message = f"""## Mesh Adaptation Round {round_number}

### Problem
{problem_spec.get('description', '')[:300]}

### Strategy
{json.dumps(strategy, indent=2)[:500]}

### Before Metrics
{json.dumps(before_metrics, indent=2)}

### After Metrics
{json.dumps(after_metrics, indent=2)}

### Computed Improvement (%)
{json.dumps(improvement, indent=2)}

### Claim Verdict
{claim_verdict}

### Error Context
{error_context[:500] if error_context else 'None'}

Summarize this round as a structured memory entry."""

        schema = {
            "type": "object",
            "properties": {
                "scene": {"type": "object"},
                "strategy": {"type": "object"},
                "outcome": {"type": "object"},
                "tags": {"type": "array", "items": {"type": "string"}},
                "roles": {"type": "array", "items": {"type": "string"}},
            },
            "required": ["scene", "strategy", "outcome", "tags", "roles"],
        }

        result = await self.agent.run_structured(user_message, schema)

        scene = result.get("scene", {})
        if not scene:
            scene = {"pde_type": problem_spec.get("pde_type", ""), "description": problem_spec.get("description", "")[:100]}

        outcome = result.get("outcome", {})
        outcome["improvement"] = improvement
        outcome["round"] = round_number

        entry_id = self.store.put(
            scene=scene,
            strategy=result.get("strategy", strategy),
            outcome=outcome,
            roles=result.get("roles", ["strategist"]),
            tags=result.get("tags", []),
        )

        return entry_id

    def get_failure_statistics(self, pde_type: str | None = None) -> dict[str, Any]:
        """Analyze failure patterns for L4 self-evolution."""
        all_entries = self.store.list_all()
        failures = [e for e in all_entries if e.get("type") == "ineffective"]
        if pde_type:
            failures = [e for e in failures if e.get("pde_type", "").lower() == pde_type.lower()]

        effective = [e for e in all_entries if e.get("type") == "effective"]
        if pde_type:
            effective = [e for e in effective if e.get("pde_type", "").lower() == pde_type.lower()]

        # Count failure reasons
        reasons: dict[str, int] = {}
        for f_entry in failures:
            entry = self.store.get(f_entry["id"])
            if entry:
                reason = entry.get("outcome", {}).get("error_summary", "unknown")
                key = reason[:80]
                reasons[key] = reasons.get(key, 0) + 1

        return {
            "total_entries": len(all_entries),
            "total_failures": len(failures),
            "total_effective": len(effective),
            "failure_reasons": dict(sorted(reasons.items(), key=lambda x: -x[1])[:5]),
            "effectiveness_rate": len(effective) / max(len(effective) + len(failures), 1),
        }

    def text_summary(self) -> str:
        """Human-readable summary of all stored knowledge."""
        return self.store.to_text()
