#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "monster.h"
#include "quests.h"
#include "tables/misdat.h"
#include "tables/monstdat.h"

using namespace devilution;

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
