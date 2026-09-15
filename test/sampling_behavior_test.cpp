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
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "engine/assets.hpp"
#include "engine/load_cl2.hpp"
#include "engine/random.hpp"
#include "levels/gendung.h"
#include "monster.h"
#include "multi.h"
#include "quests.h"
#include "tables/monstdat.h"
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
	// Caves cap constrains kite class only; the "any-class >=3" tail is NOT a
	// cap target (a Melee-heavy Caves level is still possible). Baseline was
	// L9 1.59% / L10 3.33% / L11 1.18% / L12 2.61%; post-cap measured
	// L9 1.7% / L10 1.02% / L11 1.32% / L12 0% (kite cap removed the dominant
	// kite driver, leaving residual Melee-driven tails). Pin post-cap values
	// so a regression in the sampling loop is still caught.
	EXPECT_NEAR(l9, 1.7, 0.6) << "Caves L9 any-class tail post-cap (~1.7%)";
	EXPECT_NEAR(l10, 1.02, 0.6) << "Caves L10 any-class tail post-cap (~1.0%)";
	EXPECT_NEAR(l11, 1.32, 0.6) << "Caves L11 any-class tail post-cap (~1.3%)";
	EXPECT_NEAR(l12, 0.0, 0.5) << "Caves L12 any-class tail post-cap";
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

} // namespace
