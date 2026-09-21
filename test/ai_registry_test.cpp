#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <set>

#include <magic_enum/magic_enum.hpp>

#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

namespace {
// Defect D-1: MonsterAIID::FireMan has no implementation in AiProc, while monster data
// references it (the unique "Warpfire Hellspawn"). MonsterAIID::Custom (55) is reserved for
// data-driven/mod AI use and is likewise absent from the built-in table; the semantic enforced
// for both is "built-in data must not reference an AI that the built-in table cannot dispatch".
// Keep this list explicit and minimal: the dispatch is guarded in UpdateMonsterAi, but any
// *new* unimplemented AI must not slip in unnoticed, and removing an entry here must make
// these tests fail.
const std::set<MonsterAIID> KnownUnimplementedAis { MonsterAIID::FireMan, MonsterAIID::Custom };
} // namespace

TEST(AiRegistryTest, ExistingTypesHaveValidFunctionPointers)
{
	// Iterate every declared enum value instead of hardcoding the current last implemented one,
	// so a newly declared AI cannot silently escape this guard.
	for (const MonsterAIID ai : magic_enum::enum_values<MonsterAIID>()) {
		if (static_cast<int>(ai) < 0)
			continue; // sentinels such as MonsterAIID::Invalid are not dispatchable AIs.
			          // If a new negative sentinel is ever declared, evaluate it here explicitly.
		if (KnownUnimplementedAis.contains(ai))
			continue;
		const size_t idx = static_cast<size_t>(ai);
		ASSERT_LT(idx, AiProc.size());
		EXPECT_NE(AiProc[idx], nullptr)
		    << "AI type " << static_cast<int>(ai) << " has null function pointer";
	}
}

// Data-driven counterpart (review finding): the enum loop above cannot see whether the *data*
// references an AI that has no implementation, which is exactly how defect D-1 stayed hidden -
// the table was fine, the data was not. This walks the loaded monster and unique tables.
TEST(AiRegistryTest, DataReferencedAisAreImplemented)
{
	LoadMonsterData();
	// Never let an empty (or truncated) dataset pass silently: without a lower bound these
	// loops would iterate zero times and the test would be green for the wrong reason.
	ASSERT_GE(MonstersData.size(), 100u) << "monster table unexpectedly small; the data-driven check would be vacuous";
	ASSERT_GE(UniqueMonstersData.size(), 20u) << "unique table unexpectedly small; the data-driven check would be vacuous";
	for (const MonsterData &monsterData : MonstersData) {
		if (monsterData.availability == MonsterAvailability::Never)
			continue; // never spawns, so it can never reach the dispatch
		if (KnownUnimplementedAis.contains(monsterData.ai))
			continue;
		const size_t idx = static_cast<size_t>(monsterData.ai);
		ASSERT_LT(idx, AiProc.size());
		EXPECT_NE(AiProc[idx], nullptr) << "spawnable monster references an unimplemented AI";
	}
	for (const UniqueMonsterData &uniqueData : UniqueMonstersData) {
		if (KnownUnimplementedAis.contains(uniqueData.mAi))
			continue;
		const size_t idx = static_cast<size_t>(uniqueData.mAi);
		ASSERT_LT(idx, AiProc.size());
		EXPECT_NE(AiProc[idx], nullptr) << "unique monster references an unimplemented AI";
	}
}
