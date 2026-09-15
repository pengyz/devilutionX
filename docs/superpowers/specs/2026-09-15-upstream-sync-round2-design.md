# 上游同步（第二轮）：28 个提交 / 4 处冲突

**日期**：2026-09-15（v2 —— 探测深入后的修订，见 §3「探测深入」）
**分类**：Infra
**状态**：已实施（2026-09-15；实施计划见 `plans/2026-09-15-upstream-sync-round2.md`，实施记录与验收逐项结果见 §7）
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
| 本分支侧改动文件 | 265 | `git diff --name-only $(git merge-base HEAD origin/master) HEAD` |
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

初稿预判「合并后 fork 侧消费者会因缺少声明而编译失败」——**该预判已被证伪**。上游把 `QuestLogIsOpen` / `pQLogCel` / `DrawQuestLog` 从 `quests.h`+`quests.cpp` 迁到新建的 `Source/panels/quest_log.{hpp,cpp}` 时，**同时给每个消费者文件补了 `#include "panels/quest_log.hpp"`**；fork 对这些文件的本地改动不与那一行重叠，故 git 自动合并即完成迁移。复验（`git -c rerere.enabled=false merge --no-commit --no-ff origin/master`）：

- 12 个消费者文件（`minitext.cpp`、`qol/chatlog.cpp`、`diablo.cpp`、`stores.cpp`、`control/control_panel.cpp`、`controls/game_controls.cpp`、`controls/touch/gamepad.cpp`、`controls/touch/renderers.cpp`、`controls/plrctrls.cpp`、`engine/render/scrollrt.cpp`、`test/panel_state_test.cpp`、`test/ui_test.hpp`）**各含 1 个**该 include；
- `Source/quests.cpp` 对这三个符号的引用计数为 **0**（上游的删除与 fork 的 2 行 include 冲突不重叠，自动合并已生效）→ 无重复定义风险。

**因此本轮预期零编译连带**，实施计划的任务 7 相应改为「验证零改动」，仅在编译真的报错时才现场诊断。另：`test/ui_test.hpp` 与 `test/panel_state_test.cpp` 也在 24 文件交集内（上游 `53b91fd7a`），自动合并成功；前者被 13 个测试二进制共享，故其健康度由全量门禁兜底。

**结论修正（2026-09-15 实施后补记）**：上述「零编译连带」的推断**只对非冲突文件成立**。`Source/controls/plrctrls.cpp` 虽是这 12 个消费者文件之一，**同时又是本次 merge 的 4 个手工冲突文件之一**——冲突的 hunk 被人工解决时，不会继承上游在同一 hunk 自动合并进来的 `#include "panels/quest_log.hpp"`，于是它以「有使用、无声明」的形态在编译期报了 7 处错误（该文件用到 `QuestLogIsOpen` / `QuestlogUp` / `QuestlogDown` / `StartQuestlog`）。该 include 由提交 `91e805149` 补上（1 行，位置符合 include 字母序）。**教训：冲突文件不适用「改动不重叠即自动合并安全」的推断，必须单独列入待补 include 的检查清单。**

## 4. 方案

### 集成方式：merge（不 rebase）

与上一轮同判据：本分支 224 个提交已推送（`myrepo/feature/qol-upgrades`），rebase 会重写公共历史。merge 产生一个 merge commit，4 处冲突一次解决，并保留"已同步到哪个上游提交"的语义，便于后续再次同步与 bisect。

不选「逐个 cherry-pick 28 个提交」：同一批文件会被反复触碰，冲突解决总量高于一次 merge，且丢失同步语义。

### 执行顺序

1. `git fetch origin master` 固化基线（**已完成**）。
2. 在 `feature/qol-upgrades` 上 `git merge origin/master`。
3. 解 4 处冲突（逐处判定依据）：
   - `quests.h` —— **取上游全文（保持 LF）**，并把 fork 的唯一增量 `QuestData::scriptName` 补进 `Source/tables/questdat.hpp` 的 `struct QuestData`（理由与行尾判据见 §3「探测深入」）。
   - `scrollrt.cpp` —— 保留上游新增的 `this_sdl_thread::yield();`，`DrawMain` 调用点保留 fork 形态（`DrawMain(hgt, false, ...)`），**不回退** fork 移除 `drawInfoBox` 脏矩形重构的改动（该处是**语义冲突**，原写「取上游签名（`drawInfoBox` + `yield()`）」并不准确，详见 §7「执行中与规格不符/未预料之处」第 1 点，裁决 R8）。
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
| ~~上游 quest 重构导致 fork 消费者编译失败~~ **已证伪**：上游补 include 的改动随自动合并进入 fork，12 个消费者文件均无脱钩 | 复验见 §3「探测深入：quest_log 重构」 | 无需缓解；保留任务 7 作为「零连带」的验证关卡（编译报错才现场诊断） |

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 分类已判定 | Infra，第 2 节给出判定树逐步结果 |
| 2 | 问题陈述指向具体症状 | 是：merge-base 停滞 2026-07-26、落后 28 提交、heroname 已成 fork-only 分叉、上游 6 个面向玩家的修复未合入。非"感觉该同步了" |
| 3 | 数值标注出处 | 是，第 3 节每项均有命令依据 |
| 4 | 「已实施」需有非测试调用者且验收标准全部通过 | 已于 2026-09-15 改标「已实施」：非测试调用者为仓库内全部生产代码（配置/构建/线程/渲染路径在合入后即消费新版上游实现），§6 十项验收逐项通过（逐项实测见 §7） |

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
| 9 | timedemo quarantine 状态 | `cd build && ./timedemo_test --gtest_filter='Timedemo.*'`，或按名字查 `build/Testing/Temporary/LastTest.log` | 仍为 `Skipped`（**按用例名确认**，不依赖聚合 skipped 计数；若上游改动使其变化，记录原因） |
| 10 | CI 绿 | `gh run list --workflow=better-d1-ci.yml --limit 1` | 最新一次为 `success` |

## 7. 状态

**已实施（2026-09-15）。** 合并提交 `8e92687a9`（双父：`fd2956ea8` + 上游 `e00b7260f`），`git rev-list --count HEAD..origin/master` = `0`。§6 十项验收逐项实测结果如下（数据取自 2026-09-15 本轮实测，未做任何推算）：

| # | 验收项 | 实测结果 | 状态 |
|---|---|---|---|
| 1 | merge 完成且无残留冲突标记 | `git grep -nE "^(<<<<<<<\|>>>>>>>\|=======)$" -- Source/ test/ CMake/ CMakeLists.txt` 无输出 | PASS |
| 2 | 已同步到上游 tip | `git rev-list --count HEAD..origin/master` = `0` | PASS |
| 3 | 构建通过 | `python3 tools/run_tests.py --json /tmp/ci-sync.json` → `steps.build.ok == true` | PASS |
| 4 | 全量测试通过 | 同次 JSON：`ctest.total=698`、`ctest.failed=0`、`ctest.passed_pct=100`、`ctest.skipped=3`（与基线 698/0/3 一致） | PASS |
| 5 | eval smoke 门禁 | `python3 -m tools.eval.backend --smoke` exit 0，**36/36 PASS**（产物 `eval/results/20260915-183246/eval-summary.json`，其 `git_head` 为 `91e805149`） | PASS |
| 6 | 漂移校验 | `drift.drift_ok == true`、`drift.passes == 5`；同步前后输出差异**只有** `merge-base:` 行（`b3e52b1ea` → `e00b7260f`），5 项仍全 PASS | PASS |
| 7 | 行尾未被破坏 | check C / C2 PASS；`Source/quests.h` 保持上游的 LF（CRLF 0 / LF 32） | PASS |
| 8 | heroname 分叉状态已复核并记录 | 上游 tip `e00b7260f` 上仍是 **5 处裸字段读取**（`Source/msg.cpp:1094/1360/1368/1398`、`Source/pack.cpp:445`），fork 修复完好未被覆盖；已由 `d1e122ab6` 留档 | PASS |
| 9 | timedemo quarantine 状态 | 按用例名确认 `Timedemo.WarriorLevel1to2` 仍为 `Skipped`（`build/Testing/Temporary/LastTest.log`），quarantine 未变 | PASS |
| 10 | CI 绿 | `gh run 34959598050`（`better-d1-ci.yml`，HEAD `d1e122ab6`）conclusion = **`success`** | PASS |

本轮新增/相关提交：`8e92687a9`（merge: sync upstream master，28 commits / tip `e00b7260f`）、`91e805149`（`fix(build): add panels/quest_log.hpp include after the upstream quest split`）、`d1e122ab6`（`docs(knowledge): record heroname divergence state after the upstream sync`）、`88035420d`（`fix(tools): build sampling_behavior_test in the gate`，最终全分支评审 F1 修复）。

### §7 补充验收（F1 修复后重跑门禁，2026-09-15）

修复 `TEST_TARGETS` 脱钩并重跑 `python3 tools/run_tests.py --json /tmp/ci-sync2.json` 后：

| 检查项 | 结果 |
|---|---|
| `ctest.total` / `failed` / `passed_pct` | `698` / `0` / `100` |
| `drift.drift_ok` / `drift.passes` | `true` / `5` |
| `build/sampling_behavior_test` mtime | `2026-09-15 19:10:33`（晚于本轮构建，此前为 `2026-08-13 22:30:58` 的旧二进制） |
| `SamplingBaselineTest.*` 用例数与结果 | 13 个用例全部出现在 `LastTest.log` 且均为 `[ OK ]` |
| `git rev-list --count HEAD..origin/master` | `0` |

冲突实际为 **4 处**（与探测一致）。另一项探测期风险本轮**未触发**：`questdat.hpp` 行尾维持 CRLF，本轮唯一 LF 的仍是 `Source/quests.h` 与上游新增的 7 个文件。

`tools/run_tests.py` 的 `TEST_TARGETS` 与 `CMake/Tests.cmake` 的脱钩需要更正表述：本轮上游只改了 benchmark 的链接依赖，**未新增/删除/改名任何 `tests`/`standalone_tests` 成员**——这一点属实；但**既有的 `sampling_behavior_test`**（在合并前就已注册于 `Tests.cmake` 的 `tests` 列表与 CTest）**早已处于脱钩状态**：它未被列入 `TEST_TARGETS`，导致 `run_tests.py` 的构建阶段从不重建它，ctest 长期用旧二进制跑它的 13 个 `SamplingBaselineTest.*` 用例（合并本身未引入或加重这个脱钩，pre-merge 的 `Tests.cmake` 同样含 `sampling_behavior_test`）。该缺口由最终全分支评审发现（F1），已由本轮提交 `88035420d`（`fix(tools): build sampling_behavior_test in the gate`）修复：`sampling_behavior_test` 加入 `TEST_TARGETS` 后重跑门禁验证，见下方补充验收数据。

### 执行中与规格不符/未预料之处

**1. `scrollrt.cpp` 的冲突是语义冲突，不是文本冲突（裁决 R8）**

- **现象**：上游在该 hunk 使用的 `drawInfoBox` 变量，已被 fork 自己的提交 `ed1dad939`（`refactor: remove drawInfoBox dirty rect`）在设计上移除——双方对同一处代码各有意图，git 无法按行取舍；`DrawMain(int dwHgt, bool drawDesc, ...)` 签名两侧一致。规格把它当作普通「取上游一侧」的行级冲突来写，是错的。
- **证据**：`git show <rev>:Source/engine/render/scrollrt.cpp | grep -c drawInfoBox` → merge-base `b3e52b1ea` = **4**、fork 侧（HEAD）= **0**、上游侧 `e00b7260f` = **4**；`DrawMain` 签名逐字一致。
- **处置/裁决**：实现者未擅自取舍，**正确停机上报**；控制者裁决 **R8** —— **保留**上游新增的 `this_sdl_thread::yield();`，**保留** fork 的 `DrawMain(hgt, false, ...)` 调用形态，**不回退** fork 的重构。§4 步骤 3 的原文「取上游签名」已按 R8 改写。

**2. `quests.cpp` 首次解决时被整块取了一侧（Critical，裁决 R9/R10）**

- **现象**：首次解决该冲突时 blob 与 merge 前**逐字节相同** —— 上游对该文件的 **341 行删除**（把 15+ 个函数与全局变量迁往 `Source/levels/drlg_quests.cpp`、`Source/panels/quest_log.cpp`、`Source/tables/questdat.cpp`）被整体丢弃，于是与新文件**重复定义符号**，链接期必然失败。
- **证据**：`git diff --stat b3e52b1ea origin/master -- Source/quests.cpp` = `2 insertions(+), 341 deletions(-)`；按 git 真实三方合并结果重建后该文件为 **638 行、纯 CRLF**（CRLF 638 / LF 638 / LF-only 0）；include 冲突块只保留 `#include "lua/lua_event.hpp"`，**删除** `#include "minitext.h"`（其消费者 `InitQTextMsg` 已随迁移消失，实测合并结果里无 `minitext.h` 符号被使用）。
- **处置/裁决**：任务评审发现后，控制者裁决 **R9/R10** —— 按 **git 真实三方合并结果**重建该文件，修复直接 **amend 进 merge commit `8e92687a9`**，未新增独立提交。

**3. §3「quest_log 重构零编译连带」的结论不完整（由 `91e805149` 落实）**

- **现象**：`Source/controls/plrctrls.cpp` 是 quest_log 重构的消费者，**同时又是本次 merge 的 4 个手工冲突文件之一**；冲突文件不会继承上游在同一 hunk 自动合并进来的 `#include "panels/quest_log.hpp"`，因此触发 **7 处编译错误**（该文件使用了 `QuestLogIsOpen` / `QuestlogUp` / `QuestlogDown` / `StartQuestlog` 却无声明）。
- **证据**：§3 的「12 个消费者文件各含 1 个 include」结论来自**非冲突文件**的自动合并；`plrctrls.cpp` 恰是唯一同时落在「消费者集合」与「4 个冲突文件集合」交集中的文件。修复为 1 行，位置符合 include 字母序（`minitext.h` 与 `missiles.h` 之间）。
- **处置/裁决**：由提交 `91e805149` 补上该 include。教训已回写 §3 末尾：**冲突文件不适用「改动不重叠即自动合并安全」的推断，必须单独列入待补 include 的检查清单。**

**4. §4 对 `quests.h` 的处置需显式给出「保持 LF」的判据来源**

- **现象**：§4 步骤 3 的处置本身（取上游全文并保持 LF）与 §3 判据一致，但初稿未把判据来源显式回指 §3，读者无从得知「保持 LF」是硬要求还是风格偏好；§4 风险表对应行虽已引用 §3，步骤 3 与风险表之间未互相闭合。
- **证据**：判据来自实测——`.gitattributes` 为 `* -text`（git 不做行尾归一化），门禁 C 以**新的 merge-base（= 上游 tip `e00b7260f`）**为基线比对行尾类型，把 `quests.h` 写回 CRLF 会直接 FAIL，且会保证以后每次同步都整文件冲突。本轮实测 `Source/quests.h` = CRLF 0 / LF 32，drift check C PASS。
- **处置**：§4 步骤 3、§4 风险表、§3「探测深入」、§6 第 7 项四处判据来源已一致（步骤 3 维持「取上游全文（保持 LF）」并注明「理由与行尾判据见 §3『探测深入』」）。**未发现 §3 与 §4 之间存在与实测矛盾的陈述。**

**文档内部不一致（本次一并修正）**：本规格原有三处对同一状态字段的表述互相矛盾——头部元数据写「**状态**：已批准」、§7 写「**草案（待批准）**」、§5 通用红线 #4 写「本规格状态为『草案』」。本次按实测统一为「**已实施（2026-09-15）**」，并同步更新 §5 通用红线 #4 的判定依据。

相关规格：

- `2026-07-27-upstream-integration-design.md` —— 上一轮上游集成（已实施；本轮前置条件由此满足）
- `docs/knowledge/pattern_fixed_width_field_reads.md` —— heroname/玩家名定长读取规则（fork-only 分叉点的记录依据）
- `docs/knowledge/gotcha_vendor_predicate_seed_drift.md` —— 同步后若上游改动物品生成路径，测试数据漂移的排查依据