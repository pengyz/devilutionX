#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "drlg_test.hpp" // TestInitGame / GetTileCount（本仓既有测试夹具）
#include "engine/assets.hpp"
#include "engine/load_file.hpp"
#include "engine/random.hpp"
#include "game_mode.hpp" // gbIsHellfire / gbIsSpawn（A2 的 HF 门控套件用）
#include "levels/gendung.h"
#include "levels/trigs.h" // InitL1Triggers 等 + Freeupstairs（CreateLevel 逻辑复刻用）
#include "monster.h"
#ifndef UNPACKED_MPQS
#include "mpq/mpq_reader.hpp" // hellfire.mpq 直接挂载（见 HF 套件注释；仅打包 MPQ 路径需要）
#endif
#include "multi.h"
#include "player.h"
#include "quests.h"
#include "tables/level_roster.h"
#include "tables/monstdat.h"
#include "utils/file_util.h"
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
	// A2: L17-20 (Nest) / L21-24 (Crypt) live under nlevels\ and only exist in the
	// Hellfire assets. Paths mirror diablo.cpp:LoadLvlGFX (:1429-1438), same as the
	// four Diablo cases above.
	case DTYPE_NEST:
		til = "nlevels\\l6data\\l6.til";
		break;
	case DTYPE_CRYPT:
		til = "nlevels\\l5data\\l5.til";
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
	// A2: same dispatch CreateLevel uses for the Hellfire level types
	// (diablo.cpp:1481-1486).
	case DTYPE_NEST:
		InitHiveTriggers();
		break;
	case DTYPE_CRYPT:
		InitCryptTriggers();
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
// Indexing: both tables are indexed BY LEVEL NUMBER and sized 25 so level 24 is
// a valid index rather than a buffer overrun. L16 stays UNCONSTRAINED, spelled as
// the sentinel 1.0 (a share can never exceed 1.0) rather than 0.0. 0.0 would read
// as "ceiling zero" and turn a missing baseline into a silent misjudgement - a
// guaranteed failure, or worse, a pass that means nothing. The sentinel is kept
// (not deleted now that L1-15 are filled) precisely so a future level added
// without a baseline is unconstrained-by-declaration rather than accidentally
// judged against zero. Adding such a level to the assertion loop REQUIRES filling
// its baseline here first.
//
// A2 (RA3): the tables were sized 17 while L17-24 ran the R28 legacy fallback. Task
// 2 gives those levels real roster/params rows, so they are now filled from the
// PRE-CHANGE measurement A2 task 1 took with the same fixture, seed count and
// numerator/denominator convention ([ A2BASELINE ] output, 200 seeds per level,
// seed base 61000, placed-monster denominator, ranged = RangedTurret + RangedKite):
//
//   L17 0/16188      L18 0/16188      L19 1983/16388   L20 3054/16342
//   L21 0/22619      L22 0/22712      L23 3337/22648   L24 0/22653
//
// Five of those baselines are exactly 0, so their ceiling is a hard 0.05: on those
// levels the data may add AT MOST a 5%-of-placed ranged presence measured under this
// taxonomy. That is a real constraint on L24 in particular (see the report's budget
// note), NOT a formality - but it is a WEAK one across the band, because
// GetBehaviorClass() sends most HF ranged casters to its Boss arm (see the caveat on
// HellfireLevelBaselineTest below, spec appendix E items 8/9). RB37 therefore adds a
// SECOND, non-share guard for L17-24: HellfireRangedCasterFloor further down, which
// counts monsters by their REAL ai instead of by this taxonomy.
constexpr double kRangedShareTolerance = 0.05;
constexpr double kRangedShareUnconstrained = 1.0;

constexpr std::array<double, 25> kRangedShareBaseline {
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
	// A2 task 1's [ A2BASELINE ] measurement, same auditable numerator/denominator
	// form as L1-15 above. These are Hellfire-only levels: the ceiling below is only
	// ever asserted by the Hellfire suite, which skips without Hellfire assets.
	(0.0 + 0.0) / 16188.0,    // L17
	(0.0 + 0.0) / 16188.0,    // L18
	(0.0 + 1983.0) / 16388.0, // L19
	(0.0 + 3054.0) / 16342.0, // L20
	(0.0 + 0.0) / 22619.0,    // L21
	(0.0 + 0.0) / 22712.0,    // L22
	(0.0 + 3337.0) / 22648.0, // L23
	(0.0 + 0.0) / 22653.0,    // L24
};

// A ceiling of exactly kRangedShareUnconstrained stays unconstrained: adding the
// tolerance to the sentinel would push it above 1.0 and obscure that reading.
constexpr std::array<double, 25> kRangedShareCeiling {
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
	kRangedShareBaseline[17] + kRangedShareTolerance,
	kRangedShareBaseline[18] + kRangedShareTolerance,
	kRangedShareBaseline[19] + kRangedShareTolerance,
	kRangedShareBaseline[20] + kRangedShareTolerance,
	kRangedShareBaseline[21] + kRangedShareTolerance,
	kRangedShareBaseline[22] + kRangedShareTolerance,
	kRangedShareBaseline[23] + kRangedShareTolerance,
	kRangedShareBaseline[24] + kRangedShareTolerance,
};

// Same-seed vanilla ranged share: measured 2026-09-18 by VanillaRangedShareBaseline with the
// out-of-range roster, 200 seeds per level and seed base 9000 - i.e. the EXACT fixture, seed
// count and seed base PlacedClassMixWithinBaseline uses, which makes the two a same-seed A/B.
// It is NOT kRangedShareBaseline: that array holds the historical pre-change measurement taken
// with a different fixture and seed base (and, on L13-15, under A2 task 1's conventions), and
// the two disagree by up to 14.6 points on a level (L15 0.4371 here vs 0.5835 there; L14 9.4,
// L10 4.8, L9 3.9 points apart). The primary criterion must therefore use THIS array, while
// kRangedShareCeiling stays the alarm line so the historical contract keeps its meaning.
// L17-24 reuse kRangedShareBaseline by construction: no roster existed on those levels before
// A2, so their pre-A2 measurement IS their vanilla value.
constexpr std::array<double, 25> kVanillaShareBaseline {
	kRangedShareUnconstrained,   // L0 (unused)
	0.0 / 18585.0,               // L1  (no ranged type in the L1 pool at all)
	2174.0 / 23724.0,            // L2
	2862.0 / 25748.0,            // L3
	4730.0 / 25313.0,            // L4
	3460.0 / 23459.0,            // L5
	4689.0 / 17651.0,            // L6
	4973.0 / 18026.0,            // L7
	4508.0 / 17226.0,            // L8
	8135.0 / 16627.0,            // L9
	7801.0 / 17341.0,            // L10
	7706.0 / 16549.0,            // L11
	8056.0 / 17256.0,            // L12
	5560.0 / 23450.0,            // L13
	10979.0 / 23601.0,           // L14
	10164.0 / 23253.0,           // L15
	kRangedShareUnconstrained,   // L16 (registration-only; never asserted)
	kRangedShareBaseline[17],    // L17 (pre-A2 measurement == vanilla here)
	kRangedShareBaseline[18],    // L18
	kRangedShareBaseline[19],    // L19
	kRangedShareBaseline[20],    // L20
	kRangedShareBaseline[21],    // L21
	kRangedShareBaseline[22],    // L22
	kRangedShareBaseline[23],    // L23
	kRangedShareBaseline[24],    // L24
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

/**
 * @brief True when `ai` is dispatched to a ranged attack routine by the engine.
 *
 * Derived from AiProc (Source/monster.cpp:3138-3180), the array the engine indexes by
 * monster.ai every tick: these are exactly the entries wired to AiRanged or
 * AiRangedAvoidance, i.e. the AIs whose whole behaviour is "keep distance and throw a
 * missile from GetMissileType()". Nothing here is a judgement call about flavour - a
 * monster is counted iff the engine runs a ranged routine for it.
 *
 * Deliberately independent of GetBehaviorClass() (RB33: that taxonomy stays
 * like-for-like so the L1-16 baselines keep their measurement premise). The overlap is
 * expected and harmless: Magma/Storm/Acid/BoneDemon are RangedKite there and ranged
 * here, Succubus/GoatRanged/Counselor are RangedTurret there and ranged here.
 *
 * Excluded on purpose: SkeletonRanged (SkeletonBowAi) and the unique-only AIs
 * (LazarusSuccubus, HorkDemon) fire missiles from their own routines rather than
 * through AiProc's ranged entries. They are absent from L17-24's candidate pool
 * anyway - HorkDemon and Defiler reach these levels only as PLACE_UNIQUE pre-adds -
 * so widening the predicate would only add noise this guard cannot act on.
 */
bool IsRangedCasterAi(MonsterAIID ai)
{
	switch (ai) {
	// Diablo AIs on AiProc's ranged entries, listed for completeness: none of them
	// appears in L17-24's pool, but the predicate must not silently depend on that.
	case MonsterAIID::GoatRanged:
	case MonsterAIID::Succubus:
	case MonsterAIID::Counselor:
	case MonsterAIID::Magma:
	case MonsterAIID::Storm:
	case MonsterAIID::Acid:
	case MonsterAIID::AcidUnique:
	case MonsterAIID::Diablo:
	// Hellfire AIs - the ones acceptance 8 cannot see, since GetBehaviorClass() sends
	// all but BoneDemon to its Boss arm.
	case MonsterAIID::FireBat:
	case MonsterAIID::Torchant:
	case MonsterAIID::Lich:
	case MonsterAIID::ArchLich:
	case MonsterAIID::Psychorb:
	case MonsterAIID::Necromorb:
	case MonsterAIID::BoneDemon:
		return true;
	default:
		return false;
	}
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

// The largest per-axis leader-to-minion distance PlaceGroup's leash can produce.
//
// DERIVED, not measured: PlaceGroup's accepted-candidate test is
// `!(leashed && (|xp - x1| >= 4 || |yp - y1| >= 4))`, so an accepted tile is within 3 of the
// FIRST candidate (x1, y1) on each axis; that first candidate is
// `leader->position.tile + Direction(GenerateRnd(8))`, one step from the leader on each axis
// (Displacement::fromDirection has |dx|, |dy| <= 1). 3 + 1 = 4.
constexpr int kLeashedAxisBound = 4;

struct SquadObservation {
	/** Squad minions: leashed minions under an ORDINARY (non-unique) leader. */
	size_t leashedMinions = 0;
	/** Distinct ordinary leaders that ended up with >= 1 leashed minion. */
	size_t leadersWithMinions = 0;
	/** Squad leaders holding a leashed minion while reporting packSize == 0 (must be 0). */
	size_t leadersWithZeroPackSize = 0;
	/** Squad minions whose ai differs from their own type's ai (G2 violation; must be 0). */
	size_t minionsWithOverwrittenAi = 0;
	/** Squad minions further from their leader than the engine leash allows (must be 0). */
	size_t minionsOutsideLeash = 0;
	/**
	 * Squad minions sitting exactly ON the leash bound (Chebyshev 4 from the leader).
	 *
	 * This is what makes minionsOutsideLeash a real guard rather than a number nobody can
	 * fail (fix review I1): the bound is only meaningful if the data reaches it. Asserted
	 * NON-zero, so the "no minion past 4" check is known to sit on the clamp's boundary.
	 */
	size_t minionsAtLeashBound = 0;
	/** Squad minions whose leader is NOT a core roster member of this level (must be 0). */
	size_t minionsUnderNonCoreLeader = 0;
	/**
	 * Squad minions whose own type EQUALS their leader's type (must be 0).
	 *
	 * Spec 4.3.3 requires a squad to mix two DIFFERENT core types: a same-type squad is just the
	 * legacy same-type group with leash semantics bolted on, i.e. exactly the degenerate outcome
	 * the feature exists to avoid. PickCorePartnerTypeIndex enforces it with a single
	 * `entry.type == leaderType` skip, and review measured that deleting that skip left all eight
	 * squad cases green - so the rule is recorded per minion here and asserted below.
	 */
	size_t minionsSharingLeaderType = 0;
	/** Leashed minions under a UNIQUE leader: the pre-existing boss-pack path. */
	size_t uniquePackMinions = 0;
	size_t placed = 0;
	/** Squad minions the loop placed for a leader this level, leashed or not (sample size). */
	size_t partnersPlaced = 0;
	/** Largest leader-to-minion Chebyshev distance the loop recorded this level. */
	size_t maxPartnerLeaderDistance = 0;
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
	// The leader<->minion pairing of an UNLEASHED squad is not recoverable from the placed
	// monsters (spec 4.3.4 applies no setLeader, so those minions keep leaderRelation None and
	// Monster::NoLeader), so the adjacency figures come from the loop's own per-level counters
	// instead of being re-derived here. Read straight after the run, like the roll counters -
	// InitLevelMonsters() resets them per level.
	obs.partnersPlaced = GetSquadRollStats().partnersPlaced;
	obs.maxPartnerLeaderDistance = GetSquadRollStats().maxPartnerLeaderDistance;

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
		// The leash bound, derived - and TIGHT (fix review I1).
		//
		// PlaceGroup measures its 4-tile leash from the FIRST candidate tile, which is a
		// NEIGHBOUR of the leader (leader->position.tile + Direction(GenerateRnd(8))), not
		// from the leader itself: the accepted-candidate check is |xp - x1| < 4, i.e.
		// <= 3, with x1 = leader +/- 1 (Displacement::fromDirection has |dx|,|dy| <= 1).
		// So the true per-axis ceiling relative to the LEADER is 3 + 1 = 4, and asserting
		// < 4 here would fail on legitimate placements (the first run of this case failed
		// that way on L9 seed 2).
		//
		// What review I1 objected to is that "dx > 4 must be 0" alone cannot be READ as a
		// guard: nothing in the case showed that 4 is the real ceiling rather than a number
		// far above the data, so the reader could not tell which change would turn it red.
		// The bound is now named and, crucially, its TIGHTNESS is measured: 4 is actually
		// REACHED by real placements (minionsAtLeashBound, asserted non-zero in
		// SquadFormsAroundACoreLeader). That pins the metric to the boundary the clamp
		// produces, so widening the clamp (or dropping the leash term from it) immediately
		// pushes minions to 5+ and this counter goes non-zero.
		const int dx = std::abs(monster.position.tile.x - leader->position.tile.x);
		const int dy = std::abs(monster.position.tile.y - leader->position.tile.y);
		if (dx > kLeashedAxisBound || dy > kLeashedAxisBound)
			obs.minionsOutsideLeash++;
		if (dx == kLeashedAxisBound || dy == kLeashedAxisBound)
			obs.minionsAtLeashBound++;
		if (!IsCoreMemberOfLevel(level, leader->type().type))
			obs.minionsUnderNonCoreLeader++;
		if (monster.type().type == leader->type().type)
			obs.minionsSharingLeaderType++;
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
	//
	// History (task D, L14): L14 wanted the standard squad_chance of 30 but measured
	// 0.615081 with its original 3-core set (1 Melee + 1 RangedTurret + 1 RangedKite),
	// breaching the 0.60876 ceiling - squad partners are drawn from the OTHER cores,
	// so 2 of the 3 cores being ranged biased every squad ranged. Per R4 the DATA
	// changed again: L14's RangedKite core (MT_XACID) was dropped, leaving a
	// melee-leaning 1 Melee + 1 RangedTurret core, which measures 0.543672 at
	// squad_chance 30 (6.5pp of headroom, versus 0.36pp at the old chance of 10).
	// The kite core is the ONLY one that could go: L14's six unique bases are 4 Melee
	// + 2 RangedTurret, and the B1 cap is 2 per class, so a second core in either of
	// those classes shuts that class's tail draw and costs AC9c reachability - which
	// is exactly what raising L14's Melee floor to 2 did (6/6 -> 2/6, reverted).
	// RangedKite carries no L14 unique base, so dropping it costs no reachability;
	// HellUniqueBasesRemainReachable still measures L14 at 6/6.
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
		          << " (" << ranged << "/" << total << "), vanilla " << kVanillaShareBaseline[level]
		          << ", baseline " << kRangedShareBaseline[level]
		          << ", ceiling " << kRangedShareCeiling[level] << std::endl;
		// Primary criterion (contract §4.4, R4 inverted): the roster must not push a level's
		// ranged share above VANILLA, where vanilla is the same-seed legacy measurement in
		// kVanillaShareBaseline - see its comment: the historical kRangedShareBaseline array is
		// NOT that value (they differ by up to 14.6 points on a level). kRangedShareCeiling
		// stays as the ALARM line below: breaching it still fails, but it no longer authorises
		// trimming content to pass, because the density floors
		// (ContentDensityWithinVanillaFloor, HellfirePerSeedVariety*) are the first criterion.
		EXPECT_LE(share, kVanillaShareBaseline[level])
		    << "level " << static_cast<int>(level) << " ranged share is above vanilla";
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
// Vanilla (pre-roster) ranged share, for the density contract's A4 primary criterion. The
// fixture loads the out-of-range roster exactly like VanillaRosterFloorsTest does (L1-15 get
// no core rows and no params rows, so GetLevelMTypes() takes the R28 legacy path) and
// restores the shipped tables in TearDown. Same fixture, seed count and seed base as
// PlacedClassMixWithinBaseline, so the two runs are a same-seed A/B.
class VanillaRangedShareTest : public LevelRosterBaselineTest {
protected:
	void SetUp() override
	{
		if (missingMpqAssets_)
			return;
		savedAssetsPath_ = paths::AssetsPath();
		paths::SetAssetsPath(paths::BasePath() + "test/fixtures/");
		LoadLevelRosterFromFiles("txtdata\\monsters\\level_rosters_out_of_range.tsv",
		    "txtdata\\monsters\\level_roster_params_out_of_range.tsv");
		paths::SetAssetsPath(savedAssetsPath_);
	}

	void TearDown() override
	{
		paths::SetAssetsPath(savedAssetsPath_);
		if (!missingMpqAssets_)
			LoadLevelRoster();
	}

private:
	std::string savedAssetsPath_;
};

TEST_F(VanillaRangedShareTest, VanillaRangedShareBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	constexpr int kSeeds = 200;
	std::cout << "\n[ VANILLASHARE ] level share numerator denominator" << std::endl;
	for (uint8_t level = 1; level <= 15; level++) {
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
		ASSERT_GT(total, 0u) << "level " << static_cast<int>(level) << " placed nothing";
		std::cout << "[ VANILLASHARE ] " << static_cast<int>(level) << ' '
		          << static_cast<double>(ranged) / static_cast<double>(total) << ' '
		          << ranged << ' ' << total << std::endl;
	}
}

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
	size_t totalMinionsAtLeashBound = 0;
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
			// G2's own named case is SquadMinionsKeepOwnAi below (spec 6, acceptance
			// row 6); kept here as well so a failure of this case reports it too.
			EXPECT_EQ(obs.minionsWithOverwrittenAi, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": G2 requires squad minions to keep their own AI";
			EXPECT_EQ(obs.minionsSharingLeaderType, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": spec 4.3.3 requires a squad to mix two DIFFERENT core types";
			EXPECT_EQ(obs.minionsOutsideLeash, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": a leashed minion must sit within kLeashedAxisBound (" << kLeashedAxisBound
			    << ") tiles of its leader on each axis";
			totalMinionsAtLeashBound += obs.minionsAtLeashBound;
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
	// Also the sample size behind the per-sample "count of violations == 0" assertions above
	// (own AI, distinct partner type, leash, core leadership): all of them are vacuous on a run
	// that placed no squad minion.
	EXPECT_GT(totalLeashedMinions, 0u)
	    << "no squad minion was placed at all, so the per-sample violation counts above"
	    << " measured nothing";

	// Fix review I1: the leash bound must be TIGHT, i.e. actually reached by real
	// placements. Without this the "nothing beyond kLeashedAxisBound" assertions above
	// could be satisfied by a bound far above the data, and no code change could turn
	// them red. Printed unconditionally so the margin stays readable.
	std::cout << "[ SQUADLEASH ] minions " << totalLeashedMinions
	          << " at bound " << totalMinionsAtLeashBound
	          << " beyond bound 0 (bound " << kLeashedAxisBound << ")" << std::endl;
	EXPECT_GT(totalMinionsAtLeashBound, 0u)
	    << "no squad minion reached Chebyshev " << kLeashedAxisBound << " from its leader over"
	    << " L9-12 x 10 seeds, so kLeashedAxisBound is above what the clamp produces and the"
	    << " minionsOutsideLeash assertions cannot fail on any placement";

	// Fix review I2: squad_size had NO guard - replacing PlaceGroup's `num` argument in the
	// squad branch with a constant 1 left all ten squad cases green, because `formed` only
	// needs >= 1 minion, `realised` only needs packSize > 0, and the floor / adjacency /
	// distinct-type / AI checks are all per minion rather than about how MANY there are.
	//
	// The guard is deliberately WEAK but falsifiable: minions per realised squad must stay
	// >= 1.5. Measured on the shipped table through this very loop (L9-12 x 10 seeds,
	// squads_always fixture): 1.98 minions per squad at squad_size = 2 - i.e. nearly every
	// squad places both its minions, which matches SquadFormationRate's ~99.9% formation
	// rate. Forcing the second argument to 1 caps the ratio at exactly 1.0, so it fails by a
	// wide margin. 1.5 sits in the empty gap between those two values and does NOT pin the
	// data: a level legitimately moving to squad_size 2 with worse placement luck, or the
	// mean drifting to 1.6, still passes. It is a separator between "squad_size is honoured"
	// and "squad_size is ignored", not a measured bound.
	constexpr double kMinMinionsPerSquad = 1.5;
	const double minionsPerSquad = totalLeadersWithMinions == 0
	    ? 0.0
	    : static_cast<double>(totalLeashedMinions) / static_cast<double>(totalLeadersWithMinions);
	std::cout << "[ SQUADSIZE ] leaders " << totalLeadersWithMinions
	          << " minions " << totalLeashedMinions
	          << " per-squad " << minionsPerSquad
	          << " floor " << kMinMinionsPerSquad << std::endl;
	EXPECT_GE(minionsPerSquad, kMinMinionsPerSquad)
	    << "squads averaged " << minionsPerSquad << " minions over L9-12 x 10 seeds, under the"
	    << " floor " << kMinMinionsPerSquad << ": at squad_size 2 the measured mean is 1.98, and"
	    << " a squad path that ignores squad_size (placing one minion per squad) reads exactly"
	    << " 1.0. Check that the scatter loop still passes rosterParams->squadSize to PlaceGroup";
}

// Spec 6, acceptance row 6 names this case: G2 requires a squad minion to keep its OWN AI, so
// PlaceGroup must restore the `ai` that setLeader() overwrites (MinionOptions::inheritAi false).
// It lives as its own case rather than an inline assertion inside
// SquadFormsAroundACoreLeader because the acceptance row points at a case name, and because a
// failure of the AI rule should be reported on its own rather than mixed into a case that also
// checks packSize, leashing, core leadership and the placement cap.
//
// The sample size is asserted too: this is a "count of violations == 0" shape, which a run that
// placed no squad minion at all would satisfy while measuring nothing.
TEST_F(SquadPlacementTest, SquadMinionsKeepOwnAi)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_always.tsv");

	size_t minions = 0;
	size_t overwritten = 0;
	for (uint8_t level = 9; level <= 12; level++) {
		for (uint32_t seed = 0; seed < 10; seed++) {
			const SquadObservation obs = RunLevel(level, 21000 + seed);
			minions += obs.leashedMinions;
			overwritten += obs.minionsWithOverwrittenAi;
			EXPECT_EQ(obs.minionsWithOverwrittenAi, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": G2 requires a squad minion to keep its own type's AI, but setLeader's"
			    << " leader AI survived on " << obs.minionsWithOverwrittenAi << " minion(s)";
		}
	}
	EXPECT_EQ(overwritten, 0u);
	EXPECT_GT(minions, 0u)
	    << "no squad minion was placed over L9-12 x 10 seeds, so the AI assertions above"
	    << " measured nothing";
}

TEST_F(SquadPlacementTest, SquadsAreAbsentWhenTheTableDisablesThem)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// The A side of the A/B: identical levels and seeds to SquadFormsAroundACoreLeader, and
	// the fixture differs from the shipped table in exactly ONE column - squad_chance, set to
	// 0 on every level (the shipped value is 30 on every level).
	//
	// Fix review M1: the fixture used to zero squad_size as well, so the claim "A and B differ
	// in one variable" - the premise every squad case argues from - was not true of the file
	// it pointed at. squad_size is now held at the shipped 2. That is legal input (the loader
	// only rejects the inverse, `squad_chance > 0 && squad_size == 0`, see
	// Source/tables/level_roster.cpp) and it is also strictly better for this case: with
	// squad_size still 2, a zero squad count here can only be attributed to squad_chance,
	// whereas zeroing both left open which of the two suppressed the squads.
	//
	// Everything the squad branch produces must vanish here, which is what makes the B side's
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
	// but applies no leash, no setLeader and no packSize. BOTH halves of that are
	// asserted, because only the first is a property of the fallback's own code:
	//
	//   (a) no leashed minion at all - a same-seed A/B on the one column that differs;
	//   (b) the minions are still ADJACENT to their leader, i.e. the leader really is
	//       still passed to PlaceGroup.
	//
	// (b) is the fallback's core guarantee and used to be unguarded: review changed
	// the `&leader` argument to nullptr and every case stayed green, because an
	// unleashed minion carries leaderRelation None and no back-pointer, so this
	// suite could not even see it. That is why the pairing is measured inside the
	// loop (SquadRollCounters::maxPartnerLeaderDistance) and read back here.
	//
	// Deriving the bound. PlaceGroup with a leader and NO leash gives:
	//   * the anchor is leader->position.tile + Direction(GenerateRnd(8)), so the first
	//     candidate tile is always a NEIGHBOUR of the leader - Chebyshev 1, since
	//     Displacement::fromDirection has |dx| <= 1 and |dy| <= 1;
	//   * each rejected candidate walks one Displacement(Direction(...)) step, moving x and y
	//     by at most 1 each (the `yp += ...deltaX` bug in that loop does not widen this -
	//     deltaX is also in [-1, 1]);
	//   * the inner loop abandons a try after try2 == 100 rejections.
	// So the analytic ceiling is 1 + 100 = 101, and that number is USELESS as a guard: the
	// scatter area is [16, 96), so any two tiles on the map are within 80 of each other and 101
	// can never fail - not even for a group placed with no leader at all, which is precisely the
	// mutation this case has to catch. The bound must therefore sit below what the leaderless
	// anchor (GenerateRnd(80) + 16, uniform over the whole map) produces.
	//
	// Both sides were measured (see the fix report):
	//   * as shipped, over L1-15 x 40 seeds / 25204 placed minions: max 17;
	//   * with `&leader` replaced by nullptr, same L9-12 x 10 seeds as below: 66.
	// 32 is picked out of that gap: ~1.9x the widest real placement observed anywhere, and less
	// than half the leaderless figure. It tolerates placement luck on levels this case does not
	// sweep while still failing the moment the anchor stops being a neighbour of the leader. It
	// is not a spec constant and means nothing beyond that separation.
	//
	// Nothing tighter is available honestly: the walk is 100 unbounded steps, so no smaller
	// number is provable from the code alone, and the two measurements are what make 32 a
	// defensible separator rather than a guess. The measured maximum is printed unconditionally
	// so drift toward the bound is visible before it becomes a failure.
	constexpr size_t kUnleashedAdjacencyBound = 32;

	LoadSquadParams("txtdata\\monsters\\level_roster_params_squads_unleashed.tsv");
	size_t unleashedLeashedMinions = 0;
	size_t unleashedPartners = 0;
	size_t unleashedMaxDistance = 0;
	for (uint8_t level = 9; level <= 12; level++) {
		for (uint32_t seed = 0; seed < 10; seed++) {
			const SquadObservation obs = RunLevel(level, 21000 + seed);
			unleashedLeashedMinions += obs.leashedMinions;
			unleashedPartners += obs.partnersPlaced;
			unleashedMaxDistance = std::max(unleashedMaxDistance, obs.maxPartnerLeaderDistance);
			EXPECT_LE(obs.maxPartnerLeaderDistance, kUnleashedAdjacencyBound)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": spec 4.3.4 keeps passing the leader to PlaceGroup, so an unleashed"
			    << " minion must still be seeded from the leader's neighbourhood";
		}
	}
	std::cout << "[ SQUADFALLBACK ] unleashed partners " << unleashedPartners
	          << " max leader distance " << unleashedMaxDistance
	          << " bound " << kUnleashedAdjacencyBound << std::endl;
	// Without a sample the distance assertion above is vacuous: an unleashed squad
	// places no leashed minion by construction, so partnersPlaced is the only
	// evidence that the fallback ran at all.
	EXPECT_GT(unleashedPartners, 0u)
	    << "the unleashed runs placed no squad minion at all, so the adjacency assertions"
	    << " above measured nothing";

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
	// part - every level FORMS squads (see the fix-review note on the `formed`
	// assertion below for why the zero-rolls detector must not be leash-gated),
	// no level loses ALL of its rolls, and neither counter ever exceeds rolls.
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
		size_t formed = 0;
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
			formed += counters.formed;
			minions += obs.leashedMinions;

			EXPECT_EQ(obs.leadersWithZeroPackSize, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
			EXPECT_EQ(obs.minionsWithOverwrittenAi, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
			EXPECT_EQ(obs.minionsUnderNonCoreLeader, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed;
			EXPECT_EQ(obs.minionsSharingLeaderType, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << ": spec 4.3.3 requires a squad to mix two DIFFERENT core types";
		}
		const double rollRate = eligible == 0 ? 0.0 : static_cast<double>(rolls) / static_cast<double>(eligible);
		const double realisation = rolls == 0 ? 0.0 : static_cast<double>(realised) / static_cast<double>(rolls);
		std::cout << "[ SHIPPEDSQUAD ] level " << static_cast<int>(level) << " eligible " << eligible
		          << " rolls " << rolls << " rollRate " << rollRate << " realised " << realised
		          << " realisation " << realisation << " formed " << formed << " minions " << minions
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
		// Fix review F1 (RB23): the numerator here MUST be `formed`, not the
		// leash-gated `realised`. `realised` is only ever incremented via
		// obs.leadersWithMinions, which walks getLeader() on LEASHED minions -
		// setLeader() (and therefore the leader<->minion back-pointer this reads)
		// is only called when PlaceGroup runs with leashed=true. A level that
		// takes the spec 4.3.4 fallback (squad_leashed = 0) is therefore
		// GUARANTEED to read realised == 0 forever, by construction, with no
		// placement defect at all. Guarding on `realised` here would make this
		// case fail the moment the spec-sanctioned remedy for a LOW formation
		// rate (Task 4's own kSquadFormationFloor guard) is applied to any level,
		// and the failure text below would misreport a compliant fallback as "no
		// squad formed at all". `formed` (SquadRollCounters::formed) counts a
		// roll that placed >= 1 minion for its leader regardless of leash, so it
		// stays a true zero-roll detector across that fallback.
		EXPECT_GT(formed, 0u)
		    << "level " << static_cast<int>(level) << " formed no squad at all over " << kSeeds
		    << " seeds despite " << rolls << " rolls";
		EXPECT_LE(realised, rolls)
		    << "level " << static_cast<int>(level) << " reports more realised squads than rolls";
		EXPECT_LE(formed, rolls)
		    << "level " << static_cast<int>(level) << " reports more formed squads than rolls";
	}
	AppendMeasurementReport(report);
}

// ---------------------------------------------------------------------------
// Task 4 (spec 2026-09-15-level-rosters-design §6 acceptance 7, §4.3.4).
//
// The formation rate is "of the squad rolls that were ATTEMPTED, what share
// really put down at least one minion". Its floors below are DERIVED FROM
// MEASUREMENT, not chosen: see kSquadFormationFloor.
//
// Two things about the metric matter for the guard to mean anything:
//
//   * numerator = SquadRollCounters::formed, which counts a roll that placed
//     >= 1 minion whether or not the squad is leashed. The older `realised`
//     counter is gated on leader.packSize > 0, and PlaceGroup only writes
//     packSize when leashed, so a `realised`-based guard would read 0% on any
//     level that took the §4.3.4 fallback - i.e. it would fail precisely
//     because the remedy it is supposed to trigger had been applied.
//   * denominator = SquadRollCounters::rolls, the loop's OWN count of rolls
//     that entered the squad branch. It is not re-derived from squad_chance,
//     so a level is judged on the rolls it actually made rather than on the
//     ones the table nominally promises. This mattered while L14 shipped a
//     reduced chance of 10; task D raised it back to 30 by dropping L14's
//     RangedKite core, so the loop's own denominator keeps this case correct
//     across such a data change with no edit here.
//
// Counters are per level and reset by InitLevelMonsters(), so they are read
// straight after each run (a before/after difference underflows across the
// reset - the defect Task 3 hit).
// ---------------------------------------------------------------------------

// Per-level formation-rate floors (spec §6 acceptance 7: "threshold derived from
// measurement").
//
// SOURCE: this task's own measurement, run on the SHIPPED tables through the
// same production chain this case drives (CreateDungeonForMeasurement ->
// GetLevelMTypes -> InitMonsters), 500 seeds per level, seed base 51000:
//
//   L1  6691/6694 = 0.99955   L6  3084/3084 = 1.00000   L11 2697/2698 = 0.99963
//   L2  5140/5142 = 0.99961   L7  3276/3277 = 0.99970   L12 2587/2587 = 1.00000
//   L3  3998/3999 = 0.99975   L8  3446/3447 = 0.99971   L13 3200/3201 = 0.99969
//   L4  3702/3703 = 0.99973   L9  2658/2660 = 0.99925   L14 1364/1364 = 1.00000
//   L5  3199/3199 = 1.00000   L10 2715/2718 = 0.99890   L15 4854/4854 = 1.00000
//
// So every level forms a squad on essentially every roll; the worst level is L10
// at 99.89%, i.e. 3 losses in 2718 rolls. The three documented loss paths
// (totalmonsters clamp, PlaceGroup's 10-attempt give-up, the 4-tile leash)
// remove less than 0.11% of rolls at squad_size = 2, because a squad is SMALLER
// than the plain group it replaces (`na` is 3-5), so it is easier to fit, not
// harder. Task 3 measured the same picture at 50 seeds and found the
// leader-placement and no-partner counters flat 0.
//
// WHY 0.95 AND NOT THE MEASURED VALUE: the floor is one uniform value, set
// about 45x the worst measured loss below the worst measured rate. That margin
// is deliberate and is what the floor buys:
//
//   * it is a real guard, not a tautology. The rate is a ratio of the loop's own
//     counters; break the placement of minions and it goes to 0.0, break it
//     partially and it falls off a cliff, because nothing in the shipped
//     configuration produces a rate between 0.95 and 0.998. The falsification
//     runs in the task-4 report show the L10-only mutation landing at 0.0.
//   * it does not turn a data edit into a test failure. squad_size, tail_draw
//     and the core sets are Task-5-and-later knobs; a level that legitimately
//     moves from 0.9989 to 0.99 must not go red, since the spec's criterion is
//     "squads form", not "squads form exactly this often". The plan's own
//     suggested lower bound was 50%; 0.95 is far stricter than that while still
//     leaving the data free.
//
// It is NOT a tight bound and NOT a spec constant: it is the separator between
// "the squad path works" and "the squad path is broken", placed inside a gap the
// measurements above show to be empty. The measured value is printed
// unconditionally on every run ([ SQUADFORM ]), so a level drifting from 0.9989
// toward 0.95 is visible long before it fails.
//
// Indexed BY LEVEL, sized 17 so level 16 is a valid index. L16 returns before
// the roster path (spec 4.2.7) and has no params row, so it is unconstrained by
// declaration rather than judged against a floor that cannot apply.
constexpr double kSquadFormationUnconstrained = 0.0;
constexpr std::array<double, 17> kSquadFormationFloor {
	kSquadFormationUnconstrained, // L0 (unused)
	0.95,                         // L1
	0.95,                         // L2
	0.95,                         // L3
	0.95,                         // L4
	0.95,                         // L5
	0.95,                         // L6
	0.95,                         // L7
	0.95,                         // L8
	0.95,                         // L9
	0.95,                         // L10
	0.95,                         // L11
	0.95,                         // L12
	0.95,                         // L13
	0.95,                         // L14
	0.95,                         // L15
	kSquadFormationUnconstrained, // L16
};

TEST_F(SquadPlacementTest, SquadFormationRate)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN (monsters\\monsters\\genrl.trn) not available - skipping test";

	// The SHIPPED tables, deliberately: this case pins the formation rate of the
	// configuration the game actually ships (squad_chance 30 on every level), which is
	// what acceptance 7 is about. The squads_always fixture would measure a
	// different table and hide a shipped-data regression.
	LoadLevelRoster(); // shipped tables (squad_chance 30)

	constexpr uint32_t kSeeds = 500;
	std::string report = "\n## Task 4: squad formation rate per level (SHIPPED table, 500 seeds)\n\n"
	                     "| level | squad_leashed | rolls | formed | formation rate | floor |\n"
	                     "|---|---|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 15; level++) {
		size_t rolls = 0;
		size_t formed = 0;
		size_t leashedRealised = 0;
		// Fix review F3: the two loss counters that explain a SHORTFALL between
		// `formed` and `rolls`, so the failure text below can name the actual
		// cause instead of always pointing at squad_leashed.
		size_t leaderFailures = 0;
		size_t noPartner = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			RunLevel(level, 51000 + seed);
			const SquadRollCounters &counters = GetSquadRollStats();
			rolls += counters.rolls;
			formed += counters.formed;
			leashedRealised += counters.realised;
			leaderFailures += counters.leaderPlacementFailed;
			noPartner += counters.noPartnerAvailable;
		}

		const LevelRosterParams *params = GetLevelRosterParams(level);
		ASSERT_NE(params, nullptr) << "level " << static_cast<int>(level) << " has no params row";

		// A level with no rolls has no formation rate to judge, and reporting 0.0
		// for it would fail the floor for the wrong reason. The plan allows
		// skipping such a level - but it must be skipped because the TABLE
		// disabled squads, not because the branch quietly stopped being reached:
		// the latter is the regression this case exists to catch.
		if (rolls == 0) {
			EXPECT_EQ(params->squadChance, 0u)
			    << "level " << static_cast<int>(level) << " has squad_chance "
			    << static_cast<int>(params->squadChance) << " but made no squad roll in " << kSeeds
			    << " seeds, so the squad branch is no longer reached on it";
			// Fix review F4: this line used to unconditionally print "squads
			// disabled by the table (squad_chance 0)" even when squadChance was
			// NOT 0 (the EXPECT above is what actually catches that case, and it
			// goes red - but this cout line kept asserting the opposite of what
			// just happened, which is self-contradictory when read in the log).
			// Report the real squadChance and rolls instead of assuming why rolls
			// is 0.
			std::cout << "[ SQUADFORM ] level " << static_cast<int>(level)
			          << " squad_chance=" << static_cast<int>(params->squadChance)
			          << " but the loop never rolled a squad (0 rolls in " << kSeeds << " seeds)"
			          << std::endl;
			continue;
		}

		const double rate = static_cast<double>(formed) / static_cast<double>(rolls);
		// Unconditional, pass or fail: the floor is loose on purpose, so drift has
		// to be readable before it becomes a failure. F3: report the loss
		// counters too, so a level sitting near its floor shows WHY, not just
		// the resulting rate.
		std::cout << "[ SQUADFORM ] level " << static_cast<int>(level)
		          << " leashed " << (params->squadLeashed ? 1 : 0)
		          << " rolls " << rolls << " formed " << formed << " rate " << rate
		          << " noPartnerAvailable " << noPartner << " leaderPlacementFailed " << leaderFailures
		          << " floor " << kSquadFormationFloor[level] << std::endl;
		report += StrCat("| ", level, " | ", params->squadLeashed ? 1 : 0, " | ", rolls, " | ", formed, " | ");
		report += StrCat(static_cast<int>(rate * 10000.0 + 0.5), "/10000 | ");
		report += StrCat(static_cast<int>(kSquadFormationFloor[level] * 10000.0 + 0.5), "/10000 |\n");

		// Acceptance 7. F3: the old failure text opened a single prescription -
		// "set this level's squad_leashed to 0" - regardless of WHY the level
		// fell short. That is actively misleading when the real cause is
		// upstream of leashing: `noPartnerAvailable` means the level had no
		// second registered/available core type to build a mixed squad from (spec
		// 4.3.3), and `leaderPlacementFailed` means the leader itself could not
		// be placed (today's defensive, unreachable fallback in monster.cpp -
		// see its comment there). Unleashing does not fix either: an unleashed
		// squad still calls PickCorePartnerTypeIndex and still needs the leader
		// placed BEFORE the leash/no-leash branch is ever reached. So the message
		// points at whichever counter is actually nonzero, and reserves the
		// squad_leashed suggestion for when neither of those loss paths explains
		// the shortfall (i.e. the loss is genuinely coming from PlaceGroup's
		// leash-gated placement attempts for the minion group itself).
		std::string diagnosis;
		if (noPartner > 0) {
			diagnosis = StrCat(noPartner, " roll(s) had no available core partner"
			                              " (PickCorePartnerTypeIndex found nothing) - the level's"
			                              " roster needs a second usable core type for spec 4.3.3's"
			                              " mixed squad, NOT squad_leashed: unleashing a squad that"
			                              " never gets a partner chosen changes nothing");
		} else if (leaderFailures > 0) {
			diagnosis = StrCat(leaderFailures, " roll(s) failed to place the LEADER itself"
			                                   " (monster.cpp's defensive leaderPlacementFailed"
			                                   " fallback is now live) - this is a placement/slot"
			                                   " failure that happens before the leash decision, so"
			                                   " squad_leashed cannot fix it");
		} else {
			diagnosis = "leader placement and partner selection both succeeded on every roll"
			            " (noPartnerAvailable=0, leaderPlacementFailed=0), so the loss is in"
			            " PlaceGroup's own minion placement (the totalmonsters clamp, its"
			            " 10-attempt give-up, or - only while squad_leashed=1 - the 4-tile leash):"
			            " per spec 4.3.4 this level's squad_leashed goes to 0 and the measurement is"
			            " recorded, rather than the floor being lowered";
		}
		EXPECT_GE(rate, kSquadFormationFloor[level])
		    << "level " << static_cast<int>(level) << " formed only " << formed << " squads out of "
		    << rolls << " rolls (" << rate << "), under its measured floor "
		    << kSquadFormationFloor[level] << "; " << diagnosis;

		// Premise guard: `formed` must never exceed the rolls it is counted from -
		// one roll places at most one squad. This is the invariant that caught the
		// underflowing before/after accounting in Task 3.
		EXPECT_LE(formed, rolls)
		    << "level " << static_cast<int>(level) << " reports more formed squads than rolls,"
		    << " so the denominator is not the loop's";

		// The two counters must agree while the level is leashed. This is what
		// keeps `formed` honest: if it ever started counting something other than
		// a real squad, it would drift away from the packSize-gated figure that
		// Task 3's cases assert against.
		if (params->squadLeashed) {
			EXPECT_EQ(leashedRealised, formed)
			    << "level " << static_cast<int>(level) << " is leashed, so every formed squad must"
			    << " also show up as a packSize-carrying leader";
		} else {
			// Fix review F2 (honesty about this branch): `realised` is gated on
			// `leader.packSize > 0`, and PlaceGroup only ever writes packSize when
			// its `leashed` argument is true (see SquadRollCounters::realised /
			// PlaceGroup in monster.cpp). So the moment `params->squadLeashed` is
			// false, `leashedRealised` reads 0 unconditionally - this branch is
			// NOT exercising any code path today; the shipped table leashes every
			// level (see level_roster_params.tsv), so this `else` is dead in the
			// current data and this EXPECT is currently a tautology, not a live
			// discriminator. The assertion is kept (not weakened) because it is
			// the ONLY thing that will actually observe a future §4.3.4 fallback
			// if a level's squad_leashed is ever flipped to 0 in response to
			// SquadFormationRate failing its floor: on that day this branch stops
			// being dead, `formed` will read > 0 for that level while
			// `leashedRealised` still reads 0, and this line is what confirms the
			// fallback produced squads without leashing them rather than silently
			// reading as a formation collapse.
			EXPECT_EQ(leashedRealised, 0u)
			    << "level " << static_cast<int>(level) << " runs the 4.3.4 fallback, so no roll may"
			    << " report a leashed leader";
		}
	}
	AppendMeasurementReport(report);
}

// ---------------------------------------------------------------------------
// A2 Task 1, step 5: the PRE-CHANGE baseline for L17-24 (Hellfire Nest/Crypt).
//
// Why this case exists: A2 task 2 will add core/tail rows and per-level params for
// L17-24. Acceptance 8 judges that data against the ranged share these levels have
// BEFORE the data lands (<= baseline + 5pp, same R4 rule and same 5-point band as
// kRangedShareCeiling above). That baseline can only be measured now, while L17-24
// still run the R28 legacy fallback (no params row, no roster rows), so this case
// measures and PRINTS it unconditionally rather than asserting a threshold: there is
// nothing to assert against yet, and inventing one would be a fabricated ceiling.
// The printed numbers are transcribed into the task report; task 2 fills
// kRangedShareBaseline[17..24] from them before adding those levels to the
// PlacedClassMixWithinBaseline loop.
//
// CAVEAT that travels with these numbers (measured, not assumed): §4.5's behaviour
// taxonomy - GetBehaviorClass() in Source/tables/monstdat.cpp - enumerates the DIABLO
// AIs and sends everything else to its `default:` arm, BehaviorClass::Boss. Ten AIs
// that only appear on L17-24 land there: ArchLich, Diablo, FireBat, FireMan, Lich,
// Necromorb, Psychorb, Scavenger, Torchant (plus BoneDemon, which IS enumerated as
// RangedKite). Several of those are ranged casters in gameplay terms, so the ranged
// share printed below UNDERSTATES the real ranged pressure on these levels, and the
// large Boss counts are that default arm, not eight boss-filled levels.
//
// That gap is NOT fixed here: re-classifying AIs would change §4.5's taxonomy (a
// controller-maintained spec) and would silently move the L1-16 numbers the existing
// kRangedShareBaseline entries were measured with. It is recorded so acceptance 8 is
// read correctly - a like-for-like comparison, both sides measured under this same
// taxonomy - rather than as an absolute statement about ranged pressure.
//
// Asset gating: CI only ships spawn.mpq. Under the base monstdat.tsv every L17-24
// monster is availability=Never (the hf overlay re-ships the whole table with them
// available), and the Nest/Crypt .til assets live under nlevels\ in the Hellfire
// data, so without Hellfire assets there is no measurement to take - skip, never
// fail. HaveHellfire() is the right probe here (unlike PlacedClassMixReport's
// genrl.trn probe, which deliberately admits a retail-but-not-HF install): these
// levels do not exist at all without Hellfire.
//
// Global state: this suite mounts the hf overlay and flips gbIsHellfire, which the
// Diablo-data suites above measure against. It therefore snapshots and restores
// every global it touches, following HellfireNoParamsSamplingTest in
// sampling_behavior_test.cpp (R30) - including reloading the Diablo monster/roster
// data in TearDownTestSuite so a later suite (or --gtest_shuffle) cannot inherit
// Hellfire data.
// ---------------------------------------------------------------------------
// The priority LoadHellfireArchives() mounts hellfire.mpq at; UnloadModArchives()
// clears the whole 8000-8999 band, so using the real value keeps teardown correct.
constexpr int kHellfireMpqPriority = 8000;

class HellfireLevelBaselineTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadGameArchives();
		if (!HaveMainData()) {
			missingMpqAssets_ = true;
			return;
		}

		savedQuests_.assign(std::begin(Quests), std::end(Quests));
		savedPlayerCount_ = Players.size();
		savedMyPlayerIsFirst_ = !Players.empty() && MyPlayer == &Players[0];
		savedOriginalCathedral_ = Players.empty() ? true : Players[0].pOriginalCathedral;
		savedGameInitInfo_ = sgGameInitInfo;
		savedIsMultiplayer_ = gbIsMultiplayer;
		savedIsHellfire_ = gbIsHellfire;
		savedIsSpawn_ = gbIsSpawn;
		savedPrefPath_ = paths::PrefPath();
		snapshotTaken_ = true;

		gbIsSpawn = false;
		// TestInitGame() mounts the hf ARCHIVE but does not touch gbIsHellfire (the
		// mod's init.lua, which calls hellfire.enable(), is not run by tests). The
		// flag has to be set explicitly, and it matters twice over here: it selects
		// the Hellfire level range and it is what MaxValidatedDungeonLevel() reads,
		// so LoadLevelRoster() below validates L17-24 in full rather than skipping
		// them - which is precisely the scoping this task added.
		gbIsHellfire = true;
		paths::SetPrefPath(paths::BasePath() + "test/fixtures/");
		TestInitGame(/*fullQuests=*/true, /*originalCathedral=*/true, /*hellfire=*/true);
		// TestInitGame() mounts the hf MOD archive (mods/hf), which supplies the L17-24
		// monstdat overlay - but NOT the Nest/Crypt graphics: nlevels\\l6data\\l6.til and
		// nlevels\\l5data\\l5.til live in hellfire.mpq itself. Without them pMegaTiles
		// stays null and DRLG_LPass3 segfaults on the first L17 dungeon (observed).
		//
		// The production route is the mod's init.lua calling hellfire.loadData() ->
		// LoadHellfireArchives() (Source/lua/modules/hellfire.cpp:13). That function is
		// deliberately NOT used here: it also requires hfmonk/hfmusic/hfvoice.mpq and
		// calls DisplayFatalErrorAndExit() when any is missing, which would abort the whole
		// test binary on an install that has hellfire.mpq but not the split extras. This
		// measurement needs level TILES only - no monk graphics, music or voice - so the
		// archive is mounted directly at Hellfire's own priority (8000, the value
		// LoadHellfireArchives uses; UnloadModArchives() clears 8000-8999, so TearDown
		// still unmounts it).
		//
		// F1 (task 1 fix review round 1): this whole self-mount is packed-MPQ-only. Under
		// UNPACKED_MPQS, MpqArchiveT is std::string (a directory path, see assets.hpp), so
		// there is no MpqArchive to construct or insert here, and UnloadModArchives()'s
		// cleanup below is itself compiled out under that macro (see its #ifndef in
		// assets.cpp), i.e. the unload half of this mount has no defined semantics in that
		// configuration either. Rather than fabricate a directory-path equivalent for a
		// measurement this task does not require under UNPACKED_MPQS, the whole block is
		// skipped and the suite falls through to missingHellfire_ = true via the .til probe
		// below (nlevels\ tiles are never found without the mount), so every case in this
		// fixture GTEST_SKIPs cleanly instead of failing to compile.
#ifndef UNPACKED_MPQS
		for (const std::string &searchPath : { paths::BasePath(), paths::PrefPath(), paths::ConfigPath() }) {
			if (MpqArchives.find(kHellfireMpqPriority) != MpqArchives.end())
				break;
			for (const char *name : { "hellfire.mpq", "HELLFIRE.MPQ" }) {
				const std::string candidate = searchPath + name;
				if (!FileExists(candidate))
					continue;
				auto archive = MpqArchive::Open(candidate.c_str());
				if (archive.has_value()) {
					MpqArchives.insert_or_assign(kHellfireMpqPriority, std::move(*archive));
					break;
				}
			}
		}
#endif
		LoadMonsterData();
		LoadLevelRoster();

#ifndef UNPACKED_MPQS
		// HaveHellfire() only reports that hellfire.mpq was FOUND. What this suite
		// actually needs is the Nest/Crypt tile data readable, so probe the real
		// dependency (same principle as missingRetailTrn_ above): one .til per Hellfire
		// level type, since L17-20 and L21-24 use different files.
		size_t nestTilSize = 0;
		size_t cryptTilSize = 0;
		const AssetHandle nestTil = OpenAsset(R"(nlevels\l6data\l6.til)", nestTilSize);
		const AssetHandle cryptTil = OpenAsset(R"(nlevels\l5data\l5.til)", cryptTilSize);
		missingHellfire_ = !HaveHellfire() || !nestTil.ok() || nestTilSize == 0
		    || !cryptTil.ok() || cryptTilSize == 0;
#else
		// UNPACKED_MPQS never mounts hellfire.mpq above (see the comment on the mount
		// block), so L17-24's Nest/Crypt tiles can never be reached in this configuration:
		// skip unconditionally rather than probing assets that were never loaded.
		missingHellfire_ = true;
#endif
	}

	static void TearDownTestSuite()
	{
		if (!snapshotTaken_)
			return;

		std::copy(savedQuests_.begin(), savedQuests_.end(), std::begin(Quests));
		Players.resize(savedPlayerCount_);
		if (!Players.empty())
			Players[0].pOriginalCathedral = savedOriginalCathedral_;
		MyPlayer = savedMyPlayerIsFirst_ && !Players.empty() ? &Players[0] : nullptr;
		sgGameInitInfo = savedGameInitInfo_;
		gbIsMultiplayer = savedIsMultiplayer_;
		gbIsHellfire = savedIsHellfire_;
		gbIsSpawn = savedIsSpawn_;
		paths::SetPrefPath(savedPrefPath_);

		UnloadModArchives();
		LoadModArchives({});
		LoadMonsterData();
		LoadLevelRoster();
		snapshotTaken_ = false;
	}

	void TearDown() override
	{
		paths::SetAssetsPath(savedAssetsPath_);
		// Leave the process holding the SHIPPED tables, exactly as SquadPlacementTest
		// does: LoadPreA2Tables() below swaps process-global roster storage, and every
		// other case in this binary (and any later case here) expects the shipped rows.
		if (!missingMpqAssets_)
			LoadLevelRoster();
	}

	void SetUp() override
	{
		savedAssetsPath_ = paths::AssetsPath();
	}

	/**
	 * @brief Loads the L1-16-only copies of the shipped tables from test/fixtures/.
	 *
	 * A2 task 2 gave L17-24 real roster/params rows, so the SHIPPED tables can no
	 * longer reproduce the pre-change (R28 legacy) sampling path that
	 * A2PreChangeBaselineForHellfireLevels measures. The fixtures are the shipped
	 * tables with the L17-24 rows removed and nothing else changed, so the legacy
	 * fallback (global 4000 budget, unbounded tail) is reached the same way it was
	 * before the data landed - which is what makes the recorded baseline a real A/B
	 * against the post-change measurement rather than two different measurements.
	 */
	static void LoadPreA2Tables()
	{
		paths::SetAssetsPath(paths::BasePath() + "test/fixtures/");
		LoadLevelRosterFromFiles(
		    "txtdata\\monsters\\level_rosters_no_hf.tsv",
		    "txtdata\\monsters\\level_roster_params_no_hf.tsv");
		paths::SetAssetsPath(paths::BasePath() + "assets/");
	}

	static bool missingMpqAssets_;
	static bool missingHellfire_;

private:
	std::string savedAssetsPath_;
	static std::vector<Quest> savedQuests_;
	static size_t savedPlayerCount_;
	static bool savedMyPlayerIsFirst_;
	static bool savedOriginalCathedral_;
	static GameData savedGameInitInfo_;
	static bool savedIsMultiplayer_;
	static bool savedIsHellfire_;
	static bool savedIsSpawn_;
	static std::string savedPrefPath_;
	static bool snapshotTaken_;
};

bool HellfireLevelBaselineTest::missingMpqAssets_ = false;
bool HellfireLevelBaselineTest::missingHellfire_ = false;
std::vector<Quest> HellfireLevelBaselineTest::savedQuests_;
size_t HellfireLevelBaselineTest::savedPlayerCount_ = 0;
bool HellfireLevelBaselineTest::savedMyPlayerIsFirst_ = false;
bool HellfireLevelBaselineTest::savedOriginalCathedral_ = true;
GameData HellfireLevelBaselineTest::savedGameInitInfo_ {};
bool HellfireLevelBaselineTest::savedIsMultiplayer_ = false;
bool HellfireLevelBaselineTest::savedIsHellfire_ = false;
bool HellfireLevelBaselineTest::savedIsSpawn_ = false;
std::string HellfireLevelBaselineTest::savedPrefPath_;
bool HellfireLevelBaselineTest::snapshotTaken_ = false;

TEST_F(HellfireLevelBaselineTest, A2PreChangeBaselineForHellfireLevels)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingHellfire_)
		GTEST_SKIP() << "Hellfire assets required: L17-24 do not exist without them";

	// A2 task 2: the SHIPPED tables now carry L17-24 rows, so the pre-change path has
	// to be reproduced from the L1-16-only fixtures (see LoadPreA2Tables). Task 1
	// asserted the shipped tables themselves still lacked those rows; that premise was
	// correct then and is deliberately inverted now, because the very thing this case
	// exists to be a baseline FOR has landed.
	LoadPreA2Tables();

	// Premise 1 (unchanged in substance): the range really is on the pre-change (R28
	// legacy) path for this measurement. Recording anything else here would corrupt
	// acceptance 8's reference. It now checks the LOADED tables rather than the shipped
	// ones, so a fixture that silently gained L17-24 rows still fails loudly.
	for (uint8_t level = 17; level <= 24; level++) {
		ASSERT_EQ(GetLevelRosterParams(level), nullptr)
		    << "level " << static_cast<int>(level) << " has a params row after loading the pre-A2"
		    << " fixtures: those fixtures must be the shipped tables MINUS the L17-24 rows";
		ASSERT_TRUE(GetLevelRoster(level).empty())
		    << "level " << static_cast<int>(level) << " has roster rows: see above";
	}

	// Same fixture, seed count and denominator convention as
	// PlacedClassMixWithinBaseline, so the numbers are directly comparable with the
	// kRangedShareBaseline entries: 200 seeds, placed-monster denominator, ranged =
	// RangedTurret + RangedKite.
	constexpr uint32_t kSeeds = 200;
	std::string report = "\n## A2 pre-change baseline: L17-24 (Hellfire, 200 seeds per level)\n\n"
	                     "| level | placed | types | ranged | ranged share | Melee | RangedTurret |"
	                     " RangedKite | Rally | Charge | Sneak | Summon | Boss |\n"
	                     "|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";

	for (uint8_t level = 17; level <= 24; level++) {
		std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> sum {};
		size_t placed = 0;
		size_t typeCountSum = 0;
		// RB37: the ranged-caster count is accumulated on the SAME pre-change samples
		// as the class mix above, because it is the baseline for the RB37 guard and a
		// baseline measured on a different run (or derived by arithmetic from the Boss
		// column) would not be an A/B against anything.
		size_t preChangeCasters = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			CreateDungeonForMeasurement(level, 61000 + seed);
			{
				const auto getTypesResult = GetLevelMTypes();
				ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
			}
			{
				const auto initResult = InitMonsters();
				ASSERT_TRUE(initResult.has_value()) << initResult.error();
			}
			const auto mix = MeasurePlacedClassMix();

			// Premise 2 (per sample): the level really did field monsters. A level
			// that placed nothing would report a 0/0 share, which reads as "0% ranged"
			// - a baseline that can never be exceeded, i.e. a silently useless
			// reference for acceptance 8.
			ASSERT_GT(ActiveMonsterCount, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed << " placed no monster";
			EXPECT_LE(ActiveMonsterCount, MaxMonsters - 10)
			    << "level " << static_cast<int>(level) << " seed " << seed << " exceeds engine placement cap";
			size_t mixTotal = 0;
			for (const size_t count : mix)
				mixTotal += count;
			EXPECT_EQ(mixTotal, ActiveMonsterCount) << "class-mix sum must equal placed count";

			for (size_t i = 0; i < mix.size(); i++)
				sum[i] += mix[i];
			placed += ActiveMonsterCount;
			typeCountSum += LevelMonsterTypeCount;
			for (size_t i = 0; i < ActiveMonsterCount; i++) {
				if (IsRangedCasterAi(Monsters[ActiveMonsters[i]].ai))
					preChangeCasters++;
			}
		}

		const size_t ranged = sum[static_cast<size_t>(BehaviorClass::RangedTurret)]
		    + sum[static_cast<size_t>(BehaviorClass::RangedKite)];
		const double rangedShare = static_cast<double>(ranged) / static_cast<double>(placed);
		const double avgTypes = static_cast<double>(typeCountSum) / static_cast<double>(kSeeds);

		// UNCONDITIONAL print (pass or fail): this output IS the deliverable - task 2
		// transcribes it into kRangedShareBaseline[17..24]. The numerator/denominator
		// are printed alongside the share so the constant can be written in the same
		// auditable "numerator/denominator" form the existing table uses.
		std::cout << "[ A2BASELINE ] level " << static_cast<int>(level)
		          << " placed " << placed
		          << " types " << avgTypes
		          << " ranged " << ranged
		          << " rangedShare " << rangedShare
		          << " (" << sum[static_cast<size_t>(BehaviorClass::RangedTurret)]
		          << "+" << sum[static_cast<size_t>(BehaviorClass::RangedKite)]
		          << ")/" << placed
		          << " Melee " << sum[static_cast<size_t>(BehaviorClass::Melee)]
		          << " RangedTurret " << sum[static_cast<size_t>(BehaviorClass::RangedTurret)]
		          << " RangedKite " << sum[static_cast<size_t>(BehaviorClass::RangedKite)]
		          << " Rally " << sum[static_cast<size_t>(BehaviorClass::Rally)]
		          << " Charge " << sum[static_cast<size_t>(BehaviorClass::Charge)]
		          << " Sneak " << sum[static_cast<size_t>(BehaviorClass::Sneak)]
		          << " Summon " << sum[static_cast<size_t>(BehaviorClass::Summon)]
		          << " Boss " << sum[static_cast<size_t>(BehaviorClass::Boss)]
		          << std::endl;

		// RB37's baseline, measured on these same pre-change samples and printed in the
		// same auditable numerator/denominator form. This line IS the source of
		// kRangedCasterShareBaseline[17..24]; transcribing anything else there (for
		// instance the Boss column above, which is NOT the same set - it excludes
		// Acid/BoneDemon kites and includes non-caster AIs like Scavenger) makes the
		// RB37 guard compare two different measurements.
		std::cout << "[ A2CASTERBASE ] level " << static_cast<int>(level)
		          << " rangedCasterShare " << (static_cast<double>(preChangeCasters) / static_cast<double>(placed))
		          << " (" << preChangeCasters << "/" << placed << ")" << std::endl;

		// The caveat above, printed next to the numbers it qualifies: a reader who sees
		// only this log line must not read a 0 ranged share as "no ranged pressure".
		if (level == 17) {
			std::cout << "[ A2BASELINE ] NOTE: GetBehaviorClass() maps HF-only AIs (Lich, ArchLich,"
			             " Necromorb, Psychorb, FireBat, FireMan, Torchant, Scavenger, ...) to its"
			             " default arm BehaviorClass::Boss, so the Boss column absorbs several ranged"
			             " casters and rangedShare understates real ranged pressure on L17-24."
			             " Acceptance 8 compares like for like under this same taxonomy."
			          << std::endl;
		}

		report += StrCat("| ", level, " | ", placed, " | ",
		    static_cast<int>(avgTypes * 100.0 + 0.5), "/100 | ", ranged, " | ",
		    static_cast<int>(rangedShare * 10000.0 + 0.5), "/10000 |");
		for (const size_t count : sum)
			report += StrCat(" ", count, " |");
		report += "\n";
	}
	AppendMeasurementReport(report);
}

// ---------------------------------------------------------------------------
// A2 Task 2, acceptance 8 for the Hellfire band: the POST-change ranged share of
// L17-24 against kRangedShareBaseline[17..24] + 5pp.
//
// Why this is a separate case from PlacedClassMixWithinBaseline rather than an
// extension of its loop: that case lives in LevelRosterBaselineTest, which measures
// under DIABLO data (gbIsHellfire false, no hf overlay). L17-24 do not exist there -
// every one of their monsters is availability=Never in the base monstdat.tsv - so its
// loop CANNOT be widened to 24 without either mounting Hellfire in that fixture (which
// would change the data every L1-15 baseline was measured under) or measuring eight
// levels that place nothing. The guard therefore lives in the Hellfire suite, reads
// the SAME kRangedShareBaseline/kRangedShareCeiling table, and uses the same 200
// seeds / placed-monster denominator / ranged = RangedTurret + RangedKite convention,
// so the two halves of the range are judged by one rule.
TEST_F(HellfireLevelBaselineTest, PlacedClassMixWithinBaselineForHellfireLevels)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingHellfire_)
		GTEST_SKIP() << "Hellfire assets required: L17-24 do not exist without them";

	// Premise: this measures the SHIPPED tables. The suite's TearDown restores them,
	// but a case that ran LoadPreA2Tables() earlier in the same process would leave a
	// stale view if that ever changed, and measuring the fixtures here would silently
	// re-measure the baseline and always pass.
	for (uint8_t level = 17; level <= 24; level++) {
		ASSERT_NE(GetLevelRosterParams(level), nullptr)
		    << "level " << static_cast<int>(level) << " has no params row: acceptance 8 for the"
		    << " Hellfire band judges A2 task 2's shipped data, so the data must be loaded";
		ASSERT_FALSE(GetLevelRoster(level).empty())
		    << "level " << static_cast<int>(level) << " has no roster rows: see above";
	}

	constexpr uint32_t kSeeds = 200;
	for (uint8_t level = 17; level <= 24; level++) {
		// Same anti-vacuity guard PlacedClassMixWithinBaseline uses: a level still
		// holding the unconstrained sentinel has ceiling 1.0 and can never fail here.
		ASSERT_LT(kRangedShareBaseline[level], kRangedShareUnconstrained)
		    << "level " << static_cast<int>(level) << " is asserted but still holds the unconstrained"
		    << " sentinel; fill its A2 baseline row before adding it to this loop";

		size_t total = 0;
		size_t ranged = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			CreateDungeonForMeasurement(level, 61000 + seed);
			{
				const auto getTypesResult = GetLevelMTypes();
				ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
			}
			{
				const auto initResult = InitMonsters();
				ASSERT_TRUE(initResult.has_value()) << initResult.error();
			}
			const auto mix = MeasurePlacedClassMix();
			ASSERT_GT(ActiveMonsterCount, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed << " placed no monster";
			total += ActiveMonsterCount;
			ranged += mix[static_cast<size_t>(BehaviorClass::RangedTurret)]
			    + mix[static_cast<size_t>(BehaviorClass::RangedKite)];
		}
		ASSERT_GT(total, 0u) << "level " << static_cast<int>(level) << " placed nothing - the fixture is broken";
		const double share = static_cast<double>(ranged) / static_cast<double>(total);
		// Seed base 61000 matches A2PreChangeBaselineForHellfireLevels, so the printed
		// share is a same-seed A/B against the baseline printed beside it.
		std::cout << "[ A2MEASURED ] level " << static_cast<int>(level) << " ranged share " << share
		          << " (" << ranged << "/" << total << "), vanilla " << kVanillaShareBaseline[level]
		          << ", baseline " << kRangedShareBaseline[level]
		          << ", ceiling " << kRangedShareCeiling[level] << std::endl;
		// Primary criterion (contract §4.4, R4 inverted): the roster must not push a level's
		// ranged share above VANILLA, where vanilla is the same-seed legacy measurement in
		// kVanillaShareBaseline - see its comment: the historical kRangedShareBaseline array is
		// NOT that value (they differ by up to 14.6 points on a level). kRangedShareCeiling
		// stays as the ALARM line below: breaching it still fails, but it no longer authorises
		// trimming content to pass, because the density floors
		// (ContentDensityWithinVanillaFloor, HellfirePerSeedVariety*) are the first criterion.
		EXPECT_LE(share, kVanillaShareBaseline[level])
		    << "level " << static_cast<int>(level) << " ranged share is above vanilla";
		EXPECT_LE(share, kRangedShareCeiling[level])
		    << "level " << static_cast<int>(level) << " ranged share " << share
		    << " exceeds baseline " << kRangedShareBaseline[level] << " + 5pp"
		    << " (" << ranged << "/" << total << ")";
	}
}

// ---------------------------------------------------------------------------
// RB37 (spec appendix E item 9): a NON-SHARE guard for L17-24.
//
// The problem it solves, measured rather than assumed: on L17-24 the taxonomy
// GetBehaviorClass() applies puts RangedTurret at a constant 0 and routes most HF
// ranged casters into its Boss arm, so the ONLY monster that can move rangedShare
// across the whole band is BoneDemon (the one enumerated RangedKite). Acceptance 8 is
// therefore nearly formal there - it would not notice a roster that filled every one
// of the eight levels with Liches, ArchLiches, Psychorbs, Necromorbs, Firebats and
// Torchants. RB33 keeps the taxonomy unchanged (like-for-like with L1-16), so the
// second guard has to judge by something else.
//
// What this guard measures: the ranged-caster share of placed monsters computed from
// each monster's REAL ai - the same predicate the engine itself dispatches on. It is
// deliberately NOT a re-classification: GetBehaviorClass() is untouched, and this
// number is used by nothing but this case.
//
// Direction: an UPPER bound, per red line 14 ("the depth layer must not silently
// raise ranged pressure"). A floor would be the wrong instrument here - it would
// reward stuffing the levels with casters. The bound is a MEASURED separating line
// against the same pre-change fixture path the share baseline came from (both numbers
// are printed unconditionally, so the margin is always visible), plus the same 5-point
// band acceptance 8 uses, so the two guards fail for the same class of regression.
//
// Why it can actually fail (the property RB37 asks for), demonstrated rather than
// asserted: L18's roster draft originally cored MT_PSYCHORB, the level's only ranged
// caster. Measured with that ONE row changed and nothing else (200 seeds, same fixture):
//
//   [ A2CASTER   ] level 18 ranged-caster share 0.203546 (3295/16188) > ceiling 0.198629  -> RED
//   [ A2MEASURED ] level 18 ranged share       0        (0/16188)     <= ceiling 0.05     -> GREEN
//
// That is exactly the blind spot RB37 named: acceptance 8 cannot see a level whose real
// ranged pressure rose by 5.5 points, because Psychorb lands in GetBehaviorClass()'s
// Boss arm and rangedShare stays a flat zero. Per R4 the breach was fixed in the DATA -
// L18 cores MT_STINGER instead - not by raising the bound. The same swap-a-core-caster
// -for-a-non-caster fix was applied to L20 (MT_NECRMORB -> MT_LASHWORM) and L23
// (MT_HELLBAT -> MT_CRYPTDMN), which this guard also caught red on the first run.

// Ranged-caster share of placed monsters, by real ai. Every row below is transcribed
// from the [ A2CASTERBASE ] line A2PreChangeBaselineForHellfireLevels prints, i.e. it is
// accumulated on the SAME pre-change samples (L1-16-only fixtures via LoadPreA2Tables,
// 200 seeds per level, seed base 61000, placed-monster denominator) that produced
// kRangedShareBaseline[17..24]. That co-measurement is the point: a bound derived by
// arithmetic from the printed Boss column instead would be a DIFFERENT set - the Boss
// arm excludes the Acid/BoneDemon kites and includes non-caster AIs such as Scavenger -
// so the guard would compare two populations rather than run an A/B.
//
//   L17 0.213677 (3459/16188)   L18 0.148629 (2406/16188)
//   L19 0.375763 (6158/16388)   L20 0.558744 (9131/16342)
//   L21 0.420576 (9513/22619)   L22 0.417577 (9484/22712)
//   L23 0.439288 (9949/22648)   L24 0.510043 (11554/22653)
//
// These are MEASURED separating lines, not spec constants: the band is acceptance 8's
// own 5 points, so a roster that pushes real ranged pressure up by more than that on
// any of the eight levels fails here even when rangedShare cannot see it. Per R4 a
// breach is fixed in the DATA - never by raising these numbers.
constexpr std::array<double, 25> kRangedCasterShareBaseline {
	kRangedShareUnconstrained, // L0-L16 are not measured by this guard: it exists for
	kRangedShareUnconstrained, // the Hellfire band, where the GetBehaviorClass() gap
	kRangedShareUnconstrained, // makes acceptance 8 nearly formal (appendix E item 9).
	kRangedShareUnconstrained, // L1-16 keep acceptance 8 as their quantitative guard.
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	kRangedShareUnconstrained,
	3459.0 / 16188.0,  // L17 [ A2CASTERBASE ] 0.213677
	2406.0 / 16188.0,  // L18 [ A2CASTERBASE ] 0.148629
	6158.0 / 16388.0,  // L19 [ A2CASTERBASE ] 0.375763
	9131.0 / 16342.0,  // L20 [ A2CASTERBASE ] 0.558744
	9513.0 / 22619.0,  // L21 [ A2CASTERBASE ] 0.420576
	9484.0 / 22712.0,  // L22 [ A2CASTERBASE ] 0.417577
	9949.0 / 22648.0,  // L23 [ A2CASTERBASE ] 0.439288
	11554.0 / 22653.0, // L24 [ A2CASTERBASE ] 0.510043
};

TEST_F(HellfireLevelBaselineTest, HellfireRangedCasterShareWithinBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingHellfire_)
		GTEST_SKIP() << "Hellfire assets required: L17-24 do not exist without them";

	// Premise 1: the predicate must actually select something in this band. If every
	// candidate answered false, the measured share would be a constant 0 and the whole
	// case would be a tautology - the exact failure mode RB37 is guarding against in
	// acceptance 8, reproduced one level down.
	size_t casterTypesInBand = 0;
	for (uint8_t level = 17; level <= 24; level++) {
		for (size_t i = 0; i < MonstersData.size(); i++) {
			const MonsterData &data = MonstersData[i];
			if (data.availability == MonsterAvailability::Never)
				continue;
			if (level < data.minDunLvl || level > data.maxDunLvl)
				continue;
			if (IsRangedCasterAi(data.ai))
				casterTypesInBand++;
		}
	}
	ASSERT_GT(casterTypesInBand, 0u)
	    << "no L17-24 candidate has a ranged-caster ai: this guard would be a tautology";

	// Premise 2: this case judges the SHIPPED A2 tables. If the rows were missing the
	// measurement would silently fall back to the legacy path and re-measure the
	// baseline against itself.
	for (uint8_t level = 17; level <= 24; level++) {
		ASSERT_NE(GetLevelRosterParams(level), nullptr)
		    << "level " << static_cast<int>(level) << " has no shipped params row";
		ASSERT_FALSE(GetLevelRoster(level).empty())
		    << "level " << static_cast<int>(level) << " has no shipped roster rows";
	}

	constexpr uint32_t kSeeds = 200;
	for (uint8_t level = 17; level <= 24; level++) {
		ASSERT_LT(kRangedCasterShareBaseline[level], kRangedShareUnconstrained)
		    << "level " << static_cast<int>(level) << " is asserted but still holds the unconstrained"
		    << " sentinel; fill its measured row before adding it to this loop";

		size_t placed = 0;
		size_t casters = 0;
		std::set<MonsterAIID> casterAis;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			CreateDungeonForMeasurement(level, 61000 + seed);
			{
				const auto getTypesResult = GetLevelMTypes();
				ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
			}
			{
				const auto initResult = InitMonsters();
				ASSERT_TRUE(initResult.has_value()) << initResult.error();
			}
			ASSERT_GT(ActiveMonsterCount, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed << " placed no monster";
			for (size_t i = 0; i < ActiveMonsterCount; i++) {
				const Monster &monster = Monsters[ActiveMonsters[i]];
				if (!IsRangedCasterAi(monster.ai))
					continue;
				casters++;
				casterAis.insert(monster.ai);
			}
			placed += ActiveMonsterCount;
		}

		const double share = static_cast<double>(casters) / static_cast<double>(placed);
		const double ceiling = kRangedCasterShareBaseline[level] + kRangedShareTolerance;
		// Unconditional print: a passing run must still show the headroom, otherwise the
		// next roster edit has no reference point (same rule as [ MEASURED ]).
		std::cout << "[ A2CASTER ] level " << static_cast<int>(level) << " ranged-caster share " << share
		          << " (" << casters << "/" << placed << "), distinct caster ais " << casterAis.size()
		          << ", baseline " << kRangedCasterShareBaseline[level]
		          << ", ceiling " << ceiling << std::endl;
		EXPECT_LE(share, ceiling)
		    << "level " << static_cast<int>(level) << " ranged-caster share " << share
		    << " exceeds baseline " << kRangedCasterShareBaseline[level] << " + 5pp"
		    << " (" << casters << "/" << placed << "); per R4 fix the roster data, not this bound";
		// Per-level non-emptiness: the band-wide premise above cannot tell whether an
		// individual level realises a caster, and a level that realises none would be
		// judged against a bound it cannot approach.
		EXPECT_GT(casters, 0u)
		    << "level " << static_cast<int>(level) << " placed no ranged caster across " << kSeeds
		    << " seeds, so its bound is unreachable and measures nothing";
	}
}

// Per-seed variety guard for the Hellfire levels (content density contract §4.1 invariant:
// no level may be deterministic). Each level must offer at least two distinct type sets
// across seeds; the failure mode this catches is a tail pool that the level's caps and
// tail_draw reduce to a single combination (L17 measured 1 before its core was reshaped).
TEST_F(HellfireLevelBaselineTest, HellfirePerSeedVarietyHasAtLeastTwoCombinations)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found";
	if (missingHellfire_)
		GTEST_SKIP() << "Hellfire assets required";

	constexpr int kSeeds = 500;
	for (uint8_t level = 17; level <= 24; level++) {
		std::set<std::vector<_monster_id>> combinations;
		std::set<_monster_id> unionTypes;
		size_t typeSum = 0;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			leveltype = GetLevelType(level);
			InitLevelMonsters();
			SetRndSeed(41000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			typeSum += LevelMonsterTypeCount;
			std::vector<_monster_id> key;
			for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
				key.push_back(LevelMonsterTypes[i].type);
				unionTypes.insert(LevelMonsterTypes[i].type);
			}
			std::sort(key.begin(), key.end());
			combinations.insert(key);
		}
		std::cout << "[ HFVARIETY ] level " << static_cast<int>(level)
		          << " combinations " << combinations.size()
		          << " unionTypes " << unionTypes.size() << " (";
		for (const _monster_id t : unionTypes)
			std::cout << MonstersData[t].name << ",";
		std::cout << ")" << std::endl;
		// Density contract invariant: no level may collapse to a single per-seed combination.
		// (This assertion was claimed in an earlier commit but the patch that was supposed to
		// insert it silently no-op'd, so the case stayed green while L20 was deterministic.)
		EXPECT_GE(combinations.size(), 2u)
		    << "Hellfire level " << static_cast<int>(level)
		    << " is deterministic: every seed yields the same type set";
		// Band ceiling for the Hellfire levels as well (spec B2): the same dilution guard the
		// Cathedral/Caves/Hell bands assert in sampling_behavior_test.cpp, so no level in the
		// game can be pushed past "everything is a signature, so nothing is".
		EXPECT_LE(static_cast<double>(typeSum) / kSeeds, 8.0)
		    << "Hellfire level " << static_cast<int>(level) << " exceeds 8 types per run";
		for (const auto &c : combinations) {
			std::cout << "    combo:";
			for (const _monster_id t : c)
				std::cout << " " << MonstersData[t].name;
			std::cout << std::endl;
		}
	}
}

TEST_F(SquadPlacementTest, SquadRateMatchesMeasuredBaseline)
{
	// Task 2 of the pressure leg: the realised squad rate must match the pinned baseline, so a
	// mis-wired squad_chance (or a broken counter/placement path) shows up here. The baseline is
	// a recorded measurement (test/fixtures/pressure/squad_rates.txt), not a value derived from
	// the same table, otherwise retuning a level would silently pass.
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	gbIsSpawn = false;
	paths::SetPrefPath(paths::BasePath() + "test/fixtures/");
	TestInitGame();
	LoadMonsterData();
	LoadLevelRoster();

	std::ifstream ratesFile(paths::BasePath() + "../test/fixtures/pressure/squad_rates.txt");
	ASSERT_TRUE(ratesFile.is_open()) << "measured squad-rate baseline is missing";
	std::map<int, double> expected;
	std::string line;
	while (std::getline(ratesFile, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty() || line[0] == '#')
			continue;
		const size_t tab = line.find('\t');
		ASSERT_NE(tab, std::string::npos) << line;
		expected[std::stoi(line.substr(0, tab))] = std::stod(line.substr(tab + 1));
	}
	ASSERT_EQ(expected.size(), 15u) << "measured baseline should cover levels 1-15";

	constexpr int N = 100;
	constexpr double Tolerance = 0.05;
	for (const auto &[level, expectedRate] : expected) {
		ASSERT_NE(GetLevelRosterParams(static_cast<uint8_t>(level)), nullptr) << "level " << level;
		size_t eligible = 0, rolls = 0, built = 0;
		for (uint32_t seed = 0; seed < N; seed++) {
			CreateDungeonForMeasurement(static_cast<uint8_t>(level), seed);
			const auto getTypesResult = GetLevelMTypes();
			if (!getTypesResult.has_value())
				continue;
			const auto initResult = InitMonsters();
			if (!initResult.has_value())
				continue;
			const SquadRollCounters &stats = GetSquadRollStats();
			eligible += stats.eligibleCoreDraws;
			rolls += stats.rolls;
			built++;
		}
		ASSERT_EQ(built, N) << "level " << level << " did not build every seed";
		ASSERT_GE(eligible, 100u) << "level " << level << " produced too few eligible draws";
		const double rate = static_cast<double>(rolls) / static_cast<double>(eligible);
		std::cout << "[ SQUADRATE ] level " << level << " measured " << rate << " baseline " << expectedRate << std::endl;
		EXPECT_NEAR(rate, expectedRate, Tolerance)
		    << "level " << level << " squad rate moved (measured " << rate << ", baseline " << expectedRate << ")";
	}
}
