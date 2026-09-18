#!/usr/bin/env python3
"""Derive the squad-off playtest variant from the CURRENT shipped roster params table.

Why this exists: the committed fixture
`test/fixtures/txtdata/monsters/level_roster_params_squads_off.tsv` was generated before the
content density contract, so using it as the A/B control silently reverts four things at once
(L13/L14 tail_draw 4 -> 1/2, L15's class_floors, and the entire L17-24 block, which the
fixture predates). A playtest whose two arms differ in four variables measures nothing.

This script derives the variant from the shipped table and asserts that squad_chance is the
ONLY difference, so the control stays single-variable no matter how the shipped table evolves.

Usage:
  python3 tools/make_playtest_variant.py            # overwrite the shipped table (variant B)
  python3 tools/make_playtest_variant.py --check F  # assert F differs only in squad_chance
  python3 tools/make_playtest_variant.py --restore   # git checkout the shipped table
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import pathlib
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
SHIPPED = REPO / "assets" / "txtdata" / "monsters" / "level_roster_params.tsv"
SQUAD_CHANCE = "squad_chance"


def read_table(path: pathlib.Path) -> list[list[str]]:
    with path.open(encoding="utf-8", newline="") as handle:
        return [row for row in csv.reader(handle, delimiter="\t") if row]


def write_table(path: pathlib.Path, rows: list[list[str]]) -> None:
    text = "\r\n".join("\t".join(row) for row in rows) + "\r\n"
    path.write_bytes(text.encode("utf-8"))


def digest(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()[:16]


def squad_index(rows: list[list[str]]) -> int:
    if SQUAD_CHANCE not in rows[0]:
        raise SystemExit(f"{SHIPPED} has no {SQUAD_CHANCE} column")
    return rows[0].index(SQUAD_CHANCE)


def only_squad_chance_differs(shipped: pathlib.Path, variant: pathlib.Path) -> list[str]:
    """Return the list of differences outside the squad_chance column (empty when clean)."""
    a, b = read_table(shipped), read_table(variant)
    problems: list[str] = []
    if a[0] != b[0]:
        problems.append(f"header differs: {a[0]} vs {b[0]}")
        return problems
    idx = squad_index(a)
    levels_a = {row[0] for row in a[1:]}
    levels_b = {row[0] for row in b[1:]}
    if levels_a != levels_b:
        problems.append(f"level sets differ: only-shipped={sorted(levels_a - levels_b)} only-variant={sorted(levels_b - levels_a)}")
    by_level_a = {row[0]: row for row in a[1:]}
    by_level_b = {row[0]: row for row in b[1:]}
    for level in sorted(levels_a & levels_b, key=lambda value: int(value)):
        ra, rb = by_level_a[level], by_level_b[level]
        if len(ra) != len(rb):
            problems.append(f"L{level}: column count {len(ra)} vs {len(rb)}")
            continue
        for column, (x, y) in enumerate(zip(ra, rb)):
            if column != idx and x != y:
                problems.append(f"L{level}: column {a[0][column]} differs ({x} -> {y})")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", metavar="VARIANT", help="assert VARIANT differs from the shipped table only in squad_chance")
    parser.add_argument("--restore", action="store_true", help="restore the shipped table via git checkout")
    parser.add_argument("--out", metavar="PATH", help="write the variant here instead of over the shipped table")
    args = parser.parse_args()

    if args.restore:
        subprocess.run(["git", "checkout", "--", str(SHIPPED.relative_to(REPO))], cwd=REPO, check=True)
        print(f"restored {SHIPPED.relative_to(REPO)} ({digest(SHIPPED)})")
        return 0

    if args.check:
        variant = pathlib.Path(args.check)
        problems = only_squad_chance_differs(SHIPPED, variant)
        if problems:
            print("NOT a single-variable control:")
            for problem in problems:
                print(f"  - {problem}")
            return 1
        print(f"single-variable control OK ({variant} differs only in {SQUAD_CHANCE})")
        return 0

    rows = read_table(SHIPPED)
    idx = squad_index(rows)
    before = digest(SHIPPED)
    for row in rows[1:]:
        row[idx] = "0"
    target = pathlib.Path(args.out) if args.out else SHIPPED
    write_table(target, rows)
    print(f"variant written: {target.relative_to(REPO) if target.is_relative_to(REPO) else target}")
    print(f"  shipped hash {before} -> variant hash {digest(target)}; squad_chance set to 0 on {len(rows) - 1} levels")
    print("  run: ninja -C build devilutionx_mpq")
    print("  restore: python3 tools/make_playtest_variant.py --restore")
    return 0


if __name__ == "__main__":
    sys.exit(main())
