## Task 12: 推送、CI 与规格状态更新

**文件：**
- 修改：`docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`（§7 状态改为「已实施」+ 实施记录）

**接口：**
- 依赖输入：任务 1-11 全部完成
- 对外产出：远端同步、CI 绿、规格状态闭环

- [ ] **步骤 1：推送**

```bash
git push myrepo feature/qol-upgrades
```

预期：推送成功（**不是** `master`）。

- [ ] **步骤 2：等 CI 并确认结论**

```bash
gh run list --repo pengyz/devilutionX --workflow=better-d1-ci.yml --limit 3 \
  --json databaseId,headSha,conclusion --jq '.[] | "\(.databaseId) \(.conclusion) \(.headSha[0:9])"'
```

预期：对应本次 HEAD 的运行 conclusion 为 `success`。

- [ ] **步骤 3：更新规格状态并把实测结论写进去**

在 `docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md` 的 §7 追加（内容必须来自实测，不得照抄本计划）：

```markdown
## 7. 状态

**已实施（<YYYY-MM-DD>）。** §6 全部验收项通过：`ctest` <N> 项零失败、drift 5 项 PASS、eval smoke 36/36、CI <run id> success。

冲突实际为 4 处（与探测一致）。执行中与规格不符/未预料之处：

- <逐条记录，例如：上游把 quests.h 重写为 LF 导致整文件冲突——规格未预料，已按「保持与上游一致」处置并写明判据>
```

- [ ] **步骤 4：提交并推送文档更新**

```bash
git add docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md
git commit -m "docs(spec): mark upstream sync round 2 as implemented + record findings"
git push myrepo feature/qol-upgrades
```

---

## 对抗性复核记录（2026-09-15，kiro 子代理 + 主 agent 复验）

对初稿做过一次独立对抗性复核（要求只读、只写可验证结论）。发现与处置：

| id | 严重度 | 问题 | 处置 |
|---|---|---|---|
| F1 | blocker | 任务 7 的「预期编译连带」是**假问题**：探测性 merge（rerere 关闭）显示 12 个消费者文件已由上游改动自动带上 `#include "panels/quest_log.hpp"`，`quests.cpp` 对三个符号的引用计数为 0 → 既无未声明错误也无重复定义 | 任务 7 重写为「验证零连带」（机器判据：12 文件各 1 个 include、引用计数 0），仅在编译真报错时进入诊断分支；规格 §3 对应段落同步改写 |
| F2 | high | 事实表「本分支侧改动文件 = 263」错误，实测 **265** | 计划与规格两处均改为 265（复验命令 `git diff --name-only $(git merge-base HEAD origin/master) HEAD \| wc -l`） |
| F3 | high | `test/ui_test.hpp`、`test/panel_state_test.cpp` 在 24 文件交集内（上游 `53b91fd7a` 改过）却未被文档提及 | 已在「探测阶段已确认的事实」与规格 §3 写明：自动合并成功、无需人工改动，由任务 9 全量门禁兜底（`ui_test.hpp` 被 13 个测试二进制共享） |
| F4 | medium | 任务 3 步骤 5 对 `questdat.hpp` 行尾的预期写成 LF，实际上游是 **CRLF**（141/141）；本轮唯一 LF 的是 `Source/quests.h` 与上游新增的 7 个文件 | 预期输出改为 `CRLF 141 LF 141` 并说明原因 |
| F5 | medium | `tools/run_tests.py` 的 `TEST_TARGETS` 手工列表与 `CMake/Tests.cmake` 存在结构性脱钩风险（`check_drift.py` A 项不校验该列表）。本轮未触发，但计划对此沉默 | 任务 9 增加步骤 2c：用 `git diff <merge-base> origin/master -- CMake/Tests.cmake` 过滤集合成员增删，有输出则必须同步 `TEST_TARGETS` |
| F8 | low | 任务 9 只校验聚合 `skipped` 计数，无法发现 timedemo 从 Skipped 翻转成别的状态（`run_tests.py` 不记录被跳过用例名） | 任务 9 增加步骤 2b：按名字查 `LastTest.log` 确认 `Timedemo.WarriorLevel1to2 (Skipped)` |

复核同时确认以下断言为真（不再展开）：merge-base/tip/28 提交；4 个冲突文件与冲突块行数、三处小冲突原文逐字一致；`quests.h` 149 CRLF vs 32 LF 与 markers 位置；`questdat.hpp` 的 `QuestData` 字段只缺 `scriptName`；`git checkout --theirs` 与 `git commit -F -` 在 merge 状态下的语义；`libdevilutionx`/`devilutionx` 目标名存在；heroname 上游 5 处仍未修；24 个交集文件中仅 `quests.h` 行尾不一致；`.editorconfig`/`.gitattributes`/`CMakePresets.json` 未被上游改动。

复核也**自我纠正**了两条早期误判（一度以为 `quests.cpp` 会与 `panels/quest_log.cpp` 重复定义而构成 blocker；实测该定义已被自动合并删除），这也说明本节的结论以复验命令为准。

## 自审

**1. 规格覆盖度（规格 §6 十项验收 → 本计划任务）**

| 规格验收项 | 对应任务 |
|---|---|
| 1 无残留冲突标记 | 任务 6 步骤 3 |
| 2 `HEAD..origin/master` 计数为 0 | 任务 6 步骤 4 |
| 3 构建通过 | 任务 8 |
| 4 全量测试零失败 | 任务 9 |
| 5 eval smoke exit 0 | 任务 10 步骤 1 |
| 6 漂移校验 5 项 PASS | 任务 10 步骤 2 |
| 7 行尾未破坏 | 任务 10 步骤 3 |
| 8 heroname 状态有结论并留档 | 任务 11 |
| 9 timedemo quarantine 状态 | 任务 9 步骤 2（全量 ctest 输出中确认 `Timedemo.WarriorLevel1to2` 仍为 `Skipped`；若变化，记录原因） |
| 10 CI 绿 | 任务 12 步骤 2 |

**2. 占位符扫描**

本计划不含 TBD/TODO/「后续补充」。任务 7 经对抗性复核后从「处理预期编译连带」改为**「验证零连带」**——原写法让执行者去修一个 git 自动合并已经解决的问题（假问题），属于会误导执行者的缺陷；现版本给出机器可判的计数判据（12 个文件各 1 个 include、`quests.cpp` 引用计数 0），仅在编译真的报错时才进入现场诊断分支。任务 8 步骤 3、任务 9 步骤 3 是条件分支，各自写明触发条件、命令与提交信息。

**3. 类型与命名一致性**

- `QuestData::scriptName`（任务 3 定义）与 `Source/quests.cpp:286-287` 的 `questData.scriptName` 一致。
- `panels/quest_log.hpp`（任务 7）与上游 `Source/panels/quest_log.hpp:9-12` 的声明一致。
- 冲突文件的路径与 4 处探测结果逐字一致。

**4. 规格缺口（本计划新发现，需回写规格）**

规格 §4 的风险表未预料「行尾差异导致整文件冲突」这一项，§6 也未给出对应的判据。执行时应把以下内容回写到规格（任务 12 步骤 3 一并处理）：

- 事实：`.gitattributes` 为 `* -text`，git 不做行尾归一化；上游把 `Source/quests.h` 重写为 LF，而 fork 侧是 CRLF（24 个交集文件中**唯一**一个行尾不同的）。
- 判据：被上游重写为 LF 的文件**保持 LF**——门禁 C 以新的 merge-base（= 上游 tip）为基线比对，改成 CRLF 会 FAIL，且会保证以后每次同步都整文件冲突。
