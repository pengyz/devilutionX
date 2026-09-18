# Task 2a 第二轮复审（R2）

基线 `d62cc26ab` → Head `c0592301a`（含预期的规格措辞提交 `c51a03977`）
只读评审，未改动工作区/暂存区/HEAD（`git status --porcelain` 干净）。

## 问题裁决

### F2【重要｜R22 自证式用例】— **ADDRESSED**

四个核实点逐条结论：

**① 测试调用的是生产函数，而非测试内本地副本 — 通过**
`test/level_roster_test.cpp:170-171`：
```cpp
SortRosterByLevel(entries);
const std::span<const LevelRosterEntry> level1 = FindLevelRoster(entries, 1);
```
两者是 `Source/tables/level_roster.h:141`（`SortRosterByLevel`）与 `:149`（`FindLevelRoster`）导出的声明，实现在 `Source/tables/level_roster.cpp:145-150` 与 `:152-159`。用例内**已不存在**任何 `stable_sort` / `find_if` 副本（diff 行 216-223 删除了上一轮重抄的排序 + 双 `find_if` 区间扫描）。测试文件仍 `#include <algorithm>`（`:3`），但已无 `<algorithm>` 用法——属残留 include，无害。

**② 两个函数确实有生产调用者 — 通过**
- `LoadLevelRoster()` → `SortRosterByLevel(Entries)`：`Source/tables/level_roster.cpp:251`
- `GetLevelRoster()` → `FindLevelRoster(Entries, level)`：`Source/tables/level_roster.cpp:263`（函数体已退化为一行转发）

独立核实：本地跑 `python3 tools/check_drift.py --base origin/master` → `PASS E  no test-only production functions`（5/5 全 PASS），确认这两个新导出符号不是 test-only。

**③ 删掉生产端排序，用例是否会失败 — 会失败（逻辑判定，未改代码）**
输入 `[L1 WSKELAX, L2 SNEAK, L1 NZOMBIE]`。若 `SortRosterByLevel` 体内的 `stable_sort`（`:147-149`）被移除，序列保持原序，`FindLevelRoster(entries, 1)` 从 index 0 起找到 `level==1`，随即在 index 1（`level==2`）终止区间 → 返回 size 1，`ASSERT_EQ(level1.size(), 2u)`（`:173`）失败。若把 `stable_sort` 换成非稳定 `sort`，同层两行顺序可能互换，`:174-175` 的 `EXPECT_EQ(level1[0].type, MT_WSKELAX)` / `level1[1] == MT_NZOMBIE` 可捕获。**用例已真正锚定生产逻辑，不再自证。**

*残留缺口（非阻塞，R22 已明示推迟）*：删掉 `LoadLevelRoster()` 里第 251 行那次**调用**（而非函数体）该用例仍会通过——因为用例直接驱动纯函数，不经 `LoadLevelRoster()`。接线本身的端到端锚定（真实交错 TSV fixture → `LoadLevelRoster()` → `GetLevelRoster(1)`）由 R22 自身判给 Task 2b（报告 `task-2a-report.md:154`），且漂移检查 E 至少强制了"生产调用者必须存在"。R22 规定的闭合方式已按字面完成。

**④ 覆盖"第二批同类成员不丢" — 通过**
`:173` `ASSERT_EQ(level1.size(), 2u)` 断言两批 L1 都在；`:174-175` 额外断言批内顺序（稳定性）。用例名与注释（`:159-163`）也明确指向该语义。

### Low 1【错误文案中英混排】— **ADDRESSED**

`Source/tables/level_roster.cpp:229` 已改为纯英文：
```
"... but only ", effective, " candidates exist (", available, " raw candidates, B1 sampling caps this class to ", cap, ")"
```
原"被 caps 截断"已移除（diff 行 67→68）。全文件核实：`grep -P '[^\x00-\x7F]'` 对 `Source/tables/level_roster.cpp` 与 `.h` **零命中**。断言依赖的关键词 `class floor` / `caps` 均保留，`:130`/`:156` 两条用例仍成立（实测通过）。

## 修复 Diff 中的新增破坏

**无 Critical / Important 破坏。**

已逐项核实无回归：
- 公共 API 新增 2 个导出符号（`level_roster.h:141,149`），带完整 doc 契约（`FindLevelRoster` 明确要求入参已排序）；`SortRosterByLevel` 取可变 `std::span<LevelRosterEntry>`，`FindLevelRoster` 取 `const` span，签名与 R22 要求一致。
- `GetLevelRoster()`/`LoadLevelRoster()` 行为**等价重构**：抽出的函数体与原内联代码逐字一致（diff 行 90-93、105-110），无语义漂移。
- `level_roster.cpp:248-250` 的注释仍提及 `GetLevelRoster()`，不触发漂移检查 E（`tools/check_drift.py:220` 跳过注释行；且 `GetLevelRoster` 在 `test/` 下已无任何命中，check E 根本不评估它）。
- 行尾未变：`.cpp`/`.h`/测试文件均仍为 CRLF（`file` 确认），drift C/C2 PASS。
- `c51a03977` 的规格措辞修订（`docs/.../2026-09-15-level-rosters-design.md:177`）只是加了 L16 cap 不可达的注解，与代码 `BehaviorClassCapForLevel`（`:133-143`，本轮未改）一致。

**报告陈述核实**：
- "17 用例全过" — 核实通过。`grep -c '^TEST_F' test/level_roster_test.cpp` = 17；实跑 `./build/level_roster_test --gtest_filter='LevelRosterTest.*'` → `[ PASSED ] 17 tests.`；二进制含新用例名与新英文文案（`strings` 命中），即已包含本轮改动。
- "drift 5/5 含 E" — 核实通过（本地实跑，A/B/C/C2/E 全 PASS）。
- "全量 721 / passed_pct 100" — **未复跑**（按指令不重跑整套）；间接支持：聚焦测试 17/17 通过 + 漂移 5/5 通过，且改动仅限本模块 3 个文件。

## 范围外观察（非阻塞）

1. `Params` 仍未排序 / 未查重（同层重复 params 行时 `GetLevelRosterParams` 返回首个）——R21 已判给 Task 2b，本轮如实未处理。
2. `test/level_roster_test.cpp:3` 的 `#include <algorithm>` 在删掉本地 `stable_sort` 后已成死 include，可清理。
3. `test/level_roster_test.cpp:209` 注释含非 ASCII 数学符号 `∪`（文件为 UTF-8）。它在注释而非错误文案里，不属 Low 1 范围，但若项目要求源文件纯 ASCII 可顺手换成 `union of`。
4. `FindLevelRoster` 返回指向调用方缓冲的 span，头文件只在 `LoadLevelRoster()` 的 doc 里说明重载失效契约；作为公开纯函数，可考虑在其自身 doc 里补一句"返回视图的生命周期随 `entries`"。

## 裁决

**修复轮次：全部问题已处理且无新增 Critical/Important 破坏。**

- F2 — ADDRESSED（4 个核实点全部通过；唯一残留是"接线本身未被测试锚定"，由 R22 自身判给 Task 2b，不构成本轮未关闭项）
- Low 1 — ADDRESSED

未关闭项：无。
