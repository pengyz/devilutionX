#!/bin/bash
# Hook: PostToolUse (Edit|Write) / CI dual-mode
# 行尾合规检查 — .editorconfig 声明 [*] CRLF，例外后缀 LF（.md/.py/.yml/.sh/.json/.lua 等）。
# 跨平台：Linux/macOS 用 sh，Windows 用 Git Bash（settings.json shell:"bash"）。
# WSL 抢占 bash 的已知问题：Windows 上显式配 CLAUDE_CODE_GIT_BASH_PATH 指向 Git Bash。
# CI 模式: CI_MODE=1 CI_FILE=<path>
#
# 注意：此脚本在 CI 的 GitHub Actions Windows runner 上**不触发**（hooks 仅 Claude Code
# 交互会话），CI 行尾合规由 tools/check_drift.py 独立覆盖。

if [ "${CI_MODE:-}" = "1" ]; then
  FILE_PATH="${CI_FILE:-}"
else
  # 用 sed 提取 JSON 字段（避免依赖 jq，Git Bash 自带 sed）
  INPUT=$(cat /dev/stdin)
  TOOL_NAME=$(echo "$INPUT" | sed -n 's/.*"tool_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
  FILE_PATH=""
  if [ "$TOOL_NAME" = "Edit" ] || [ "$TOOL_NAME" = "Write" ]; then
    FILE_PATH=$(echo "$INPUT" | sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
  fi
fi

[ -z "$FILE_PATH" ] || [ ! -f "$FILE_PATH" ] && exit 0

# 跳过非文本与 .editorconfig 声明 LF 的文件。
case "$FILE_PATH" in
  *.png|*.clx|*.pal|*.cel|*.dun|*.raw|*.sv|*.mpq|*.jpg|*.jpeg|*.wav|*.ttf|*.ico|*.bin|*.gif|*.zip|*.gz|*.so|*.a|*.dll|*.exe|*.pdf|*.trn|*.smk)
    exit 0 ;;
  *.md|*.py|*.yml|*.yaml|*.sh|*.json|*.lua|*.java|*.rb|*.xml|*.plist|*.desktop|*.pot|*.po)
    exit 0 ;;  # .editorconfig 或项目约定为 LF
esac

# 此路径必须 CRLF。检测是否含裸 LF（非 \r\n 的 \n）。
total=$(grep -c $'\n' "$FILE_PATH" 2>/dev/null || true)
crlf=$(grep -c $'\r$' "$FILE_PATH" 2>/dev/null || true)
if [ "$total" -gt 0 ] && [ "$total" -ne "$crlf" ]; then
  bare=$((total - crlf))
  echo "WARNING line-ending: $FILE_PATH has $bare bare LF (should be CRLF). Run: sed -i 's/\$/\r/' '$FILE_PATH'"
fi

exit 0
