"""Assertion engine: applies an eval case's `expected{}` block to a run result.

Every check returns a (ok, failure_message) pair. The case passes iff ALL
checks pass (AND semantics). Assertion authority is always the gtest binary's
JSON report + captured output — never an LLM.
"""

from __future__ import annotations

from .models import EvalCase
from .runner import RunResult


def apply_assertions(case: EvalCase, result: RunResult) -> tuple[bool, list[str]]:
    """Return (passed, failure_messages). Passed = no failures and exit ok."""
    failures: list[str] = []
    expected = case.expected

    # Timeout is always a failure.
    if result.timed_out:
        failures.append(f"timed out after {case.timeout}s")
        return False, failures

    # Process exit code.
    want_exit = expected.get('exit_code', 0)
    if result.exit_code != want_exit:
        failures.append(f"exit_code={result.exit_code}, expected {want_exit}")

    # Machine JSON assertions.
    report = result.json
    if report is not None and 'steps' in report:
        single = report['steps'].get('single', {})
        max_failed = expected.get('failed', 0)
        got_failed = single.get('failed', 0)
        if got_failed > max_failed:
            failures.append(f"failed={got_failed} > allowed {max_failed}")

        min_passed = expected.get('passed_min', 1)
        got_passed = single.get('passed', 0)
        if got_passed < min_passed:
            failures.append(f"passed={got_passed} < required {min_passed} (vacuous pass guard)")

        max_skipped = expected.get('skipped_max')
        if max_skipped is not None:
            got_skipped = single.get('skipped', 0)
            if got_skipped > max_skipped:
                failures.append(f"skipped={got_skipped} > allowed {max_skipped}")

    # Output regex assertions.
    for pattern in expected.get('output_contains', []):
        if pattern not in result.stdout:
            failures.append(f"stdout missing: {pattern!r}")
    for pattern in expected.get('output_not_contains', []):
        if pattern in result.stdout:
            failures.append(f"stdout unexpectedly contains: {pattern!r}")

    # Metrics (phase 2: numeric perf thresholds).
    for name, spec in (expected.get('metrics') or {}).items():
        op = spec.get('op', 'gte')
        want = spec.get('value')
        source = spec.get('source', '')
        if not source:
            failures.append(f"metrics.{name}: missing 'source' regex")
            continue
        # source: "stdout-regex:<pattern>:<group_index>"
        import re
        m = re.search(source, result.stdout)
        if not m:
            failures.append(f"metrics.{name}: pattern not found in stdout")
            continue
        try:
            got = float(m.group(1))
        except (IndexError, ValueError):
            failures.append(f"metrics.{name}: cannot parse value from stdout")
            continue
        ok = {'gte': got >= want, 'lte': got <= want, 'eq': got == want, 'ne': got != want}[op]
        if not ok:
            failures.append(f"metrics.{name}: {got} !{op} {want}")

    return not failures, failures


def classify_skip(case: EvalCase, mpq_present: bool, include_side_effects: bool) -> str | None:
    """Return a skip reason, or None if the case should run."""
    if case.mpq_required and not mpq_present:
        return "MPQ assets (spawn.mpq or DIABDAT.MPQ) not found"
    if case.side_effect and not include_side_effects:
        return "side_effect case; use --include-side-effects"
    return None
