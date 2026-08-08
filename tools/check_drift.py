#!/usr/bin/env python3
"""Mechanical checks for the classes of drift that repeatedly went unnoticed.

Spec: docs/superpowers/specs/2026-07-28-drift-check-design.md

Every check is exact except E, which is a heuristic over grep output and
therefore carries an explicit allowlist. Run from the repository root:

    python3 tools/check_drift.py

Exit code 0 means every check passed.
"""
from __future__ import annotations

import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent

# Extensions that are not text and must never be inspected for line endings.
BINARY_SUFFIXES = {
    '.png', '.clx', '.pal', '.cel', '.dun', '.raw', '.sv', '.mpq', '.jpg',
    '.jpeg', '.webp', '.jar', '.wav', '.ttf', '.ico', '.icns', '.bin', '.gif',
    '.zip', '.gz', '.xz', '.so', '.a', '.dll', '.exe', '.pdf', '.trn', '.smk',
}

# Suffixes that .editorconfig declares as LF. Everything else text falls under
# [*] and must be CRLF.
LF_SUFFIXES = {'.md', '.py', '.java', '.pot', '.po', '.rb', '.yml', '.sh',
               '.xml', '.plist', '.desktop', '.json', '.lua'}

# Check E allowlist. Every entry was verified present in Source/ at the
# merge-base, i.e. it is pre-existing upstream state and not this fork's doing.
# See the spec's fact basis for the verification command.
E_ALLOWLIST = {
    'DiscardMultipleFields': 'upstream, Source/data/parser.hpp',
    'GetNumTownerTypes': 'upstream, Source/towners.h',
    'GetVisualStoreItemCount': 'upstream, Source/qol/visual_store.h',
    'GetVisualStorePageCount': 'upstream, Source/qol/visual_store.h',
    'ParseFixed6Fraction': 'upstream, Source/utils/parse_int.hpp',
    'ResizeFile': 'upstream, Source/utils/file_util.h',
    'SetAssetsPath': 'upstream, Source/utils/paths.h',
    'TestRotateBlockedMissile': 'upstream, Source/missiles.h',
    # Upstream pagination is not wired into the UI; the two
    # VisualStoreTest.Pagination_* tests are Skipped in ctest.
    'VisualStoreNextPage': 'upstream, Source/qol/visual_store.h',
    'VisualStorePreviousPage': 'upstream, Source/qol/visual_store.h',
}


def run(*args: str) -> str:
    return subprocess.run(args, capture_output=True, text=True, cwd=REPO).stdout


def read_text(path: pathlib.Path) -> str:
    return path.read_text(encoding='utf-8', errors='replace')


def line_ending(path: pathlib.Path) -> str | None:
    """'CRLF', 'LF', or None when the file has no lines."""
    data = path.read_bytes()
    if not data:
        return None
    total = data.count(b'\n')
    if total == 0:
        return None
    return 'CRLF' if data.count(b'\r\n') == total else 'LF'


def is_text(name: str) -> bool:
    return pathlib.PurePath(name).suffix.lower() not in BINARY_SUFFIXES


def expected_ending(name: str) -> str:
    return 'LF' if pathlib.PurePath(name).suffix.lower() in LF_SUFFIXES else 'CRLF'


def merge_base(base_ref: str = 'origin/master') -> str | None:
    base = run('git', 'merge-base', base_ref, 'HEAD').strip()
    return base or None


# --------------------------------------------------------------------------- A

def check_tests_cmake() -> list[str]:
    """Every target listed in Tests.cmake has a matching source file."""
    path = REPO / 'CMake' / 'Tests.cmake'
    if not path.exists():
        return [f'{path} not found']
    failures = []
    listing = False
    for raw in read_text(path).splitlines():
        line = raw.replace('\r', '')
        if re.match(r'^set\((tests|standalone_tests|benchmarks)$', line):
            listing = True
            continue
        if listing and line.startswith(')'):
            listing = False
            continue
        if not listing:
            continue
        target = line.strip()
        if not target or target.startswith('#'):
            continue
        if not (REPO / 'test' / f'{target}.cpp').exists():
            failures.append(f'Tests.cmake lists "{target}" but test/{target}.cpp does not exist')
    return failures


# --------------------------------------------------------------------------- B

def check_placeholder_tests() -> list[str]:
    """No test asserts a tautology."""
    out = run('grep', '-rn', '-e', 'EXPECT_TRUE(true)', '-e', 'ASSERT_TRUE(true)', 'test')
    return [f'placeholder assertion: {line}' for line in out.splitlines() if line]


# --------------------------------------------------------------------------- C

def check_modified_line_endings(base: str) -> list[str]:
    """Files modified relative to the merge-base keep their line ending type."""
    failures = []
    names = run('git', 'diff', '--name-only', '--diff-filter=M', base, 'HEAD').split('\n')
    for name in filter(None, names):
        if not is_text(name):
            continue
        path = REPO / name
        if not path.exists():
            continue
        old_blob = subprocess.run(['git', 'show', f'{base}:{name}'],
                                  capture_output=True, cwd=REPO).stdout
        if not old_blob:
            continue
        old_total = old_blob.count(b'\n')
        if old_total == 0:
            continue
        old = 'CRLF' if old_blob.count(b'\r\n') == old_total else 'LF'
        new = line_ending(path)
        if new is not None and old != new:
            failures.append(f'{name}: line endings changed {old} -> {new}')
    return failures


# -------------------------------------------------------------------------- C2

def check_added_line_endings(base: str) -> list[str]:
    """Files added relative to the merge-base match .editorconfig."""
    failures = []
    names = run('git', 'diff', '--name-only', '--diff-filter=A', base, 'HEAD').split('\n')
    for name in filter(None, names):
        if not is_text(name):
            continue
        path = REPO / name
        if not path.exists():
            continue
        actual = line_ending(path)
        if actual is None:
            continue
        want = expected_ending(name)
        if actual != want:
            failures.append(f'{name}: line endings are {actual}, .editorconfig wants {want}')
    return failures


# --------------------------------------------------------------------------- E

DECL = re.compile(
    r'^\s*(?:DVL_API_FOR_TEST\s+)?[A-Za-z_][\w:<>,\s&*\[\]]*?\b([A-Z][A-Za-z0-9_]*)\s*\(')


def declared_symbols() -> dict[str, str]:
    """Function-like names declared in Source headers, mapped to the header."""
    found: dict[str, str] = {}
    for header in sorted((REPO / 'Source').rglob('*.h*')):
        rel = header.relative_to(REPO).as_posix()
        for line in read_text(header).splitlines():
            stripped = line.strip()
            if not stripped.endswith(';') or stripped.startswith(('//', '*', '#')):
                continue
            match = DECL.match(stripped)
            if match:
                found.setdefault(match.group(1), rel)
    return found


def grep_symbol(symbol: str, root: str) -> list[str]:
    out = run('grep', '-rnw', '--include=*.cpp', '--include=*.h', '--include=*.hpp',
              symbol, root)
    return [line for line in out.splitlines() if line]


def is_definition(text: str, symbol: str) -> bool:
    """A definition starts at column 0; an indented line is always a call.

    Getting this backwards produces false negatives, which are worse than false
    positives: they hide real dead code. CanLearnSpell was missed this way when
    the rule also treated any line ending in '{' as a definition, because
    'if (!CanLearnSpell(...)) {' matched.
    """
    if text[:1] in (' ', '\t'):
        return False
    return re.match(rf'^[\w:<>,\s&*\[\]]*\b{symbol}\s*\(', text) is not None


def check_test_only_functions() -> list[str]:
    failures = []
    for symbol, header in declared_symbols().items():
        if symbol in E_ALLOWLIST:
            continue
        if not grep_symbol(symbol, 'test'):
            continue
        production = []
        for line in grep_symbol(symbol, 'Source'):
            path, _lineno, text = line.split(':', 2)
            stripped = text.strip()
            if path == header and stripped.endswith(';'):
                continue                          # its own declaration
            if stripped.startswith(('//', '*', '/*')):
                continue                          # comment
            if is_definition(text, symbol):
                continue                          # its own definition
            production.append(line)
        if not production:
            failures.append(
                f'{symbol} ({header}) has test callers but no production caller')
    return failures


# ------------------------------------------------------------------------ main

def main() -> int:
    import argparse
    parser = argparse.ArgumentParser(description='Mechanical drift checks for the Better D1 fork.')
    parser.add_argument('--base', default='origin/master',
                        help='baseline ref to compare against (default: origin/master)')
    args = parser.parse_args()
    base = merge_base(args.base)
    checks: list[tuple[str, list[str]]] = [
        ('A  Tests.cmake entries have source files', check_tests_cmake()),
        ('B  no placeholder assertions in tests', check_placeholder_tests()),
    ]
    if base is None:
        checks.append((f'C  modified files keep line endings (base {args.base})', ['no merge-base; skipped']))
        checks.append((f'C2 added files match .editorconfig (base {args.base})', ['no merge-base; skipped']))
    else:
        checks.append(('C  modified files keep line endings', check_modified_line_endings(base)))
        checks.append(('C2 added files match .editorconfig', check_added_line_endings(base)))
    checks.append(('E  no test-only production functions', check_test_only_functions()))

    failed = 0
    for name, failures in checks:
        if failures:
            failed += 1
            print(f'FAIL {name}')
            for item in failures:
                print(f'       {item}')
        else:
            print(f'PASS {name}')

    if base is not None:
        print(f'\nmerge-base: {base[:9]}   check E allowlist: {len(E_ALLOWLIST)} upstream symbols')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
