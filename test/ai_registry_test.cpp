#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

namespace {

int g_testAiCalledWith = -1;
int g_testAiFirstCalled = 0;
int g_testAiSecondCalled = 0;

void TestSetGoalFn(Monster &monster)
{
	g_testAiCalledWith = static_cast<int>(monster.ai);
	monster.goal = MonsterGoal::Normal;
}

void TestFirstFn(Monster &)
{
	g_testAiFirstCalled++;
}

void TestSecondFn(Monster &)
{
	g_testAiSecondCalled++;
}

}

TEST(AiRegistryTest, ExistingTypesHaveValidFunctionPointers)
{
	for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++) {
		auto ai = static_cast<MonsterAIID>(i);
		if (ai == MonsterAIID::FireMan)
			continue;
		size_t idx = static_cast<size_t>(i);
		ASSERT_LT(idx, AiProc.size());
		if (ai != MonsterAIID::FireMan) {
			EXPECT_NE(AiProc[idx], nullptr)
			    << "AI type " << i << " has null function pointer";
		}
	}
}

TEST(AiRegistryTest, RegisterCustomAiFunction)
{
	constexpr MonsterAIID customId = static_cast<MonsterAIID>(55);
	g_testAiCalledWith = -1;

	RegisterAiFunction(customId, TestSetGoalFn);

	Monster monster {};
	monster.ai = customId;

	AiProc[static_cast<size_t>(customId)](monster);

	EXPECT_EQ(g_testAiCalledWith, static_cast<int>(customId));
	EXPECT_EQ(monster.goal, MonsterGoal::Normal);
}

TEST(AiRegistryTest, OverwriteRegisteredFunction)
{
	constexpr MonsterAIID customId = static_cast<MonsterAIID>(56);
	g_testAiFirstCalled = 0;
	g_testAiSecondCalled = 0;

	RegisterAiFunction(customId, TestFirstFn);
	RegisterAiFunction(customId, TestSecondFn);

	Monster monster {};
	monster.ai = customId;
	AiProc[static_cast<size_t>(customId)](monster);

	EXPECT_EQ(g_testAiFirstCalled, 0);
	EXPECT_EQ(g_testAiSecondCalled, 1);
}

TEST(AiRegistryTest, FallbackAiDoesNotCrash)
{
	constexpr MonsterAIID customId = static_cast<MonsterAIID>(57);

	RegisterAiFunction(customId, nullptr);

	Monster monster {};
	monster.ai = customId;
	monster.mode = MonsterMode::Stand;
	monster.hitPoints = 10;

	EXPECT_NO_THROW({
		AiProc[static_cast<size_t>(customId)](monster);
	});

	SUCCEED();
}
