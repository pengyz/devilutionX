# SDD ledger — plan: docs/superpowers/plans/2026-09-15-upstream-sync-round2.md

规格：`docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`（已批准）
分支：`feature/qol-upgrades`（非 main/master）
起始 HEAD：`fa678569e`

## 飞行前冲突扫描（计划内部 + 任务间）

| # | 任务/任务对 | 共享面 | 发现 |
|---|---|---|---|
| 1 | T3 / T4 / T5 / T6 | 同一个 merge 索引的 4 个未合并文件 | 必须严格串行；只有 T6 提交。技能本身禁止并行实现者 → 一致 |
| 2 | T6 → T7 | T6 产出 merge commit，T7 消费 | 一致（T7 必须在 merge commit 之后） |
| 3 | T7 → T8 | `build/` 状态 | 一致（T7 若报错须修完再进 T8） |
| 4 | T8 → T9 → T10 | `build/` 与门禁产物 | 一致（顺序门禁） |
| 5 | T11 → T8/T9/T10 | T11 的「上游已修」分支会 `git checkout` Source 文件 | **潜在冲突**：在门禁之后改源码会使门禁失效 → 裁决 R2 |
| 6 | T11 → 知识库文本 | `docs/knowledge/pattern_fixed_width_field_reads.md` 现写「上游至今未修」 | 若走「已修」分支，该文本须同步（T11 步骤 3 已含）→ 一致 |
| 7 | T12 → T9/T10 | T12 只改 docs + push | 一致（docs 不影响门禁） |
| 8 | T1 自洽 | 只读；「基线非 698 则停下回报」 | 自洽；判据裁决见 R4 |
| 9 | T2 自洽 | 「出现第 5 个冲突文件则停下回报」 | 自洽 |
| 10 | T3 自洽 | `git checkout --theirs` + 改 `questdat.hpp`；步骤 5 判据已按复验改为 CRLF | 自洽（F4 已修） |
| 11 | T4 自洽 | 取上游两行；若 `drawInfoBox` 不在此作用域则停下回报 | 自洽 |
| 12 | T5 自洽 | 两个 include 取并集 | 自洽 |
| 13 | T6 自洽 | 保留 fork include；`git add` 5 个文件后 `git commit -F -` | 自洽（复核已验证：自动合并文件已由 merge 暂存，`-F -` 在 MERGE_HEAD 下生成 merge commit） |
| 14 | T7 自洽 | 预期零改动 + 条件提交分支 | 自洽（F1 已重写为「验证零连带」） |
| 15 | T8 自洽 | 配置 + 编译 + 条件提交 | 自洽 |
| 16 | T9 自洽 | 全量 + 按名字查 timedemo + TEST_TARGETS 检查 | 自洽（F5/F8 已补） |
| 17 | T10 自洽 | smoke + drift 前后对比 | 自洽 |
| 18 | T11 自洽 | 上游复核 + 留档（含死分支） | 自洽（配合 R2） |
| 19 | T12 自洽 | push + CI + 规格状态 | 自洽；push 为跨工作区副作用 → 裁决 R3 |

## 飞行前裁决

- **R1 — 不另建隔离 worktree，直接在 `feature/qol-upgrades` 上执行** — 理由：分支非 main/master；计划明确以该分支为目标且需推送；同一分支无法在第二个 worktree 中检出。若错的代价：低（merge 可 `git reset --hard` / `git revert` 撤销）。
- **R2 — T11 的「上游已修」分支视为死分支**（复核已确认上游 5 处未修）；万一触发，必须在 T12 之前重跑 T8→T10。若错的代价：中（漏跑门禁，可能放过编译/测试回归）。
- **R3 — T12 的 push 视为已预授权**（执行计划本身已获用户批准，计划 §4/§6 明写推送与 CI 验证）。若错的代价：低（可强推回退，但会打扰协作方）。
- **R4 — T1 若基线为绿但用例数不是 698：按「零失败」判据继续执行并记录实际基线** — 理由：规格 §6 明确「数量变化须记录原因」而非硬编码 698。若错的代价：低。
- **R5 — 计划任务标题由 `### 任务 N：` 改为 `## Task N:`** — 理由：`task-brief` 工具只识别 `Task N` 标题，且仓库上一轮计划（`2026-07-27-upstream-integration.md`）即此体例。若错的代价：无。

## 进度

（每任务完成后追加一行 `Task <N>: complete (...)`）
## 任务进度

Task 1: complete (commits fd2956ea8 = 控制者的 docs 标题重命名；任务本身只读无提交；review clean，3 Minor 已延期)
Task 1: minor (deferred): 简报步骤 2 缺少「工作区不干净时」的显式分支 → 实现者自行裁决继续（合理但超字面授权）；后续简报应写明显式容错路径
Task 1: minor (deferred): 实现者额外跑了 `ctest -R` 单用例复核 skipped 口径（只读、无副作用，但属简报未要求的多余动作）；可用读 LastTestsDisabled.log 静态核实
Task 1: minor (deferred): DONE_WITH_CONCERNS 状态未置于报告顶部，易被下游忽略（已由控制者处理，工作区已干净）

### 执行期追加裁决

- **R6 — 把 Task 2-6 合并为一次实现者分发**（merge → 复现 4 冲突 → 逐处解决 → 提交 merge commit）— 理由：五个任务共享同一个 merge 索引状态与同一个评审面；T4/T5 是行级微小改动，T3 的验收已在简报中逐条写明；分开分发会产生 5 次重复的上下文重建。若错的代价：中（某个冲突的复核粒度变粗）→ 缓解：分发中列出全部 4 处冲突的逐条验收，并把评审面聚焦到冲突文件本身。
- **R7 — merge 任务的评审面不用全区间 diff**，改用「4 个冲突文件相对两个父提交的受限 diff」（`git diff <pre-merge> <merge> -- <4 files>` 与 `git diff origin/master <merge> -- <4 files>`）— 理由：全区间 diff 会混入 100+ 个自动合并文件（那不是本任务的产出），把真正的 4 处人工判定淹掉。若错的代价：低。

### 执行期追加裁决（merge 阶段）

- **R8 — `scrollrt.cpp` 冲突：保留上游新增的 `this_sdl_thread::yield();` + 保留 fork 的 `DrawMain(hgt, false, …)`，不把 `drawInfoBox` 带回来** — 依据：`DrawMain(int dwHgt, bool drawDesc, …)` 签名两侧完全一致（第二参数都叫 `drawDesc`）；merge-base 有 4 处 `drawInfoBox`，fork 侧 0 处（fork 提交 `ed1dad939` 主动移除该脏矩形机制），上游侧 4 处；该 hunk 里上游真正的新增只有 `yield()`。带回 `drawInfoBox` 等于用一次 Infra 同步静默回退 fork 的既有重构，超出范围。若错的代价：低（该参数为常量 false 只影响信息框脏矩形优化，编译与行为由门禁 3-4 覆盖）。**独立评审已确认 R8 取舍正确且落地准确。**
- **R9 — 修复 F1 时直接 amend merge commit（而不是追加修复提交）** — 依据：merge 尚未推送；不 amend 会在历史里留下一个链接必然失败的提交。若错的代价：低（`eb788000c`/`e7be9dc8c` 已被 `8e92687a9` 取代，无远端影响）。
- **R10 — `quests.cpp` 的 include 冲突块只保留 `#include "lua/lua_event.hpp"`，删除 `#include "minitext.h"`** — 依据：fork 的 `CheckQuests()` lua 消费块在合并结果中存活（需要 lua_event.hpp）；而 `minitext.h` 的消费者 `InitQTextMsg` 已随上游迁移消失，合并结果里没有任何 minitext.h 符号被使用（grep 为空）。简报 T6 的"两个都保留"前提被上游迁移打破，按简报自带的分支条件（消费者消失则取上游）逐 include 判定。若错的代价：低（若仍缺符号，门禁 2 立即暴露，回退一步即可）。

### 任务进度（merge 阶段）

Task 2-6: 实现者按简报执行 merge，恰好复现 4 个冲突；其中 3 个（quests.h/quests.cpp 的 include 块/plrctrls.cpp）曾由 rerere 给出结果、plrctrls 的 rerere 结果被实现者手工纠正为并集；scrollrt.cpp 遇语义冲突后**正确上报 NEEDS_CONTEXT**（未擅自选择一侧）
Task 2-6: Ruling R8 下达 → 新实现者收尾 → merge commit `eb788000c`（提交信息中 scrollrt 描述与 R8 不符 → 控制者 amend 为 `e7be9dc8c`）
Task 2-6: 评审发现 **F1 (Critical)** —— `Source/quests.cpp` blob 与 merge 前逐字节相同，上游对该文件 341 行删除（15+ 函数/全局迁移）被整体丢弃，且与新文件重复定义 → 链接期必失败；评审同时确认 quests.h / questdat.hpp / plrctrls.cpp / scrollrt.cpp 四项正确、R8 取舍正确
Task 2-6: fix round 1/5 (1 addressed, 0 open — F1 修复：按真实三方合并重建 quests.cpp 为 638 行，保留 fork lua 块与 FloatingInfoString，删除 minitext.h；amend 进 merge commit `8e92687a9`)；F3 已由控制者 amend 提交信息修复；F4 已由本台账的 R8/R9/R10 条目关闭；F2 延期
Task 2-6: minor (deferred): 实现者自查只核对了冲突块内的几行，未核对整个文件相对三方的一致性（F2）——后续同类任务的自查清单必须要求整文件三方核对
Task 2-6: complete (commits fd2956ea8..8e92687a9, review clean after fix round 1) — 范围化复审裁定 F1/F2/F3/F4 全部 ADDRESSED，无新破环

- **R11 — 把 Task 7-10（编译验证 + 门禁 1-2 + 门禁 3-4 + eval smoke/drift）合并为一次分发** — 理由：四者共享同一个 build 状态与同一份证据面（都是"跑门禁并记录真实输出"），分开分发会产生 4 次上下文重建；失败诊断的判据已写在简报里。若错的代价：中（若门禁失败并需要修码，修复的评审粒度会偏粗）→ 缓解：要求实现者在报告里逐条贴真实输出与错误原文，评审者按证据核对，且最终全分支评议会再看一遍。
Task 7-10: complete (commits 8e92687a9..91e805149, review clean — 2 Minor 延期)
Task 7-10: 关键偏差：Task 7 简报「预期零连带」对 `plrctrls.cpp` 不成立（它是 4 个手工冲突文件之一，不继承上游在同一 hunk 自动合并进来的 `#include "panels/quest_log.hpp"`）→ 触发 7 处编译错误 → 按简报步骤 3 的允许判据修复并单独提交 `91e805149`（1 行、位置符合 include 字母序、CRLF 未变）
Task 7-10: minor (deferred): 应在 docs/knowledge/ 沉淀「merge 冲突文件不继承上游自动合并的新增行」这条经验（Task 11 一并处理）
Task 7-10: minor (deferred): 评审指令里「预期无改动」的检查范围跨了 merge 边界，产生假阳性（应限定在本批次提交区间）

- **R12 — 把 Task 11 与 Task 12 的任务评审并入最终全分支评审**（不单独起一轮）— 理由：两者都是纯文档改动（一个知识库 .md、一个规格 .md），最终评议会显式包含它们的 diff；另起一轮只是重复同一评审面。若错的代价：低（文档错误可能只被审一次；缓解：最终评审包显式列出这两个提交并要求逐条看）。
- **R13 — Task 11 的「整理上游 PR 文案」子步骤不做** — 理由：原始简报要求把 PR 文案写进 `docs/superpowers/plans/`，但那是为**对外动作**（向 diasurgical/DevilutionX 提 issue/PR）做准备的；对外动作需用户单独授权（用户此前明确选择「先做上游同步立项」，未授权发帖）。若错的代价：低（文案素材已在规格 §4 步骤 5 与知识库条目里，随时可整理）。

## 任务进度（收尾）

Task 11: complete (commits 91e805149..d1e122ab6, review folded into the final review per R12) — 上游 tip `e00b7260f` 复核：heroname 5 处裸读**仍未修**；fork 修复完好未被 merge 覆盖；知识库 `pattern_fixed_width_field_reads.md` 已更新状态
Task 12: 推送完成（控制者，R3）→ CI run `34959598050` conclusion=**success**；文档闭环提交 `5cce05c30`（规格 §7 改「已实施」+ 十项验收实测表 + 「执行中与规格不符/未预料之处」4 点；并修正了头部「已批准」与 §5 红线 #4「草案」的内部矛盾）
Task 12: pending — 规格改动（`5cce05c30`）尚未推送（docs-only，不触发 CI）

## 未关闭的延期项（交最终评比分拣）

- 应在 `docs/knowledge/` 沉淀「**merge 冲突文件不继承上游自动合并的新增行**」——`plrctrls.cpp` 漏 include 的根因（T7-T10 评审 F1 提出，Task 11 因作用域限制未做）
- 上游 PR 文案/ASan 复现步骤的整理（R13：需对外动作授权）
- Task 1 的 3 条流程 Minor / Task 2-6 的 1 条自查方法论 Minor / Task 7-10 的 1 条评审范围 Minor

## 最终评审与修复波（收尾）

最终全分支评审（opus）裁决：**有条件通过**——用比前序评审更强的证据认可合并本身（1490 个共存文件逐文件真三方复算 0 mismatch、恰好 4 个冲突、文件计数恒等式 1490+17+195=1702 闭合、双向树级审计 3 个可疑项全部证伪、R8 未越权、门禁时间线自洽），放行条件是四条非代码修正。
- **F1 (Important, 必修)**：`sampling_behavior_test` 注册在 `Tests.cmake`/CTest 却缺席 `TEST_TARGETS` → 13 个用例长期跑 2026-08-13 的旧二进制。→ 修复 `88035420d` + 重跑门禁（`/tmp/ci-sync2.json`：698/0/100、drift 5/5；`sampling_behavior_test` mtime 刷新为 19:10:33；13 个 `SamplingBaselineTest.*` 全 OK）
- **F2 (Important, 必修)**：规格 §7「TEST_TARGETS 未脱钩」与实测相反 → `14bd27503` 改写为准确表述并补重跑验收数据
- **F3 (Minor, 必修)**：`5cce05c30` 未推送 → 已推送（连同修复波，见下）
- **F5 (Minor, 必修)**：冲突文件丢失自动合并行的根因未进 `docs/knowledge/` → `14bd27503` 新增 `gotcha_conflict_file_loses_auto_merged_lines.md` + MEMORY 索引
- **F4 (Minor, 可留后续)**：merge commit `8e92687a9` 本身不可编译（缺 include，到 `91e805149` 才修好）→ 采纳评审建议「接受现状并留档」：force-push 已推送历史的代价高于收益；该提交信息末尾已写明 "To be verified by the compile gate"。**留档于本条。**

修复波范围化复审（sonnet）：**F1/F2/F5 全部 ADDRESSED，无新破环，无超范围观察。**

- **R14 — 保留 SDD 工作区（不 rm -rf）** — 理由：它是延期 minor 清单与全部裁决历史的唯一载体，且 `docs/superpowers/ledgers/.gitignore` 使其对仓库零污染（`git status` 干净）。技能默认在最终评审通过后删除工作区（记录已在 git 历史），但本轮的价值记录（简报/评审模板改进 backlog）不在 git 里。若错的代价：可忽略（一个被忽略的目录）。

Task 12: complete (commits 5cce05c30, 88035420d, 14bd27503) — 推送完成 → CI run `34962472198` conclusion=**success**；规格闭环 + 门禁覆盖缺口修复

## 收尾状态

- HEAD `14bd27503`，`HEAD..origin/master = 0`，工作区干净，与 `myrepo/feature/qol-upgrades` 同步（领先 0）
- 最终门禁：`/tmp/ci-sync2.json` 698 total / 0 failed / 3 skipped / passed_pct 100 / drift 5/5；eval smoke 36/36 exit 0
- CI：`34962472198`（HEAD `14bd27503`）success
- heroname：上游 tip `e00b7260f` 仍未修（5 处），fork 修复完好；上游 PR 需用户单独授权（R13）

- **R15 — 收尾方式：保持 `feature/qol-upgrades` 为工作分支并推送到 `myrepo`，不合并进 `master`** — 理由：CLAUDE.md 明定本 fork 的工作分支与推送目标（`myrepo`）；`master` 直推被 hook 拦截且非本项目流程；上游同步本就是"把上游合入工作分支"而非"把工作分支合回 master"。若错的代价：低（分支随时可再处理）。
