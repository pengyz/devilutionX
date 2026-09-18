## Task 1: 基线实测（placed class mix 与名册规模）

**文件：**
- 新建：`test/level_roster_baseline_test.cpp`
- 修改：`CMake/Tests.cmake`（在 `sampling_behavior_test` 之后追加 `level_roster_baseline_test`）

**接口：**
- 对外产出：`std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> MeasurePlacedClassMix(uint8_t level, uint32_t seed)` —— 生成该层真实关卡、跑采样与放置，按**已放置怪物数**统计行为类别；返回数组的元素和等于该次放置的 `ActiveMonsterCount`。
- 对外产出：`std::vector<_monster_id> MeasureRealisedTypes(uint8_t level, uint32_t seed)` —— 该次采样的 `LevelMonsterTypes` 类型集合（供任务 3 复用同一夹具）。

- [ ] **步骤 1：编写失败的测试**

```cpp
// test/level_roster_baseline_test.cpp
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "drlg_test.hpp" // TestInitGame / GetTileCount（本仓既有测试夹具）
#include "levels/gendung.h"
#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

namespace {

// 无 fixture 的建关：与 TestCreateDungeon 相同的前置，但不做 fixture 断言，
// 因此可以使用任意种子。
void CreateDungeonForMeasurement(uint8_t level, uint32_t seed)
{
	LevelSeeds[level] = std::nullopt;
	currlevel = level;
	leveltype = GetLevelType(level);
	pMegaTiles = std::make_unique<MegaTile[]>(GetTileCount(leveltype));
	CreateDungeon(seed, ENTRY_MAIN);
	CreateThemeRooms();
}

} // namespace

TEST(LevelRosterBaseline, PlacesMonstersForCathedralL1)
{
	TestInitGame();
	CreateDungeonForMeasurement(1, 1000);
	InitLevelMonsters();
	ASSERT_TRUE(GetLevelMTypes().has_value());
	ASSERT_TRUE(InitMonsters().has_value());

	const auto mix = MeasurePlacedClassMix(1, 1000);

	size_t total = 0;
	for (const size_t count : mix)
		total += count;
	EXPECT_GT(total, 0u) << "level 1 must place at least one monster";
	EXPECT_EQ(total, ActiveMonsterCount);
}
```

- [ ] **步骤 2：运行测试并确认其失败**

运行：`python3 tools/run_tests.py --test level_roster_baseline_test --filter 'LevelRosterBaseline.*'`
预期：构建失败，提示 `MeasurePlacedClassMix` 未声明（同时漂移检查 A 会因为未注册而报错——本步骤只需确认编译失败）

- [ ] **步骤 3：编写最小实现**

```cpp
// 追加到 test/level_roster_baseline_test.cpp（namespace 内）
std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> MeasurePlacedClassMix(uint8_t level, uint32_t seed)
{
	std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> mix {};
	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		const Monster &monster = Monsters[ActiveMonsters[i]];
		mix[static_cast<size_t>(GetBehaviorClass(monster.ai))]++;
	}
	return mix;
}

std::vector<_monster_id> MeasureRealisedTypes(uint8_t level, uint32_t seed)
{
	std::vector<_monster_id> types;
	for (size_t i = 0; i < LevelMonsterTypeCount; i++)
		types.push_back(LevelMonsterTypes[i].type);
	return types;
}
```

（`MeasurePlacedClassMix` 不重新建关：调用方负责先 `TestInitGame` + `CreateDungeonForMeasurement` + `InitLevelMonsters` + `GetLevelMTypes` + `InitMonsters`，使同一个 seed 的"采样→放置"链路只跑一次。步骤 1 的测试已经这样调用。）

- [ ] **步骤 4：注册并运行，确认通过**

在 `CMake/Tests.cmake` 的 `sampling_behavior_test` 之后追加一行 `  level_roster_baseline_test`。

运行：`python3 tools/run_tests.py --test level_roster_baseline_test --filter 'LevelRosterBaseline.*'`
预期：PASS

- [ ] **步骤 5：产出基线数据（200 seeds × L1-16）**

```cpp
TEST(LevelRosterBaseline, PlacedClassMixReport)
{
	TestInitGame();
	std::string report = "\n## A-baseline: placed class mix (200 seeds per level)\n\n"
	                     "| level | placed | Melee | RangedTurret | RangedKite | Rally | Charge | Sneak | Summon | Boss |\n"
	                     "|---|---|---|---|---|---|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 16; level++) {
		std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> sum {};
		size_t placed = 0;
		for (uint32_t seed = 0; seed < 200; seed++) {
			CreateDungeonForMeasurement(level, 5000 + seed);
			InitLevelMonsters();
			ASSERT_TRUE(GetLevelMTypes().has_value());
			ASSERT_TRUE(InitMonsters().has_value());
			const auto mix = MeasurePlacedClassMix(level, 5000 + seed);
			for (size_t i = 0; i < mix.size(); i++)
				sum[i] += mix[i];
			placed += ActiveMonsterCount;
		}
		report += StrCat("| ", level, " | ", placed, " |");
		for (const size_t count : sum)
			report += StrCat(" ", count, " |");
		report += "\n";
	}
	AppendMeasurementReport(report); // 复用 sampling_behavior_test.cpp 的同名约定（见步骤 5 注）
}
```

注：`AppendMeasurementReport`/`SAMPLING_REPORT` 约定已在 `test/sampling_behavior_test.cpp` 中存在（P0 实测套件）。本文件复制同一实现（约 12 行，`std::getenv("SAMPLING_REPORT")` + `std::ofstream` 追加），**不要**跨测试二进制共享头文件。

运行：
```bash
cmake --build build --target level_roster_baseline_test -j 20
rm -f /tmp/a-baseline.md && cd build && SAMPLING_REPORT=/tmp/a-baseline.md ./level_roster_baseline_test --gtest_filter='LevelRosterBaseline.PlacedClassMixReport'
```
预期：PASS，并在 `/tmp/a-baseline.md` 得到 L1-16 的基线表；**把该表复制进本计划的「实施记录」小节**（后续任务的阈值以此为准）。

- [ ] **步骤 6：提交**

```bash
git add test/level_roster_baseline_test.cpp CMake/Tests.cmake docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md
git commit -m "test(roster): measure the placed class-mix baseline per level"
```

---

