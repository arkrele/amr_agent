"""Role-routed memory injection (Phase 2 L3).

Each memory entry carries a 'roles' field listing which Agent roles
should receive this lesson. The router filters memories by target role
before injecting them into prompts.

From Sibyl H4: "lessons have audiences — code-gen lessons route only to
Programmer, quality lessons route only to Reviewer. Irrelevant agents
should not be polluted by irrelevant lessons."
"""

from __future__ import annotations

from mesh_agent.memory.retriever import MemoryRetriever

# Standard role names used throughout the agent system
ROLE_STRATEGIST = "strategist"
ROLE_OPTIMIST = "optimist"
ROLE_SKEPTIC = "skeptic"
ROLE_GATEKEEPER = "gatekeeper"
ROLE_PROGRAMMER = "programmer"
ROLE_REVIEWER = "reviewer"

ALL_ROLES = [ROLE_STRATEGIST, ROLE_OPTIMIST, ROLE_SKEPTIC, ROLE_GATEKEEPER, ROLE_PROGRAMMER, ROLE_REVIEWER]


class RoleRouter:
    """Routes memories to agents based on target_role field.

    - A memory tagged with roles=["programmer"] only reaches the Programmer.
    - A memory tagged with roles=["strategist", "optimist"] reaches both.
    - Memories tagged with ["all"] or no roles reach everyone.
    """

    def __init__(self, retriever: MemoryRetriever):
        self.retriever = retriever

    def get_for_role(
        self,
        role: str,
        scene_fingerprint: dict,
        top_k: int = 3,
    ) -> list[dict]:
        """Get memories relevant to a specific agent role."""
        return self.retriever.retrieve(
            scene_fingerprint=scene_fingerprint,
            role_filter=role,
            top_k=top_k,
            min_similarity=0.2,  # Lower threshold for role-specific
        )

    def get_for_all(
        self,
        scene_fingerprint: dict,
        top_k: int = 8,
    ) -> dict[str, list[dict]]:
        """Get memories for all roles at once. Returns {role: [memories]}."""
        result = {}
        for role in ALL_ROLES:
            memories = self.get_for_role(role, scene_fingerprint, top_k=2)
            if memories:
                result[role] = memories
        return result

    def inject_memory_context(
        self,
        role: str,
        base_prompt: str,
        scene_fingerprint: dict,
    ) -> str:
        """Build a prompt for a role with relevant memory injected."""
        memories = self.get_for_role(role, scene_fingerprint)
        if not memories:
            return base_prompt

        memory_text = "\n\n## Relevant Past Experience (role-routed)\n"
        for m in memories:
            outcome = m.get("outcome", {})
            strategy = m.get("strategy", {})
            label = outcome.get("label", "unknown")
            memory_text += (
                f"- [{label.upper()}] {strategy.get('rationale', strategy.get('strategy_id', ''))[:120]}\n"
                f"  Outcome: {outcome.get('improvement', outcome.get('summary', ''))}\n"
            )

        return base_prompt + memory_text

    def get_lessons_for_code_generation(self, scene_fingerprint: dict) -> str:
        """Get lessons specifically for Programmer/Reviewer."""
        memories = (
            self.get_for_role(ROLE_PROGRAMMER, scene_fingerprint, top_k=3)
            + self.get_for_role(ROLE_REVIEWER, scene_fingerprint, top_k=3)
        )
        if not memories:
            return ""

        lines = ["\n## Code Generation Lessons from Past Attempts"]
        for m in memories:
            outcome = m.get("outcome", {})
            strategy = m.get("strategy", {})
            lines.append(f"- [{outcome.get('label', '?')}] {strategy.get('rationale', '')[:150]}")
            if outcome.get("error_summary"):
                lines.append(f"  Error: {outcome['error_summary'][:200]}")
        return "\n".join(lines)
