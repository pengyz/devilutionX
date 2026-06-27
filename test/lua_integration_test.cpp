#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "monster.h"
#include "quests.h"
#include "tables/misdat.h"
#include "tables/monstdat.h"

using namespace devilution;

namespace {

int g_aiCalled = 0;
void IntegrationTestAi(Monster &)
{
	g_aiCalled++;
}

void IntegrationTestAddFn(Missile &, AddMissileParameter &) {}

}

TEST(LuaIntegrationTest, AiRegistrationAndDispatchWorkTogether)
{
	g_aiCalled = 0;
	constexpr MonsterAIID customAi = static_cast<MonsterAIID>(55);
	RegisterAiFunction(customAi, IntegrationTestAi);

	Monster monster {};
	monster.ai = customAi;
	AiProc[static_cast<size_t>(customAi)](monster);
	EXPECT_EQ(g_aiCalled, 1);

	AiProc[static_cast<size_t>(customAi)](monster);
	EXPECT_EQ(g_aiCalled, 2);
}

TEST(LuaIntegrationTest, MissileAddFnRegistrationAndParse)
{
	RegisterMissileAddFn("IntegrationTestMissile", IntegrationTestAddFn);
	auto result = ParseMissileAddFn("IntegrationTestMissile");
	ASSERT_TRUE(result.has_value());
	EXPECT_NE(*result, nullptr);
}

TEST(LuaIntegrationTest, QuestScriptFieldAndCheckQuestsCoexist)
{
	LoadQuestData();
	ASSERT_GT(QuestsData.size(), 0u);

	Quests[0]._qactive = QUEST_ACTIVE;
	QuestsData[0].scriptName = "";

	EXPECT_NO_THROW({ CheckQuests(); });
	EXPECT_EQ(QuestsData[0].scriptName, "");
}

TEST(LuaIntegrationTest, AiProcSizeFitsAllBuiltinTypes)
{
	EXPECT_GE(AiProc.size(), 64u);
	for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++) {
		if (static_cast<MonsterAIID>(i) == MonsterAIID::FireMan) continue;
		EXPECT_NE(AiProc[static_cast<size_t>(i)], nullptr);
	}
	EXPECT_EQ(AiProc[static_cast<size_t>(MonsterAIID::FireMan)], nullptr);
}
