#!/bin/bash
# Hook: PreToolUse (Edit|Write) / CI dual-mode
# 核心文件保护 — 修改这些文件时注入警告，要求说明必要性。
# 这些是引擎/测试/规格的关键文件，误改会导致存档损坏、测试漂移或宪章违反。
# CI 模式: CI_MODE=1 CI_FILE=<path>

if [ "${CI_MODE:-}" = "1" ]; then
  FILE_PATH="${CI_FILE:-}"
else
  INPUT=$(cat /dev/stdin)
  TOOL_NAME=$(echo "$INPUT" | sed -n 's/.*"tool_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
  FILE_PATH=""
  if [ "$TOOL_NAME" = "Edit" ] || [ "$TOOL_NAME" = "Write" ]; then
    FILE_PATH=$(echo "$INPUT" | sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
  fi
fi

[ -z "$FILE_PATH" ] && exit 0

# 核心保护文件（相对路径，含子路径匹配）
PROTECTED=(
  "Source/pfile.cpp" "Source/pfile.h"
  "Source/engine/demomode.cpp"
  "Source/tables/spelldat.cpp" "Source/tables/spelldat.h"
  "Source/items.cpp" "Source/items.h"
  "test/main.cpp"
  "CMake/Tests.cmake"
  "tools/check_drift.py"
  "docs/superpowers/specs/2026-07-27-better-d1-design-charter.md"
  ".editorconfig" ".gitattributes"
)

for pat in "${PROTECTED[@]}"; do
  case "$FILE_PATH" in
    *"$pat"*)
      echo "WARNING protected file: $FILE_PATH"
      echo "  此文件是引擎/测试/规格关键文件。修改会触发漂移校验或破坏存档兼容。"
      echo "  请确认改动必要性；改后必须跑: python3 tools/run_tests.py --json /tmp/ci.json"
      exit 0
      ;;
  esac
done

exit 0
