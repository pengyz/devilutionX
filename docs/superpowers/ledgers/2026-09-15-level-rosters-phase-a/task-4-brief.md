# Task 4 简报（阶段 A 收尾：阈值 + eval + 台账）

> **前置**：任务 1（placed class mix **基线**，见计划「实施记录」/「A-baseline」小节）、任务 2a/2b（名册表 + 加载/校验/生产接线）、任务 3（采样接入：core 预加 + 有界尾池 + 配额 + 逐层预算 + R28 legacy fallback）均已完成并入库。
>
> **本任务要交付**：
> 1. **`PlacedClassMixWithinBaseline` 用例**（加进 `test/level_roster_baseline_test.cpp`，复用该文件的忠实建关夹具）：按**已放置怪物数**统计 L13-15 的远程类占比（`RangedTurret + RangedKite`），与**改动前基线**比较——基线值见计划「实施记录」：`L13 = (3249+2076)/23405 = 22.8%`、`L14 = (9674+3425)/23443 = 55.9%`、`L15 = 13533/23193 = 58.3%`；判据为"**远程类占比 ≤ 基线 + 5 个百分点**"（阈值写进用例内的具名常量数组，并注明其来源）。
>    **若超限**：按裁决 **R4** 调整**名册或 `class_floors`**（例如把某个远程 core 换成近战 core、或调低/去掉某个远程 floor），**不得**放宽阈值或删断言。调整后必须重跑 `level_roster_test`（数据自洽）与 `level_roster_baseline_test`。
> 2. **eval 集成用例** `eval/cases/rng/level-rosters.yaml`（结构对标 `eval/cases/rng/sampling-anti-monopoly.yaml`）：断言本阶段新增的采样/名册相关 gtest 用例全过；并跑 `python3 -m tools.eval.backend --smoke`（exit 0）。
> 3. **格式台账**：在 `docs/knowledge/decision_save_format_policy.md` 的台账表追加一行（日期 / 变更："逐层名册改变采样顺序 → `monster.levelType` 索引语义变化" / 触碰：`Source/monster.cpp`、`assets/txtdata/monsters/level_rosters.tsv`、`level_roster_params.tsv` / 兼容处置：**不做兼容**，宪章决策 35）。
> 4. **两处顺手修**（均来自任务 3 的复审，成本极低但都是"判别力/CI"问题）：
>    - **O1（提高断言判别力）**：`test/sampling_behavior_test.cpp` 的 `EnginePreAddClassCount` 目前把该层**全部** unique 计入豁免，而引擎只**无条件预加** Golem 与 **6 个 quest unique 的 base**（`Source/monster.cpp` 的 `PlaceQuestMonsters` 一带，约 `:3464-3475`）；其余 unique 由 `PlaceUniqueMonsters()` 在**已有类型**中挑选、不占新槽位。请把豁免收紧为"Golem + 6 个 quest unique 的 base（按 `mlevel` 与 quest 可用性）"，使 `RosterQuotasSatisfied` 在 L9 RangedKite / L13 全类等格子上**不再恒真**；并加一条能体现判别力的用例（例如构造一个 core 恰好压满 cap 的层，断言断言仍能抓住越界）。**注意**：不要为了让它通过而放宽 `EXPECT_LE`。
>    - **include 顺序（CI 格式）**：`test/sampling_behavior_test.cpp:38` 新增的 `#include "tables/questdat.hpp"` 违反 `SortIncludes: true`（clang-format 18 的 `test` 路径检查会标出）→ 移到 `tables/monstdat.h` 之前（按仓库既有顺序），并本地跑一次 `clang-format` 确认无差异。
> 5. **全量门禁**：`python3 tools/run_tests.py --json /tmp/ci.json` → `failed==0 && passed_pct==100 && drift_ok==true`。
>
> **不在本任务范围**：阶段 B（核心小队 / G1 / G2）；阶段 A2（HF overlay 的 L17-24 名册表，L17-24 当前走 R28 legacy 路径）；范围外观察 **O2**（加载期校验是否应把 quest 预加计入 cap——属规格 §4.2.2 预算语义，已裁定留阶段 B）。
>
## Task 4: class mix 验收、eval、夹具与台账

**文件：**
- 修改：`test/level_roster_baseline_test.cpp`（加入阈值用例）
- 新建：`eval/cases/rng/level-rosters.yaml`
- 修改：`docs/knowledge/decision_save_format_policy.md`（台账追加一行）

**接口：**
- 依赖输入：任务 1 的基线表（写入本计划「实施记录」）、任务 3 的采样行为

- [ ] **步骤 1：写入基线阈值用例（失败）**

在 `test/level_roster_baseline_test.cpp` 中，用任务 1 实测得到的基线值（示例：假设基线 L13 远程类占比为 X%，则阈值为 X+5）写成断言：

```cpp
TEST(LevelRosterBaseline, PlacedClassMixWithinBaseline)
{
	TestInitGame();
	constexpr int kSeeds = 200;
	for (uint8_t level = 13; level <= 15; level++) {
		size_t total = 0;
		size_t ranged = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			CreateDungeonForMeasurement(level, 9000 + seed);
			InitLevelMonsters();
			ASSERT_TRUE(GetLevelMTypes().has_value());
			ASSERT_TRUE(InitMonsters().has_value());
			const auto mix = MeasurePlacedClassMix(level, 9000 + seed);
			total += ActiveMonsterCount;
			ranged += mix[static_cast<size_t>(BehaviorClass::RangedTurret)] + mix[static_cast<size_t>(BehaviorClass::RangedKite)];
		}
		const double share = static_cast<double>(ranged) / static_cast<double>(total);
		EXPECT_LE(share, kRangedShareCeiling[level]) << "level " << static_cast<int>(level);
	}
}
```

`kRangedShareCeiling` 以任务 1 的基线 + 5 个百分点填充（写在文件内的 `constexpr std::array<double, 17>`）。

- [ ] **步骤 2：运行并确认其失败或通过**

运行：`python3 tools/run_tests.py --test level_roster_baseline_test`
预期：若名册抬高了远程占比则 FAIL —— 此时按规格 §4.5 **调整 `class_floors` 或 core 名单**（不改阈值），直到 PASS

- [ ] **步骤 3：新增 eval 用例**

`eval/cases/rng/level-rosters.yaml`（结构对标 `eval/cases/rng/sampling-anti-monopoly.yaml`）：断言 `level_roster_test` 与 `sampling_behavior_test` 的 Roster* 用例全过。

- [ ] **步骤 4：重生成受影响夹具 + 台账**

（夹具重生成已在任务 3 完成；本任务只需确认全量门禁仍绿。）

在 `docs/knowledge/decision_save_format_policy.md` 台账追加一行（日期 / 变更："逐层名册改变采样顺序 → `monster.levelType` 索引语义变化" / 触碰：`Source/monster.cpp`、`assets/txtdata/monsters/level_rosters.tsv` / 兼容处置：**不做兼容**（决策 35））。

- [ ] **步骤 5：全量门禁 + eval + 漂移**

```bash
python3 -m tools.eval.backend --smoke; echo "exit=$?"
python3 tools/run_tests.py --json /tmp/ci.json
```
预期：eval exit 0；`ctest.failed == 0`、`passed_pct == 100`、`drift.drift_ok == true`

- [ ] **步骤 6：提交**

```bash
git add test/level_roster_baseline_test.cpp eval/cases/rng/level-rosters.yaml docs/knowledge/decision_save_format_policy.md
git commit -m "test(roster): pin placed class mix to the measured baseline and add the eval case"
```
