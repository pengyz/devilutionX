#!/usr/bin/env python3
"""D1-Eval: YAML-driven integration eval for DevilutionX.

A case is a declarative scenario that SELECTS a real assertion mechanism
(an existing gtest binary + filter) and adds meaning: category, difficulty,
tolerances, scoring, report aggregation. Assertion authority is always the
gtest binary's exit code + machine JSON — never an LLM.

Usage:
  python3 tools/eval/backend.py --check [path]      # validate YAML schema
  python3 tools/eval/backend.py --list [path]        # inventory table
  python3 tools/eval/backend.py --run <case-id>      # run one case (glob)
  python3 tools/eval/backend.py --run-file <path>    # run explicit file
  python3 tools/eval/backend.py --dir <category-dir> # all cases in dir
  python3 tools/eval/backend.py --set <name>         # run eval/cases/_<name>.yaml
  python3 tools/eval/backend.py --smoke              # = --set smoke (gate)
  python3 tools/eval/backend.py --filter <glob>      # cases matching id glob
  python3 tools/eval/backend.py --include-side-effects
  python3 tools/eval/backend.py --json <path>        # write eval-summary.json

Exit codes: 0=all evaluated passed; 1=any failed (incl. missing binary);
2=no cases matched; 3=invalid YAML/case not found.
"""

from __future__ import annotations

import argparse
import fnmatch
import subprocess
import sys
import time
from pathlib import Path

from . import assertions, models, runner, scoring, writer

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
CASES_DIR = REPO_ROOT / "eval" / "cases"
RESULTS_ROOT = REPO_ROOT / "eval" / "results"
DEFAULT_BUILD_DIR = REPO_ROOT / "build"
CHECK_DRIFT = REPO_ROOT / "tools" / "check_drift.py"


def git_head() -> str:
    try:
        out = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=REPO_ROOT,
                             capture_output=True, text=True, timeout=10)
        return out.stdout.strip() or 'unknown'
    except Exception:
        return 'unknown'


def mpq_present(build_dir: Path | None = None) -> bool:
    """Detect whether game data (spawn.mpq/DIABDAT.MPQ) is reachable.
    Checks both the standard home pref path and the build directory (where CI
    downloads spawn.mpq next to the test binaries)."""
    import os
    pref = os.path.expanduser('~/.local/share/diasurgical/devilution')
    candidates = [Path(pref)]
    if build_dir is not None:
        candidates.append(build_dir)
    names = ('spawn.mpq', 'DIABDAT.MPQ', 'diabdat.mpq')
    return any((p / name).exists() for p in candidates for name in names)


def find_cases(selector: str | None, cases_dir: Path) -> list[models.EvalCase]:
    """Resolve cases by id-glob; all cases if selector is None."""
    all_cases = models.load_cases_dir(cases_dir)
    if not selector:
        return all_cases
    return [c for c in all_cases if fnmatch.fnmatch(c.id, selector)]


def run_case(case: models.EvalCase, build_dir: Path, include_side_effects: bool) -> dict:
    """Run one case and produce the result dict for the report."""
    start = time.monotonic()
    skip_reason = assertions.classify_skip(case, mpq_present(build_dir), include_side_effects)
    if skip_reason:
        return {
            'case_id': case.id, 'category': case.category, 'difficulty': case.difficulty,
            'passed': False, 'skipped': True, 'skip_reason': skip_reason,
            'score': 0, 'max_score': case.max_score, 'failures': [],
            'duration_ms': 0, 'run': {'backend': 'gtest', 'binary': case.binary, 'filter': case.gtest_filter},
            'stdout': None, 'report_json': None,
        }

    if not runner.binary_exists(build_dir, case):
        return {
            'case_id': case.id, 'category': case.category, 'difficulty': case.difficulty,
            'passed': False, 'skipped': False, 'skip_reason': None,
            'score': 0, 'max_score': case.max_score,
            'failures': [f"binary not found in build/: {case.binary} (run tools/run_tests.py first)"],
            'duration_ms': 0, 'run': {'backend': 'gtest', 'binary': case.binary, 'filter': case.gtest_filter},
            'stdout': None, 'report_json': None,
        }

    result = runner.run_case(case, build_dir, verbose=False)
    passed, failures = assertions.apply_assertions(case, result)
    score = scoring.score_case(case, result, passed, failures)
    return {
        'case_id': case.id, 'category': case.category, 'difficulty': case.difficulty,
        'passed': passed, 'skipped': False, 'skip_reason': None,
        'score': score, 'max_score': case.max_score, 'failures': failures,
        'duration_ms': int((time.monotonic() - start) * 1000),
        'run': {'backend': 'gtest', 'binary': case.binary, 'filter': case.gtest_filter},
        'stdout': result.stdout, 'report_json': result.json,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='validate YAML schema (use with --path or all)')
    parser.add_argument('--list', action='store_true', help='list cases (use with --path or all)')
    parser.add_argument('--path', type=Path, help='path to a case file or directory (for --check/--list)')
    parser.add_argument('--run', help='run case(s) by id glob')
    parser.add_argument('--run-file', type=Path)
    parser.add_argument('--dir', type=Path, help='run all cases under directory')
    parser.add_argument('--set', help='run suite eval/cases/_<name>.yaml')
    parser.add_argument('--smoke', action='store_true', help='run _smoke.yaml as gate')
    parser.add_argument('--filter', help='alias for --run with glob')
    parser.add_argument('--tag', help='run cases with this tag')
    parser.add_argument('--include-side-effects', action='store_true')
    parser.add_argument('--json', type=Path, help='write eval-summary.json to path')
    parser.add_argument('--build-dir', type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()

    try:
        # ---- validation / listing modes ------------------------------------
        if args.check:
            path = args.path
            if path and path.is_file():
                files = [path]
            elif path and path.is_dir():
                files = [p for p in sorted(path.rglob('*.yaml')) if not p.name.startswith('_')]
            else:
                files = [c.path for c in models.load_cases_dir(CASES_DIR)]
            errors = []
            for f in files:
                try:
                    models.load_case(f)
                except models.EvalError as e:
                    errors.append(str(e))
            if errors:
                for e in errors:
                    print(f"FAIL {e}")
                return 1
            print(f"OK: {len(files)} case(s) valid")
            return 0

        if args.list:
            path = args.path
            cases = models.load_cases_dir(path) if path and path.is_dir() else models.load_cases_dir(CASES_DIR)
            print(f"{'id':40s} {'category':20s} {'difficulty':10s} {'binary':30s} filter")
            for c in cases:
                print(f"{c.id:40s} {c.category:20s} {c.difficulty:10s} {c.binary or '':30s} {c.gtest_filter or ''}")
            return 0

        # ---- resolve case selection ------------------------------------------
        selected: list[models.EvalCase] = []
        if args.run_file:
            selected = [models.load_case(args.run_file)]
        elif args.run or args.filter:
            selector = args.run or args.filter
            selected = find_cases(selector, CASES_DIR)
            if not selected:
                print(f"No cases match '{selector}'", file=sys.stderr)
                return 2
        elif args.dir:
            selected = models.load_cases_dir(args.dir)
        elif args.set or args.smoke:
            suite_name = 'smoke' if args.smoke else args.set
            selected = models.load_suite(CASES_DIR / f'_{suite_name}.yaml', CASES_DIR)
        else:
            selected = find_cases(None, CASES_DIR)

        if args.tag:
            selected = [c for c in selected if args.tag in c.tags]

        if not selected:
            print("No cases selected.", file=sys.stderr)
            return 2

        # ---- execute ----------------------------------------------------------
        print(f"D1-EVAL: running {len(selected)} case(s) (git {git_head()[:10]})", file=sys.stderr)
        results = [run_case(c, args.build_dir.resolve(), args.include_side_effects) for c in selected]

        summary_path, summary = writer.write_report(
            selected, results, RESULTS_ROOT, git_head())
        writer.print_table(summary)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(__import__('json').dumps(summary, indent=2), encoding='utf-8')
            print(f"\nReport written to {args.json}", file=sys.stderr)

        evaluated = [r for r in results if not r['skipped']]
        failed = [r for r in evaluated if not r['passed']]
        if failed:
            return 1
        if not evaluated:
            # All skipped: gate fails (vacuous-truth guard), non-gate mode warns.
            print("WARNING: all cases skipped", file=sys.stderr)
            return 1 if (args.smoke or args.set) else 0
        return 0

    except models.EvalError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 3


if __name__ == '__main__':
    sys.exit(main())
