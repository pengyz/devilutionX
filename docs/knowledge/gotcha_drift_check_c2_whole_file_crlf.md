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

## 追加（2026-09-18）：C2 的两个反直觉点

1. **C2 不读 `.editorconfig`。** 它用的是 `tools/check_drift.py` 里的**硬编码 `LF_SUFFIXES` 集合**（`tools/check_drift.py:31-32`），其余后缀一律要求 CRLF。因此在 `.editorconfig` 里加 `[*.{diff,patch}] end_of_line = lf` **对漂移检查毫无作用**（只对编辑器有效）——要改判定必须改那个集合。归档的评审补丁（`docs/superpowers/ledgers/**/*.diff`，32 个）就是这样让 CI 红掉的：它们是 `git diff` 的产物、天然 LF，而 CRLF 会让 `git apply` 把 CR 塞进源码。
2. **C2 只看 HEAD 里的文件**：`git diff --name-only --diff-filter=A <base> HEAD`。所以**只 `git add` 不 `commit` 就跑漂移 = 假绿**（我这次就是这样：本地报 6/6 通过，CI 立刻红）。**正确做法：先 commit，再跑 `python3 tools/check_drift.py --base origin/master`**，然后才 push。

**为什么：** 这两点各自都会让"本地绿、CI 红"重演：前者让你以为改了 `.editorconfig` 就够，后者让你在还没进入 HEAD 的树上验证出假绿。

**何时使用：** 任何时候新增/归档非源码文本文件（补丁、日志、台账），以及每次 push 前的漂移校验。
