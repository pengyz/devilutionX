#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "drlg_test.hpp" // TestInitGame / GetTileCount（本仓既有测试夹具）
#include "levels/gendung.h"
#include "monster.h"
#include "tables/monstdat.h"
#include "utils/paths.h"
#include "utils/str_cat.hpp"

using namespace devilution;

namespace {

// SAMPLING_REPORT 约定：复制自 test/sampling_behavior_test.cpp（不共享头文件，
// 按简报要求在本 TU 内独立实现）。未设置该环境变量时静默跳过写文件。
std::string MeasurementReportPath()
{
	const char *path = std::getenv("SAMPLING_REPORT");
	return path == nullptr ? std::string {} : std::string { path };
}

void AppendMeasurementReport(const std::string &text)
{
	const std::string path = MeasurementReportPath();
	if (path.empty())
		return;
	std::ofstream out(path, std::ios::app);
	out << text;
}

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

} // namespace

TEST(LevelRosterBaseline, PlacesMonstersForCathedralL1)
{
	// 建关过程中可能触发任务专属 set-piece（如 rnd6.dun），这些文件只作为测试夹具存在于
	// test/fixtures/levels/ 下，需在 TestInitGame 之前设置 PrefPath 使其通过覆盖路径解析
	// （与既有 drlg_l1_test.cpp 等测试调用 LoadExpectedLevelData 的顺序一致）。
	// InitMonsters() 内部的 PlaceQuestMonsters/PlaceUniqueMonst 需要真实素材（如
	// monsters\monsters\genrl.trn），仅靠 TestInitGame 加载的核心归档不够，需像
	// sampling_behavior_test.cpp 的 SetUpTestSuite 一样额外加载游戏归档。
	LoadGameArchives();
	ASSERT_TRUE(HaveMainData()) << "requires spawn.mpq/DIABDAT.MPQ in build dir";
	gbIsSpawn = false; // 与 sampling_behavior_test.cpp 一致：仅有 spawn.mpq 时不清空任务

	paths::SetPrefPath(paths::BasePath() + "test/fixtures/");
	TestInitGame();
	LoadMonsterData();
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

TEST(LevelRosterBaseline, PlacedClassMixReport)
{
	// 同上：InitMonsters() 的任务/唯一怪放置需要真实素材（genrl.trn 等），须额外加载游戏归档。
	LoadGameArchives();
	ASSERT_TRUE(HaveMainData()) << "requires spawn.mpq/DIABDAT.MPQ in build dir";
	gbIsSpawn = false;

	// L2-L4 建关可能触发 skngdo.dun/banner2.dun 等任务 set-piece，同样只以夹具形式存在。
	paths::SetPrefPath(paths::BasePath() + "test/fixtures/");
	TestInitGame();
	LoadMonsterData();
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
