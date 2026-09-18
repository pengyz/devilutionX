---
name: pattern_sdd_ledger_location
description: SDD 台账的位置约定——canonical 台账在 docs/superpowers/ledgers/（进 git），.superpowers/sdd/ 只是草稿区
type: pattern
created: 2026-09-18
sources:
  - docs/superpowers/ledgers/README.md
  - .superpowers/sdd/.gitignore
---

SDD 台账（`progress.md` + 任务书/报告/评审/diff 存档）的 canonical 位置是 **`docs/superpowers/ledgers/<date>-<topic>/`**，随仓库进 git；`.superpowers/sdd/` 只作草稿区（被 `.superpowers/sdd/.gitignore` 忽略）。

**为什么：** 台账是"当时为什么这么裁决、实测到什么"的唯一记录；留在被忽略的草稿区会导致**下次会话/换人后彻底看不到**（本仓库 2026-09-18 之前就是如此），而规格/计划只记录结论、不记录过程与失败方案。

**何时使用：** 新建任何一个 SDD 工作区时（写 `progress.md` 的那一刻）；以及引用历史证据时——路径要写 `docs/superpowers/ledgers/...`，不要再写 `.superpowers/sdd/...`。

**附**：机器生成的 `.diff`/`.patch` 是 `.editorconfig` 的 **LF 例外**（`[*.{diff,patch}]`）——git 产出即 LF，改成 CRLF 会让 `git apply` 把 CR 塞进源码。
