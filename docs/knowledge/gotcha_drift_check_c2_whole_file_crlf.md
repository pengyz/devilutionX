---
name: 漂移检查 C2 对"相对 merge-base 新增的文件"要求整文件 CRLF
description: check_drift.py 的 C 只管既有文件的"保持既有行尾"，而 C2 管"相对 merge-base 新增的文件必须整体匹配 .editorconfig"。给一个新增的 C++/TSV 测试文件加几行注释时若用了 LF，哪怕只有一行，C2 也会 FAIL。
type: gotcha
created: 2026-09-16
sources:
  - tools/check_drift.py（check_modified_line_endings = C；check_added_line_endings = C2）
  - docs/superpowers/ledgers/2026-09-15-level-rosters-phase-a2/task-1-report.md（首跑 FAIL C2 的实测）
---

**事实**：`tools/check_drift.py` 有两条行尾检查：
- **C**：对**已存在**的文件，要求"改动后仍保持它原有的行尾类型"（可只关心你改的区域）。
- **C2**：对**相对 merge-base 属于新增**的文件，要求**整个文件**匹配 `.editorconfig`（C++/TSV → CRLF）。

**坑**：一个在**本分支上新增**的测试文件（例如 `test/level_roster_baseline_test.cpp`，相对上游 merge-base 是 added），你往里加几行 LF 注释就会 **FAIL C2** —— 因为 C2 看的是**整文件**，而不是"你新加的那几行"。同一文件若被判定为"已存在"，则只受 C 约束，观感完全不同。

**正确做法**：改动前先确认该文件相对 merge-base 是 added 还是 modified（`git diff --stat <merge-base> -- <file>`），added 的话把它**整文件规范化**（`sed -i 's/$/\r/'` 之前务必确认原文件无 CR 混入，或直接用编辑器统一 CRLF），再提交。

**何时使用**：编辑任何"新文件"（尤其测试）时；排查 `FAIL C2` 时。
