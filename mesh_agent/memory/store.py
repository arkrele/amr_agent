"""Hybrid memory store: structured JSON + vector embeddings.

Memory entries are stored as JSON with structured fields for exact
queries, plus optional embedding vectors for similarity search.

File layout:
    <memory_dir>/
        index.json          # [{id, type, roles[], tags[], summary, file}]
        entries/
            <id>.json       # Full entry {id, scene, strategy, outcome, ...}
            <id>.npy        # Optional embedding vector
"""

from __future__ import annotations

import json
import os
import time
import uuid
from pathlib import Path
from typing import Any, Optional

import numpy as np


class MemoryStore:
    """Append-only memory store with structured + vector storage."""

    def __init__(self, memory_dir: str | Path):
        self.dir = Path(memory_dir)
        self.dir.mkdir(parents=True, exist_ok=True)
        self.entries_dir = self.dir / "entries"
        self.entries_dir.mkdir(exist_ok=True)
        self.index_path = self.dir / "index.json"
        self._index: list[dict[str, Any]] = []
        self._load_index()

    # ── Index ──────────────────────────────────────────────────

    def _load_index(self) -> None:
        if self.index_path.exists():
            try:
                self._index = json.loads(self.index_path.read_text(encoding="utf-8"))
            except (json.JSONDecodeError, IOError):
                self._index = []

    def _save_index(self) -> None:
        self.index_path.write_text(json.dumps(self._index, indent=2, ensure_ascii=False), encoding="utf-8")

    # ── Write ──────────────────────────────────────────────────

    def put(
        self,
        scene: dict[str, Any],
        strategy: dict[str, Any],
        outcome: dict[str, Any],
        roles: list[str] | None = None,
        tags: list[str] | None = None,
        embedding: np.ndarray | None = None,
    ) -> str:
        """Store a memory entry. Returns the entry ID."""
        entry_id = str(uuid.uuid4())[:8]
        entry = {
            "id": entry_id,
            "timestamp": int(time.time()),
            "scene": scene,
            "strategy": strategy,
            "outcome": outcome,
            "roles": roles or ["strategist"],
            "tags": tags or [],
        }
        entry_path = self.entries_dir / f"{entry_id}.json"
        entry_path.write_text(json.dumps(entry, indent=2, ensure_ascii=False), encoding="utf-8")

        if embedding is not None:
            np.save(str(self.entries_dir / f"{entry_id}.npy"), embedding)

        summary = f"[{outcome.get('label', 'unknown')}] {scene.get('pde_type', '')} {scene.get('description', '')[:80]}"
        self._index.append({
            "id": entry_id,
            "type": outcome.get("label", "unknown"),
            "roles": roles or ["strategist"],
            "tags": tags or [],
            "summary": summary,
            "pde_type": scene.get("pde_type", ""),
            "timestamp": entry["timestamp"],
        })
        self._save_index()
        return entry_id

    # ── Read ───────────────────────────────────────────────────

    def get(self, entry_id: str) -> Optional[dict[str, Any]]:
        entry_path = self.entries_dir / f"{entry_id}.json"
        if entry_path.exists():
            return json.loads(entry_path.read_text(encoding="utf-8"))
        return None

    def get_embedding(self, entry_id: str) -> Optional[np.ndarray]:
        emb_path = self.entries_dir / f"{entry_id}.npy"
        if emb_path.exists():
            return np.load(str(emb_path))
        return None

    def list_all(self) -> list[dict[str, Any]]:
        return list(self._index)

    def search_by_field(self, **kwargs) -> list[dict[str, Any]]:
        """Exact/near match on structured fields."""
        results = []
        for item in self._index:
            match = True
            for key, value in kwargs.items():
                if key not in item:
                    match = False
                    break
                if isinstance(value, str):
                    if value.lower() not in str(item[key]).lower():
                        match = False
                        break
                elif item[key] != value:
                    match = False
                    break
            if match:
                results.append(item)
        return results

    def search_by_tags(self, tags: list[str]) -> list[dict[str, Any]]:
        """Any-tag match."""
        results = []
        tags_lower = [t.lower() for t in tags]
        for item in self._index:
            item_tags = [t.lower() for t in item.get("tags", [])]
            if any(t in item_tags for t in tags_lower):
                results.append(item)
        return results

    def count(self) -> int:
        return len(self._index)

    # ── Summarize ──────────────────────────────────────────────

    def build_scene_fingerprint(self, problem_spec: dict[str, Any], mesh_stats: dict[str, Any]) -> dict[str, Any]:
        """Build a scene fingerprint for matching."""
        return {
            "pde_type": problem_spec.get("pde_type", ""),
            "description": problem_spec.get("description", ""),
            "geometry": problem_spec.get("geometry", ""),
            "total_cells": mesh_stats.get("total_cells", 0),
            "max_cells": mesh_stats.get("max_cells", 0),
        }

    def to_text(self) -> str:
        """Dump entire memory store as human-readable text for LLM context."""
        parts = []
        for item in self._index:
            entry = self.get(item["id"])
            if not entry:
                continue
            outcome = entry.get("outcome", {})
            scene = entry.get("scene", {})
            strategy = entry.get("strategy", {})
            parts.append(
                f"### Memory {item['id']}\n"
                f"- Type: {item.get('type', 'unknown')}\n"
                f"- PDE: {item.get('pde_type', '')}\n"
                f"- Scene: {scene.get('description', '')[:100]}\n"
                f"- Strategy: {strategy.get('rationale', strategy.get('strategy_id', ''))[:150]}\n"
                f"- Outcome: label={outcome.get('label')}, improvement={outcome.get('improvement', {})}\n"
            )
        return "\n".join(parts) if parts else "(empty memory)"
