"""Execution of eval cases against the DevilutionX test suite.

Each case runs through tools/run_tests.py --no-build --test <binary> --filter,
which already produces a machine-readable JSON report. This module wraps that
contract and captures stdout for assertion checks.
"""

from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path

from .models import EvalCase

REPO_ROOT = Path(__file__).resolve().parent.parent.parent


class RunResult:
    __slots__ = ('exit_code', 'stdout', 'json', 'timed_out')

    def __init__(self, exit_code: int, stdout: str, report: dict | None, timed_out: bool = False):
        self.exit_code = exit_code
        self.stdout = stdout
        self.json = report
        self.timed_out = timed_out


def run_case(case: EvalCase, build_dir: Path, no_build: bool = True, verbose: bool = False) -> RunResult:
    """Execute a case and return its raw result (before assertion).

    Runs the gtest binary directly so we capture its real stdout for
    output_contains assertions, then parses passed/failed/skipped from it.
    """
    binary = build_dir / case.binary
    cmd = [str(binary)]
    if case.gtest_filter:
        cmd += ["--gtest_filter=" + case.gtest_filter]
    cmd += case.run_args

    env = {**os.environ, **case.env}
    try:
        proc = subprocess.run(
            cmd, cwd=build_dir, capture_output=True, text=True,
            timeout=case.timeout, env=env,
        )
    except subprocess.TimeoutExpired:
        return RunResult(-1, f"<timeout after {case.timeout}s>", None, timed_out=True)

    stdout = (proc.stdout or '')
    if proc.stderr:
        stdout += proc.stderr
    if verbose:
        print(stdout[-2000:], file=sys.stderr)

    # Parse gtest summary into the same shape run_tests.py emits (steps.single).
    passed = failed = skipped = 0
    for line in stdout.splitlines():
        m = re.search(r"\[\s*PASSED\s*\]\s+(\d+) tests?", line)
        if m:
            passed = int(m.group(1))
        m = re.search(r"\[\s*FAILED\s*\]\s+(\d+) tests?", line)
        if m:
            failed = int(m.group(1))
        m = re.search(r"\[\s*SKIPPED\s*\]\s+(\d+) tests?", line)
        if m:
            skipped = int(m.group(1))
    report = {
        'steps': {'single': {
            'test': case.binary, 'filter': case.gtest_filter,
            'returncode': proc.returncode, 'passed': passed, 'failed': failed, 'skipped': skipped,
        }}
    }
    return RunResult(proc.returncode, stdout, report)


def binary_exists(build_dir: Path, case: EvalCase) -> bool:
    if not case.binary:
        return False
    return (build_dir / case.binary).exists()
