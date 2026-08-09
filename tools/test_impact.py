#!/usr/bin/env python3
"""Map changed source files to the test binaries that exercise them.

AI agents can call this after editing Source/ files to find the minimal set
of tests to run, instead of always running the full 670-test suite.

Usage:
  python3 tools/test_impact.py Source/items.cpp          # direct
  python3 tools/test_impact.py --changed files.txt        # from file list
  python3 tools/test_impact.py --diff                     # from git diff (unstaged)
  python3 tools/test_impact.py --diff HEAD~1              # from git diff vs ref

Output: newline-separated test binary names (stdout) + explanation (stderr).
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def _binary_path(build_dir: Path, name: str) -> Path:
    """Windows: test binaries carry .exe; exists() does not resolve it."""
    p = build_dir / name
    return p if p.exists() else build_dir / (name + ".exe")

# source file (or directory prefix) -> test binaries that exercise it.
# Curated from the test suite structure; keep in sync with CMake/Tests.cmake.
IMPACT_MAP = {
    "Source/items.cpp": ["items_test", "inv_test", "pack_test", "stores_test", "vendor_test", "store_transaction_test", "visual_store_test", "consumable_stack_test"],
    "Source/items.h": ["items_test", "inv_test", "pack_test", "stores_test", "vendor_test"],
    "Source/inv.cpp": ["inv_test", "inventory_ui_test"],
    "Source/player.cpp": ["player_test", "char_panel_test", "timedemo_test"],
    "Source/player.h": ["player_test", "char_panel_test", "inv_test"],
    "Source/spells.cpp": ["spell_ui_test", "spell_ux_test", "spelldat_test", "spell_tooltip_test"],
    "Source/tables/spelldat": ["spelldat_test", "spell_tooltip_test", "spell_ux_test"],
    "Source/missiles.cpp": ["missiles_test"],
    "Source/options.cpp": ["game_menu_test", "diablo_test"],
    "Source/options.h": ["game_menu_test", "diablo_test"],
    "Source/levels/gendung": ["drlg_common_test", "drlg_l1_test", "drlg_l2_test", "drlg_l3_test", "drlg_l4_test", "scrollrt_test", "vision_test", "can_target_test"],
    "Source/levels/drlg_l1": ["drlg_l1_test"],
    "Source/levels/drlg_l2": ["drlg_l2_test"],
    "Source/levels/drlg_l3": ["drlg_l3_test"],
    "Source/levels/drlg_l4": ["drlg_l4_test"],
    "Source/cursor.cpp": ["cursor_test", "can_target_test"],
    "Source/track.cpp": ["cursor_test", "can_target_test"],
    "Source/controls/plrctrls.cpp": ["can_target_test", "cursor_test"],
    "Source/engine/render/scrollrt": ["scrollrt_test", "text_render_integration_test"],
    "Source/automap.cpp": ["automap_test"],
    "Source/lighting.cpp": ["vision_test", "scrollrt_test", "dark_expedition_light_test"],
    "Source/stores.cpp": ["stores_test", "vendor_test", "store_transaction_test"],
    "Source/pack.cpp": ["pack_test", "writehero_test", "timedemo_test"],
    "Source/loadsave.cpp": ["pack_test", "writehero_test", "timedemo_test"],
    "Source/quests.cpp": ["quests_test", "quest_script_test"],
    "Source/object.cpp": ["tile_properties_test", "dark_expedition_scroll_test"],
    "Source/townerdat.cpp": ["townerdat_test"],
    "Source/lua/": ["lua_integration_test", "quest_script_test"],
    "assets/txtdata/spells/spelldat.tsv": ["spelldat_test", "spell_tooltip_test"],
    "assets/txtdata/items/": ["items_test", "pack_test", "stores_test"],
    "mods/hf/txtdata/": ["spelldat_test", "items_test"],
    "test/": [],  # test files themselves: caller runs that binary
}

# Tests that are broad enough to catch anything (fallback when no mapping hits).
DEFAULT_TESTS = ["diablo_test", "items_test", "inv_test", "player_test", "pack_test", "spelldat_test"]


def changed_files_from_git(ref: str | None) -> list[str]:
    """Return changed file paths from git."""
    cmd = ["git", "diff", "--name-only"]
    if ref:
        cmd.append(ref)
    result = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        return []
    return [l for l in result.stdout.splitlines() if l.strip()]


def impact_for_file(path: str) -> set[str]:
    """Return test binaries for one changed file."""
    matched = set()
    for prefix, tests in IMPACT_MAP.items():
        if path.startswith(prefix):
            matched.update(tests)
    return matched


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="*", help="changed file paths")
    parser.add_argument("--changed", type=Path, help="file containing newline-separated paths")
    parser.add_argument("--diff", nargs="?", const="", default=None, help="git diff vs ref (default: unstaged)")
    args = parser.parse_args()

    changed = list(args.files)
    if args.changed:
        changed += [l for l in args.changed.read_text().splitlines() if l.strip()]
    if args.diff is not None:
        ref = args.diff or None
        changed += changed_files_from_git(ref)

    if not changed:
        print("No changed files detected.", file=sys.stderr)
        parser.print_help()
        return 1

    tests: set[str] = set()
    unmapped: list[str] = []
    for path in changed:
        hits = impact_for_file(path)
        if hits:
            tests.update(hits)
        else:
            unmapped.append(path)

    if not tests:
        tests.update(DEFAULT_TESTS)
        print(f"No mapping for {unmapped}; using default set: {DEFAULT_TESTS}", file=sys.stderr)
    else:
        if unmapped:
            print(f"Unmapped files (no specific tests): {unmapped}", file=sys.stderr)

    # Filter to existing test binaries.
    build_dir = REPO_ROOT / "build"
    existing = []
    for t in sorted(tests):
        if _binary_path(build_dir, t).exists():
            existing.append(t)
    if not existing:
        print("No matching test binaries built; run tools/run_tests.py first.", file=sys.stderr)
        return 1

    print("\n".join(existing))
    print(f"\n→ {len(existing)} tests recommended (from {len(changed)} changed files)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
