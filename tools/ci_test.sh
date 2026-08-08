#!/usr/bin/env bash
# AI-friendly one-shot integration test runner for DevilutionX.
#
# Full pipeline: build all test targets → full ctest → drift check → JSON report.
# Exit 0 = everything green; nonzero = something failed.
#
# Usage:
#   tools/ci_test.sh                 # full pipeline
#   tools/ci_test.sh --no-build      # skip rebuild
#   tools/ci_test.sh --test inv_test # single test binary
#   tools/ci_test.sh --filter InvTest.*
#   tools/ci_test.sh --json /tmp/report.json
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

exec python3 tools/run_tests.py "$@"
