#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "drlg_test.hpp" // TestInitGame / GetTileCount（本仓既有测试夹具）
#include "engine/assets.hpp"
#include "engine/load_file.hpp"
#include "levels/gendung.h"
#include "levels/trigs.h" // InitL1Triggers 等 + Freeupstairs（CreateLevel 逻辑复刻用）
#include "monster.h"
#include "tables/level_roster.h"
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

// 加载真实的 .til 素材到 pMegaTiles。TestCreateDungeon（drlg_test.hpp）用
// std::make_unique<MegaTile[]>(...) 分配一块全零的假缓冲区就够用，因为它只校验
// dungeon[][]/dTransVal[][]（DRLG_LPass3 之前的抽象层）。但本测试要测的是
// IsTileSolid -> dPiece[][] -> pMegaTiles[dungeon[x][y]-1] 这条链路（用于
// InitMonsters 内的可放置面积统计），全零的 pMegaTiles 会让 dPiece 恒为
// SOLData[0]，同样制造出一个与关卡几何无关的常量放置结果（F1 同一类问题的延伸）。
// 因此这里复刻 diablo.cpp:LoadLvlGFX 里按 leveltype 选择 .til 路径的部分
// （该函数本身也在匿名命名空间内，无法直接调用），只加载 .til，不加载
// .cel/special cels（纯图形数据，不影响怪物放置)。
void LoadRealMegaTiles(dungeon_type levelType)
{
	const char *til = nullptr;
	switch (levelType) {
	case DTYPE_CATHEDRAL:
		til = "levels\\l1data\\l1.til";
		break;
	case DTYPE_CATACOMBS:
		til = "levels\\l2data\\l2.til";
		break;
	case DTYPE_CAVES:
		til = "levels\\l3data\\l3.til";
		break;
	case DTYPE_HELL:
		til = "levels\\l4data\\l4.til";
		break;
	default:
		FAIL() << "LoadRealMegaTiles: unexpected leveltype " << static_cast<int>(levelType);
		return;
	}
	auto result = LoadFileInMemWithStatus<MegaTile>(til);
	ASSERT_TRUE(result.has_value()) << "Failed to load " << til << ": " << result.error();
	pMegaTiles = std::move(*result);
}

// 建关 + 建怪前置：对齐 diablo.cpp:LoadGameLevel 中「SOLData 早于 InitMonsters」
// 「触发器早于 InitMonsters（否则 trigs[] 全零, UBSan 会在 lighting.cpp 报越界）」
// 两条真实执行顺序约束（复核轮 2 F1/F5）。
//
// 复核者建议的写法直接调用 CreateLevel(ENTRY_MAIN)，但 CreateLevel 定义在
// diablo.cpp 的匿名命名空间内（`nm` 确认符号为内部链接的
// `_ZN10devilution12_GLOBAL__N_111CreateLevelE...`，小写 `t` binding），
// 测试 TU 无法链接到它。这里改为内联复刻 CreateLevel 的真实函数体：
// CreateDungeon() 建关 -> 按关卡类型分派触发器初始化 -> Freeupstairs()
// 收楼梯（L1-16 的 leveltype 恒不为 DTYPE_TOWN，故恒执行）。
// LoadRndLvlPal() 在 HeadlessMode 下是 no-op（已读源码确认），本测试省略。
//
// 已知的残留 fidelity 差距（不在复核者 F1-F5 范围内，本轮未修，仅记录）：
// 未调用 HoldThemeRooms()/InitThemes()/InitGolems()/InitObjects()，因此
// zharlib 在多次调用间不会被重置、主题房间不参与 Populated 排除逻辑、
// 4 个 golem 预留位不会计入 ActiveMonsterCount。复核者给出的建议流程同样未
// 包含这些调用，此处保持一致，避免在复核明确要求之外扩大改动面。
void CreateDungeonForMeasurement(uint8_t level, uint32_t seed)
{
	currlevel = level;
	leveltype = GetLevelType(level);
	LevelSeeds[level] = std::nullopt;
	DungeonSeeds[currlevel] = seed;
	LoadRealMegaTiles(leveltype);

	InitLevelMonsters();

	CreateDungeon(DungeonSeeds[currlevel], ENTRY_MAIN);
	switch (leveltype) {
	case DTYPE_CATHEDRAL:
		InitL1Triggers();
		break;
	case DTYPE_CATACOMBS:
		InitL2Triggers();
		break;
	case DTYPE_CAVES:
		InitL3Triggers();
		break;
	case DTYPE_HELL:
		InitL4Triggers();
		break;
	default:
		FAIL() << "CreateDungeonForMeasurement: unexpected leveltype for level " << static_cast<int>(level);
		return;
	}
	Freeupstairs();

	ASSERT_TRUE(LoadLevelSOLData().has_value()) << "LoadLevelSOLData must succeed before InitMonsters";
}

// Placed class mix acceptance threshold (spec §4.5, plan task 4).
//
// The BASELINE is the pre-change measurement recorded in the plan's "A-baseline"
// table (200 seeds per level, seed base 5000, placed-monster denominator), taken
// with this same fixture before the roster sampling landed. The fractions are
// written out as numerator/denominator exactly as that table reports them, so the
// numbers stay auditable against their source instead of being rounded literals:
//
//   L13 (3249 RangedTurret + 2076 RangedKite) / 23405 placed = 22.8%
//   L14 (9674 + 3425) / 23443                               = 55.9%
//   L15 (13533 + 0)   / 23193                               = 58.3%
//
// The criterion is "ranged share <= baseline + 5 percentage points" (R4): the
// roster may reshape a level's mix, but it must not turn a level into a ranged
// gallery. The 5-point band absorbs seed noise, not a design shift. If a level
// exceeds it, the roster or its class_floors is what changes - never this ceiling.
//
// I4 (task 5 review): L1-12 used to be sentinels, so widening those levels'
// max_image from 4000 to 16000/18000 had no quantitative guard at all - the very
// levels whose budget grew four-fold were the ones nothing measured. They are now
// filled from the SAME A-baseline table, same 200-seed fixture, same
// numerator/denominator convention, so the whole roster range L1-15 is bounded.
//
// Indexing: both tables are indexed BY LEVEL NUMBER and sized 17 so level 16 is
// a valid index rather than a buffer overrun. L16 stays UNCONSTRAINED, spelled as
// the sentinel 1.0 (a share can never exceed 1.0) rather than 0.0. 0.0 would read
// as "ceiling zero" and turn a missing baseline into a silent misjudgement - a
// guaranteed failure, or worse, a pass that means nothing. The sentinel is kept
// (not deleted now that L1-15 are filled) precisely so a future level added
// without a baseline - phase A2's L17-24 - is unconstrained-by-declaration rather
// than accidentally judged against zero. Adding such a level to the assertion
// loop REQUIRES filling its baseline here first.
constexpr double kRangedShareTolerance = 0.05;
constexpr double kRangedShareUnconstrained = 1.0;

constexpr std::array<double, 17> kRangedShareBaseline {
	kRangedShareUnconstrained,   // L0 (unused)
	(0.0 + 0.0) / 18235.0,       // L1  (no ranged type in the L1 pool at all)
	(2149.0 + 0.0) / 23453.0,    // L2
	(2819.0 + 0.0) / 25642.0,    // L3
	(4145.0 + 0.0) / 25291.0,    // L4
	(3446.0 + 0.0) / 23180.0,    // L5
	(3592.0 + 950.0) / 17646.0,  // L6
	(3640.0 + 1619.0) / 18063.0, // L7
	(1561.0 + 2652.0) / 17321.0, // L8
	(514.0 + 8281.0) / 16648.0,  // L9
	(0.0 + 8514.0) / 17100.0,    // L10
	(0.0 + 7717.0) / 16550.0,    // L11
	(1931.0 + 5910.0) / 17280.0, // L12
	(3249.0 + 2076.0) / 23405.0, // L13
	(9674.0 + 3425.0) / 23443.0, // L14
	(13533.0 + 0.0) / 23193.0,   // L15
	kRangedShareUnconstrained,   // L16
};

// A ceiling of exactly kRangedShareUnconstrained stays unconstrained: adding the
// tolerance to the sentinel would push it above 1.0 and obscure that reading.
constexpr std::array<double, 17> kRangedShareCeiling {
	kRangedShareUnconstrained, // L0 (unused)
	kRangedShareBaseline[1] + kRangedShareTolerance,
	kRangedShareBaseline[2] + kRangedShareTolerance,
	kRangedShareBaseline[3] + kRangedShareTolerance,
	kRangedShareBaseline[4] + kRangedShareTolerance,
	kRangedShareBaseline[5] + kRangedShareTolerance,
	kRangedShareBaseline[6] + kRangedShareTolerance,
	kRangedShareBaseline[7] + kRangedShareTolerance,
	kRangedShareBaseline[8] + kRangedShareTolerance,
	kRangedShareBaseline[9] + kRangedShareTolerance,
	kRangedShareBaseline[10] + kRangedShareTolerance,
	kRangedShareBaseline[11] + kRangedShareTolerance,
	kRangedShareBaseline[12] + kRangedShareTolerance,
	kRangedShareBaseline[13] + kRangedShareTolerance,
	kRangedShareBaseline[14] + kRangedShareTolerance,
	kRangedShareBaseline[15] + kRangedShareTolerance,
	kRangedShareUnconstrained, // L16
};

std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> MeasurePlacedClassMix()
{
	std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> mix {};
	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		const Monster &monster = Monsters[ActiveMonsters[i]];
		mix[static_cast<size_t>(GetBehaviorClass(monster.ai))]++;
	}
	return mix;
}

} // namespace

class LevelRosterBaselineTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadGameArchives();

		// CI 只带 spawn.mpq；本地缺素材时整套跳过而不是失败整个套件。
		if (!HaveMainData()) {
			missingMpqAssets_ = true;
			return;
		}
		gbIsSpawn = false; // 与 sampling_behavior_test.cpp 一致：仅有 spawn.mpq 时不清空任务

		// 建关过程中可能触发任务专属 set-piece（如 rnd6.dun），这些文件只作为测试夹具存在于
		// test/fixtures/levels/ 下，需在 TestInitGame 之前设置 PrefPath 使其通过覆盖路径解析。
		paths::SetPrefPath(paths::BasePath() + "test/fixtures/");
		TestInitGame();
		LoadMonsterData();
		// GetLevelMTypes() 现在按逐层名册采样（core 预加 + 有界尾池），生产侧在
		// diablo.cpp 里紧跟 LoadMonsterData() 调用；这里必须复刻该顺序，否则
		// GetLevelRoster()/GetLevelRosterParams() 全空 → 无 PLACE_SCATTER 类型 →
		// 该层放不出任何怪（本用例的 placed 基线会全零）。
		LoadLevelRoster();

		// gbIsSpawn=false 让 Q_SKELKING 在单人模式下于 currlevel==3 保持可用
		// （InitQuests 只在 gbIsSpawn 时清空任务），PlaceQuestMonsters() 因此会尝试
		// PlaceUniqueMonst(SkeletonKing, ...) -> InitTRNForUniqueMonster()，加载
		// monsters\monsters\genrl.trn。该文件只存在于零售版(DIABDAT.MPQ)/Hellfire
		// 资产中，spawn.mpq 里没有（已用 smpq -l 核实）。探测这个真实依赖一次，
		// 而不是用 HaveHellfire()：未来只有 DIABDAT.MPQ（零售、非 HF）的环境也应
		// 能跑这些以零售语义测量的基线用例。
		size_t trnSize = 0;
		const AssetHandle trnHandle = OpenAsset(R"(monsters\monsters\genrl.trn)", trnSize);
		missingRetailTrn_ = !trnHandle.ok() || trnSize == 0;
	}

	static bool missingMpqAssets_;
	static bool missingRetailTrn_;
};

bool LevelRosterBaselineTest::missingMpqAssets_ = false;
bool LevelRosterBaselineTest::missingRetailTrn_ = false;

TEST_F(LevelRosterBaselineTest, PlacesMonstersForCathedralL1)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	CreateDungeonForMeasurement(1, 1000);
	{
		const auto getTypesResult = GetLevelMTypes();
		ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
	}
	{
		const auto initResult = InitMonsters();
		ASSERT_TRUE(initResult.has_value()) << initResult.error();
	}

	const auto mix = MeasurePlacedClassMix();

	size_t total = 0;
	for (const size_t count : mix)
		total += count;
	EXPECT_GT(total, 0u) << "level 1 must place at least one monster";
	EXPECT_LE(total, MaxMonsters - 10) << "placed count must respect engine's MaxMonsters-10 cap";
	EXPECT_EQ(total, ActiveMonsterCount);
}

TEST_F(LevelRosterBaselineTest, PlacedClassMixReport)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// 默认只跑小样本并断言真实不变量（F3）；只有显式设置 SAMPLING_REPORT 时才跑
	// 200 seeds x 16 levels 的完整基线报告并写文件。
	const bool fullReport = std::getenv("SAMPLING_REPORT") != nullptr;
	const uint32_t seedsPerLevel = fullReport ? 200 : 5;

	std::string report;
	if (fullReport) {
		report = "\n## A-baseline: placed class mix (200 seeds per level)\n\n"
		         "| level | placed | Melee | RangedTurret | RangedKite | Rally | Charge | Sneak | Summon | Boss |\n"
		         "|---|---|---|---|---|---|---|---|---|---|\n";
	}

	for (uint8_t level = 1; level <= 16; level++) {
		std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> sum {};
		size_t placed = 0;
		for (uint32_t seed = 0; seed < seedsPerLevel; seed++) {
			CreateDungeonForMeasurement(level, 5000 + seed);
			{
				const auto getTypesResult = GetLevelMTypes();
				ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
			}
			{
				const auto initResult = InitMonsters();
				ASSERT_TRUE(initResult.has_value()) << initResult.error();
			}
			const auto mix = MeasurePlacedClassMix();

			// F3: 对每个样本断言真实不变量，而不仅仅是「引擎调用没报错」。
			EXPECT_GT(ActiveMonsterCount, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed << " must place at least one monster";
			EXPECT_LE(ActiveMonsterCount, MaxMonsters - 10)
			    << "level " << static_cast<int>(level) << " seed " << seed << " exceeds engine placement cap";
			size_t mixTotal = 0;
			for (const size_t count : mix)
				mixTotal += count;
			EXPECT_EQ(mixTotal, ActiveMonsterCount) << "class-mix sum must equal placed count";

			for (size_t i = 0; i < mix.size(); i++)
				sum[i] += mix[i];
			placed += ActiveMonsterCount;
		}
		if (fullReport) {
			report += StrCat("| ", level, " | ", placed, " |");
			for (const size_t count : sum)
				report += StrCat(" ", count, " |");
			report += "\n";
		}
	}
	if (fullReport)
		AppendMeasurementReport(report);
}

TEST_F(LevelRosterBaselineTest, PlacedClassMixWithinBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// AC (spec §4.5): the roster must not push a level's ranged share more than 5
	// points above the pre-change baseline. This drives the PRODUCTION fixture
	// (CreateDungeonForMeasurement -> GetLevelMTypes -> InitMonsters) and counts
	// the monsters the engine actually placed, so the measurement comes from real
	// placement rather than a re-simulation of the sampling rules.
	//
	// Range (I4, task 5 review): L1-15, the full roster range - not just L13-15.
	// L16 is excluded because its hardcoded branch returns before the roster path
	// runs (spec 4.2.7), so its mix is a property of that fixed list, not of the
	// roster; its baseline stays the unconstrained sentinel.
	// There is deliberately NO exception list here: every level in the asserted
	// range is held to its real R4 ceiling.
	//
	// History (round 2 of the task-5 fix wave): extending this case from L13-15 to
	// L1-15 exposed four levels whose roster pushed the ranged share past
	// baseline + 5pp - L2 (+3.3pp), L3 (+2.9pp), L4 (+14.7pp), L8 (+16.5pp). Those
	// breaches were introduced by the roster feature itself (the baselines are
	// pre-change measurements), so per R4 the DATA was fixed, never the ceiling:
	// L2/L3 gained cheap Charge/Melee cores, L4 dropped one of its two
	// RangedTurret cores and gained two Melee cores, L8 dropped its RangedTurret
	// core and gained Sneak/Charge/Melee cores, with tail_draw / max_image raised
	// where the core set no longer fit. Post-fix measurements are in the
	// [ MEASURED ] lines below; all four now sit under their ceilings with margin.
	constexpr int kSeeds = 200;
	for (uint8_t level = 1; level <= 15; level++) {
		// A level inside the asserted range must have a real baseline: with the
		// sentinel its ceiling is 1.0 and the EXPECT_LE below can never fail, so
		// the level would look guarded while being unguarded - exactly the I4
		// defect. Fail loudly instead.
		ASSERT_LT(kRangedShareBaseline[level], kRangedShareUnconstrained)
		    << "level " << static_cast<int>(level) << " is asserted but still holds the unconstrained"
		    << " sentinel; fill its A-baseline row before adding it to this loop";

		size_t total = 0;
		size_t ranged = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			CreateDungeonForMeasurement(level, 9000 + seed);
			{
				const auto getTypesResult = GetLevelMTypes();
				ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
			}
			{
				const auto initResult = InitMonsters();
				ASSERT_TRUE(initResult.has_value()) << initResult.error();
			}
			const auto mix = MeasurePlacedClassMix();
			total += ActiveMonsterCount;
			ranged += mix[static_cast<size_t>(BehaviorClass::RangedTurret)]
			    + mix[static_cast<size_t>(BehaviorClass::RangedKite)];
		}
		ASSERT_GT(total, 0u) << "level " << static_cast<int>(level) << " placed nothing - the fixture is broken";
		const double share = static_cast<double>(ranged) / static_cast<double>(total);
		// Always report the measurement: a passing run must still show how much
		// headroom is left, otherwise the next roster edit has no reference point.
		std::cout << "[ MEASURED ] level " << static_cast<int>(level) << " ranged share " << share
		          << " (" << ranged << "/" << total << "), baseline " << kRangedShareBaseline[level]
		          << ", ceiling " << kRangedShareCeiling[level] << std::endl;
		EXPECT_LE(share, kRangedShareCeiling[level])
		    << "level " << static_cast<int>(level) << " ranged share " << share
		    << " exceeds baseline " << kRangedShareBaseline[level] << " + 5pp"
		    << " (" << ranged << "/" << total << ")";
	}
}
