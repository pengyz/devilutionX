# SDD 台账（subagent-driven development ledgers）

本目录是**随仓库长期留存**的执行台账：规格/计划之外，记录"当时为什么这么裁决、实测到了什么"。

## 位置约定（2026-09-18 起）

- **canonical 台账在本目录**：`docs/superpowers/ledgers/<date>-<topic>/`
- `.superpowers/sdd/` 仍是**草稿区**（被 `.superpowers/sdd/.gitignore` 忽略），只放过程性产物；**定稿的台账必须落本目录**，否则不进 git、下次会话看不到。
- 与 `docs/superpowers/specs/`（设计）和 `plans/`（计划）同处，便于交叉引用。

## 每个台账目录里应有什么

| 文件 | 内容 |
|---|---|
| `progress.md` | **主台账**：逐任务的结论、裁决（写法：`Ruling: … — 理由 — 若判断错会付什么代价`）、实测数值（**证据内联**，不得只给 `/tmp` 路径）、CI run id 与结论、外溢/延期项 |
| `task-*-brief.md` / `task-*-dispatch.md` | 当时发给实现者的任务书（用于复盘"我们当时要求了什么"） |
| `task-*-report.md` / `*-review*.md` | 实现者报告与独立评审结论（含被驳回的发现） |
| `*.diff` | 被中断/放弃工作的存档（LF；见 `.editorconfig` 的 `[*.{diff,patch}]` 例外） |

## 写法硬要求

1. **证据内联**：结论旁边直接给数字/命令/输出摘要；不得只引用临时路径。
2. **裁决必须三件套**：结论、理由、判错代价。
3. **未闭合项单列**：明确写"谁负责、什么时候能关"。
4. **失败也要记**：被否决的方案与原因比成功方案更有复用价值。
