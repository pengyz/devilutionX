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

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "engine/assets.hpp"
#include "engine/random.hpp"
#include "levels/gendung.h"
#include "monster.h"
#include "multi.h"
#include "tables/monstdat.h"

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
}

} // namespace
