#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

TEST(AiRegistryTest, ExistingTypesHaveValidFunctionPointers)
{
	for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++) {
		auto ai = static_cast<MonsterAIID>(i);
		if (ai == MonsterAIID::FireMan)
			continue;
		size_t idx = static_cast<size_t>(i);
		ASSERT_LT(idx, AiProc.size());
		EXPECT_NE(AiProc[idx], nullptr)
		    << "AI type " << i << " has null function pointer";
	}
}
