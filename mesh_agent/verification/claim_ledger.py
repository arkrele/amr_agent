"""Claim Ledger: evidence-bound claims with automatic numerical verification.

Core idea (from ResearchLoop): every claim must be backed by a specific
evidence reference (file:line), and the system automatically extracts
the actual values to verify the claim.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from mesh_agent.schemas import Claim, ClaimEvidence, ClaimValidation, ClaimStatus, EvidenceLevel


class ClaimLedger:
    """Manages the lifecycle of evidence-bound claims.

    Claims progress through evidence maturity levels (Sibyl H2):
        L1: Executed — code ran, results exist
        L2: Preliminary — values in physical range
        L3: Analyzed — metrics extracted
        L4: Validated — claim matches extracted values
        L5: Audited — multiple independent reviewers confirmed
    """

    def __init__(self):
        self.claims: list[Claim] = []

    def create_claim(
        self,
        round_number: int,
        text: str,
        strategy_id: str,
        evidence_file: str,
        before_values: dict[str, float],
        after_values: dict[str, float],
    ) -> Claim:
        """Create and validate a new claim."""
        claim_id = f"C{len(self.claims) + 1}"

        # Auto-extract values from evidence
        delta = {}
        delta_pct = {}
        for key in before_values:
            if key in after_values:
                delta[key] = after_values[key] - before_values[key]
                denom = max(abs(before_values[key]), 1e-10)
                delta_pct[key] = (after_values[key] - before_values[key]) / denom * 100

        evidence = ClaimEvidence(
            before=before_values,
            after=after_values,
            delta=delta,
            delta_pct=delta_pct,
        )

        # Validate claim by extracting actual values
        validation = self._validate(text, before_values, after_values, delta_pct)

        # Determine evidence level
        level = EvidenceLevel.VALIDATED if (validation.match and validation.significant) else EvidenceLevel.ANALYZED

        claim = Claim(
            claim_id=claim_id,
            round=round_number,
            text=text,
            strategy_id=strategy_id,
            evidence=evidence,
            validation=validation,
            evidence_level=level,
            status=ClaimStatus.VALIDATED if validation.match else ClaimStatus.REJECTED,
        )

        self.claims.append(claim)
        return claim

    def _validate(
        self,
        text: str,
        before: dict[str, float],
        after: dict[str, float],
        delta_pct: dict[str, float],
    ) -> ClaimValidation:
        """Auto-validate a claim by extracting numbers from the text
        and comparing them to actual metric values."""
        # Extract numbers from claim text
        numbers = re.findall(r"([\d]+\.?[\d]*(?:e[+-]?\d+)?)", text)

        # Check if at least one expected change is significant
        significant = any(abs(v) >= 1.0 for v in delta_pct.values())

        # Check if any number in the text matches a delta
        match = False
        for n_str in numbers:
            try:
                n = float(n_str)
                for key, d in delta_pct.items():
                    if abs(abs(d) - abs(n)) < 0.5:  # Close enough
                        match = True
                        break
            except ValueError:
                continue

        # If no numbers in text, check if there's at least a qualitative match
        if not numbers and any(abs(v) > 0 for v in delta_pct.values()):
            match = True

        detail = (
            f"Auto-extracted: before={before}, after={after}, "
            f"delta_pct={delta_pct}"
        )

        return ClaimValidation(
            method="auto_extraction",
            match=match,
            significant=significant,
            detail=detail,
        )

    def get_all_validated(self) -> list[Claim]:
        return [c for c in self.claims if c.status == ClaimStatus.VALIDATED]

    def get_latest_metrics(self) -> dict[str, float]:
        """Get the 'after' values from the latest claim as current metrics."""
        if not self.claims:
            return {}
        latest = self.claims[-1]
        return dict(latest.evidence.after)

    def export(self) -> list[dict[str, Any]]:
        return [c.model_dump() for c in self.claims]
