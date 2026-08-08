"""Report writing: eval-summary.json + human-readable table.

Ports the shape of osbot's writer.py: timestamped result dir, per-case
artifacts (stdout + report json), category rollup, rolling GC.
"""

from __future__ import annotations

import json
import shutil
import time
from collections import defaultdict
from pathlib import Path

from .models import EvalCase


def write_report(
    cases: list[EvalCase],
    results: list[dict],
    results_root: Path,
    git_head: str,
) -> tuple[Path, dict]:
    """Write eval-summary.json + per-case artifacts. Returns (summary_path, summary_dict)."""
    ts = time.strftime('%Y%m%d-%H%M%S')
    run_dir = results_root / ts
    run_dir.mkdir(parents=True, exist_ok=True)

    evaluated = [r for r in results if not r['skipped']]
    passed = [r for r in evaluated if r['passed']]
    failed = [r for r in evaluated if not r['passed']]
    skipped = [r for r in results if r['skipped']]

    def rate(nums):
        return round(sum(nums) / len(nums), 3) if nums else 0.0

    # Category rollup.
    cats: dict[str, dict] = defaultdict(lambda: {'passed': 0, 'total': 0, 'evaluated': 0, 'skipped': 0})
    for r in results:
        c = cats[r['category']]
        c['total'] += 1
        if r['skipped']:
            c['skipped'] += 1
        else:
            c['evaluated'] += 1
            if r['passed']:
                c['passed'] += 1
    for c in cats.values():
        c['pass_rate'] = rate([1] * c['passed'] + [0] * (c['evaluated'] - c['passed']))

    summary = {
        'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
        'git_head': git_head,
        'total_cases': len(results),
        'evaluated_cases': len(evaluated),
        'passed_cases': len(passed),
        'failed_cases': len(failed),
        'skipped_cases': len(skipped),
        'pass_rate': rate([1] * len(passed) + [0] * len(failed)),
        'evaluated_pass_rate': rate([1] * len(passed) + [0] * len(failed)),
        'total_score': sum(r['score'] for r in evaluated),
        'max_total_score': sum(r['max_score'] for r in evaluated),
        'score_rate': rate([r['score'] / r['max_score'] for r in evaluated if r['max_score'] > 0]),
        'categories': dict(cats),
        'cases': [],
        'skipped_results': [],
    }

    for r in results:
        case_dir = run_dir / r['case_id']
        case_dir.mkdir(exist_ok=True)
        artifact_paths = {}
        if r.get('stdout') is not None:
            so = case_dir / 'stdout.txt'
            so.write_text(r['stdout'], encoding='utf-8')
            artifact_paths['stdout'] = str(so)
        if r.get('report_json') is not None:
            rj = case_dir / 'report.json'
            rj.write_text(json.dumps(r['report_json'], indent=2), encoding='utf-8')
            artifact_paths['json'] = str(rj)
        entry = {
            'case_id': r['case_id'],
            'category': r['category'],
            'difficulty': r['difficulty'],
            'passed': r['passed'],
            'skipped': r['skipped'],
            'skip_reason': r.get('skip_reason'),
            'score': r['score'],
            'max_score': r['max_score'],
            'failures': r['failures'],
            'duration_ms': r.get('duration_ms', 0),
            'run': r['run'],
            'artifacts': artifact_paths,
        }
        if r['skipped']:
            summary['skipped_results'].append(entry)
        else:
            summary['cases'].append(entry)

    summary_path = run_dir / 'eval-summary.json'
    summary_path.write_text(json.dumps(summary, indent=2), encoding='utf-8')

    # Rolling GC: keep only the last 3 result dirs.
    _gc(results_root, keep=3)
    return summary_path, summary


def _gc(root: Path, keep: int) -> None:
    dirs = sorted([d for d in root.iterdir() if d.is_dir() and d.name[:8].isdigit()], reverse=True)
    for d in dirs[keep:]:
        shutil.rmtree(d, ignore_errors=True)


def print_table(summary: dict) -> None:
    """Human-readable report."""
    print(f"\nD1-EVAL {summary['timestamp']} (git {summary['git_head'][:10]})")
    print(f"  evaluated: {summary['evaluated_cases']}  "
          f"passed: {summary['passed_cases']}  failed: {summary['failed_cases']}  "
          f"skipped: {summary['skipped_cases']}  pass_rate: {summary['pass_rate']}")
    for entry in summary['cases']:
        status = 'PASS' if entry['passed'] else 'FAIL'
        print(f"  [{status}] [{entry['category']}] {entry['case_id']} "
              f"({entry['score']}/{entry['max_score']}) "
              f"{' | '.join(entry['failures'])[:100] if entry['failures'] else ''}")
    if summary['skipped_results']:
        print("  skipped:")
        for entry in summary['skipped_results']:
            print(f"    [{entry['case_id']}] {entry['skip_reason']}")
    if summary['categories']:
        print("  by category:")
        for name, c in summary['categories'].items():
            print(f"    {name:20s} {c['passed']}/{c['evaluated']} ({c['pass_rate']})")
