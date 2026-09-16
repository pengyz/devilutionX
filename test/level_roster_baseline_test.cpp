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
#include "engine/random.hpp"
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

// ---------------------------------------------------------------------------
// Task 3 squad measurement helpers (spec 2026-09-15-level-rosters-design 4.3.3).
//
// A "realised squad" is what the level actually got, not what the table asked
// for: a squad roll can still end up with zero minions because PlaceGroup
// clamps `num` against totalmonsters, gives up after 10 placement attempts, and
// (when leashed) additionally requires every minion within 4 tiles of the
// leader. So the acceptance figure the brief asks for is measured from the
// placed monsters, never inferred from squad_chance.
// ---------------------------------------------------------------------------

struct SquadObservation {
	/** Squad minions: leashed minions under an ORDINARY (non-unique) leader. */
	size_t leashedMinions = 0;
	/** Distinct ordinary leaders that ended up with >= 1 leashed minion. */
	size_t leadersWithMinions = 0;
	/** Squad leaders holding a leashed minion while reporting packSize == 0 (must be 0). */
	size_t leadersWithZeroPackSize = 0;
	/** Squad minions whose ai differs from their own type's ai (G2 violation; must be 0). */
	size_t minionsWithOverwrittenAi = 0;
	/** Squad minions further from their leader than the engine leash allows. */
	size_t minionsOutsideLeash = 0;
	/** Squad minions whose leader is NOT a core roster member of this level (must be 0). */
	size_t minionsUnderNonCoreLeader = 0;
	/** Leashed minions under a UNIQUE leader: the pre-existing boss-pack path. */
	size_t uniquePackMinions = 0;
	size_t placed = 0;
};

bool IsCoreMemberOfLevel(uint8_t level, _monster_id type)
{
	const std::span<const LevelRosterEntry> roster = GetLevelRoster(level);
	return std::any_of(roster.begin(), roster.end(), [type](const LevelRosterEntry &e) {
		return e.type == type && e.role == LevelRosterRole::Core;
	});
}

// Squad minions are separated from unique boss-pack minions by their LEADER's
// uniqueness, not by any squad-specific marker.
//
// This is required, not cosmetic: PlaceUniqueMonsters() runs BEFORE the scatter
// loop and already leashes minions to unique leaders with the default
// MinionOptions{} (AI inherited, leader's base type not necessarily a core
// roster member). Counting every leashed minion therefore mixes the two
// populations - the first run of these cases failed exactly that way, reporting
// 8 "non-core leaders" and 50 "squads" on a table with squad_chance 0.
//
// The scatter loop's squad path is the only producer of leashed minions under an
// ORDINARY leader (G1's fix, task 1, changed how such a leader's death is
// handled precisely because nothing else creates one), so leader->isUnique() is
// the exact discriminator. SquadsAreAbsentWhenTheTableDisablesThem pins that
// claim: with squads off, the ordinary-leader count must be 0 while the unique
// count stays non-zero.
SquadObservation ObserveSquads(uint8_t level)
{
	SquadObservation obs;
	obs.placed = ActiveMonsterCount;

	std::array<bool, MaxMonsters> counted {};
	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		const Monster &monster = Monsters[ActiveMonsters[i]];
		if (monster.leaderRelation != LeaderRelation::Leashed)
			continue;
		const Monster *leader = monster.getLeader();
		if (leader == nullptr)
			continue;
		if (leader->isUnique()) {
			obs.uniquePackMinions++;
			continue;
		}
		obs.leashedMinions++;

		if (monster.ai != monster.data().ai)
			obs.minionsWithOverwrittenAi++;
		// PlaceGroup measures its 4-tile leash from the FIRST candidate tile,
		// which is a NEIGHBOUR of the leader (leader->position.tile +
		// Direction(GenerateRnd(8))), not from the leader itself: the check is
		// |xp - x1| < 4 with x1 = leader +/- 1. So a leashed minion may sit up to
		// 4 tiles from the leader, and asserting < 4 here would fail on legitimate
		// placements - the first run of this case failed that way on L9 seed 2.
		const int dx = std::abs(monster.position.tile.x - leader->position.tile.x);
		const int dy = std::abs(monster.position.tile.y - leader->position.tile.y);
		if (dx > 4 || dy > 4)
			obs.minionsOutsideLeash++;
		if (!IsCoreMemberOfLevel(level, leader->type().type))
			obs.minionsUnderNonCoreLeader++;
		if (leader->packSize == 0)
			obs.leadersWithZeroPackSize++;

		const size_t leaderId = leader->getId();
		if (leaderId < counted.size() && !counted[leaderId]) {
			counted[leaderId] = true;
			obs.leadersWithMinions++;
		}
	}
	return obs;
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

// ---------------------------------------------------------------------------
// Task 3: core squads in the scatter loop (spec 4.3.3).
//
// These cases drive the REAL placement chain (CreateDungeonForMeasurement ->
// GetLevelMTypes -> InitMonsters) and read the squads off the placed monsters,
// so they measure what the level actually got rather than re-simulating the
// rules.
//
// Every squad assertion below is an A/B against a same-seed, same-level run of
// the SAME production code with only the params table swapped, never an
// absolute band. That matters here specifically: a squad's realised minion
// count is bounded by totalmonsters clamping, PlaceGroup's 10-attempt give-up
// and the 4-tile leash, so any absolute "at least N squads" figure would be a
// guess about placement luck. The A/B pins the DIFFERENCE the feature makes.
//
// The two override tables are full copies of the shipped params table with only
// the squad columns changed (test/fixtures/txtdata/monsters/
// level_roster_params_squads_{off,always,unleashed}.tsv), so max_image,
// tail_draw and class_floors - everything else that shapes a level - are held
// identical across the A and B runs.
class SquadPlacementTest : public LevelRosterBaselineTest {
protected:
	void SetUp() override
	{
		savedAssetsPath_ = paths::AssetsPath();
	}

	void TearDown() override
	{
		paths::SetAssetsPath(savedAssetsPath_);
		// Leave the process holding the SHIPPED tables: this fixture mutates
		// process-global roster storage, and the other suites in this binary
		// (and any later test in this one) expect the shipped rows.
		if (!missingMpqAssets_)
			LoadLevelRoster();
	}

	// Load one of the squad override tables from test/fixtures/. The roster table
	// itself is never overridden - only the params table's squad columns differ.
	static void LoadSquadParams(std::string_view paramsFile)
	{
		paths::SetAssetsPath(paths::BasePath() + "test/fixtures/");
		LoadLevelRosterFromFiles("txtdata\\monsters\\level_rosters.tsv", paramsFile);
		paths::SetAssetsPath(paths::BasePath() + "assets/");
	}

	static SquadObservation RunLevel(uint8_t level, uint32_t seed)
	{
		CreateDungeonForMeasurement(level, seed);
		const auto getTypesResult = GetLevelMTypes();
		EXPECT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
		const auto initResult = InitMonsters();
		EXPECT_TRUE(initResult.has_value()) << initResult.error();
		return ObserveSquads(level);
	}

private:
	std::string savedAssetsPath_;
};

TEST_F(SquadPlacementTest, SquadFormsAroundACoreLeader)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// squad_chance = 100 makes the branch deterministic-by-table; the realised
	// minion count still depends on placement, which is exactly what is measured.
	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_always.tsv");

	size_t totalLeadersWithMinions = 0;
	size_t totalLeashedMinions = 0;
	for (uint8_t level = 9; level <= 12; level++) {
		for (uint32_t seed = 0; seed < 10; seed++) {
			const SquadObservation obs = RunLevel(level, 21000 + seed);
			totalLeadersWithMinions += obs.leadersWithMinions;
			totalLeashedMinions += obs.leashedMinions;

			// (2)/(3) and the review's checks 1-2, asserted per sample so a
			// violation names the level and seed rather than a global total.
			EXPECT_EQ(obs.leadersWithZeroPackSize, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": a leader holding a leashed minion must report packSize >= 1";
			EXPECT_EQ(obs.minionsWithOverwrittenAi, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": G2 requires squad minions to keep their own AI";
			EXPECT_EQ(obs.minionsOutsideLeash, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": a leashed minion must sit within the engine's 4-tile leash";
			EXPECT_EQ(obs.minionsUnderNonCoreLeader, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": squads may only form around a CORE roster member of that level";
			// totalmonsters is a monster.cpp internal (not exported), so this
			// asserts the engine-wide cap InitMonsters itself clamps
			// totalmonsters to. Placing past totalmonsters would normally
			// also breach this bound, since totalmonsters <= MaxMonsters - 10.
			EXPECT_LE(obs.placed, MaxMonsters - 10)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": the squad path must not place past the engine placement cap";
		}
	}

	// (1) squads must actually happen. This is the only non-A/B floor in the
	// case and it is deliberately the weakest possible one (> 0 over 40 runs at
	// squad_chance 100): the quantitative per-level rate lives in
	// SquadRateIsMeasuredPerLevel, and a stronger absolute floor here would be a
	// bet on placement luck.
	EXPECT_GT(totalLeadersWithMinions, 0u)
	    << "L9-12 at squad_chance 100 produced no leader with a leashed minion";
	EXPECT_GT(totalLeashedMinions, 0u);
}

TEST_F(SquadPlacementTest, SquadsAreAbsentWhenTheTableDisablesThem)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// The A side of the A/B: identical levels and seeds to
	// SquadFormsAroundACoreLeader, only squad_chance = 0. Everything the squad
	// branch produces must vanish here, which is what makes the B side's
	// non-zero count attributable to the FEATURE rather than to unique boss
	// packs - the pre-existing leashed-minion source that runs on these levels
	// either way (PlaceUniqueMonsters, before the scatter loop).
	//
	// The unique-pack count is asserted NON-zero on purpose. Without it, a
	// discriminator bug that classified every leashed minion as "unique" would
	// satisfy the squad assertion below while measuring nothing at all, and the
	// whole A/B would silently become vacuous.
	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_off.tsv");

	size_t leadersWithMinions = 0;
	size_t uniquePackMinions = 0;
	for (uint8_t level = 9; level <= 12; level++) {
		for (uint32_t seed = 0; seed < 10; seed++) {
			const SquadObservation obs = RunLevel(level, 21000 + seed);
			leadersWithMinions += obs.leadersWithMinions;
			uniquePackMinions += obs.uniquePackMinions;
			EXPECT_EQ(obs.leadersWithZeroPackSize, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
		}
	}
	EXPECT_EQ(leadersWithMinions, 0u)
	    << "squad_chance 0 must not produce any leashed squad under an ordinary leader on L9-12";
	EXPECT_GT(uniquePackMinions, 0u)
	    << "these levels place unique boss packs either way; a zero here means the"
	    << " squad/unique discriminator is misclassifying, making the assertion above vacuous";
}

TEST_F(SquadPlacementTest, UnleashedFallbackPlacesNeighboursWithoutLeashing)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// Spec 4.3.4 fallback: squad_leashed = 0 keeps passing the leader to
	// PlaceGroup (so minions are still seeded from the leader's neighbourhood)
	// but applies no leash, no setLeader and no packSize. The observable
	// consequence is that this configuration produces NO leashed minion at all,
	// while the leashed configuration (same seeds, same levels) does - a
	// same-seed A/B on the one column that differs.
	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_unleashed.tsv");
	size_t unleashedLeashedMinions = 0;
	for (uint8_t level = 9; level <= 12; level++) {
		for (uint32_t seed = 0; seed < 10; seed++)
			unleashedLeashedMinions += RunLevel(level, 21000 + seed).leashedMinions;
	}

	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_always.tsv");
	size_t leashedLeashedMinions = 0;
	for (uint8_t level = 9; level <= 12; level++) {
		for (uint32_t seed = 0; seed < 10; seed++)
			leashedLeashedMinions += RunLevel(level, 21000 + seed).leashedMinions;
	}

	EXPECT_EQ(unleashedLeashedMinions, 0u)
	    << "squad_leashed 0 must not leash minions (no setLeader / packSize / regroup)";
	EXPECT_GT(leashedLeashedMinions, 0u)
	    << "the leashed reference run produced nothing, so the comparison above proves nothing";
}

TEST_F(SquadPlacementTest, SquadRateIsMeasuredPerLevel)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// The brief's acceptance figure: for each level, the share of squad ROLLS
	// that produced a real squad (>= 1 leashed minion). Measured at
	// squad_chance = 100 so every core draw rolls a squad and the denominator is
	// the number of rolls the loop actually made - the loss is then attributable
	// purely to placement (totalmonsters clamp, 10-attempt give-up, 4-tile
	// leash), which is what the brief asks to report.
	//
	// GetSquadRollStats() exposes the loop's own counters, so the denominator is
	// the production code's, not a re-derivation. The counters are PER LEVEL -
	// InitLevelMonsters() (which CreateDungeonForMeasurement calls) resets them -
	// so each run's totals are read straight after that run. Taking a
	// before/after difference would underflow across the reset, which is how the
	// first version of this measurement produced rolls = 2^64-1.
	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_always.tsv");

	constexpr uint32_t kSeeds = 50;
	std::string report = "\n## Task 3: realised squad rate per level (squad_chance = 100, 50 seeds)\n\n"
	                     "| level | squad rolls | realised squads | rate | leashed minions | minions/squad"
	                     " | lost: leader / no partner |\n"
	                     "|---|---|---|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 15; level++) {
		size_t eligible = 0;
		size_t rolls = 0;
		size_t realised = 0;
		size_t minions = 0;
		size_t leaderFailures = 0;
		size_t noPartner = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			const SquadObservation obs = RunLevel(level, 31000 + seed);
			const SquadRollCounters &counters = GetSquadRollStats();
			eligible += counters.eligibleCoreDraws;
			rolls += counters.rolls;
			leaderFailures += counters.leaderPlacementFailed;
			noPartner += counters.noPartnerAvailable;
			realised += obs.leadersWithMinions;
			minions += obs.leashedMinions;
		}
		const double rate = rolls == 0 ? 0.0 : static_cast<double>(realised) / static_cast<double>(rolls);
		const double perSquad = realised == 0 ? 0.0 : static_cast<double>(minions) / static_cast<double>(realised);
		std::cout << "[ SQUADRATE ] level " << static_cast<int>(level) << " rolls " << rolls
		          << " realised " << realised << " rate " << rate << " minions " << minions
		          << " per-squad " << perSquad << " lost(leader/partner) "
		          << leaderFailures << "/" << noPartner << std::endl;
		report += StrCat("| ", level, " | ", rolls, " | ", realised, " | ");
		report += StrCat(static_cast<int>(rate * 1000.0 + 0.5), "/1000 | ", minions, " | ");
		report += StrCat(static_cast<int>(perSquad * 100.0 + 0.5), "/100 | ");
		report += StrCat(leaderFailures, " / ", noPartner, " |\n");

		// The realised count can never exceed the number of rolls that entered the
		// branch: a roll places at most one leader. This is the invariant that
		// caught the underflowing before/after accounting the first run used.
		EXPECT_LE(realised, rolls)
		    << "level " << static_cast<int>(level) << " reports more realised squads than rolls,"
		    << " so the roll denominator is not the loop's";

		// Every level must actually roll squads: a level whose core is never
		// drawn by the scatter loop would silently report rate 0 and look
		// "measured" while being unreachable.
		EXPECT_GT(rolls, 0u)
		    << "level " << static_cast<int>(level) << " never entered the squad branch at squad_chance 100";
		// At squad_chance 100 every eligible core draw must roll: a gap here would
		// mean the branch is gated on something beyond the documented conditions.
		EXPECT_EQ(rolls, eligible)
		    << "level " << static_cast<int>(level) << " skipped eligible core draws at squad_chance 100";
	}
	AppendMeasurementReport(report);
}

TEST_F(SquadPlacementTest, ShippedSquadChanceRealisesSquadsOnEveryLevel)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// The figure the task brief asks to REPORT: with the table the game actually
	// ships (squad_chance 30), what share of squad-eligible core draws ends up as
	// a real squad on each level? Two rates are reported because they answer
	// different questions and only the second is a property of the code:
	//
	//   roll rate      = rolls / eligible core draws  -> tracks squad_chance
	//   realisation    = realised / rolls             -> tracks placement loss
	//                                                    (totalmonsters clamp,
	//                                                    PlaceGroup's 10 attempts,
	//                                                    the 4-tile leash)
	//
	// No absolute band is asserted on either: both are placement- and
	// roster-dependent, and pinning a literal here would turn a table tweak
	// (task 4's job) into a test failure. What IS asserted is the structural
	// part - every level realises squads, no level loses ALL of its rolls, and
	// realised never exceeds rolls.
	LoadLevelRoster(); // shipped tables (squad_chance 30)

	constexpr uint32_t kSeeds = 50;
	std::string report = "\n## Task 3: realised squad rate per level (SHIPPED table, squad_chance 30, 50 seeds)\n\n"
	                     "| level | eligible core draws | rolls | roll rate | realised | realisation rate"
	                     " | leashed minions | lost: leader / no partner |\n"
	                     "|---|---|---|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 15; level++) {
		size_t eligible = 0;
		size_t rolls = 0;
		size_t realised = 0;
		size_t minions = 0;
		size_t leaderFailures = 0;
		size_t noPartner = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			const SquadObservation obs = RunLevel(level, 41000 + seed);
			const SquadRollCounters &counters = GetSquadRollStats();
			eligible += counters.eligibleCoreDraws;
			rolls += counters.rolls;
			leaderFailures += counters.leaderPlacementFailed;
			noPartner += counters.noPartnerAvailable;
			realised += obs.leadersWithMinions;
			minions += obs.leashedMinions;

			EXPECT_EQ(obs.leadersWithZeroPackSize, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
			EXPECT_EQ(obs.minionsWithOverwrittenAi, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
			EXPECT_EQ(obs.minionsUnderNonCoreLeader, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
		}
		const double rollRate = eligible == 0 ? 0.0 : static_cast<double>(rolls) / static_cast<double>(eligible);
		const double realisation = rolls == 0 ? 0.0 : static_cast<double>(realised) / static_cast<double>(rolls);
		std::cout << "[ SHIPPEDSQUAD ] level " << static_cast<int>(level) << " eligible " << eligible
		          << " rolls " << rolls << " rollRate " << rollRate << " realised " << realised
		          << " realisation " << realisation << " minions " << minions
		          << " lost(leader/partner) " << leaderFailures << "/" << noPartner << std::endl;
		report += StrCat("| ", level, " | ", eligible, " | ", rolls, " | ");
		report += StrCat(static_cast<int>(rollRate * 1000.0 + 0.5), "/1000 | ", realised, " | ");
		report += StrCat(static_cast<int>(realisation * 1000.0 + 0.5), "/1000 | ", minions, " | ");
		report += StrCat(leaderFailures, " / ", noPartner, " |\n");

		// Tripwire for the squad path's defensive leader-placement fallback in
		// monster.cpp, which is unreachable today (PlaceGroup always places a
		// leaderless group of 1 on its first candidate tile, and squadEligible
		// guarantees room). If that ever stops holding, this says so rather than
		// letting the fallback become live and untested.
		EXPECT_EQ(leaderFailures, 0u)
		    << "level " << static_cast<int>(level) << ": squad leader placement started failing,"
		    << " so monster.cpp's defensive fallback is now a live path and needs its own coverage";
		EXPECT_GT(eligible, 0u)
		    << "level " << static_cast<int>(level) << " never draws a core type in the scatter loop,"
		    << " so its squad columns can never do anything";
		EXPECT_GT(realised, 0u)
		    << "level " << static_cast<int>(level) << " realised no squad at all over " << kSeeds
		    << " seeds despite " << rolls << " rolls";
		EXPECT_LE(realised, rolls)
		    << "level " << static_cast<int>(level) << " reports more realised squads than rolls";
	}
	AppendMeasurementReport(report);
}
