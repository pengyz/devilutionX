# 事实漂移机械校验

**日期**：2026-07-28
**分类**：Infra
**状态**：已批准
**评判基准**：`2026-07-27-better-d1-design-charter.md`

---

## 1. 问题陈述

宪章第 0 节的结论是「宪章必须由可判定命题构成」，但可判定命题的价值只有在被机械执行时才兑现。2026-07-27 的清理会话证明「靠自觉遵守」不成立——上一位作者违反四次，执行者本人违反一次。

该次会话找出的缺陷按根因分类：

| 缺陷 | 数量 | 根因 |
|---|---|---|
| 死代码（有测试、有文档、零生产调用者） | 7 个函数 + 1 个数据字段 | 从未被真正调用 |
| 数据损坏（`pack.cpp` 与 `msg.cpp` 的 `bId` 覆写、存档读写不对称） | 3 处 | 写入后未验证往返 |
| 测试构建损坏 | 1 处 | 删源文件未删测试及其 `Tests.cmake` 条目 |
| 从未编译过的工具 | 1 个文件 | 藏在默认关闭的 `BUILD_DEV_TOOLS` 后面 |
| 文档数值与代码不符 | 12 处 | 无人核对 |
| 错标「已实施」 | 1 处 | 只查函数调用者，未查验收标准 |
| 行尾相对基线被改变 | 6 个文件 | 无人检查 |

其中至少五类可由脚本机械挡住。具体到可复现的症状：

- `test/world_state_test.cpp` 曾 `#include "world_state.h"` 而该头文件已删除，同时其 `CMake/Tests.cmake` 条目仍在，测试构建因此损坏 3 天无人发现
- `GetMonsterActivationRadius`、三个任务奖励函数、两组注册表入口点共 7 个函数仅有测试调用者，却在文档中标注「已实现」
- `Source/diablo.cpp` 曾因行尾被改产生 7063 行 diff，其中仅 31 行是真实改动

## 2. 分类判定

按宪章第 2 节判定树：

**第一步**——是否触及 6 条平衡规则？否，脚本不读写游戏数据。

**第二步**——玩家可感知的行为是否有变化？否，脚本不参与构建产物。

**结论：Infra。**

## 3. 事实基础

均于 2026-07-28 核实。

### 四项检查在当前仓库的实测噪声

| 检查 | 当前报告数 | 性质 |
|---|---|---|
| A `Tests.cmake` 条目缺源文件 | 0 | 精确，无启发式 |
| B 占位测试（`EXPECT_TRUE(true)` / `ASSERT_TRUE(true)`） | 0 | 精确 |
| C 改动文件的行尾相对基线变化 | 0 | 精确，需 `--diff-filter=M` 排除新增文件 |
| C2 新增文件行尾符合 `.editorconfig` | 0 | 精确 |
| E 仅有测试调用者的生产函数 | 10（全部为上游既有，入白名单） | 启发式，需白名单 |

### 检查 C 的判据演进

初版按 `.editorconfig` 严格要求 CRLF，报出 15+ 假阳性：`CMake/platforms/ios.toolchain.cmake`、`Source/lua/lua_event.hpp` 等上游本身即为 LF，`CMakeLists.txt` 本身是 757/759 混合。

**可执行的规则只能是「不改变」，不能是「符合」**，因为上游自身不完全符合 `.editorconfig`。宪章禁令 7 的措辞「不改变文件行尾」恰好正确。

新增文件无基线可比，故拆为两项：改动文件比基线（检查 C），新增文件比 `.editorconfig`（检查 C2）。

### `.editorconfig` 与实践的实测关系

| 目录 | 上游 CRLF 文件数 | 上游 LF 文件数 | `.editorconfig` 规则 |
|---|---|---|---|
| `Source/` | 587 | 3 | `[*]` → crlf |
| `test/` | 55 | 11 | `[*]` → crlf |
| `tools/` | 0 | 17 | `[*.py]` → lf（第 28 行）；`.cpp` 落入 `[*]` |
| `docs/` | 0 | 20 | `[*.md]` → lf（第 59 行） |

`.editorconfig` 无需修改：它已为 `*.md` 与 `*.py` 声明 LF，与上游实践一致。

### 检查 E 的假阴性教训

初版的「定义识别」规则把任何以 `{` 结尾或匹配 `类型 符号(` 的行判为函数定义并排除，导致 `Source/spell_tooltip.cpp:560` 的 `if (!CanLearnSpell(spell, player)) {` 被误判，`CanLearnSpell` 被错报为死代码。

修正为「定义行必然从第 0 列开始，缩进行一定是调用」。修正后疑似项从 17 降至 10，消除六项假阳性：`CanLearnSpell`、`AutoPlaceItemInStash`、`CanSellToCurrentVendor`、`ComputeFileSha256`、`HexToModHash`、`UnPackNetPlayer`。

**假阴性比假阳性危险**：假阳性只浪费注意力，假阴性会导致误删生产代码。

### 检查 E 的白名单内容

10 项全部在 merge-base `b3e52b1ea` 的 `Source/` 中已存在，即全为上游既有状态，不属本 fork 责任范围：

| 符号 | 声明位置 |
|---|---|
| `DiscardMultipleFields` | `Source/data/parser.hpp` |
| `GetNumTownerTypes` | `Source/towners.h` |
| `GetVisualStoreItemCount` | `Source/qol/visual_store.h` |
| `GetVisualStorePageCount` | `Source/qol/visual_store.h` |
| `ParseFixed6Fraction` | `Source/utils/parse_int.hpp` |
| `ResizeFile` | `Source/utils/file_util.h` |
| `SetAssetsPath` | `Source/utils/paths.h` |
| `TestRotateBlockedMissile` | `Source/missiles.h` |
| `VisualStoreNextPage` | `Source/qol/visual_store.h` |
| `VisualStorePreviousPage` | `Source/qol/visual_store.h` |

`VisualStoreNextPage` / `VisualStorePreviousPage` 对应 `ctest` 中长期处于 Skipped 状态的 `VisualStoreTest.Pagination_NextAndPrevious` 与 `Pagination_ResetsHighlight`——上游的分页功能未接入 UI。这是上游既有情况，记录而不处理。

**本 fork 改动引入的死代码为零。**

## 4. 方案

新建 `tools/check_drift.py`，实现五项检查，全部通过时退出码 0，任一失败时非 0 并列出具体位置。

| 检查 | 判据 | 数据来源 |
|---|---|---|
| A | `CMake/Tests.cmake` 的 `set(tests)` / `set(standalone_tests)` / `set(benchmarks)` 中每个条目在 `test/` 下有同名 `.cpp` | 解析 `Tests.cmake`，需剥除 `\r`（该文件为 CRLF） |
| B | `test/` 下无 `EXPECT_TRUE(true)` 或 `ASSERT_TRUE(true)` | grep |
| C | 相对 `git merge-base origin/master HEAD` 的**改动**文件，行尾类型未变 | `git diff --diff-filter=M`，排除二进制扩展名 |
| C2 | 相对同一基线的**新增**文本文件，行尾符合 `.editorconfig`（`.md` / `.py` 为 LF，其余为 CRLF） | `git diff --diff-filter=A` |
| E | `Source/**/*.h(pp)` 中声明的函数，若有 `test/` 引用而无 `Source/` 生产引用，则报告；白名单内的跳过 | grep + 定义行识别 |

白名单以显式列表写在脚本内，每项附「为何豁免」的一行说明。选择显式白名单而非「只扫本 fork 改动的文件」，因为后者会漏掉在上游文件中新增的函数。

不实现的检查：

- **文档数值与源码一致**。宪章第 4 节规定的出处格式（`值 — 文件:字段（日期 核实）`）目前仅宪章自身使用，样本不足以验证解析器正确性。待该格式被多份规格采用后再建。
- **「已实施」标注的验收标准**。验收标准是散文，无法机械判定。检查 E 已覆盖其中「函数有非测试调用者」这一半。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 分类已判定 | Infra，第 2 节给出判定树逐步结果 |
| 2 | 问题陈述指向具体症状 | 是：`test/world_state_test.cpp` 的悬空 include 与 `Tests.cmake` 条目、7 个仅有测试调用者的函数、`diablo.cpp` 的 7063 行 diff 中仅 31 行真实 |
| 3 | 数值标注出处 | 是，第 3 节每项均给出文件路径或命令 |
| 4 | 「已实施」需有非测试调用者且验收标准全部通过 | 本规格状态为「已批准」。达成第 6 节验收标准后方可改标 |

### 基础设施红线

| # | 判定 | 依据 |
|---|---|---|
| I1 | 玩家可感知行为是否有变化？**否** | 脚本不参与构建产物 |
| I2 | 是否有消费者？**有，类型 (a)** | 消费者是开发流程本身：脚本可手工运行，亦可接入 pre-commit 或 CI。不是为假想的未来消费者铺路 |
| I3 | 它让哪个未来改动变便宜？**已指名** | 待立项的「Base 层未文档化改动补文档」需要判定哪些函数真有生产调用者；每次上游集成后需要确认行尾未被改变。这两项目前均靠人工，且人工已失败五次 |
| I4 | 是否引入新的运行时失败模式？**否** | 脚本不进入游戏运行路径。其自身失败模式为误报，由白名单与退出码区分 |

## 6. 验收标准

| # | 验收项 | 命令 | 通过标准 |
|---|---|---|---|
| 1 | 脚本在当前仓库零报告 | `python3 tools/check_drift.py` | 退出码 0，输出全部检查 PASS |
| 2 | 检查 A 能捕获缺失源文件 | 临时向 `Tests.cmake` 加一个不存在的条目后运行 | 检查 A 失败并指名该条目 |
| 3 | 检查 B 能捕获占位测试 | 临时在某测试文件加 `EXPECT_TRUE(true);` 后运行 | 检查 B 失败并指名文件与行号 |
| 4 | 检查 C 能捕获行尾变化 | 临时把某个已跟踪的 CRLF 源文件转为 LF 后运行 | 检查 C 失败并指名该文件 |
| 5 | 检查 E 不误报 `CanLearnSpell` | `python3 tools/check_drift.py` | `CanLearnSpell` 不在输出中 |
| 6 | 检查 E 能捕获真死代码 | 临时在某头文件加一个仅测试调用的函数声明后运行 | 检查 E 失败并指名该符号 |
| 7 | 白名单项被跳过 | 检查 E 的输出 | 10 个上游符号均不出现 |
| 8 | 脚本自身行尾合规 | `grep -c $'\r' tools/check_drift.py` | 0（`.py` 按 `.editorconfig` 为 LF） |

每项负面测试（2、3、4、6）执行后必须回滚临时改动，并确认脚本回到退出码 0。

### 执行中脚本抓出的问题

脚本首次运行即报出五个手工扫描漏掉的文件——先前的人工核查只覆盖了 `.cpp` / `.h` / `.hpp` / `.tsv` / `.py` / `.md`，遗漏了 `.bat` 与 `CMakeLists.txt`：

| 文件 | 问题 |
|---|---|
| `CMakeLists.txt` | 上游为 752/752 纯 CRLF，我方追加的 7 行 `BUILD_DEV_TOOLS` 块用 LF，造成 757/759 混合 |
| `build_test.bat` | 新增文件用 LF |
| `tools/cel2png/CMakeLists.txt` | 同上 |
| `tools/mpqextract/CMakeLists.txt` | 同上 |
| `tools/png2clx/CMakeLists.txt` | 同上 |

五项均已修正。这是脚本存在价值的直接证明：同一类问题人工查过两遍仍有遗漏。

## 7. 状态

**已实施。** 2026-07-28 完成，八项验收标准全部通过。

四项负面测试均按预期失败并精确定位：向 `Tests.cmake` 插入不存在的条目、向测试插入 `EXPECT_TRUE(true)`、把 `Source/items.cpp` 转为 LF、向 `towners.h` 插入仅测试调用的函数声明。每项执行后回滚并确认脚本回到退出码 0。

本 fork 改动引入的死代码为零：检查 E 报出的 10 项全部在 merge-base 的 `Source/` 中已存在。
