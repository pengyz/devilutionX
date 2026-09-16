---
name: feature/** 分支上实际会跑哪些 CI 工作流（格式/tidy 检查不在其中）
description: 推送到 feature/** 只会触发 better-d1-ci.yml（构建+全量 ctest+漂移）；clang-format-check.yml（clang-format 18）与 clang-tidy-check.yml 只在 push:master 与 pull_request 上触发，因此分支"CI 绿"不等于"格式合规"。
type: reference
created: 2026-09-15
sources:
  - .github/workflows/better-d1-ci.yml（on.push.branches: feature/qol-upgrades, 'feature/**', engine-mod-infra）
  - .github/workflows/clang-format-check.yml（on.push.branches: master；on.pull_request；clang-format-version '18'；check-path Source + test）
  - .github/workflows/clang-tidy-check.yml（同上，仅在 master/PR 触发；其 clang-format 检查被显式关闭，留给 format 工作流）
---

**事实**：本仓工作流众多，但推送到 `feature/**` **只会**触发 `better-d1-ci.yml`（依赖安装 → 构建 → 全量 `ctest` → 漂移校验 → 上传报告）。`clang-format-check.yml` 与 `clang-tidy-check.yml` 的触发条件是 **`push: branches: [master]` 与 `pull_request`**，**不包含 feature 分支**。

**后果**：
- 分支上"CI 绿"的含义是"构建 + 全量测试 + 漂移绿"，**不包含**格式检查。一次带格式违规的推送会在 feature 分支上全绿，但**开 PR 到 master 时**被 `clang-format-check.yml`（`check-path: Source` 与 `test`）拦下。
- 本地系统 `clang-format` 版本可能低于 CI 的 **18**（本会话实测开发机系统版为 14），仓库工具链内也没有 18。若打算开 PR，需自备 18.x 并用 `--dry-run -Werror` 对 `Source/` 与 `test/` 自查。
- `better-d1-ci.yml` 有 `paths-ignore: docs/**`：**纯文档提交不触发 CI**（因此"没看到 run"不等于"没推送成功"）。

**何时使用**：判断"我的推送到底有没有被 CI 覆盖"、或准备开 PR 前决定要不要额外跑格式检查时。
