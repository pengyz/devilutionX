#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "quests.h"

using namespace devilution;

namespace {

void initQuestArray()
{
	for (auto &quest : Quests)
		quest._qactive = QUEST_INIT;
}

}

TEST(QuestScriptTest, QuestDataHasScriptNameField)
{
	QuestData data {};
	EXPECT_TRUE(data.scriptName.empty());
	data.scriptName = "test_quest";
	EXPECT_EQ(data.scriptName, "test_quest");
}

TEST(QuestScriptTest, AllExistingQuestsHaveEmptyScriptByDefault)
{
	LoadQuestData();
	ASSERT_GT(QuestsData.size(), 0u);
	for (size_t i = 0; i < QuestsData.size(); i++) {
		EXPECT_TRUE(QuestsData[i].scriptName.empty())
		    << "Quest " << i << " has non-empty scriptName";
	}
}

TEST(QuestScriptTest, EmptyScriptDoesNotCrash)
{
	initQuestArray();
	Quests[0]._qactive = QUEST_ACTIVE;
	QuestsData[0].scriptName.clear();

	EXPECT_NO_THROW({ CheckQuests(); });
}

TEST(QuestScriptTest, ExistingLogicUnaffectedByEmptyScript)
{
	initQuestArray();
	Quests[Q_SKELKING]._qactive = QUEST_ACTIVE;
	Quests[Q_SKELKING]._qlevel = 0;
	QuestsData[Q_SKELKING].scriptName.clear();

	EXPECT_NO_THROW({ CheckQuests(); });
	EXPECT_EQ(Quests[Q_SKELKING]._qactive, QUEST_ACTIVE);
}
