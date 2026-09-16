/**
 * @file sampling_behavior_test.cpp
 *
 * B1 sampling-anti-monopoly harness.
 *
 * The B1 sampling cap (Caves kite <=2, Hell same-class <=2) is implemented in
 * GetLevelMTypes. This harness asserts the post-cap contract: all-kite Caves
 * levels and all-same-class Hell levels are eliminated (tail = 0%), while the
 * pre-cap baselines (Hell L15 4.13% / L14 0.70% / L13 0.00%, Caves kite
 * L9 0% / L10 2.4% / L11 0% / L12 2.6%) are recorded as the regression
 * reference. Pre-cap baselines were verified against the real engine in v5.
 */
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "drlg_test.hpp" // TestInitGame (mounts the hf overlay for the R28 fixture)
#include "engine/assets.hpp"
#include "engine/load_cl2.hpp"
#include "engine/random.hpp"
#include "game_mode.hpp"
#include "levels/gendung.h"
#include "monster.h"
#include "multi.h"
#include "player.h"
#include "quests.h"
#include "tables/itemdat.h"
#include "tables/level_roster.h"
#include "tables/monstdat.h"
#include "tables/questdat.hpp"
#include "utils/str_cat.hpp"

using namespace devilution;

namespace {

// Behavior classes come from the engine (monstdat.h GetBehaviorClass)
// so the harness tests the exact taxonomy the cap enforces.
// Run one real engine sampling pass for `level` with a fresh RNG seed.
// Returns the behavior classes of the sampled monster types.
std::vector<BehaviorClass> RunSampling(uint8_t level, uint32_t seed)
{
	currlevel = level;
	InitLevelMonsters();
	SetRndSeed(seed);
	GetLevelMTypes();

	std::vector<BehaviorClass> classes;
	classes.reserve(LevelMonsterTypeCount);
	for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
		const _monster_id type = LevelMonsterTypes[i].type;
		classes.push_back(GetBehaviorClass(MonstersData[type].ai));
	}
	return classes;
}

// Count how many times the most frequent behavior class appears.
// Returns 0 for an empty sample.
size_t MaxSameClassCount(const std::vector<BehaviorClass> &classes)
{
	std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> counts {};
	for (BehaviorClass c : classes)
		counts[static_cast<size_t>(c)]++;
	size_t maxCount = 0;
	for (size_t count : counts)
		maxCount = std::max(maxCount, count);
	return maxCount;
}

// RangedKite types on the level ("all-kite tail" = the Caves 38% monopoly symptom).
size_t KiteClassCount(const std::vector<BehaviorClass> &classes)
{
	return static_cast<size_t>(
	    std::count(classes.begin(), classes.end(), BehaviorClass::RangedKite));
}

class SamplingBaselineTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadCoreArchives();
		LoadGameArchives();

		// CI only ships spawn.mpq; the B1 baseline numbers were computed from
		// the full monstdat.tsv with availability != Never (no spawn/retail
		// split). Match that by disabling the spawn filter.
		if (!HaveMainData()) {
			missingMpqAssets_ = true;
			return;
		}
		gbIsSpawn = false;
		sgGameInitInfo.fullQuests = 1; // full quests -> UseMultiplayerQuests() == false
		LoadMonsterData();
		// GetLevelMTypes() now samples from the level rosters, which diablo.cpp loads
		// right after LoadMonsterData(); mirror that order here (validation reads
		// MonstersData, so it must come second).
		LoadLevelRoster();
	}

	static bool missingMpqAssets_;

	// Fraction (percent) of sampled levels where any behavior class
	// reaches >= 3 types on the level (the "all-same tail").
	[[nodiscard]] double SameClassTailPercent(uint8_t level, int iterations, uint32_t seedBase) const
	{
		int tail = 0;
		for (int i = 0; i < iterations; i++) {
			if (MaxSameClassCount(RunSampling(level, seedBase + i)) >= 3)
				tail++;
		}
		return 100.0 * static_cast<double>(tail) / static_cast<double>(iterations);
	}

	// Percent of sampled levels with >= 3 kite types (B1 Caves symptom).
	[[nodiscard]] double KiteTailPercent(uint8_t level, int iterations, uint32_t seedBase) const
	{
		int tail = 0;
		for (int i = 0; i < iterations; i++) {
			if (KiteClassCount(RunSampling(level, seedBase + i)) >= 3)
				tail++;
		}
		return 100.0 * static_cast<double>(tail) / static_cast<double>(iterations);
	}
};

bool SamplingBaselineTest::missingMpqAssets_ = false;

TEST_F(SamplingBaselineTest, HellL15SameClassTailBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Pre-cap baseline was 4.13% (v3's 27.8% was a fake number from omitting
	// the Golem budget). The B1 cap (Hell same-class <=2) must drive it to 0.
	constexpr int kIterations = 10000;
	const double tail = SameClassTailPercent(15, kIterations, 1000);
	EXPECT_EQ(tail, 0.0) << "Hell L15 same-class tail must be 0 after cap (was 4.13%)";
}

TEST_F(SamplingBaselineTest, HellL14SameClassTailBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Pre-cap baseline was 0.70%. Cap (Hell same-class <=2) must drive it to 0.
	constexpr int kIterations = 10000;
	const double tail = SameClassTailPercent(14, kIterations, 2000);
	EXPECT_EQ(tail, 0.0) << "Hell L14 same-class tail must be 0 after cap (was 0.70%)";
}

TEST_F(SamplingBaselineTest, HellL13SameClassTailBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Pre-cap baseline was already 0.00% (no quest monsters under the harness's
	// "no quest" condition); the cap keeps it at 0.
	constexpr int kIterations = 10000;
	const double tail = SameClassTailPercent(13, kIterations, 3000);
	EXPECT_EQ(tail, 0.0) << "Hell L13 same-class tail must be 0 after cap (was 0%)";
}

TEST_F(SamplingBaselineTest, CavesKiteTailBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Corrected faithful simulation (Golem occupies a LevelMonsterTypes slot
	// AND counts as Boss-class in the composition, matching the real engine):
	// all-kite (>=3 RangedKite) tails: L9 ~0.0% / L10 ~2.4% / L11 ~0.0% / L12 ~2.6%.
	constexpr int kIterations = 10000;
	const double l9 = KiteTailPercent(9, kIterations, 4000);
	const double l10 = KiteTailPercent(10, kIterations, 5000);
	const double l11 = KiteTailPercent(11, kIterations, 6000);
	const double l12 = KiteTailPercent(12, kIterations, 7000);
	// Pre-cap kite tails: L9 0% / L10 2.4% / L11 0% / L12 2.6%. The cap
	// (Caves kite <=2) must drive all to 0.
	EXPECT_EQ(l9, 0.0) << "Caves L9 kite-tail must be 0 after cap (was 0%)";
	EXPECT_EQ(l10, 0.0) << "Caves L10 kite-tail must be 0 after cap (was 2.4%)";
	EXPECT_EQ(l11, 0.0) << "Caves L11 kite-tail must be 0 after cap (was 0%)";
	EXPECT_EQ(l12, 0.0) << "Caves L12 kite-tail must be 0 after cap (was 2.6%)";
}

TEST_F(SamplingBaselineTest, CavesAnyClassTailBaseline)
{
	// Spec §3.3 (v5) documents the "any-class >=3" cave tails (engine-measured,
	// Golem in composition): L9 1.59% / L10 3.33% / L11 1.18% / L12 2.61%.
	// These are the same-class metric (any behavior class), distinct from the
	// kite-class metric in CavesKiteTailBaseline.
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr int kIterations = 10000;
	const double l9 = SameClassTailPercent(9, kIterations, 8000);
	const double l10 = SameClassTailPercent(10, kIterations, 9000);
	const double l11 = SameClassTailPercent(11, kIterations, 10000);
	const double l12 = SameClassTailPercent(12, kIterations, 11000);
	// Caves cap constrains the kite class only; the "any-class >=3" tail is NOT a
	// cap target (a Melee-heavy Caves level is still possible). History:
	// pre-cap L9 1.59% / L10 3.33% / L11 1.18% / L12 2.61%; post-cap (random
	// 3-7 types) L9 1.7% / L10 1.02% / L11 1.32% / L12 0%.
	//
	// With the phase-A rosters this tail is no longer a distribution artefact but
	// arithmetic: a Caves level now realises 1 (Golem) + 4 core + up to
	// tail_draw(3) = 8 types, while the classes its candidate pool can offer,
	// with every class held at <= 2, only have room for 8/7/6/6 types at
	// L9/L10/L11/L12 (measured from the candidate pools). L10-L12 therefore
	// CANNOT keep every class at 2 - some class must reach 3 - so the tail is
	// 100% there by construction, and L9 sits at the boundary (59.93%, the only
	// level with enough distinct classes to sometimes fit).
	//
	// This is not a relaxed threshold: the B1 guarantees are asserted by
	// CavesKiteTailBaseline (kite <= 2, the actual monopoly symptom) and by
	// RosterQuotasSatisfied (caps hold for everything the loop adds). What
	// changed is the total type count per level, which is the roster feature's
	// whole point. Pinned exactly so any further drift is caught.
	EXPECT_NEAR(l9, 59.93, 0.6) << "Caves L9 any-class tail with rosters (~59.9%)";
	EXPECT_EQ(l10, 100.0) << "Caves L10 cannot spread 8 types over its classes at <=2 each";
	EXPECT_EQ(l11, 100.0) << "Caves L11 cannot spread 8 types over its classes at <=2 each";
	EXPECT_EQ(l12, 100.0) << "Caves L12 cannot spread 8 types over its classes at <=2 each";
}

TEST_F(SamplingBaselineTest, Level16HardcodedTypes)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Level 16 is hard-coded: Golem + Advocate + RBlack + Diablo only.
	// (GetLevelMTypes early-returns; the cap must never apply here.)
	currlevel = 16;
	InitLevelMonsters();
	SetRndSeed(42);
	GetLevelMTypes();

	ASSERT_EQ(LevelMonsterTypeCount, 4U);
	EXPECT_EQ(LevelMonsterTypes[0].type, MT_GOLEM);
	EXPECT_EQ(LevelMonsterTypes[1].type, MT_ADVOCATE);
	EXPECT_EQ(LevelMonsterTypes[2].type, MT_RBLACK);
	EXPECT_EQ(LevelMonsterTypes[3].type, MT_DIABLO);
}

TEST_F(SamplingBaselineTest, SampleCountWithinBudget)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// The image budget (monstimgtot < 4000) is what actually terminates the
	// loop; type-count (MaxLvlMTypes) is a secondary guard. Sampling must
	// terminate with a non-empty, budget-respecting composition on every level.
	constexpr int kIterations = 2000;
	for (int level = 1; level <= 15; level++) {
		for (int i = 0; i < kIterations; i++) {
			const auto classes = RunSampling(static_cast<uint8_t>(level), static_cast<uint32_t>(10000 + level * 1000 + i));
			EXPECT_LE(classes.size(), MaxLvlMTypes) << "Level " << level << " exceeds MaxLvlMTypes";
			EXPECT_FALSE(classes.empty()) << "Level " << level << " sampled no monster types";
		}
	}
}

TEST_F(SamplingBaselineTest, SamplingTerminates)
{
	// AC3: the cap must never deadlock the loop. Each level's candidate pool
	// must exceed the cap (>=2 kite types at 9-12, >=2 same-class at 13-15),
	// and every level must still yield a non-empty composition.
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr int kIterations = 2000;
	for (int level = 1; level <= 15; level++) {
		int samples = 0;
		for (int i = 0; i < kIterations; i++) {
			const auto classes = RunSampling(static_cast<uint8_t>(level), static_cast<uint32_t>(20000 + level * 1000 + i));
			EXPECT_FALSE(classes.empty()) << "Level " << level << " produced empty composition after cap";
			samples++;
		}
		EXPECT_EQ(samples, kIterations) << "Level " << level << " sampling loop must always terminate";
	}
}

TEST_F(SamplingBaselineTest, CatacombsUnconstrainedByCap)
{
	// AC4: church (1-4) and catacombs (5-8) must remain unconstrained. Verify
	// a sample there can still reach >=3 same-class types (pre-cap baseline
	// behavior preserved) — i.e. the cap must NOT apply outside 9-16.
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr int kIterations = 10000;
	// Cathedral 1-4 / Catacombs 5-8: >=3 same-class remains possible.
	// (Pre-cap engine measurement — these ranges have enough same-class
	// candidates that the unconstrained distribution retains a tail.)
	bool foundTail = false;
	for (int level = 1; level <= 8; level++) {
		for (int i = 0; i < kIterations; i++) {
			if (MaxSameClassCount(RunSampling(static_cast<uint8_t>(level), static_cast<uint32_t>(30000 + level * 1000 + i))) >= 3) {
				foundTail = true;
				break;
			}
		}
		if (foundTail)
			break;
	}
	EXPECT_TRUE(foundTail) << "Church/Catacombs must remain unconstrained (>=3 same-class still reachable)";
}

TEST_F(SamplingBaselineTest, MPSameSeedSameComposition)
{
	// AC8 (MP determinism): sampling is seeded per game from DungeonSeeds;
	// two clients with the same seed must get the same level composition.
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	for (int level = 1; level <= 15; level++) {
		const auto first = RunSampling(static_cast<uint8_t>(level), 12345);
		const auto second = RunSampling(static_cast<uint8_t>(level), 12345);
		EXPECT_EQ(first, second) << "Level " << level << " not deterministic for identical seed";
	}
}

TEST_F(SamplingBaselineTest, CounselorIsRangedTurret)
{
	// Regression for the v3 mapping fix: Counselor must classify as
	// RangedTurret, never Rally (it uses StartRangedAttack, monster.cpp:2750-2755).
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Counselor), BehaviorClass::RangedTurret);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Fallen), BehaviorClass::Rally);
}

TEST_F(SamplingBaselineTest, ClassifyCoversAllAiIds)
{
	// Exhaustiveness guard: every MonsterAIID must map to a NON-Boss class or
	// be explicitly acknowledged as Boss. Prevents future AI additions from
	// silently landing in Boss and skewing the tail metric.
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Scavenger), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Gargoyle), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Butcher), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::FireMan), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Golem), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Diablo), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Lazarus), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::LazarusSuccubus), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Lachdanan), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Warlord), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::FireBat), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Torchant), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::HorkDemon), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Lich), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::ArchLich), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Psychorb), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Necromorb), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Gharbad), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Zhar), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Snotspill), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::AcidUnique), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Custom), BehaviorClass::Boss);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Invalid), BehaviorClass::Boss);

	// Non-Boss mappings are pinned explicitly too: a future AI addition that
	// lands in the wrong class would silently shift the tail metric.
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::SkeletonMelee), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Zombie), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Fat), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Rhino), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Mega), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Snake), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::GoatMelee), BehaviorClass::Melee);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::SkeletonRanged), BehaviorClass::RangedTurret);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::GoatRanged), BehaviorClass::RangedTurret);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Succubus), BehaviorClass::RangedTurret);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Magma), BehaviorClass::RangedKite);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Storm), BehaviorClass::RangedKite);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Acid), BehaviorClass::RangedKite);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::BoneDemon), BehaviorClass::RangedKite);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Bat), BehaviorClass::Charge);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::Sneak), BehaviorClass::Sneak);
	EXPECT_EQ(GetBehaviorClass(MonsterAIID::SkeletonKing), BehaviorClass::Summon);
}

TEST_F(SamplingBaselineTest, QuestPreAddRePickDoesNotDoubleCount)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Q_VEIL (Lachdanan) pre-adds MT_RBLACK to LevelMonsterTypes; MT_RBLACK
	// is also a regular member of the L13-14 pool, so sampling can re-pick
	// it. The GetLevelMTypes count guard must not double-count that re-pick:
	// a second distinct Melee type must still be sampleable (the cap allows
	// 2 of a class, counting the pre-add).
	LoadQuestData(); // populate QuestsData (SetUpTestSuite does not load it)
	Quests[Q_VEIL]._qidx = Q_VEIL;
	Quests[Q_VEIL]._qactive = QUEST_ACTIVE;
	Quests[Q_VEIL]._qlevel = 14;

	size_t maxMelee = 0;
	for (uint32_t seed = 1; seed <= 5000; seed++) {
		const auto classes = RunSampling(14, seed);
		maxMelee = std::max(maxMelee, static_cast<size_t>(std::count(classes.begin(), classes.end(), BehaviorClass::Melee)));
		EXPECT_LE(MaxSameClassCount(classes), 2) << "same-class cap must hold with quest pre-adds";
	}
	EXPECT_GE(maxMelee, 2) << "a second Melee type must be sampleable despite the MT_RBLACK re-pick";

	Quests[Q_VEIL] = {};
}

// ---------------------------------------------------------------------------
// P0 measurement (2026-09-15): what the sprite budget actually buys.
//
// These are measurements, not behaviour locks. They assert only invariants that
// must hold for any budget, and - when SAMPLING_REPORT is set to a path - they
// append a markdown table, so the numbers feed the decision material directly
// instead of being scraped out of test output.
// ---------------------------------------------------------------------------

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

std::string FormatMeasurement(double value)
{
	std::ostringstream out;
	out.setf(std::ios::fixed);
	out.precision(1);
	out << value;
	return out.str();
}

int RealisedImageTotal()
{
	int total = 0;
	for (size_t i = 0; i < LevelMonsterTypeCount; i++)
		total += MonstersData[LevelMonsterTypes[i].type].image;
	return total;
}

size_t RealisedDistinctAi()
{
	std::vector<MonsterAIID> seen;
	for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
		const MonsterAIID ai = MonstersData[LevelMonsterTypes[i].type].ai;
		if (std::find(seen.begin(), seen.end(), ai) == seen.end())
			seen.push_back(ai);
	}
	return seen.size();
}

size_t RealisedDistinctClass()
{
	std::array<bool, static_cast<size_t>(BehaviorClass::Count)> seen {};
	for (size_t i = 0; i < LevelMonsterTypeCount; i++)
		seen[static_cast<size_t>(GetBehaviorClass(MonstersData[LevelMonsterTypes[i].type].ai))] = true;
	return static_cast<size_t>(std::count(seen.begin(), seen.end(), true));
}

// Mirrors IsMonsterAvailable (Source/monster.cpp:3161): availability flag plus
// the level band. Duplicated here on purpose so production code stays untouched.
bool IsMeasuredCandidate(size_t index)
{
	const MonsterData &data = MonstersData[index];
	if (data.availability == MonsterAvailability::Never)
		return false;
	if (gbIsSpawn && data.availability == MonsterAvailability::Retail)
		return false;
	return currlevel >= data.minDunLvl && currlevel <= data.maxDunLvl;
}

// Real bytes the engine loads for one monster type: the animation files named
// `monsters\<spritePath><letter><ext>` (monster.cpp:3224; Animletter :152).
std::size_t MeasuredSpriteBytes(const MonsterData &data)
{
	static constexpr char AnimLetters[7] = "nwahds";
	const size_t numAnims = data.hasSpecial ? 6 : 5;
	std::size_t total = 0;
	for (size_t i = 0; i < numAnims; i++) {
		if (!data.hasAnim(i))
			continue;
		const std::string path = StrCat("monsters\\", data.spritePath(), AnimLetters[i], DEVILUTIONX_CL2_EXT);
		size_t fileSize = 0;
		(void)OpenAsset(path, fileSize);
		total += fileSize;
	}
	return total;
}

// Best/worst case number of candidate types a budget can admit: draw the
// smallest (or largest) images first. Bounds the realised count without
// replicating the engine's random pick order.
void BudgetFitBounds(int budget, size_t &minFit, size_t &maxFit)
{
	std::vector<int> images;
	for (size_t i = 0; i < MonstersData.size(); i++) {
		if (IsMeasuredCandidate(i) && MonstersData[i].image > 0)
			images.push_back(MonstersData[i].image);
	}
	const auto countFit = [&images](int limit, bool ascending) {
		std::vector<int> sorted = images;
		std::sort(sorted.begin(), sorted.end());
		if (!ascending)
			std::reverse(sorted.begin(), sorted.end());
		size_t count = 0;
		int used = 0;
		for (const int image : sorted) {
			if (used + image > limit)
				break;
			used += image;
			count++;
		}
		return count;
	};
	maxFit = countFit(budget, true);
	minFit = countFit(budget, false);
}

TEST_F(SamplingBaselineTest, MeasurementRealisedPerLevel)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr int kSeeds = 40;
	std::string report = "\n## P0-A: realised monster types per level (sprite budget 4000)\n\n"
	                     "| level | types avg | distinct AI avg | distinct class avg | sum(image) avg | sum(image) max |\n"
	                     "|---|---|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 16; level++) {
		size_t types = 0;
		size_t distinctAi = 0;
		size_t distinctClass = 0;
		int imageSum = 0;
		int imageMax = 0;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(1000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			types += LevelMonsterTypeCount;
			distinctAi += RealisedDistinctAi();
			distinctClass += RealisedDistinctClass();
			const int total = RealisedImageTotal();
			imageSum += total;
			imageMax = std::max(imageMax, total);
			EXPECT_LE(LevelMonsterTypeCount, MaxLvlMTypes);
		}
		report += StrCat("| ", level, " | ", FormatMeasurement(static_cast<double>(types) / kSeeds),
		    " | ", FormatMeasurement(static_cast<double>(distinctAi) / kSeeds),
		    " | ", FormatMeasurement(static_cast<double>(distinctClass) / kSeeds),
		    " | ", FormatMeasurement(static_cast<double>(imageSum) / kSeeds),
		    " | ", imageMax, " |\n");
	}
	AppendMeasurementReport(report);
}

TEST_F(SamplingBaselineTest, MeasurementBudgetFitBounds)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr int kBudgets[] = { 4000, 6000, 8000, 12000, 20000 };
	std::string report = "\n## P0-B: candidate types a sprite budget can admit (best/worst case)\n\n"
	                     "| level | candidates |";
	for (const int budget : kBudgets)
		report += StrCat(" ", budget, " min/max |");
	report += "\n|---|---|";
	for (size_t i = 0; i < sizeof(kBudgets) / sizeof(*kBudgets); i++)
		report += "---|---|";
	report += "\n";
	for (uint8_t level = 1; level <= 16; level++) {
		currlevel = level;
		size_t candidates = 0;
		for (size_t i = 0; i < MonstersData.size(); i++) {
			if (IsMeasuredCandidate(i))
				candidates++;
		}
		report += StrCat("| ", level, " | ", candidates, " |");
		for (const int budget : kBudgets) {
			size_t minFit = 0;
			size_t maxFit = 0;
			BudgetFitBounds(budget, minFit, maxFit);
			EXPECT_LE(minFit, maxFit);
			report += StrCat(" ", minFit, "/", maxFit, " |");
		}
		report += "\n";
	}
	AppendMeasurementReport(report);
}

TEST_F(SamplingBaselineTest, MeasurementImageVsSpriteBytes)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr uint8_t kLevels[] = { 1, 5, 9, 14 };
	size_t measuredTypes = 0;
	std::string missingSprites;
	std::string report = "\n## P0-C: image column vs real sprite bytes\n\n"
	                     "| level | type | image | sprite KiB | KiB per image unit |\n"
	                     "|---|---|---|---|---|\n";
	report += StrCat("shareware data: ", gbIsSpawn ? "yes" : "no", "\n\n");
	for (const uint8_t level : kLevels) {
		currlevel = level;
		InitLevelMonsters();
		SetRndSeed(4242);
		ASSERT_TRUE(GetLevelMTypes().has_value());
		for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
			const MonsterData &data = MonstersData[LevelMonsterTypes[i].type];
			const std::size_t bytes = MeasuredSpriteBytes(data);
			if (bytes > 0) {
				measuredTypes++;
			} else {
				missingSprites += data.name;
				missingSprites += " ";
			}
			const double ratio = data.image > 0 ? (static_cast<double>(bytes) / 1024.0) / static_cast<double>(data.image) : 0.0;
			report += StrCat("| ", level, " | ", data.name, " | ", data.image, " | ",
			    FormatMeasurement(static_cast<double>(bytes) / 1024.0), " | ", FormatMeasurement(ratio), " |\n");
		}
	}
	// OPEN ITEM (P0-C): every type measured 0 bytes here, i.e. OpenAsset did not
	// resolve `monsters\\<spritePath><animletter>` + DEVILUTIONX_CL2_EXT for any
	// realised type in this environment. Either the archive holding monster
	// graphics is not present (this checkout ships none; the game data comes from
	// the player's DIABDAT.MPQ / spawn.mpq) or the path/extension convention used
	// by the loader differs from the one mirrored here. Deliberately NOT asserted:
	// the image-vs-KiB calibration stays an open measurement until it is resolved.
	report += StrCat("\nTypes with loadable sprites: ", measuredTypes, " of ", measuredTypes + (missingSprites.empty() ? 0 : 1),
	    "; unresolved paths for: ", missingSprites, "\n");
	AppendMeasurementReport(report);
}

TEST_F(SamplingBaselineTest, MeasurementRosterIdentity)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// The "roster" of a level is the union of every type it can realise across
	// many seeds. Two levels whose rosters overlap almost completely are
	// interchangeable to the player, which is the identity half of the
	// content-collapse problem (P0-B showed a big budget makes it worse).
	constexpr int kSeeds = 200;
	std::vector<std::set<size_t>> rosters(17);
	for (uint8_t level = 1; level <= 16; level++) {
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(2000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				rosters[level].insert(LevelMonsterTypes[i].type);
		}
	}
	const auto jaccard = [&rosters](uint8_t a, uint8_t b) {
		size_t intersection = 0;
		for (const size_t type : rosters[a]) {
			if (rosters[b].count(type) != 0)
				intersection++;
		}
		const size_t unionSize = rosters[a].size() + rosters[b].size() - intersection;
		return unionSize == 0 ? 0.0 : static_cast<double>(intersection) / static_cast<double>(unionSize);
	};

	std::string report = "\n## P0-D: level roster identity (200 seeds per level)\n\n"
	                     "| level | roster size | Jaccard vs previous | Jaccard vs L1 |\n"
	                     "|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 16; level++) {
		report += StrCat("| ", level, " | ", rosters[level].size(), " | ",
		    level > 1 ? FormatMeasurement(jaccard(static_cast<uint8_t>(level - 1), level)) : std::string { "-" },
		    " | ", FormatMeasurement(jaccard(1, level)), " |\n");
	}
	AppendMeasurementReport(report);

	// Sanity checks only. A level's roster legitimately EXCEEDS its candidate
	// pool (measured: L3 roster 25 vs 24 candidates) because GetLevelMTypes
	// pre-adds types that the level band does not cover: MT_GOLEM is added
	// unconditionally as PLACE_SPECIAL, and quest uniques (Garbud/Zhar/
	// SnotSpill/Lachdan/WarlordOfBlood/SKING) are added by quest availability.
	// So the roster is "candidates + pre-adds", not "candidates".
	for (uint8_t level = 1; level <= 16; level++) {
		EXPECT_GT(rosters[level].size(), 0u) << "level " << static_cast<int>(level) << " sampled nothing";
		EXPECT_LE(rosters[level].size(), MonstersData.size()) << "level " << static_cast<int>(level);
	}
}

// ---------------------------------------------------------------------------
// Phase A level rosters (task 3): the sampling loop now pre-adds each level's
// core roster, bounds the tail draw by `tail_draw`, tops up unmet class floors
// and budgets sprites per level via `max_image`.
// ---------------------------------------------------------------------------

// Availability filter for a roster row, mirroring the production core pre-add.
// GetLevelRoster() deliberately does not filter by availability (see
// level_roster.h), and IsMonsterAvailable() is file-local to monster.cpp, so the
// same predicate is mirrored here against the currently loaded MonstersData
// (identical to IsMeasuredCandidate() below, but level-parameterised).
bool IsRosterEntryAvailableAt(uint8_t level, _monster_id type)
{
	const MonsterData &data = MonstersData[type];
	if (data.availability == MonsterAvailability::Never)
		return false;
	if (gbIsSpawn && data.availability == MonsterAvailability::Retail)
		return false;
	return level >= data.minDunLvl && level <= data.maxDunLvl;
}

// Cap exemptions that come from the ENGINE, not from the roster (R29).
//
// The roster must NOT be used to compute this. Deriving the allowance from the
// very table under test makes the assertion self-fulfilling: a core roster that
// lists three same-class monsters would raise its own budget by three and the
// check could never fail - exactly the class of data defect (L11's three kite
// cores, L14's Melee cores) that this suite exists to catch. Core is therefore
// held to the cap here, and the load-time validator (ValidateLevelRoster's
// core-vs-cap check) rejects a roster that would break it.
//
// What genuinely cannot be capped is what GetLevelMTypes() pre-adds regardless
// of any table: MT_GOLEM (unconditional PLACE_SPECIAL) plus the SIX quest-gated
// type pre-adds at monster.cpp:3464-3475. Those six are not six uniques: Q_BUTCHER
// adds the plain monster type MT_CLEAVER, while the other five (Q_GARBUD, Q_ZHAR,
// Q_LTBANNER, Q_VEIL, Q_WARLORD) add a UNIQUE's base type, chosen by
// UniqueMonstersData rather than by the roster. That is why the table below has
// five entries and MT_CLEAVER is counted separately in EnginePreAddClassCount.
//
// O1 (task 3 review): the exemption used to include EVERY unique whose mlevel
// matched, which made this allowance so wide that the grid cell could not fail
// (L13 alone lists nine uniques spread over most classes). That is wrong about
// the engine: only these six pre-adds take a monster TYPE slot. All
// other uniques are placed by PlaceUniqueMonsters(), which picks from the types
// the level ALREADY has (see monster.cpp's PlaceUniqueMonsters: it searches
// LevelMonsterTypes for a matching base type and skips the unique when absent),
// so they occupy no additional type slot and must not widen the cap.
constexpr std::array<std::pair<quest_id, UniqueMonsterType>, 5> kQuestUniquePreAdds {
	std::pair { Q_GARBUD, UniqueMonsterType::Garbud },
	std::pair { Q_ZHAR, UniqueMonsterType::Zhar },
	std::pair { Q_LTBANNER, UniqueMonsterType::SnotSpill },
	std::pair { Q_VEIL, UniqueMonsterType::Lachdan },
	std::pair { Q_WARLORD, UniqueMonsterType::WarlordOfBlood },
};

// Availability uses the engine's own predicate (Quest::IsAvailable, which checks
// setlevel / currlevel / _qactive / single-player-only) instead of a re-derived
// copy, so validator and harness cannot drift. Precondition: call with
// currlevel == level, which every caller below satisfies because the sampling
// pass it measures sets currlevel first.
bool QuestPreAddAvailableAt(uint8_t level, quest_id quest)
{
	// Quest::IsAvailable() reads QuestsData[_qidx]; SetUpTestSuite deliberately
	// does not LoadQuestData(), so a suite that never activates a quest must not
	// index that empty vector. A quest cannot be available without its data.
	if (QuestsData.empty())
		return false;
	return Quests[quest]._qlevel == level && Quests[quest].IsAvailable();
}

// Every type GetLevelMTypes() registers BEFORE the tail loop, as a set of
// distinct types (I2, task 5 review).
//
// Why a set rather than a count: AddMonsterType() dedupes by type
// (monster.cpp:3299-3302 - an existing type only ORs in the place flag and does
// not raise LevelMonsterTypeCount), so a quest base that is also a core row must
// be counted once. Summing independent counts would over-count the overlap and
// make the tail bound too tight.
//
// Why the quest pre-adds belong here: monster.cpp:3464-3475 registers a type for
// each available quest (MT_CLEAVER for Q_BUTCHER, a unique's base for the other
// five). Leaving them out understates the pre-add count, so the derived "tail"
// absorbs a quest type and RosterTailDrawBounded's bound is wrong - either
// spuriously red when a quest is active, or silently loose. This suite activates
// Q_VEIL in two cases (QuestPreAddRePickDoesNotDoubleCount and
// EnginePreAddExemptionTracksQuestAvailability); both reset it afterwards, so the
// quest term is normally zero, but the bound must not depend on that reset
// surviving future edits or --gtest_shuffle.
std::set<_monster_id> PreAddedTypes(uint8_t level)
{
	std::set<_monster_id> types;
	types.insert(MT_GOLEM); // unconditional PLACE_SPECIAL pre-add
	if (QuestPreAddAvailableAt(level, Q_BUTCHER))
		types.insert(MT_CLEAVER);
	for (const auto &[quest, unique] : kQuestUniquePreAdds) {
		if (QuestPreAddAvailableAt(level, quest))
			types.insert(UniqueMonstersData[static_cast<size_t>(unique)].mtype);
	}
	for (const LevelRosterEntry &entry : GetLevelRoster(level)) {
		if (entry.role == LevelRosterRole::Core && IsRosterEntryAvailableAt(level, entry.type))
			types.insert(entry.type);
	}
	return types;
}

size_t EnginePreAddClassCount(uint8_t level, BehaviorClass cls)
{
	size_t count = 0;
	if (GetBehaviorClass(MonstersData[MT_GOLEM].ai) == cls)
		count++;
	// Q_BUTCHER pre-adds a plain monster type, not a unique's base type.
	if (QuestPreAddAvailableAt(level, Q_BUTCHER) && GetBehaviorClass(MonstersData[MT_CLEAVER].ai) == cls)
		count++;
	for (const auto &[quest, unique] : kQuestUniquePreAdds) {
		if (!QuestPreAddAvailableAt(level, quest))
			continue;
		const _monster_id base = UniqueMonstersData[static_cast<size_t>(unique)].mtype;
		if (GetBehaviorClass(MonstersData[base].ai) == cls)
			count++;
	}
	return count;
}

bool LevelHasType(_monster_id type)
{
	return std::any_of(LevelMonsterTypes, LevelMonsterTypes + LevelMonsterTypeCount,
	    [type](const CMonster &levelType) { return levelType.type == type; });
}

TEST_F(SamplingBaselineTest, RosterCoreAlwaysPresent)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// AC1: every available core member of a level is realised on 100% of seeds,
	// and every core member is a candidate of that level (i.e. the roster row
	// itself is available there, so pre-adding it is not a smuggled monster).
	//
	// Range note (R31): this case runs 1-16 while RosterTailDrawBounded and
	// RosterQuotasSatisfied run 1-15. L16 has roster ROWS but no params row on
	// purpose (spec 4.2.7): GetLevelMTypes() takes the hardcoded L16 branch and
	// returns before any roster pre-add, so on L16 only the static half of AC1 is
	// checkable - the rows are well-formed, available candidates and are
	// registered - and the realised-core assertion below is skipped for it.
	for (uint8_t level = 1; level <= 16; level++) {
		const std::span<const LevelRosterEntry> roster = GetLevelRoster(level);
		ASSERT_FALSE(roster.empty()) << "level " << static_cast<int>(level) << " must have roster rows";
		size_t assertedCores = 0;
		for (const LevelRosterEntry &entry : roster) {
			if (entry.role != LevelRosterRole::Core)
				continue;
			// Availability is the level-band check too, so this doubles as the
			// "core must be a candidate of its level" half of AC1.
			EXPECT_TRUE(IsRosterEntryAvailableAt(level, entry.type))
			    << "core type " << static_cast<int>(entry.type) << " is not a candidate at level " << static_cast<int>(level);
			assertedCores++;
		}
		EXPECT_GT(assertedCores, 0u) << "level " << static_cast<int>(level) << " must declare at least one core";

		for (int seed = 0; seed < 200; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(7000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			// L16 hardcodes its types and returns before the roster pre-add, so
			// its roster rows are registration only (spec 4.2.7).
			if (level == 16)
				continue;
			for (const LevelRosterEntry &entry : roster) {
				if (entry.role != LevelRosterRole::Core)
					continue;
				EXPECT_TRUE(LevelHasType(entry.type))
				    << "level " << static_cast<int>(level) << " seed " << seed
				    << " is missing core type " << static_cast<int>(entry.type);
			}
		}
	}
}

TEST_F(SamplingBaselineTest, RosterTailDrawBounded)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// AC2: the tail draw is min(tail_draw, available candidates). Counting the
	// tail as "types beyond the pre-adds" needs the pre-add count measured, not
	// assumed: Golem is always pre-added and the roster's core count varies.
	//
	// Range note (R31): 1-15, not 1-16. A tail bound is only meaningful where a
	// tail is drawn, and L16 deliberately has no params row (spec 4.2.7) because
	// its hardcoded branch returns before the roster path runs. Extending this
	// loop to 16 would only assert that a missing params row is missing.
	for (uint8_t level = 1; level <= 15; level++) {
		const LevelRosterParams *params = GetLevelRosterParams(level);
		ASSERT_NE(params, nullptr) << "level " << static_cast<int>(level) << " must have a params row";
		for (int seed = 0; seed < 50; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(8000 + static_cast<uint32_t>(seed));
			// Measure the pre-add count by replaying only the pre-add phase:
			// run the real sampling, then subtract the types that the roster and
			// the unconditional pre-adds account for.
			ASSERT_TRUE(GetLevelMTypes().has_value());
			// I2 (task 5 review): the pre-add set is MT_GOLEM + the quest-gated
			// pre-adds + the available cores, deduped by type - not just Golem and
			// core. PreAddedTypes() documents why each term is there; the previous
			// count omitted the quest term, so with a quest active on this level one
			// quest type leaked into the derived tail and the bound was wrong.
			// UseMultiplayerQuests() is false here (fullQuests = 1, set by
			// SetUpTestSuite), so the Q_SKELKING branch registers nothing; assert it
			// rather than assume, since that branch would add two more types.
			ASSERT_FALSE(UseMultiplayerQuests())
			    << "the MP Q_SKELKING branch pre-adds two more types; PreAddedTypes() would need to model them";
			const size_t preAdded = PreAddedTypes(level).size();
			const size_t tail = LevelMonsterTypeCount > preAdded ? LevelMonsterTypeCount - preAdded : 0;
			EXPECT_LE(tail, static_cast<size_t>(params->tailDraw))
			    << "level " << static_cast<int>(level) << " seed " << seed << " drew more tail types than tail_draw";
		}
	}

	// I2, second half: exercise the term that was missing. With Q_VEIL active on
	// L14 the engine pre-adds Lachdanan's base MT_RBLACK, so a pre-add count of
	// "Golem + core" is one too low. Assert the correction concretely - the
	// pre-add set must contain the quest base, and its size must exceed the old
	// Golem+core formula - so this case fails if the quest term is dropped again
	// instead of only passing because no quest happens to be active.
	LoadQuestData(); // SetUpTestSuite deliberately does not load it
	Quests[Q_VEIL]._qidx = Q_VEIL;
	Quests[Q_VEIL]._qactive = QUEST_ACTIVE;
	Quests[Q_VEIL]._qlevel = 14;
	currlevel = 14;

	const _monster_id lachdanBase = UniqueMonstersData[static_cast<size_t>(UniqueMonsterType::Lachdan)].mtype;
	const std::set<_monster_id> withQuest = PreAddedTypes(14);
	EXPECT_EQ(withQuest.count(lachdanBase), 1u)
	    << "an active Q_VEIL pre-adds MT_RBLACK on L14; the pre-add set must include it";

	size_t golemAndCore = 1; // MT_GOLEM
	for (const LevelRosterEntry &entry : GetLevelRoster(14)) {
		if (entry.role == LevelRosterRole::Core && IsRosterEntryAvailableAt(14, entry.type))
			golemAndCore++;
	}
	// MT_RBLACK is not an L14 core row, so the quest genuinely widens the set.
	EXPECT_GT(withQuest.size(), golemAndCore)
	    << "the quest pre-add must widen the pre-add set beyond Golem + core, otherwise the tail bound"
	    << " absorbs a quest type and is too loose";

	// And the bound itself still holds under that condition.
	for (int seed = 0; seed < 50; seed++) {
		currlevel = 14;
		InitLevelMonsters();
		SetRndSeed(8500 + static_cast<uint32_t>(seed));
		ASSERT_TRUE(GetLevelMTypes().has_value());
		const size_t preAdded = PreAddedTypes(14).size();
		const size_t tail = LevelMonsterTypeCount > preAdded ? LevelMonsterTypeCount - preAdded : 0;
		EXPECT_LE(tail, static_cast<size_t>(GetLevelRosterParams(14)->tailDraw))
		    << "L14 seed " << seed << " with Q_VEIL active drew more tail types than tail_draw";
	}

	Quests[Q_VEIL] = {};
}

TEST_F(SamplingBaselineTest, RosterQuotasSatisfied)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// AC3: each level's class floors are met and the B1 caps are never broken.
	// Caps come from BehaviorClassCapForLevel (the single source of truth the
	// validator uses), so a drift between validator and sampling loop fails here.
	//
	// Range note (R31): 1-15 for the same reason as RosterTailDrawBounded - floors
	// and caps are properties of the roster sampling loop, which L16 never enters
	// (hardcoded branch + early return, spec 4.2.7). L16's cap guarantee is
	// therefore a property of that hardcoded list, covered by Level16HardcodedTypes.
	for (uint8_t level = 1; level <= 15; level++) {
		const LevelRosterParams *params = GetLevelRosterParams(level);
		ASSERT_NE(params, nullptr) << "level " << static_cast<int>(level);
		for (int seed = 0; seed < 200; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(9000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());

			std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> counts {};
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				counts[static_cast<size_t>(GetBehaviorClass(MonstersData[LevelMonsterTypes[i].type].ai))]++;

			for (const auto &[cls, floor] : params->classFloors) {
				EXPECT_GE(counts[static_cast<size_t>(cls)], static_cast<size_t>(floor))
				    << "level " << static_cast<int>(level) << " seed " << seed
				    << " misses class floor " << static_cast<int>(cls);
			}
			for (size_t i = 0; i < counts.size(); i++) {
				const uint8_t cap = BehaviorClassCapForLevel(level, static_cast<BehaviorClass>(i));
				if (cap == 0)
					continue;
				// R29: the allowance is cap + the ENGINE's unconditional pre-adds
				// (Golem and this level's quest uniques). It deliberately does not
				// include the core roster: core is subject to the same cap, enforced
				// at load time by ValidateLevelRoster, so a core row that broke a B1
				// guarantee fails here instead of quietly raising its own budget.
				const size_t enginePreAdds = EnginePreAddClassCount(level, static_cast<BehaviorClass>(i));
				EXPECT_LE(counts[i], static_cast<size_t>(cap) + enginePreAdds)
				    << "level " << static_cast<int>(level) << " seed " << seed
				    << " breaks the B1 cap for class " << i;
			}
		}
	}
}

size_t AvailableCoreClassCount(uint8_t level, BehaviorClass cls)
{
	size_t count = 0;
	for (const LevelRosterEntry &entry : GetLevelRoster(level)) {
		if (entry.role != LevelRosterRole::Core)
			continue;
		if (!IsRosterEntryAvailableAt(level, entry.type))
			continue;
		if (GetBehaviorClass(MonstersData[entry.type].ai) == cls)
			count++;
	}
	return count;
}

// O1 discriminating-power proof for RosterQuotasSatisfied's cap half.
//
// That case asserts `counts[cls] <= cap + EnginePreAddClassCount(level, cls)`.
// An allowance can pass for the wrong reason: if it is wider than what the engine
// actually pre-adds, no composition can ever reach it and the assertion is true
// regardless of the sampling loop. This case proves the allowance is BINDING -
// i.e. the sampling loop is already at the limit, so admitting one extra type of
// that class would fail RosterQuotasSatisfied rather than slip through.
//
// The level is not hardcoded: it is discovered from the shipped roster (the level
// whose available core rows exactly fill a capped class). L15 is that level today
// (Balrog+Gsnake = 2 Melee, Snowwitch+Magistrate = 2 RangedTurret, cap 2), and if
// the data moves the search follows it instead of silently testing nothing.
TEST_F(SamplingBaselineTest, RosterQuotaAllowanceIsBinding)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	std::vector<std::pair<uint8_t, BehaviorClass>> saturated;
	for (uint8_t level = 1; level <= 15; level++) {
		for (size_t i = 0; i < static_cast<size_t>(BehaviorClass::Count); i++) {
			const auto cls = static_cast<BehaviorClass>(i);
			const uint8_t cap = BehaviorClassCapForLevel(level, cls);
			if (cap == 0)
				continue;
			if (AvailableCoreClassCount(level, cls) == static_cast<size_t>(cap))
				saturated.emplace_back(level, cls);
		}
	}
	ASSERT_FALSE(saturated.empty())
	    << "no level's core saturates a capped class, so RosterQuotasSatisfied's cap bound is never exercised at the limit";

	for (const auto &[level, cls] : saturated) {
		const uint8_t cap = BehaviorClassCapForLevel(level, cls);
		// No quest is active in this suite, so the engine's only pre-add is
		// MT_GOLEM (Boss). A capped non-Boss class therefore gets NO slack: the
		// allowance equals the cap exactly. Before O1 the exemption counted every
		// unique whose mlevel matched (nine of them on L13), which added slack the
		// engine never grants and made the bound unreachable.
		const size_t allowance = static_cast<size_t>(cap) + EnginePreAddClassCount(level, cls);
		if (cls != BehaviorClass::Boss) {
			EXPECT_EQ(allowance, static_cast<size_t>(cap))
			    << "level " << static_cast<int>(level) << " class " << static_cast<int>(cls)
			    << " must get no cap slack while no quest is active";
		}

		// And the real sampling loop reaches that allowance, so it is binding.
		size_t maxRealised = 0;
		for (int seed = 0; seed < 200; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(21000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			size_t realised = 0;
			for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
				if (GetBehaviorClass(MonstersData[LevelMonsterTypes[i].type].ai) == cls)
					realised++;
			}
			maxRealised = std::max(maxRealised, realised);
			// The bound RosterQuotasSatisfied asserts must still hold here.
			EXPECT_LE(realised, allowance)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << " breaks the cap for class " << static_cast<int>(cls);
		}
		EXPECT_EQ(maxRealised, allowance)
		    << "level " << static_cast<int>(level) << " class " << static_cast<int>(cls)
		    << " never reaches its allowance, so the cap assertion cannot detect a one-type regression";
	}
}

// The tightened exemption must TRACK quest availability, not be blanket: a quest
// that is not available grants no slack, and activating exactly one quest widens
// exactly its own class by exactly one.
TEST_F(SamplingBaselineTest, EnginePreAddExemptionTracksQuestAvailability)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	LoadQuestData(); // Quest::IsAvailable() reads QuestsData; SetUpTestSuite omits it.
	currlevel = 14;

	// Lachdanan (Q_VEIL) is the L14 quest unique whose base type MT_RBLACK the
	// engine pre-adds; MT_RBLACK is SkeletonMelee -> Melee.
	const BehaviorClass questClass = GetBehaviorClass(MonstersData[MT_RBLACK].ai);
	ASSERT_EQ(questClass, BehaviorClass::Melee);

	std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> inactive {};
	for (size_t i = 0; i < inactive.size(); i++)
		inactive[i] = EnginePreAddClassCount(14, static_cast<BehaviorClass>(i));

	// With every quest NOTAVAIL only Golem (Boss) is exempt.
	for (size_t i = 0; i < inactive.size(); i++) {
		const size_t expected = static_cast<BehaviorClass>(i) == BehaviorClass::Boss ? 1u : 0u;
		EXPECT_EQ(inactive[i], expected)
		    << "no quest is active, so class " << i << " must not be exempt";
	}

	Quests[Q_VEIL]._qidx = Q_VEIL;
	Quests[Q_VEIL]._qactive = QUEST_ACTIVE;
	Quests[Q_VEIL]._qlevel = 14;

	for (size_t i = 0; i < inactive.size(); i++) {
		const size_t after = EnginePreAddClassCount(14, static_cast<BehaviorClass>(i));
		const size_t expected = static_cast<BehaviorClass>(i) == questClass ? inactive[i] + 1 : inactive[i];
		EXPECT_EQ(after, expected) << "activating Q_VEIL must widen only Melee, by one (class " << i << ")";
	}

	// The same quest on a different level grants this level nothing.
	Quests[Q_VEIL]._qlevel = 13;
	EXPECT_EQ(EnginePreAddClassCount(14, questClass), inactive[static_cast<size_t>(questClass)])
	    << "a quest hosted on another level must not widen this level's cap";

	Quests[Q_VEIL] = {};
	QuestsData.clear(); // leave the suite's "no quest data" precondition intact.
}

TEST_F(SamplingBaselineTest, IdentityGuard)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// AC9: adjacent levels must not be interchangeable. Reuses the P0-D metric
	// (union of realised types over 200 seeds -> set -> Jaccard).
	constexpr int kSeeds = 200;
	std::vector<std::set<size_t>> rosters(17);
	for (uint8_t level = 1; level <= 16; level++) {
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(11000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				rosters[level].insert(LevelMonsterTypes[i].type);
		}
	}
	for (uint8_t level = 2; level <= 16; level++) {
		const std::set<size_t> &a = rosters[level - 1];
		const std::set<size_t> &b = rosters[level];
		size_t intersection = 0;
		for (const size_t type : a) {
			if (b.count(type) != 0)
				intersection++;
		}
		const size_t unionSize = a.size() + b.size() - intersection;
		const double jaccard = unionSize == 0 ? 0.0 : static_cast<double>(intersection) / static_cast<double>(unionSize);
		EXPECT_LT(jaccard, 0.9) << "levels " << static_cast<int>(level - 1) << " and " << static_cast<int>(level)
		                        << " are interchangeable (Jaccard " << jaccard << ")";
	}
}

// Realised type set for one sampling pass, sorted so it can be used as a set key.
std::vector<_monster_id> RealisedTypeKey()
{
	std::vector<_monster_id> types;
	types.reserve(LevelMonsterTypeCount);
	for (size_t i = 0; i < LevelMonsterTypeCount; i++)
		types.push_back(LevelMonsterTypes[i].type);
	std::sort(types.begin(), types.end());
	return types;
}

// AC9b (spec §6, R38): per-seed VARIETY, the dimension the other five guards
// structurally cannot see.
//
// Why this is needed: RosterCoreAlwaysPresent / RosterTailDrawBounded /
// RosterQuotasSatisfied / IdentityGuard / A1A3VariantsAreCore are all satisfied by
// a level that realises the SAME fixed composition on every seed - core is present,
// the tail is within bounds, floors and caps hold, and the union still differs from
// the neighbouring level. That is exactly the S1 regression: L13/L14/L15 each
// collapsed to ONE combination across 500 seeds, which also made 12 hell uniques
// unreachable (see HellUniqueBasesRemainReachable below). P0-D's
// MeasurementRosterIdentity already computed the per-level union on every run but
// asserted nothing about variety, so the collapse was measured and ignored.
//
// The threshold is deliberately the weakest non-trivial one (>= 2 distinct
// combinations): it is not a distribution-quality bar, it is the "sampling still
// depends on the seed at all" bar. A level that fails it is not random.
TEST_F(SamplingBaselineTest, RosterPerSeedVariety)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	constexpr int kSeeds = 500;
	for (uint8_t level = 1; level <= 15; level++) {
		std::set<std::vector<_monster_id>> combinations;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(41000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			combinations.insert(RealisedTypeKey());
		}
		std::cout << "[ MEASURED ] level " << static_cast<int>(level) << " per-seed combinations "
		          << combinations.size() << " (" << kSeeds << " seeds)" << std::endl;

		// L1 is the documented exception. Its candidate pool is only six types
		// (MT_NZOMBIE / MT_RFALLSP / MT_NSCAV / MT_WSKELAX / MT_ZOMBIE / MT_FALLSP
		// under the L1 band), of which the roster pre-adds four as core and Golem
		// takes a fifth slot; with tail_draw = 2 and the level's max_image budget the
		// realised set is the same every seed by arithmetic, not by a broken RNG.
		// Asserting >= 2 there would demand a data change (a wider L1 pool) that this
		// fix wave is not chartered to make, so L1 is skipped explicitly rather than
		// silently folded into a looser global threshold. Its composition is still
		// guarded by RosterCoreAlwaysPresent and PlacedClassMixWithinBaseline.
		if (level == 1) {
			EXPECT_EQ(combinations.size(), 1u)
			    << "L1's fixed composition is a documented arithmetic consequence of its 6-type pool;"
			    << " if it now varies, remove this exception instead of widening it";
			continue;
		}

		EXPECT_GE(combinations.size(), 2u)
		    << "level " << static_cast<int>(level) << " realises the same composition on all "
		    << kSeeds << " seeds: sampling there does not depend on the seed at all (S1 regression)";
	}
}

// AC9c (spec §6, R38): every hell unique whose base type the level can actually
// sample must be reachable on at least one seed.
//
// PlaceUniqueMonsters() (monster.cpp:505-528) only places a unique whose base type
// is ALREADY in LevelMonsterTypes - it searches for the base and skips the unique
// when absent. So a base type that sampling never realises makes its unique
// permanently unreachable, which is the player-visible half of the S1 regression
// (12 uniques lost across L13-15). This is the direct regression guard for it.
//
// Scope note 1: the denominator is "uniques whose base is a CANDIDATE at their own
// mlevel", not every unique with that mlevel. Five hell uniques fail that premise
// in the shipped data regardless of any roster - Warlord of Blood (MT_BTBLACK,
// band 14-16) and Lord of the Pit (MT_GSNAKE, band 15-16) name a base their own
// mlevel cannot sample, and Howlingire (MT_HOLOWONE) / Bloodmoon Soulfire
// (MT_PAINMSTR) / Zamphir (MT_REALWEAV) name availability=Never bases. Those are
// pre-existing data facts, not roster regressions (Warlord is placed anyway because
// Q_WARLORD pre-adds its base as a quest type), so counting them would make this
// guard unsatisfiable and therefore useless. The denominator is asserted to be
// positive so an empty one cannot make the guard vacuously true.
//
// Scope note 2 - the L13 Melee exception, and why it is a bound rather than a
// loophole. L13's candidate pool is 9 Melee / 3 RangedKite / 3 RangedTurret and the
// B1 cap is 2 per class, so if M Melee and R ranged scatter types are realised the
// placed ranged share is ~R/(M+R) (verified against the real placement path). L13's
// R4 ceiling is 0.2775, which admits only M=2, R=1 (measured 0.2362); M=1 already
// measures 0.3036 and breaks the ceiling. M=2 means BOTH Melee slots are core, so
// the Melee class sits at its cap and the tail can never draw a Melee type - which
// makes L13's three Melee unique bases (Blackskull/Rustweaver/Gorefeast) reachable
// only by coring them, and coring a unique's base needs allow_unique_boost, whose
// entire purpose (spec 4.4.3) is to gate the spawn-rate increase that would cause.
// Raising the ceiling is forbidden by R4, and relaxing the cap or granting
// allow_unique_boost are design decisions, so L13's Melee bases are expected
// unreachable here and the level is asserted at its ACHIEVABLE count instead of
// being skipped. The expectation is exact (EXPECT_EQ, not >=), so if a later change
// frees a Melee slot this case fails and forces the number to be re-derived rather
// than letting a silent regression hide under a loose bound.
TEST_F(SamplingBaselineTest, HellUniqueBasesRemainReachable)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Bases that L13's Melee cap structurally excludes (see scope note 2). Every
	// OTHER candidate base on L13-15 must be reachable.
	//
	// All three entries share ONE cause and ONE accepted-at-this-stage reason, and
	// all three fail the moment that cause is removed:
	//
	//   Why accepted now: these are L13 Melee bases. L13's R4 ceiling (0.2775)
	//   admits only 2 Melee + 1 ranged scatter types, so both Melee slots must be
	//   core; the Melee class is then AT its B1 cap of 2 and the tail loop prunes
	//   Melee entirely, so no Melee base can ever be drawn. Reaching them needs one
	//   of three DESIGN decisions this fix wave is not chartered to take: raise the
	//   ceiling (forbidden outright by R4), raise L13's Melee cap, or grant
	//   allow_unique_boost to core one of them (spec 4.4.3 gates exactly the
	//   spawn-rate increase that would cause).
	//
	//   What makes an entry fail: any change that frees an L13 Melee tail slot -
	//   BehaviorClassCapForLevel raising L13's cap above 2, L13's core dropping to
	//   one Melee type, or the base being cored under allow_unique_boost. The
	//   assertion below is EXPECT_FALSE, so such a change turns this into a failure
	//   and the entry must be deleted rather than silently outliving its reason.
	//   The blockedSeen premise likewise fails if an entry stops naming a base this
	//   loop actually checks.
	const std::set<std::pair<uint8_t, _monster_id>> kCapBlocked {
		{ 13, MT_BALROG },  // Blackskull
		{ 13, MT_RTBLACK }, // Rustweaver
		{ 13, MT_VTEXLRD }, // Gorefeast
	};

	constexpr int kSeeds = 500;
	size_t checkedUniques = 0;
	size_t blockedSeen = 0;
	for (uint8_t level = 13; level <= 15; level++) {
		std::set<_monster_id> realised;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(42000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				realised.insert(LevelMonsterTypes[i].type);
		}

		size_t reachable = 0;
		size_t candidateBases = 0;
		for (size_t u = 0; u < UniqueMonstersData.size(); u++) {
			if (UniqueMonstersData[u].mlevel != level)
				continue;
			const _monster_id base = UniqueMonstersData[u].mtype;
			// Availability check mirrors the production candidate filter.
			if (!IsRosterEntryAvailableAt(level, base))
				continue;
			candidateBases++;
			const bool isRealised = realised.count(base) != 0;
			if (isRealised)
				reachable++;

			if (kCapBlocked.count({ level, base }) != 0) {
				blockedSeen++;
				// Asserted, not skipped: this documents the cap consequence and
				// fails loudly if the constraint is ever lifted silently.
				EXPECT_FALSE(isRealised)
				    << "level " << static_cast<int>(level) << ": '" << UniqueMonstersData[u].mName
				    << "' is now reachable - a Melee slot was freed, so re-derive the L13 exception"
				    << " in this case instead of leaving a stale entry";
				continue;
			}

			checkedUniques++;
			EXPECT_TRUE(isRealised)
			    << "level " << static_cast<int>(level) << ": unique '" << UniqueMonstersData[u].mName
			    << "' has base type " << static_cast<int>(base)
			    << " which no seed realises, so PlaceUniqueMonsters() can never place it";
		}
		std::cout << "[ MEASURED ] level " << static_cast<int>(level) << " unique bases reachable "
		          << reachable << "/" << candidateBases << std::endl;
		EXPECT_GT(candidateBases, 0u)
		    << "level " << static_cast<int>(level) << " declares no unique with a samplable base;"
		    << " with an empty denominator this guard would assert nothing";
	}
	// Premise guards: the loop must really have examined the uniques it claims to.
	// L13 contributes 2 (Doomcloud, Witchmoon), L14 6, L15 2 in the shipped data.
	EXPECT_EQ(checkedUniques, 10u)
	    << "expected exactly 10 hell uniques whose base must be reachable; a different number means"
	    << " the data moved and the exception list / this bound need re-deriving";
	EXPECT_EQ(blockedSeen, kCapBlocked.size())
	    << "every entry in the L13 cap-blocked list must correspond to a real unique on that level";
}

TEST_F(SamplingBaselineTest, A1A3VariantsAreCore)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// AC10: the A1/A3 behaviour carriers must be core on their level, otherwise
	// those features can silently never appear.
	const std::vector<std::pair<uint8_t, _monster_id>> carriers {
		{ 3, MT_RSKELAX },
		{ 3, MT_XSKELAX },
		{ 9, MT_BMAGMA },
		{ 11, MT_STORML },
	};
	for (const auto &[level, type] : carriers) {
		const std::span<const LevelRosterEntry> roster = GetLevelRoster(level);
		const bool isCore = std::any_of(roster.begin(), roster.end(), [type](const LevelRosterEntry &entry) {
			return entry.type == type && entry.role == LevelRosterRole::Core;
		});
		EXPECT_TRUE(isCore) << "carrier " << static_cast<int>(type) << " must be core at level " << static_cast<int>(level);

		// And it must actually be realised by the engine, not merely listed.
		currlevel = level;
		InitLevelMonsters();
		SetRndSeed(12345);
		ASSERT_TRUE(GetLevelMTypes().has_value());
		EXPECT_TRUE(LevelHasType(type))
		    << "carrier " << static_cast<int>(type) << " must be realised at level " << static_cast<int>(level);
	}
}

// ---------------------------------------------------------------------------
// R28: levels with no roster params row must keep the LEGACY sampling
// behaviour (budget-limited, tail draw unbounded), not "tail_draw = 0".
//
// Under the phase-A tables that is L17-24 (Hellfire Nest/Crypt), whose own
// tables land in phase A2. A `tailDraw = 0` fallback makes the sampling loop
// condition `tailAdded < tailDraw` permanently false, so the loop never runs,
// no PLACE_SCATTER type is registered, and InitMonsters()'s scatter block
// (entered only when numscattypes > 0) is skipped entirely - i.e. those levels
// would ship with no scattered monsters at all, a regression against the
// pre-roster engine.
//
// The check needs Hellfire monster data: under the base monstdat.tsv every
// L17-24 monster is availability=Never, so the candidate pool there is empty
// and the level would legitimately sample nothing. The fixture therefore loads
// the `hf` mod overlay (like items_test/pack_test do) in its own suite, so it
// cannot disturb the Diablo-data state the baselines above measure.
// ---------------------------------------------------------------------------

class HellfireNoParamsSamplingTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadCoreArchives();
		LoadGameArchives();
		if (!HaveMainData()) {
			missingMpqAssets_ = true;
			return;
		}

		// R30: this suite mounts the hf overlay and runs TestInitGame(), which
		// mutates process-global state that the SamplingBaselineTest measurements
		// above depend on. Most importantly InitQuests() lifts every
		// Quests[*]._qactive out of its zero-initialised QUEST_NOTAVAIL, and an
		// active quest pre-adds its unique's base type: leaving Q_WARLORD active
		// would add Melee MT_BTBLACK to L13, pushing that level to 3 Melee types
		// and breaking HellL13SameClassTailBaseline's EXPECT_EQ(tail, 0.0) - a
		// failure that only appears once the suite order changes (--gtest_shuffle).
		// So snapshot every global TestInitGame() touches and restore the snapshot
		// in TearDownTestSuite(), rather than guessing at "initial" values.
		savedQuests_.assign(std::begin(Quests), std::end(Quests));
		// Player is not copyable, so snapshot the two things TestInitGame() changes
		// about it (the vector's size and pOriginalCathedral) rather than the object.
		savedPlayerCount_ = Players.size();
		savedMyPlayerIsFirst_ = !Players.empty() && MyPlayer == &Players[0];
		savedOriginalCathedral_ = Players.empty() ? true : Players[0].pOriginalCathedral;
		savedGameInitInfo_ = sgGameInitInfo;
		savedIsMultiplayer_ = gbIsMultiplayer;
		savedIsHellfire_ = gbIsHellfire;
		savedIsSpawn_ = gbIsSpawn;
		snapshotTaken_ = true;

		gbIsSpawn = false;
		gbIsHellfire = true;
		sgGameInitInfo.fullQuests = 1;
		// TestInitGame(..., hellfire = true) mounts the hf overlay, which is what
		// gives L17-24 a non-empty candidate pool.
		TestInitGame(/*fullQuests=*/true, /*originalCathedral=*/true, /*hellfire=*/true);
		LoadMonsterData();
		LoadLevelRoster();
		// CI only ships spawn.mpq: no hellfire.mpq means the hf overlay above did
		// not actually mount, so L17-24 would have an empty candidate pool and the
		// "still samples types" premise is vacuous. Probe once here and skip in
		// the test bodies instead of asserting, rather than failing the build.
		missingHellfire_ = !HaveHellfire();
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

		// And put the Diablo monster/roster data back, since the overlay changed it.
		UnloadModArchives();
		LoadModArchives({});
		LoadMonsterData();
		LoadLevelRoster();
		snapshotTaken_ = false;
	}

	static bool missingMpqAssets_;
	static bool missingHellfire_;

private:
	static std::vector<Quest> savedQuests_;
	static size_t savedPlayerCount_;
	static bool savedMyPlayerIsFirst_;
	static bool savedOriginalCathedral_;
	static GameData savedGameInitInfo_;
	static bool savedIsMultiplayer_;
	static bool savedIsHellfire_;
	static bool savedIsSpawn_;
	static bool snapshotTaken_;
};

bool HellfireNoParamsSamplingTest::missingMpqAssets_ = false;
bool HellfireNoParamsSamplingTest::missingHellfire_ = false;
std::vector<Quest> HellfireNoParamsSamplingTest::savedQuests_;
size_t HellfireNoParamsSamplingTest::savedPlayerCount_ = 0;
bool HellfireNoParamsSamplingTest::savedMyPlayerIsFirst_ = false;
bool HellfireNoParamsSamplingTest::savedOriginalCathedral_ = true;
GameData HellfireNoParamsSamplingTest::savedGameInitInfo_ {};
bool HellfireNoParamsSamplingTest::savedIsMultiplayer_ = false;
bool HellfireNoParamsSamplingTest::savedIsHellfire_ = false;
bool HellfireNoParamsSamplingTest::savedIsSpawn_ = false;
bool HellfireNoParamsSamplingTest::snapshotTaken_ = false;

TEST_F(HellfireNoParamsSamplingTest, LevelsWithoutParamsStillSampleTypes)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	// Guard the premise first: if the overlay did not mount, the "still samples"
	// assertion below would be vacuous. CI only ships spawn.mpq, so skip rather
	// than fail when Hellfire assets are unavailable.
	if (missingHellfire_)
		GTEST_SKIP() << "hf overlay required: L17-24 have no candidates under base monstdat";

	int levelsChecked = 0;
	for (uint8_t level = 17; level <= 24; level++) {
		// Premise 1: this level really is on the no-params path (R28's subject).
		ASSERT_EQ(GetLevelRosterParams(level), nullptr)
		    << "level " << static_cast<int>(level) << " now has a params row; move it out of this test";
		// Premise 2: and it has no core roster rows either, so every scatter type
		// it gets must come from the tail draw.
		ASSERT_TRUE(GetLevelRoster(level).empty())
		    << "level " << static_cast<int>(level) << " now has roster rows; move it out of this test";

		currlevel = level;
		size_t candidates = 0;
		for (size_t i = 0; i < MonstersData.size(); i++) {
			if (IsRosterEntryAvailableAt(level, static_cast<_monster_id>(i)))
				candidates++;
		}
		// Premise 3: the pool is non-empty, otherwise "samples nothing" would be
		// correct rather than a regression.
		ASSERT_GT(candidates, 0u) << "level " << static_cast<int>(level) << " has no candidates at all";

		for (uint32_t seed = 0; seed < 20; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(31000 + seed);
			const auto getTypesResult = GetLevelMTypes();
			ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();

			// The actual R28 assertion: at least one PLACE_SCATTER type. This is
			// what InitMonsters() counts as numscattypes; with tailDraw = 0 it
			// would be zero on L17/21/22/23 (the levels with no scatter pre-add).
			size_t scatterTypes = 0;
			for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
				if ((LevelMonsterTypes[i].placeFlags & PLACE_SCATTER) != 0)
					scatterTypes++;
			}
			EXPECT_GT(scatterTypes, 0u)
			    << "level " << static_cast<int>(level) << " seed " << seed
			    << " has no PLACE_SCATTER type: InitMonsters() would place no scattered monsters";

			// And the draw is genuinely unbounded rather than capped at some small
			// number: the legacy loop fills up to the sprite budget, so it must be
			// able to exceed the largest tail_draw the L1-16 tables use.
			EXPECT_GT(LevelMonsterTypeCount, 1u)
			    << "level " << static_cast<int>(level) << " seed " << seed << " sampled nothing beyond MT_GOLEM";
		}
		levelsChecked++;
	}
	EXPECT_EQ(levelsChecked, 8) << "all of L17-24 must be exercised";
}

TEST_F(HellfireNoParamsSamplingTest, NoParamsTailExceedsTheParameterisedCap)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingHellfire_)
		GTEST_SKIP() << "hf overlay required: L17-24 have no candidates under base monstdat";

	// Sharper form of "unbounded": some no-params level must realise more scatter
	// types than the largest tail_draw configured for L1-16. That is only true if
	// the fallback is std::numeric_limits<int>::max() (legacy) rather than any of
	// the table's small tail_draw values, and it cannot be satisfied by a
	// tautology - it is a measured count from the real engine.
	int maxTableTailDraw = 0;
	for (uint8_t level = 1; level <= 16; level++) {
		const LevelRosterParams *params = GetLevelRosterParams(level);
		if (params != nullptr)
			maxTableTailDraw = std::max(maxTableTailDraw, params->tailDraw);
	}
	ASSERT_GT(maxTableTailDraw, 0) << "L1-16 tail_draw values must be readable for this comparison";

	size_t bestScatter = 0;
	for (uint8_t level = 17; level <= 24; level++) {
		for (uint32_t seed = 0; seed < 20; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(32000 + seed);
			const auto getTypesResult = GetLevelMTypes();
			ASSERT_TRUE(getTypesResult.has_value()) << getTypesResult.error();
			size_t scatterTypes = 0;
			for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
				if ((LevelMonsterTypes[i].placeFlags & PLACE_SCATTER) != 0)
					scatterTypes++;
			}
			bestScatter = std::max(bestScatter, scatterTypes);
		}
	}
	EXPECT_GT(bestScatter, static_cast<size_t>(maxTableTailDraw))
	    << "no-params levels must draw a legacy (budget-limited) tail, not a tail_draw-sized one";
}

// ---------------------------------------------------------------------------
// G1 (spec 2026-09-15-level-rosters-design 4.3.1): an ORDINARY (non-unique)
// leader's death must release its leashed minions and clear their dangling
// `leader` index.
//
// M_UpdateRelations used to gate the release on monster.hasLeashedMinions(),
// which is `isUnique() && monsterPack == Leashed` - permanently false for an
// ordinary monster. So an ordinary leader's minions stayed Leashed forever, and
// their `leader` index kept pointing at a slot that DeleteMonster's swap plus a
// later AddMonster can hand to an unrelated LIVE monster. GroupUnity and
// FollowTheLeader then dereference that index.
//
// The unique path must NOT change: setLeader(nullptr) deliberately keeps the
// index so monhealthbar can colour buffed minions distinctly.
//
// The tests drive the engine's own death path. MonsterDeath (what the public
// entries StartMonsterDeath/M_StartKill funnel into) starts the death
// animation; M_UpdateRelations then runs when that animation reaches its last
// frame, inside the file-static MonsterDeath(Monster&) that ProcessMonsters
// calls. Running a whole ProcessMonsters tick here would also run every AI on
// the level, so the tests advance only the dying monster's death animation and
// then invoke the same engine hook the tick would - M_UpdateRelations, which
// monster.h exports.
// ---------------------------------------------------------------------------

// The sampling fixture only needs monster/roster data, but the death path also
// runs loot generation (SpawnLoot -> SpawnItem -> AllItemsList) and touches
// MyPlayer (PlayEffect, SetupAllItems). diablo.cpp loads item data and has a
// player before any monster can die; mirror that here so the tests exercise the
// real death path instead of a trimmed-down stand-in.
void PrepareDeathPathPrerequisites()
{
	LoadItemData();
	Players.resize(1);
	MyPlayer = &Players[0];
	MyPlayer->pLvlLoad = 0;
}

// Drive `leader` through the engine's death sequence up to and including the
// relation update that MonsterDeath(Monster&) performs on the last death frame.
void RunEngineDeath(Monster &leader)
{
	MonsterDeath(leader, leader.direction, /*sendmsg=*/false);
	ASSERT_EQ(leader.mode, MonsterMode::Death) << "MonsterDeath must start the death animation";

	// Advance the death animation the way ProcessMonsters does, until the
	// engine's own last-frame branch (which calls M_UpdateRelations) triggers.
	for (int tick = 0; tick < 512 && !leader.animInfo.isLastFrame(); tick++)
		leader.animInfo.processAnimation(false);
	ASSERT_TRUE(leader.animInfo.isLastFrame()) << "death animation never reached its last frame";

	// Same last-frame body as MonsterDeath(Monster&) in monster.cpp.
	leader.isInvalid = true;
	M_UpdateRelations(leader);
}

TEST_F(SamplingBaselineTest, LeaderDeathReleasesMinions)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	PrepareDeathPathPrerequisites();

	// Level 1 has no unique monsters at all, so every placed monster is ordinary.
	currlevel = 1;
	InitLevelMonsters();
	SetRndSeed(9000);
	{
		const auto typesResult = GetLevelMTypes();
		ASSERT_TRUE(typesResult.has_value()) << typesResult.error();
	}
	ASSERT_GT(LevelMonsterTypeCount, 0U);

	// Place two monsters of a sampled level type directly: this test is about
	// the leader/minion relation, not about the placement distribution, and
	// AddMonster is the same entry PlaceMonster/PlaceGroup use.
	Monster *leaderPtr = AddMonster({ 40, 40 }, Direction::South, 0, /*inMap=*/false);
	ASSERT_NE(leaderPtr, nullptr);
	Monster *minionPtr = AddMonster({ 41, 40 }, Direction::South, 0, /*inMap=*/false);
	ASSERT_NE(minionPtr, nullptr);

	Monster &leader = *leaderPtr;
	Monster &minion = *minionPtr;
	ASSERT_FALSE(leader.isUnique()) << "this test must cover the ORDINARY leader path";

	minion.setLeader(&leader);
	ASSERT_EQ(minion.leaderRelation, LeaderRelation::Leashed);
	ASSERT_EQ(minion.leader, static_cast<uint8_t>(leader.getId()));
	leader.packSize = 1;

	RunEngineDeath(leader);

	EXPECT_NE(minion.leaderRelation, LeaderRelation::Leashed)
	    << "an ordinary leader's death must not leave its minion leashed";
	EXPECT_EQ(minion.leader, Monster::NoLeader)
	    << "a stale leader index can point at a live stranger after slot reuse";
	EXPECT_EQ(minion.getLeader(), nullptr)
	    << "getLeader() must stop resolving to the dead leader's slot";
}

TEST_F(SamplingBaselineTest, UniqueLeaderDeathBehaviourUnchanged)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// The unique path keeps its documented behaviour: relation cleared, index
	// RETAINED (monhealthbar colours buffed minions from that index).
	// PrepareUniqueMonst needs the unique's TRN, which only ships with retail
	// data; skip cleanly when it is unavailable (same guard style as
	// level_roster_baseline_test).
	PrepareDeathPathPrerequisites();

	currlevel = 1;
	InitLevelMonsters();
	SetRndSeed(9100);
	{
		const auto typesResult = GetLevelMTypes();
		ASSERT_TRUE(typesResult.has_value()) << typesResult.error();
	}
	ASSERT_GT(LevelMonsterTypeCount, 0U);

	Monster *leaderPtr = AddMonster({ 40, 40 }, Direction::South, 0, /*inMap=*/false);
	ASSERT_NE(leaderPtr, nullptr);
	Monster *minionPtr = AddMonster({ 41, 40 }, Direction::South, 0, /*inMap=*/false);
	ASSERT_NE(minionPtr, nullptr);

	Monster &leader = *leaderPtr;
	Monster &minion = *minionPtr;

	// Make the leader a Leashed-pack unique. Gharbad the Weak is
	// UniqueMonsterPack::None, so pick a unique whose data really is Leashed so
	// hasLeashedMinions() (the OLD gate) is true - i.e. the exact case whose
	// behaviour must stay byte-for-byte identical.
	bool foundLeashedUnique = false;
	for (size_t u = 0; u < UniqueMonstersData.size(); u++) {
		if (UniqueMonstersData[u].monsterPack != UniqueMonsterPack::Leashed)
			continue;
		leader.uniqueType = static_cast<UniqueMonsterType>(u);
		foundLeashedUnique = true;
		break;
	}
	ASSERT_TRUE(foundLeashedUnique) << "unique monster data has no Leashed pack entry";
	ASSERT_TRUE(leader.isUnique());
	ASSERT_TRUE(leader.hasLeashedMinions())
	    << "the regression must exercise the old hasLeashedMinions() gate";

	minion.setLeader(&leader);
	const auto retainedIndex = static_cast<uint8_t>(leader.getId());
	ASSERT_EQ(minion.leaderRelation, LeaderRelation::Leashed);
	ASSERT_EQ(minion.leader, retainedIndex);
	leader.packSize = 1;

	RunEngineDeath(leader);

	EXPECT_EQ(minion.leaderRelation, LeaderRelation::None)
	    << "a unique leader's death must still clear the minion's relation";
	EXPECT_EQ(minion.leader, retainedIndex)
	    << "the unique path must RETAIN the leader index (monhealthbar colouring)";
}

// ---------------------------------------------------------------------------
// G2 (spec 2026-09-15-level-rosters-design 4.3.1, task 2): PlaceGroup's minion
// toughening (HP x2) and AI/intelligence inheritance are now controlled by an
// explicit MinionOptions argument. The default (opts = {}) reproduces the
// engine's historical behaviour byte-for-byte (unique boss packs); a caller
// that opts out (the squad-minion path, task 3) gets an unbuffed, own-AI
// minion instead.
//
// SquadMinionsUnbuffed drives PlaceGroup directly with a leashed leader/minion
// pair and every MinionOptions flag turned off. The leader/minion pair is
// deliberately chosen so the leader's AI (SkeletonMelee) differs from the
// minion's own AI (SkeletonRanged): if they matched, "minion.ai ==
// MonstersData[minionType].ai" would hold whether or not setLeader()'s AI
// overwrite actually got reverted, making that assertion vacuously true.
//
// The HP assertion is a same-seed A/B rather than a range check. Bounding the
// roll to the un-doubled [min,max] range does NOT prove opts.tough took effect:
// for MT_TSKELBW the un-doubled range is [256,512] and the doubled range is
// [512,1024], which overlap at 512, so a doubled roll can still satisfy an
// un-doubled range check. Instead the same leader/minion configuration is
// placed twice from the SAME RNG seed with only opts.tough flipped. Both runs
// consume an identical RNG sequence up to InitMonster's HP roll (opts.tough
// itself draws nothing, and the leader tiles are chosen far enough apart that
// each run's placement succeeds on its first candidate tile), so the doubled
// sample must be exactly 2x the un-doubled one - a judgement that holds for
// every possible roll.
// ---------------------------------------------------------------------------

TEST_F(SamplingBaselineTest, SquadMinionsUnbuffed)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	currlevel = 1;
	InitLevelMonsters();

	// MT_WSKELAX (SkeletonMelee) and MT_TSKELBW (SkeletonRanged) are both
	// Always-available L1-4 pool members, and their AI classes differ - the
	// precondition SquadMinionsUnbuffed's AI assertion needs to be non-vacuous
	// (see comment above).
	ASSERT_EQ(MonstersData[MT_WSKELAX].ai, MonsterAIID::SkeletonMelee);
	ASSERT_EQ(MonstersData[MT_TSKELBW].ai, MonsterAIID::SkeletonRanged);
	ASSERT_NE(MonstersData[MT_WSKELAX].ai, MonstersData[MT_TSKELBW].ai)
	    << "leader and minion AI must differ, otherwise the AI-not-overwritten"
	    << " assertion below would pass even if setLeader()'s overwrite were never reverted";

	size_t leaderType = 0;
	size_t minionType = 0;
	{
		const auto addResult = AddMonsterType(MT_WSKELAX, PLACE_SCATTER);
		ASSERT_TRUE(addResult.has_value()) << addResult.error();
		leaderType = addResult.value();
	}
	{
		const auto addResult = AddMonsterType(MT_TSKELBW, PLACE_SCATTER);
		ASSERT_TRUE(addResult.has_value()) << addResult.error();
		minionType = addResult.value();
	}
	ASSERT_LT(leaderType, LevelMonsterTypeCount);
	ASSERT_LT(minionType, LevelMonsterTypeCount);

	// One leashed placement from a fixed seed. The leader is added with
	// inMap=false so it never occupies a tile, and each call gets its own
	// leader tile: PlaceGroup's first candidate is always a neighbour of the
	// leader, so disjoint leader tiles keep every run's candidate tile
	// pristine and therefore keep the RNG draw counts identical across runs.
	struct MinionSample {
		int maxHitPoints;
		int hitPoints;
		MonsterAIID ai;
		uint8_t intelligence;
		MonsterAIID leaderAi;
		LeaderRelation leaderRelation;
	};
	const auto placeLeashedMinion = [&](Point leaderTile, MinionOptions opts) -> MinionSample {
		InitLevelMonsters();
		SetRndSeed(9200);
		Monster *leaderPtr = AddMonster(leaderTile, Direction::South, leaderType, /*inMap=*/false);
		EXPECT_NE(leaderPtr, nullptr);
		PlaceGroup(minionType, 1, leaderPtr, /*leashed=*/true, opts);
		EXPECT_EQ(ActiveMonsterCount, 2u) << "minion must be placed";
		const Monster &minion = Monsters[1];
		return MinionSample { minion.maxHitPoints, minion.hitPoints, minion.ai,
			minion.intelligence, leaderPtr->ai, minion.leaderRelation };
	};

	const MinionSample squad = placeLeashedMinion({ 40, 40 },
	    MinionOptions { .tough = false, .inheritAi = false, .inheritIntelligence = false });
	// Reference run: identical seed, leader and minion type, differing ONLY in
	// opts.tough. Everything else stays off so the two runs cannot diverge
	// through some other flag's side effects.
	const MinionSample toughened = placeLeashedMinion({ 60, 60 },
	    MinionOptions { .tough = true, .inheritAi = false, .inheritIntelligence = false });

	ASSERT_EQ(squad.leaderRelation, LeaderRelation::Leashed) << "leashed=true must still leash the minion";
	ASSERT_EQ(squad.leaderAi, MonsterAIID::SkeletonMelee);
	ASSERT_GT(squad.maxHitPoints, 0) << "a zero HP roll would make the A/B equality below trivially true";

	// (1) HP must not be doubled - proven against the same-seed toughened run
	// rather than against a roll range (see the comment above the test).
	EXPECT_EQ(toughened.maxHitPoints, 2 * squad.maxHitPoints)
	    << "the two runs must differ by exactly the opts.tough doubling; if they do not,"
	    << " the A/B lost RNG alignment and this assertion cannot distinguish anything";
	EXPECT_EQ(squad.hitPoints, squad.maxHitPoints);
	EXPECT_EQ(toughened.hitPoints, toughened.maxHitPoints);

	// (2) AI must remain the minion's OWN ai, not the leader's (the leader and
	// minion AI differ by construction above, so this is not vacuously true).
	EXPECT_EQ(squad.ai, MonstersData[MT_TSKELBW].ai) << "opts.inheritAi=false must keep the minion's own AI";
	EXPECT_NE(squad.ai, squad.leaderAi) << "the minion's AI must not have been overwritten by setLeader()";
	// The toughened reference run also opts out of AI inheritance, so flipping
	// opts.tough must not disturb AI or intelligence either.
	EXPECT_EQ(toughened.ai, squad.ai) << "opts.tough must not affect the minion's AI";

	// (3) intelligence must not be inherited from the leader.
	EXPECT_EQ(squad.intelligence, MonstersData[MT_TSKELBW].intelligence)
	    << "opts.inheritIntelligence=false must not copy the leader's intelligence";
	EXPECT_EQ(toughened.intelligence, squad.intelligence)
	    << "opts.tough must not affect the minion's intelligence";
}

// UniqueMinionsBehaviourUnchanged (regression): the unique path calls
// PlaceGroup with the DEFAULT MinionOptions{} (PrepareUniqueMonst never
// constructs one explicitly), so a Leashed-pack unique's minions must still
// come out HP-doubled and AI-inherited exactly as before this task's change.
// Mirrors UniqueLeaderDeathBehaviourUnchanged's approach of scanning
// UniqueMonstersData for a real Leashed-pack entry rather than hardcoding one.
//
// The scan additionally requires the unique's own mAi to DIFFER from its base
// monster type's ai. PrepareUniqueMonst sets leader.ai = uniqueData.mAi while
// the minion is placed with the base type's ai, so when the two match (e.g.
// MT_TSKELAX / Bonehead Keenaxe: both SkeletonMelee) the minion's own AI is
// already equal to the leader's and "minion.ai == leader.ai" holds whether or
// not opts.inheritAi was honoured - a vacuous assertion. Requiring mAi != ai
// makes the inheritance observable.
TEST_F(SamplingBaselineTest, UniqueMinionsBehaviourUnchanged)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	currlevel = 1;
	InitLevelMonsters();
	SetRndSeed(9300);

	// Candidate PRE-FILTER only (a trimmed copy of IsMonsterAvailable's
	// availability clause, same pattern as IsMeasuredCandidate above). It is
	// deliberately NOT self-evidence of asset availability:
	//   - it drops IsMonsterAvailable's dunLvl range check (irrelevant here);
	//   - its `gbIsSpawn && Retail` arm is dead code in this fixture, because
	//     SetUpTestSuite sets gbIsSpawn = false unconditionally.
	// Consequently it does NOT filter out retail-only uniques whose .trn asset
	// is absent from a spawn-only data set. Asset availability is decided
	// exclusively by the explicit TRN probe below - never inferred from this
	// loop.
	bool foundLeashedUnique = false;
	size_t uniqueIndex = 0;
	for (size_t u = 0; u < UniqueMonstersData.size(); u++) {
		if (UniqueMonstersData[u].monsterPack != UniqueMonsterPack::Leashed)
			continue;
		const MonsterData &baseData = MonstersData[UniqueMonstersData[u].mtype];
		if (baseData.availability == MonsterAvailability::Never)
			continue;
		if (gbIsSpawn && baseData.availability == MonsterAvailability::Retail)
			continue;
		// Non-vacuity precondition for assertion (2) below.
		if (UniqueMonstersData[u].mAi == baseData.ai)
			continue;
		uniqueIndex = u;
		foundLeashedUnique = true;
		break;
	}
	ASSERT_TRUE(foundLeashedUnique)
	    << "unique monster data has no available Leashed pack entry whose mAi differs from its base type's ai";

	const auto uniqueType = static_cast<UniqueMonsterType>(uniqueIndex);
	const UniqueMonsterData &uniqueMonsterData = UniqueMonstersData[uniqueIndex];
	ASSERT_EQ(uniqueMonsterData.monsterPack, UniqueMonsterPack::Leashed);
	ASSERT_NE(uniqueMonsterData.mAi, MonstersData[uniqueMonsterData.mtype].ai)
	    << "the chosen unique's mAi must differ from its base type's ai, otherwise the"
	    << " AI-inheritance assertion below would pass even if opts.inheritAi were ignored";

	// PrepareUniqueMonst -> InitTRNForUniqueMonster loads
	// "monsters\monsters\" + UniqueMonsterData::mTrnName + ".trn"
	// (Source/monster.cpp InitTRNForUniqueMonster). Those files only ship with
	// retail (DIABDAT.MPQ) / Hellfire data, not spawn.mpq - and WHICH file is
	// needed depends on which unique the scan above picked, so probe the TRN of
	// the unique actually selected rather than a hardcoded name. Mirrors
	// LevelRosterBaselineTest::missingRetailTrn_; deliberately not
	// HaveHellfire(), so a retail-only (non-HF) data set can still run this.
	{
		const std::string trnPath = StrCat(R"(monsters\monsters\)", uniqueMonsterData.mTrnName, ".trn");
		size_t trnSize = 0;
		const AssetHandle trnHandle = OpenAsset(trnPath, trnSize);
		if (!trnHandle.ok() || trnSize == 0)
			GTEST_SKIP() << "retail/HF TRN " << trnPath << " not available";
	}

	size_t minionType = 0;
	{
		const auto addResult = AddMonsterType(uniqueMonsterData.mtype, PLACE_UNIQUE);
		ASSERT_TRUE(addResult.has_value()) << addResult.error();
		minionType = addResult.value();
	}
	ASSERT_LT(minionType, LevelMonsterTypeCount);

	// bosspacksize = 1: just enough to exercise the buff/inherit branch without
	// depending on how many minions the placement loop manages to fit.
	// The unique path is run twice from the SAME seed: once through
	// PrepareUniqueMonst (which passes the DEFAULT MinionOptions{}, i.e. the
	// behaviour under regression test) and once through PlaceGroup directly with
	// tough=false. Only that flag differs, so the doubled sample must be exactly
	// 2x the un-doubled one - the same-seed A/B judgement, independent of the
	// monstdat roll range (whose doubled and un-doubled intervals overlap).
	struct UniqueMinionSample {
		int maxHitPoints;
		int hitPoints;
		MonsterAIID ai;
		MonsterAIID leaderAi;
		LeaderRelation leaderRelation;
	};
	std::string prepareError;
	const auto placeUnique = [&](Point leaderTile, bool viaPrepareUniqueMonst,
	                             std::optional<MinionOptions> opts) -> UniqueMinionSample {
		InitLevelMonsters();
		SetRndSeed(9300);
		Monster *leaderPtr = AddMonster(leaderTile, Direction::South, minionType, /*inMap=*/false);
		EXPECT_NE(leaderPtr, nullptr);
		if (viaPrepareUniqueMonst) {
			const auto prepareResult = PrepareUniqueMonst(*leaderPtr, uniqueType, minionType,
			    /*bosspacksize=*/1, uniqueMonsterData);
			if (!prepareResult.has_value())
				prepareError = prepareResult.error();
		} else {
			// Reproduce only the two PrepareUniqueMonst fields PlaceGroup's
			// inheritance can copy onto the minion, then place the pack the
			// same way PrepareUniqueMonst does.
			leaderPtr->ai = uniqueMonsterData.mAi;
			leaderPtr->intelligence = uniqueMonsterData.mint;
			PlaceGroup(minionType, 1, leaderPtr, /*leashed=*/true, *opts);
		}
		EXPECT_EQ(ActiveMonsterCount, 2u) << "the unique's one minion must have been placed";
		const Monster &minion = Monsters[1];
		return UniqueMinionSample { minion.maxHitPoints, minion.hitPoints, minion.ai,
			leaderPtr->ai, minion.leaderRelation };
	};

	const UniqueMinionSample uniquePack = placeUnique({ 40, 40 }, /*viaPrepareUniqueMonst=*/true, std::nullopt);
	ASSERT_TRUE(prepareError.empty()) << prepareError;
	const UniqueMinionSample unbuffed = placeUnique({ 60, 60 }, /*viaPrepareUniqueMonst=*/false,
	    MinionOptions { .tough = false, .inheritAi = true, .inheritIntelligence = true });

	ASSERT_EQ(uniquePack.leaderRelation, LeaderRelation::Leashed);
	ASSERT_GT(unbuffed.maxHitPoints, 0) << "a zero HP roll would make the A/B equality below trivially true";

	// (1) HP must still be doubled, proven against the same-seed tough=false run.
	EXPECT_EQ(uniquePack.maxHitPoints, 2 * unbuffed.maxHitPoints)
	    << "default MinionOptions must still double the minion's HP (regression); if this fails,"
	    << " either the doubling was lost or the A/B lost RNG alignment";
	EXPECT_EQ(uniquePack.hitPoints, uniquePack.maxHitPoints);

	// (2) AI must be inherited from the leader (the unique's own mAi, which the
	// scan above guaranteed differs from the minion's own base-type ai, so this
	// assertion fails if inheritAi stops being honoured).
	EXPECT_EQ(uniquePack.ai, uniquePack.leaderAi)
	    << "default MinionOptions must still inherit the leader's AI (regression)";
	EXPECT_NE(uniquePack.ai, MonstersData[uniqueMonsterData.mtype].ai)
	    << "the inherited AI must actually differ from the minion's own base-type AI";
}

} // namespace
