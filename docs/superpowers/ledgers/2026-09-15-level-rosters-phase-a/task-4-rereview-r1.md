# Task 4 复审 - 修复轮次 1

基线 `a1de8dcda` → Head `44e8bcb2e`（3 commits：`f9fa34bb6` `6737a1a61` `44e8bcb2e`）

## 问题裁决

- **F1（措辞与替代解记录）：ADDRESSED**
  - `docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md`（diff 行 34）：「任务 4 名册调整（R4）」行改为「这是『在我们接受的余量标准（≥5pp）下唯一』的数据解，不是绝对唯一」，并记录 L14 `tail_draw=2` 替代解（0.5964 < 0.60876，物种 4→5）。
  - `docs/superpowers/specs/2026-09-15-level-rosters-design.md` 附录 E 新增第 5 条（diff 行 70）：含实测数据、R34 三条理由（守卫变脆/自相矛盾/R32 精神）、阶段 B 两条收敛路径。
  - 数据核实：`assets/txtdata/monsters/level_roster_params.tsv` L14 行仍为 `18000  1  Melee=2,RangedKite=1`（`tail_draw` 未改），命令 `cat` 已确认。措辞与数字与评审实测一致。

- **F2（`kRangedShareCeiling` 越界与静默误判）：ADDRESSED**
  - `test/level_roster_baseline_test.cpp` 两张表均由 `std::array<double,16>` 改为 `std::array<double,17>`（diff 行 139/175），L16 现为合法下标。
  - 新增具名哨兵 `kRangedShareUnconstrained = 1.0`（diff 行 137）替代 `0.0`，注释明确说明"未设基线"语义（diff 行 124-132）。
  - ceiling 表对哨兵层直接写哨兵值、**不加** tolerance（diff 行 173-174 注释 + L0-L12/L16 均为裸 `kRangedShareUnconstrained`），避免变成 1.05。
  - 实测重跑确认三层数值未变：`level 13 ranged share 0.236493`、`level 14 ranged share 0.491774`、`level 15 ranged share 0.505309` —— 与增量 1/4 报告完全一致，纯加固无行为改变。

- **F3（`passed_min` 机制）：ADDRESSED**
  - `eval/cases/rng/level-rosters.yaml` 与 `eval/cases/rng/sampling-anti-monopoly.yaml` 顶部均加 4 行维护提醒（硬编码计数/同步要求/两种失效方向/当前值+核对日期），diff 行 76-79、95-98。
  - 计数核实（`--gtest_list_tests` 实测）：`LevelRosterBaselineTest.*` = 3（PlacesMonstersForCathedralL1/PlacedClassMixReport/PlacedClassMixWithinBaseline），与 `level-rosters.yaml` 的 `passed_min: 3` 一致；`SamplingBaselineTest.*` = 24，与 `sampling-anti-monopoly.yaml` 的 `passed_min: 24` 一致。重跑两个二进制确认 `[  PASSED  ] 3 tests.` / `[  PASSED  ] 24 tests.`。
  - 未改动 eval 工具层代码（`tools/eval/` 未出现在本次 diff 文件列表），机制缺陷仍留在报告「遗留关注点」第 1 条。

- **F4（全量门禁证据 + clang-format 版本说明）：ADDRESSED**
  - 报告 F4 小节（task-4-report.md:351-366）给出真实值：`steps.ctest.passed=743`、`failed=0`、`passed_pct=100`、`steps.drift.drift_ok=True`，`gate_exit=0`。
  - clang-format 版本说明：系统装 14.0.0，CI 用 18；本地解出 18.1.8 用于校验（task-4-report.md:368-371）。
  - 核实 `test/.clang-format`（已读取）：仅 `BasedOnStyle: webkit` + 若干开关 + `SortIncludes: true`，**没有** `IncludeCategories`/`IncludeBlocks`，与报告"排序退化为块内字典序、不依赖版本特有分组启发式"的说法一致。

- **F5（`kQuestUniquePreAdds` 注释与表不一致）：ADDRESSED**
  - `Source/monster.cpp:3464-3475`（已读取核实）：`Q_BUTCHER` 加 `MT_CLEAVER`（`AddMonsterType(MT_CLEAVER, PLACE_SPECIAL)`，非 unique），其余 5 个（`Q_GARBUD/Q_ZHAR/Q_LTBANNER/Q_VEIL/Q_WARLORD`）加 `PLACE_UNIQUE`。与执行人的解释完全一致。
  - `test/sampling_behavior_test.cpp` 注释改为「MT_GOLEM 加上 monster.cpp:3464-3475 的六处 quest 门控 type 预加。这六处不是六个 unique：Q_BUTCHER 加普通 type MT_CLEAVER，其余五个加 unique 的 base type，所以下面这张表是 5 项，MT_CLEAVER 在 EnginePreAddClassCount 里单独计数」（diff 行 221-226）。行号从 `3463-3475` 校正为 `3464-3475`，与代码首行匹配（`if (!setlevel) {` 在 3463，第一条 quest 判断在 3464）。表仍为 5 项，未错误补成 6，也未把注释硬改成"5 个 unique"了事。

## 修复 Diff 中的新增破坏
无。重跑 `level_roster_baseline_test`（`LevelRosterBaselineTest.*`，3/3 PASS）与 `sampling_behavior_test`（`SamplingBaselineTest.*`，24/24 PASS）均绿，数值与报告一致；数组扩容/哨兵改动为纯加固，无生产代码（`Source/`）改动，无 TSV 数据改动，无测试新增/删除引发的计数漂移。

## 范围外观察
- 报告「遗留关注点」6 条提到本地 clang-format 18.1.8 不在仓库工具链里，后续贡献者只用系统 14 可能仍会提交出 CI 报格式错的代码——已被归类为工具层问题，未阻塞本轮，同意维持现状。

## 裁决
**修复轮次：全部问题已处理且无新增 Critical/Important 破坏。**
