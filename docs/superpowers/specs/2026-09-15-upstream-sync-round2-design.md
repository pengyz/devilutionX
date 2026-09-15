# 上游同步（第二轮）：28 个提交 / 4 处冲突

**日期**：2026-09-15（v2 —— 探测深入后的修订，见 §3「探测深入」）
**分类**：Infra
**状态**：已批准（实施计划见 `plans/2026-09-15-upstream-sync-round2.md`）
**评判基准**：`2026-07-27-better-d1-design-charter.md`
**前置**：`2026-07-27-upstream-integration-design.md`（已实施；其三个全局迁移阻塞项已消解）

---

## 1. 问题陈述

本分支与上游的 merge-base 停在 `b3e52b1ea`（2026-07-26）。此后上游前进 **28 个提交**（tip `e00b7260f`，2026-09-15），本分支前进 224 个提交，两者再未交汇。

具体症状（均可复现）：

| # | 症状 | 证据 |
|---|---|---|
| 1 | 已修的上游 OOB 现在成了 **fork-only 分叉** | 上游 master 至今保留耳朵名 5 处越界读（`msg.cpp:1094/1360/1368/1391-1398`、`pack.cpp:445`）；fork 修于 `a82c0a827`。不同步 = 这段分叉随每次上游改动继续变大 |
| 2 | 漂移校验基线停留在 2026-07-26 | `check_drift.py` 报 `merge-base: b3e52b1ea`；同步后基线前移，校验口径随之收敛 |
| 3 | 上游面向玩家的修复进不来 | `c90181d54 Fix directy rect blitting`、`e00b7260f Prevent inlining of blitter operations`、`3f71a5b8c Yield for audio thread on systems with cooperative threading`、`d198263f3 Fix performance on real DOS hardware`、`cefa2c511 Fix SDLC_PushEvent() return value for SDL1`、`f841ac78a Update mpqfs for endian fixes` |
| 4 | 分叉成本单调增长 | 本分支每次改 `Source/items.cpp`、`loadsave.cpp`、`pack.cpp`、`CMake/Tests.cmake` 都在扩大第 3 节的交集清单 |

**不属于症状**（避免"感觉该同步了"）：上一轮的三个全局迁移阻塞项**已完成**——`CMakeLists.txt:344` 已是 `set(CMAKE_CXX_STANDARD 23)`，`Source/` 中 `fmt::` 仅剩 1 处注释（`Source/lua/modules/log.cpp:40`），`tl::expected` 已在上一轮替换完毕。本轮没有迁移成本，纯粹是"把 28 个提交接上"。

## 2. 分类判定

**第一步**——是否触及 6 条平衡规则？

| 规则 | 触及？ | 依据 |
|---|---|---|
| 1 TSV 数值字段 | 否 | 上游 28 提交未触碰任何 `.tsv`（实测 `git log --name-only HEAD..origin/master` 无 `.tsv`） |
| 2 掉落概率或掉落表构成 | 否 | 同上 |
| 3 玩家/怪物属性、伤害、命中、抗性、生命、法力计算 | 否 | `Source/tables/*` 的改动是代码结构（`itemdat.cpp`/`objdat.*`/`playerdat.*`），非数值 |
| 4 光照半径、视野、怪物激活或仇恨判定 | 否 | 无相关改动 |
| 5 商店库存、价格、可购买清单 | 否 | 无相关改动 |
| 6 战斗中可即时使用的资源量 | 否 | 无相关改动 |

**第二步**——玩家可感知的行为是否有变化？集成动作本身**否**。

**结论：Infra。** 但须如实声明：上游那 28 个提交**包含面向玩家的既有修复**（§1 症状 3 的清单）。合入后玩家会感知到这些修复——那是**上游的既有行为**，不是本次集成新做的设计选择（与上一轮对同一问题的判定一致）。本规格不新增任何平衡改动。

## 3. 事实基础

均于 2026-09-15 实测。

| 项 | 值 | 出处 |
|---|---|---|
| merge-base | `b3e52b1ea`（2026-07-26） | `git merge-base HEAD origin/master` |
| 上游 tip | `e00b7260f`（2026-09-15） | `git fetch origin master` |
| 落后提交数 | 28 | `git rev-list --count HEAD..origin/master` |
| 上游侧触及文件 | 106 | `git log --name-only --format="" HEAD..origin/master \| sort -u` |
| 本分支侧改动文件 | 263 | `git diff --name-only $(git merge-base HEAD origin/master) HEAD` |
| 两侧交集（理论冲突面） | 24 | `comm -12` |
| 探测性 merge 实际冲突 | **4 个文件 / 4 个冲突块** | 见下 |
| 测试基线 | 698（0 failed / 3 skipped） | `python3 tools/run_tests.py` |

### 探测性 merge 结果

`git worktree add --detach /tmp/dvl-merge HEAD` → `git merge --no-commit --no-ff origin/master` → 记录 → `git merge --abort` → 移除 worktree。**未编译、未提交**。

| 冲突文件 | 冲突块 | 冲突行数 | 性质 |
|---|---|---|---|
| `Source/engine/render/scrollrt.cpp` | 1 | 6 | 上游给 `DrawMain` 加 `drawInfoBox` 参数并插入 `this_sdl_thread::yield()`；fork 同处有改动 |
| `Source/controls/plrctrls.cpp` | 1 | 5 | 行级 |
| `Source/quests.cpp` | 1 | 5 | 行级 |
| `Source/quests.h` | 1 | **184** | 上游蘑菇任务状态（#8475）落在 fork 改动同一区块，需逐段人工对照 |

其余 **100 个文件自动合并**，含交集清单里的高风险项：`CMake/Tests.cmake`、`CMakeLists.txt`、`Source/CMakeLists.txt`、`Source/items.cpp`、`Source/loadsave.cpp`、`Source/pack.cpp`、`Source/pfile.cpp`、`Source/player.h`、`Source/inv.cpp`、`Source/tables/monstdat.h`。

**`Source/msg.cpp` 不在交集内**（上游 28 提交未触及它）——本轮同步不会动到刚修的 heroname 读取路径。

### 探测深入：`quests.h` 的整文件冲突（2026-09-15 追加）

初稿按「4 个文件各 1 块冲突」记录，其中 `quests.h` 标为「184 行块，需逐段对照」——**该措辞是错的**，实测结论如下：

| 项 | 事实 | 出处 |
|---|---|---|
| 冲突形态 | **整文件冲突**（`<<<<<<<` 在第 1 行，`=======` 在第 151 行） | 探测性 merge |
| 冲突成因 | 上游把 `quests.h` **重写为 LF**（fork 侧为 CRLF；24 个交集文件中唯一行尾不同的一个），并把它 148 行内容搬到新建的 `Source/tables/questdat.hpp` | `git diff --stat b3e52b1ea origin/master -- Source/quests.h` = `32 insertions(+), 148 deletions(-)`；三方行尾对比 CRLF(merge-base/HEAD) vs LF(origin/master) |
| fork 侧相对 merge-base 的增量 | **只有 1 行**：`struct QuestData` 末尾的 `std::string scriptName;` | `git diff b3e52b1ea HEAD -- Source/quests.h` = `1 insertion(+)` |
| 上游新家缺什么 | 上游 `Source/tables/questdat.hpp:117-127` 的 `struct QuestData` 字段与 fork 完全一致，**只缺 `scriptName`** | 逐字比对 |
| 行尾装置 | `.gitattributes` 为 `* -text`（git 不做行尾归一化）→ CRLF/LF 差异一律整文件冲突 | `.gitattributes` |

**因此 `quests.h` 的正确处置不是「逐段对照取并集」，而是**：取上游全文（保持 LF），把 fork 的 `scriptName` 落到 `tables/questdat.hpp` 的 `QuestData` 里。保持 LF 是硬要求：门禁 C 以**新的 merge-base（= 上游 tip）**为基线比对行尾类型，把该文件写回 CRLF 会直接 FAIL，且会保证以后每次同步都整文件冲突。

### 探测深入：上游 quest 重构的编译连带（2026-09-15 追加）

上游把 `QuestLogIsOpen` / `pQLogCel` / `DrawQuestLog` 从 `quests.h`+`quests.cpp` 迁到新建的 `Source/panels/quest_log.{hpp,cpp}`。fork 侧消费者：`Source/minitext.cpp:165`、`Source/qol/chatlog.cpp:107`（均写 `QuestLogIsOpen = false;`），它们原先依赖 `quests.h` 的声明 → 合并后预期编译失败，修法是补 `#include "panels/quest_log.hpp"`。这类连带由编译门禁暴露，实施计划的任务 7 已按「判据 + 已确认文件」写明，不做盲目预改。

## 4. 方案

### 集成方式：merge（不 rebase）

与上一轮同判据：本分支 224 个提交已推送（`myrepo/feature/qol-upgrades`），rebase 会重写公共历史。merge 产生一个 merge commit，4 处冲突一次解决，并保留"已同步到哪个上游提交"的语义，便于后续再次同步与 bisect。

不选「逐个 cherry-pick 28 个提交」：同一批文件会被反复触碰，冲突解决总量高于一次 merge，且丢失同步语义。

### 执行顺序

1. `git fetch origin master` 固化基线（**已完成**）。
2. 在 `feature/qol-upgrades` 上 `git merge origin/master`。
3. 解 4 处冲突（逐处判定依据）：
   - `quests.h` —— **取上游全文（保持 LF）**，并把 fork 的唯一增量 `QuestData::scriptName` 补进 `Source/tables/questdat.hpp` 的 `struct QuestData`（理由与行尾判据见 §3「探测深入」）。
   - `scrollrt.cpp` —— 取上游签名（`drawInfoBox` + `yield()`），核对 fork 的 `DrawMain` 调用点全部适配并**保留** fork 同处改动。
   - `plrctrls.cpp` —— 行级判定，取两侧 include 并集（`cursor_defs.hpp` + `levels/gendung.h`）。
   - `quests.cpp` —— 保留 fork 的两个 include（`lua/lua_event.hpp`、`minitext.h`），除非确认其消费者已随上游改动消失。
4. 处理上游 quest 重构的编译连带（`panels/quest_log.hpp`），四道门禁（配置 → 编译 → 测试目标 → 全量），见 §6。
5. 同步后立即复核 heroname：`git show origin/master:Source/msg.cpp | grep -n heroname`。若上游仍未修（当前如此）→ 保留 fork 修复，按 `docs/knowledge/pattern_fixed_width_field_reads.md` 记为 fork-only 分叉点，并把该修复提为上游 PR（文案与 ASan 复现已具备）。

### 风险与缓解

| 风险 | 依据（上游提交） | 缓解 |
|---|---|---|
| 线程模型改动影响 headless/测试路径 | `1bb39d680`、`b7eb65b11`、`3f71a5b8c` | 门禁 3/4 直接暴露；headless 路径由 `timedemo_test` + eval 套件覆盖 |
| CMake 依赖重组与 fork 的 `CMake/Tests.cmake` 改动交互 | `53b91fd7a`、`9f2ea6b99` | 探测性 merge 显示无冲突；门禁 1/2 验证配置与编译 |
| `quests.h` 184 行块误取一侧导致任务逻辑回归 | `2c1a364da`（#8475） | 逐段对照 + `quests_test` / `quest_script_test` |
| 上游渲染改动与 fork 渲染改动语义冲突 | `e00b7260f`、`c90181d54`；fork 侧有浮动信息 UI、双 tooltip、暗黑远征光照 | `scrollrt_test`、`palette_blending_test`、eval `render-*`；必要时逐屏验证 |
| 同步后漂移校验口径变化掩盖真实漂移 | `check_drift.py` 以 merge-base 为基线 | 同步前后各跑一次 drift 并对比输出，结论写入实施记录 |
| 行尾差异造成整文件冲突，且处置错误会直接挂门禁 | `.gitattributes` = `* -text`；上游把 `quests.h` 重写为 LF（24 个交集文件中唯一一个） | 被上游重写为 LF 的文件**保持 LF**（判据见 §3「探测深入」）；门禁 C 在 §6 第 7 项单列 |
| 上游 quest 重构（`QuestLogIsOpen`/`DrawQuestLog` 迁往 `panels/quest_log.hpp`）导致 fork 消费者编译失败 | 上游 #8475；fork 消费者 `minitext.cpp:165`、`qol/chatlog.cpp:107` | 由编译门禁暴露后按判据补 include（实施计划任务 7），不做盲目预改 |

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 分类已判定 | Infra，第 2 节给出判定树逐步结果 |
| 2 | 问题陈述指向具体症状 | 是：merge-base 停滞 2026-07-26、落后 28 提交、heroname 已成 fork-only 分叉、上游 6 个面向玩家的修复未合入。非"感觉该同步了" |
| 3 | 数值标注出处 | 是，第 3 节每项均有命令依据 |
| 4 | 「已实施」需有非测试调用者且验收标准全部通过 | 本规格状态为「草案」，未标「已实施」。达成 §6 全部标准后方可改标 |

### 基础设施红线

| # | 判定 | 依据 |
|---|---|---|
| I1 | 玩家可感知行为是否有变化？**集成动作本身否** | 无 TSV/数值/掉落/光照改动。上游 28 提交含既有修复（§1 症状 3），那是上游行为而非本次设计决策 |
| I2 | 是否有消费者？**有，类型 (a)** | 仓库内全部生产代码：配置/构建/线程/渲染路径在合入后即消费新版上游实现 |
| I3 | 它让哪个未来改动变便宜？**已指名** | ① heroname 修复过渡为上游 PR（同步后分叉面清晰）；② 密度框架 A1/A3（触及 `Source/monster.cpp`/`monstdat.h`/`scrollrt.cpp` 相邻区域）；③ 后续每次上游同步的冲突面不再包含这 28 个提交 |
| I4 | 是否引入新的运行时失败模式？**是，已定义并将测试** | 线程模型改动改变了渲染/音频线程的让步语义；headless 与 MP 路径的时序假设可能失效。由门禁 4（全量 698 项）+ eval smoke 覆盖 |

## 6. 验收标准

| # | 验收项 | 命令或方法 | 通过标准 |
|---|---|---|---|
| 1 | merge 完成且无残留冲突标记 | `git grep -nE "^(<<<<<<<\|>>>>>>>\|=======)$" -- Source/ test/ CMake/ CMakeLists.txt` | 无输出 |
| 2 | 已同步到上游 tip | `git rev-list --count HEAD..origin/master` | `0` |
| 3 | 构建通过 | `python3 tools/run_tests.py`（build 段） | `build.ok == true` |
| 4 | 全量测试通过 | `python3 tools/run_tests.py --json /tmp/ci.json` | `ctest.failed == 0`、`ctest.passed_pct == 100`（基线 698 项，以零失败为准，数量变化须记录原因） |
| 5 | eval smoke 门禁 | `python3 -m tools.eval.backend --smoke` | exit 0（36/36） |
| 6 | 漂移校验 | 同上 JSON `steps.drift` | `drift_ok == true`，5 项 PASS |
| 7 | 行尾未被破坏 | drift check C / C2 | 两项 PASS；并确认 `Source/quests.h` 保持上游的 LF（若被写回 CRLF，C 会 FAIL——见 §3「探测深入」） |
| 8 | heroname 分叉状态已复核并记录 | `git show origin/master:Source/msg.cpp \| grep -n heroname` | 有明确结论（已修/未修）并写入实施记录；未修则提上游 PR |
| 9 | timedemo quarantine 状态 | `cd build && ./timedemo_test --gtest_filter='Timedemo.*'` | 仍为 `Skipped`（若上游改动使其变化，记录原因） |
| 10 | CI 绿 | `gh run list --workflow=better-d1-ci.yml --limit 1` | 最新一次为 `success` |

## 7. 状态

**草案（待批准）。** 批准后按 §4 执行；实施计划另立 `docs/superpowers/plans/`（本规格 §4 已给出可直接展开的步骤与门禁）。

相关规格：

- `2026-07-27-upstream-integration-design.md` —— 上一轮上游集成（已实施；本轮前置条件由此满足）
- `docs/knowledge/pattern_fixed_width_field_reads.md` —— heroname/玩家名定长读取规则（fork-only 分叉点的记录依据）
- `docs/knowledge/gotcha_vendor_predicate_seed_drift.md` —— 同步后若上游改动物品生成路径，测试数据漂移的排查依据