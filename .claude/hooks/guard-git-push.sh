#!/bin/bash
# Hook: PreToolUse (Bash) / CI dual-mode
# Git push 安全护栏 — 阻止直推 main/master 分支（含 refspec 语法绕过）与
# 在 main/master 分支上直接 push。工作分支（feature/* 等）可正常推送。
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

block() {
  echo "ERROR: 直推 main/master 被禁止。请推送到 feature 分支并走 PR。"
  echo "  git push myrepo feature/qol-upgrades"
  exit 2
}

# 1. 检查 push 目标是否为受保护分支（覆盖 refspec 语法：refs/heads/master、
#    HEAD:master、master:master、+master 等）。
if echo "$CMD" | grep -qE '((^|[[:space:]])(\+)?[^:[:space:]]*:)?(refs/heads/)?(main|master)([[:space:]]|$)'; then
  block
fi

# 2. 检查当前所在分支（git push / git push origin 无目标时）。
CURRENT=$(git branch --show-current 2>/dev/null || true)
case "$CURRENT" in
  main|master)
    block ;;
esac

exit 0
