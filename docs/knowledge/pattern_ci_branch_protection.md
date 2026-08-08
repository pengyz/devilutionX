---
name: CI 失败无法合入——分支保护是合入门禁
description: 盯 CI 状态是治标；master 分支保护（PR+审查+必选检查）才是保障
type: pattern
created: 2026-08-08
sources: [.github/workflows/better-d1-ci.yml, GitHub API]
---

开发时曾反复手动轮询 CI 状态——这治标不治本。真正的合入门禁是 **GitHub 分支保护规则**：

- `required_pull_request_reviews.required_approving_review_count = 1`（必须 PR + 1 人审查）
- `required_status_checks.contexts = ["build-and-test"]`（**CI 必选**，失败无法合入）
- `strict: true`（PR 必须基于最新 master）
- `enforce_admins: true`（管理员也强制）
- `allow_force_pushes: false`（禁 force push）

**为什么：** 轮询 CI 只是观察；保护规则是强制。CI 失败时保护规则自动阻止合入，无需人工盯。

**何时使用：** 任何「CI 必须通过才能合入」的需求。配置：`gh api -X PUT repos/<org>/<repo>/branches/master/protection --input protection.json`。本地另有 `guard-git-push` hook 拦截直推 master。正确流程：feature 分支开发 → push → PR → CI 自动跑 → 审查+CI 绿 → 合入。
