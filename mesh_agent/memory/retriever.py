"""Hybrid retriever: structured field match + embedding similarity + keyword."""

from __future__ import annotations

from typing import Any

import numpy as np

from mesh_agent.memory.store import MemoryStore


def cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    a_norm = np.linalg.norm(a)
    b_norm = np.linalg.norm(b)
    if a_norm < 1e-10 or b_norm < 1e-10:
        return 0.0
    return float(np.dot(a, b) / (a_norm * b_norm))


class MemoryRetriever:
    """Hybrid memory retrieval: structured + vector + keyword."""

    def __init__(self, store: MemoryStore):
        self.store = store

    def retrieve(
        self,
        scene_fingerprint: dict[str, Any],
        top_k: int = 5,
        role_filter: str | None = None,
        tag_filter: list[str] | None = None,
        min_similarity: float = 0.5,
    ) -> list[dict[str, Any]]:
        """Main retrieval: structured match + embedding similarity, merged."""
        candidates = self.store.list_all()

        # Role filter
        if role_filter:
            candidates = [c for c in candidates if role_filter in c.get("roles", [])]

        # Tag filter
        if tag_filter:
            tags_lower = [t.lower() for t in tag_filter]
            candidates = [c for c in candidates
                          if any(t in [rt.lower() for rt in c.get("tags", [])] for t in tags_lower)]

        if not candidates:
            return []

        # Compute similarity scores
        scored: list[tuple[float, dict[str, Any]]] = []
        query_pde = scene_fingerprint.get("pde_type", "").lower()

        for c in candidates:
            score = 0.3  # Base score for ANY matching candidate

            # Structured match: same PDE type = +0.4
            if query_pde and query_pde == c.get("pde_type", "").lower():
                score += 0.4

            # Type bonus: effective strategies get higher base score
            entry_type = c.get("type", "")
            if entry_type == "effective":
                score += 0.2
            elif entry_type == "ineffective":
                score -= 0.1

            # Embedding similarity (if available)
            query_embedding = scene_fingerprint.get("_embedding")
            if query_embedding is not None:
                stored_emb = self.store.get_embedding(c["id"])
                if stored_emb is not None:
                    sim = cosine_similarity(np.asarray(query_embedding), stored_emb)
                    score += sim * 0.6  # Embedding weight

            # Recency bonus: newer entries get slight boost
            # (omit timestamp-based for simplicity)

            if score >= min_similarity:
                scored.append((score, c))

        scored.sort(key=lambda x: x[0], reverse=True)
        top = scored[:top_k]

        # Load full entries
        results = []
        for score, index_item in top:
            entry = self.store.get(index_item["id"])
            if entry:
                entry["_score"] = score
                results.append(entry)

        return results

    def search_by_strategy_outcome(
        self,
        strategy_type: str,
        outcome_label: str | None = None,
        top_k: int = 5,
    ) -> list[dict[str, Any]]:
        """Find entries by strategy type and optional outcome filter."""
        candidates = self.store.search_by_tags([strategy_type])
        if outcome_label:
            candidates = [c for c in candidates if c.get("type") == outcome_label]
        results = []
        for c in candidates[:top_k]:
            entry = self.store.get(c["id"])
            if entry:
                results.append(entry)
        return results

    def get_failure_patterns(self, pde_type: str | None = None) -> list[dict[str, Any]]:
        """Retrieve all failed attempts, optionally filtered by PDE type."""
        candidates = self.store.search_by_field(type="ineffective")
        if pde_type:
            candidates = [c for c in candidates if c.get("pde_type", "").lower() == pde_type.lower()]
        results = []
        for c in candidates:
            entry = self.store.get(c["id"])
            if entry:
                results.append(entry)
        return results

    def get_effective_patterns(self, pde_type: str, top_k: int = 5) -> list[dict[str, Any]]:
        """Retrieve successful strategies for a given PDE type."""
        candidates = self.store.search_by_field(type="effective")
        candidates = [c for c in candidates if c.get("pde_type", "").lower() == pde_type.lower()]
        results = []
        for c in candidates[:top_k]:
            entry = self.store.get(c["id"])
            if entry:
                results.append(entry)
        return results
