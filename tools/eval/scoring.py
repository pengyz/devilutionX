"""Scoring for eval cases.

The score is a secondary "how green" metric; pass/fail from the assertion
engine is authoritative. Default weights: pass=10, each output_contains=+5,
each output_not_contains=+3.
"""

from __future__ import annotations

from .models import EvalCase
from .runner import RunResult


def score_case(case: EvalCase, result: RunResult, passed: bool, failures: list[str]) -> int:
    """Compute the score earned for a case."""
    if not passed:
        return 0
    score = case.scoring['pass']
    for pattern in case.expected.get('output_contains', []):
        if pattern in result.stdout:
            score += case.scoring['output_contains']
    for pattern in case.expected.get('output_not_contains', []):
        if pattern not in result.stdout:
            score += case.scoring['output_not_contains']
    return score
