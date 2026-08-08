#!/bin/bash
# Hook: PreToolUse (Bash) / CI dual-mode
# Git push 安全护栏 — 阻止直推 main/master 分支。
# 工作分支（feature/* 等）可正常推送。
# CI 模式: CI_MODE=1 CI_FILE=<path>

if [ "${CI_MODE:-}" = "1" ]; then
  CMD="${CI_FILE:-}"
else
  INPUT=$(cat /dev/stdin)
  CMD=$(echo "$INPUT" | sed -n 's/.*"command"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -1)
fi

# 非 git push 命令直接放行
case "$CMD" in
  *git*push*) ;;
  *) exit 0 ;;
esac

# 检查是否推送到受保护分支（main/master）
if echo "$CMD" | grep -qE "(^|[[:space:]])(main|master)([[:space:]]|$)"; then
  echo "ERROR: 直推 main/master 被禁止。请推送到 feature 分支并走 PR。"
  echo "  git push myrepo feature/qol-upgrades"
  exit 2
fi

exit 0
