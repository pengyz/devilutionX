#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <set>

#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

namespace {
// Defect D-1: MonsterAIID::FireMan has no implementation in AiProc, while monster data
// references it (the unique "Warpfire Hellspawn"). Keep this list explicit and minimal: the
// dispatch is guarded in UpdateMonsterAi, but any *new* unimplemented AI must not slip in
// unnoticed, and removing an entry here must make this test fail.
const std::set<MonsterAIID> KnownUnimplementedAis { MonsterAIID::FireMan };
} // namespace

TEST(AiRegistryTest, ExistingTypesHaveValidFunctionPointers)
{
	for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++) {
		auto ai = static_cast<MonsterAIID>(i);
		if (KnownUnimplementedAis.contains(ai))
			continue;
		size_t idx = static_cast<size_t>(i);
		ASSERT_LT(idx, AiProc.size());
		EXPECT_NE(AiProc[idx], nullptr)
		    << "AI type " << i << " has null function pointer";
	}
}
