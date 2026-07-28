#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "quests.h"

using namespace devilution;

namespace {

// QuestsData is global and starts out empty. Two of these tests index into it,
// so every test loads it in SetUp rather than relying on an earlier test in the
// same binary having done so. Without this, running a single test in isolation
// (which is how ctest invokes them, one --gtest_filter per test) dereferences a
// null pointer inside std::vector::operator[].
//
// LoadQuestData clears QuestsData before repopulating it, so repeated calls are
// safe.
class QuestScriptTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		LoadQuestData();
		ASSERT_GT(QuestsData.size(), 0u) << "quest data failed to load";
	}

	static void InitQuestArray()
	{
		for (auto &quest : Quests)
			quest._qactive = QUEST_INIT;
	}
};

} // namespace

TEST_F(QuestScriptTest, QuestDataHasScriptNameField)
{
	QuestData data {};
	EXPECT_TRUE(data.scriptName.empty());
	data.scriptName = "test_quest";
	EXPECT_EQ(data.scriptName, "test_quest");
}

TEST_F(QuestScriptTest, AllExistingQuestsHaveEmptyScriptByDefault)
{
	for (size_t i = 0; i < QuestsData.size(); i++) {
		EXPECT_TRUE(QuestsData[i].scriptName.empty())
		    << "Quest " << i << " has non-empty scriptName";
	}
}

TEST_F(QuestScriptTest, EmptyScriptDoesNotCrash)
{
	InitQuestArray();
	Quests[0]._qactive = QUEST_ACTIVE;
	QuestsData[0].scriptName.clear();

	EXPECT_NO_THROW({ CheckQuests(); });
}

TEST_F(QuestScriptTest, ExistingLogicUnaffectedByEmptyScript)
{
	ASSERT_GT(QuestsData.size(), static_cast<size_t>(Q_SKELKING));

	InitQuestArray();
	Quests[Q_SKELKING]._qactive = QUEST_ACTIVE;
	Quests[Q_SKELKING]._qlevel = 0;
	QuestsData[Q_SKELKING].scriptName.clear();

	EXPECT_NO_THROW({ CheckQuests(); });
	EXPECT_EQ(Quests[Q_SKELKING]._qactive, QUEST_ACTIVE);
}
