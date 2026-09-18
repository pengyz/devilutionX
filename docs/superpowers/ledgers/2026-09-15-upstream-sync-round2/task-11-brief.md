## Task 11: 复核 heroname 分叉状态并留档

**文件：**
- 修改：`docs/knowledge/pattern_fixed_width_field_reads.md`（追加同步后的分叉状态），必要时新建实施记录

**接口：**
- 依赖输入：任务 6 的 merge 完成
- 对外产出：heroname 修复在上游是否已修的明确结论（规格 §6 验收项 8）

- [ ] **步骤 1：在上游 tip 上复核（这是判据，不看本地）**

```bash
git show origin/master:Source/msg.cpp | grep -n "heroname"
git show origin/master:Source/pack.cpp | grep -n "heroname"
```

预期（2026-09-15 探测结果）：`msg.cpp:1094/1360/1368/1398`、`pack.cpp:445` 仍是裸字段（未修）。

- [ ] **步骤 2：按结论分支处理**

- 若**仍未修**：保留 fork 修复，在 `docs/knowledge/pattern_fixed_width_field_reads.md` 的正文追加一行状态（格式：`**上游状态（2026-09-15 同步后复核）**：仍保留 5 处裸字段读取，fork 修复见 a82c0a827`），并把上游 PR 的文案与 ASan 复现步骤整理成 `docs/superpowers/plans/` 下的一个独立小节或单独文件（不在本计划内提交 PR——对外动作需单独确认）。
- 若**已修**：比对上游实现与 fork 修复（`git show origin/master:Source/pack.cpp | grep -n "pName"`），若上游用了同样的定长读取则以**上游为准**并追加提交：

```bash
git checkout origin/master -- Source/pack.cpp Source/pfile.cpp Source/msg.cpp
git commit -am "fix: take upstream's bounded name reads (upstream caught up)"
```

- [ ] **步骤 3：提交留档**

```bash
git add docs/knowledge/pattern_fixed_width_field_reads.md
git commit -m "docs(knowledge): record heroname divergence state after the upstream sync"
```

---

