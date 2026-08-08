#!/usr/bin/env python3
"""AI-friendly integration test runner for DevilutionX.

Builds all test targets (benchmarks excluded from the default flow to avoid
the system-lib LTO issue; see CI workflow), runs ctest, the drift check, and
emits a structured JSON report that agents can parse.

Usage:
  python3 tools/run_tests.py                 # build + full ctest + drift check
  python3 tools/run_tests.py --no-build      # skip build, just run
  python3 tools/run_tests.py --test inv_test # run a single test binary directly
  python3 tools/run_tests.py --filter "InvTest.*"   # gtest filter across suite
  python3 tools/run_tests.py --json out.json # write machine-readable report
  python3 tools/run_tests.py --build-dir ../build

The working directory is the repo root; --build-dir defaults to ./build.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = REPO_ROOT / "build"

# Test binaries registered in CMake/Tests.cmake (tests + standalone_tests,
# benchmarks excluded from the default run).
TEST_TARGETS = [
    "ai_registry_test", "animationinfo_test", "appfat_test", "assets_test",
    "automap_test", "can_target_test", "cursor_test", "dead_test",
    "diablo_test", "dark_expedition_light_test", "dark_expedition_scroll_test",
    "dark_expedition_e2e_test",
    "drlg_common_test", "drlg_l1_test", "drlg_l2_test", "drlg_l3_test",
    "drlg_l4_test", "effects_test", "inv_test", "items_test",
    "lua_integration_test", "math_test", "missiles_test", "multi_logging_test",
    "pack_test", "player_test", "quests_test", "quest_script_test",
    "scrollrt_test", "stores_test", "tile_properties_test", "timedemo_test",
    "townerdat_test", "writehero_test", "vendor_test", "panel_state_test",
    "store_transaction_test", "visual_store_test", "stash_test",
    "inventory_ui_test", "spell_ui_test", "spelldat_test", "spell_tooltip_test",
    "char_panel_test", "game_menu_test", "spell_ux_test", "consumable_stack_test",
    "codec_test", "crawl_test", "data_file_test", "file_util_test",
    "format_int_test", "ini_test", "mod_identity_test", "palette_blending_test",
    "parse_int_test", "path_test", "vision_test", "random_test",
    "rectangle_test", "sheen_bidi_test", "static_vector_test", "str_cat_test",
    "utf8_test", "text_render_integration_test",
]


def run(cmd, build_dir: Path, capture: bool = False):
    """Run a command; on failure, print context and raise."""
    print("+", " ".join(shlex.quote(str(c)) for c in cmd), file=sys.stderr)
    if capture:
        result = subprocess.run(cmd, cwd=build_dir, capture_output=True, text=True)
    else:
        result = subprocess.run(cmd, cwd=build_dir)
    return result


def build_tests(build_dir: Path, n: int) -> tuple[bool, str]:
    """Build all test targets with ninja/cmake. Returns (ok, message)."""
    # Determine generator.
    gen = "ninja"
    cache = build_dir / "CMakeCache.txt"
    if cache.exists():
        text = cache.read_text(errors="replace")
        m = re.search(r"CMAKE_GENERATOR:INTERNAL=(\S+)", text)
        if m and "Unix Makefiles" in m.group(1):
            gen = "make"
    if not build_dir.exists():
        print(f"Build directory {build_dir} missing; run cmake first.", file=sys.stderr)
        return False, "build dir missing"

    targets = " ".join(TEST_TARGETS)
    if gen == "ninja":
        # Only build targets that exist in this configuration. text_render_integration_test
        # is conditional (needs PNG + not SDL1); building a missing target fails the whole
        # command. Resolve via the generated build.ninja target list.
        ninja = build_dir / "build.ninja"
        known = set()
        if ninja.exists():
            import re as _re
            for m in _re.finditer(r'^build ([^:]+):', ninja.read_text(errors='ignore'), _re.M):
                for t in m.group(1).split():
                    known.add(t)
        targets_to_build = [t for t in TEST_TARGETS if t in known]
        cmd = ["cmake", "--build", str(build_dir), "--target"] + targets_to_build + ["-j", str(n)]
    else:
        cmd = ["cmake", "--build", str(build_dir), "--target", "test", "-j", str(n)]
    result = run(cmd, build_dir)
    ok = result.returncode == 0
    return ok, "build ok" if ok else "build failed (see output)"


def parse_ctest_output(text: str) -> dict:
    """Parse ctest text output into a structured summary."""
    summary = {"passed": 0, "failed": 0, "skipped": 0, "not_run": 0, "total": 0, "failures": []}
    m = re.search(r"(\d+)% tests passed, (\d+) tests failed out of (\d+)", text)
    if m:
        summary["passed_pct"] = int(m.group(1))
        summary["failed"] = int(m.group(2))
        summary["total"] = int(m.group(3))
        summary["passed"] = summary["total"] - summary["failed"]
    # Collect failed/skipped test names from the failure list section.
    for line in text.splitlines():
        if re.search(r"Test\s+#\d+:", line) and ("***Failed" in line or "***Skipped" in line):
            m2 = re.search(r"Test\s+#\d+:\s+(\S+)", line)
            if m2:
                name = m2.group(1)
                if "Skipped" in line:
                    summary["skipped"] += 1
                else:
                    summary["failures"].append(name)
    return summary


def run_ctest(build_dir: Path) -> dict:
    print("=== Running full ctest ===", file=sys.stderr)
    result = run(["ctest", "--output-on-failure"], build_dir, capture=True)
    parsed = parse_ctest_output(result.stdout)
    parsed["returncode"] = result.returncode
    if result.returncode != 0:
        print(result.stdout[-3000:], file=sys.stderr)
    return parsed


def run_single(build_dir: Path, test: str, gtest_filter: str | None) -> dict:
    """Run one test binary directly with an optional gtest filter."""
    binary = build_dir / test
    if not binary.exists():
        return {"error": f"{test} not built; run --build first", "returncode": -1}
    cmd = [str(binary)]
    if gtest_filter:
        cmd += ["--gtest_filter=" + gtest_filter]
    result = run(cmd, build_dir, capture=True)
    parsed = {
        "test": test,
        "filter": gtest_filter,
        "returncode": result.returncode,
    }
    # Summarize from gtest output.
    for line in result.stdout.splitlines():
        m = re.search(r"\[\s*PASSED\s*\]\s+(\d+) tests?", line)
        if m:
            parsed["passed"] = int(m.group(1))
        m = re.search(r"\[\s*FAILED\s*\]\s+(\d+) tests?", line)
        if m:
            parsed["failed"] = int(m.group(1))
        m = re.search(r"\[\s*SKIPPED\s*\]\s+(\d+) tests?", line)
        if m:
            parsed["skipped"] = int(m.group(1))
    if "skipped" not in parsed:
        parsed["skipped"] = 0
    if result.returncode != 0:
        print(result.stdout[-3000:], file=sys.stderr)
    return parsed


def run_drift_check() -> dict:
    print("=== Drift check ===", file=sys.stderr)
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "tools" / "check_drift.py")],
        cwd=REPO_ROOT, capture_output=True, text=True,
    )
    ok = result.returncode == 0
    # Count PASS lines.
    passes = len(re.findall(r"^PASS", result.stdout, re.M))
    return {"drift_ok": ok, "passes": passes, "output": result.stdout}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument("--no-build", action="store_true", help="skip building")
    parser.add_argument("--test", help="run a single test binary by name")
    parser.add_argument("--filter", help="gtest filter (e.g. 'InvTest.*')")
    parser.add_argument("--skip-drift", action="store_true")
    parser.add_argument("--json", type=Path, help="write JSON report to path")
    parser.add_argument("-j", type=int, default=os.cpu_count() or 4, help="parallel jobs")
    args = parser.parse_args()

    report = {
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        "build_dir": str(args.build_dir),
        "steps": {},
    }
    build_dir = args.build_dir.resolve()

    if not args.no_build and not args.test:
        ok, msg = build_tests(build_dir, args.j)
        report["steps"]["build"] = {"ok": ok, "message": msg}
        if not ok:
            print(json.dumps(report, indent=2))
            return 1

    if args.test:
        report["steps"]["single"] = run_single(build_dir, args.test, args.filter)
    elif args.filter:
        # Filter across the whole suite: run every test binary with the filter.
        all_results = []
        for t in TEST_TARGETS:
            r = run_single(build_dir, t, args.filter)
            if r.get("returncode") == -1:
                continue  # binary not built; skip silently under --filter
            all_results.append(r)
        report["steps"]["filtered"] = {
            "filter": args.filter,
            "binaries": all_results,
            "failed_count": sum(1 for r in all_results if r.get("failed", 0) > 0),
        }
    else:
        report["steps"]["ctest"] = run_ctest(build_dir)
        if not args.skip_drift:
            report["steps"]["drift"] = run_drift_check()

    # Exit code: 0 if everything passed.
    ok = True
    if "build" in report["steps"] and not report["steps"]["build"]["ok"]:
        ok = False
    if "ctest" in report["steps"]:
        if report["steps"]["ctest"].get("failed", 0) > 0:
            ok = False
    if "single" in report["steps"]:
        single = report["steps"]["single"]
        if single.get("failed", 0) > 0:
            ok = False
        # Binary missing or run failure (returncode < 0) must fail the gate,
        # not silently pass.
        if single.get("returncode", 0) < 0:
            ok = False
            print(f"ERROR: {single.get('error', 'test binary failed to run')}",
                  file=sys.stderr)
    if "filtered" in report["steps"]:
        if report["steps"]["filtered"]["failed_count"] > 0:
            ok = False
    if "drift" in report["steps"] and not report["steps"]["drift"]["drift_ok"]:
        ok = False

    if args.json:
        args.json.write_text(json.dumps(report, indent=2))
        print(f"\nReport written to {args.json}", file=sys.stderr)

    print("\n=== SUMMARY ===")
    print(json.dumps(report, indent=2, default=str))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
