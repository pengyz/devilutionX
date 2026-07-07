#include <gtest/gtest.h>

#include "quests.h"
#include "tables/objdat.h"
#include "world_state.h"

namespace devilution {
namespace {

class WorldStateTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		// Initialize quest state for testing
		for (auto &quest : Quests) {
			quest._qactive = QUEST_NOTAVAIL;
		}
	}
};

TEST_F(WorldStateTest, WorldStateChangesAfterBossKill)
{
	// After killing Skeleton King, world state should change
	Quests[Q_SKELKING]._qactive = QUEST_DONE;

	WorldState state = GetWorldState();
	EXPECT_GE(state.stage, 1); // At least stage 1
	EXPECT_TRUE(state.skeletonKingDefeated);
}

TEST_F(WorldStateTest, TristramDarknessIncreases)
{
	// As world state progresses, Tristram should get darker
	WorldState state;
	state.stage = 3; // Late game

	int darkness = GetTristramDarkness(state);
	EXPECT_GT(darkness, 0); // Should have some darkness
	EXPECT_EQ(darkness, 6); // Stage 3 = 6 darkness
}

TEST_F(WorldStateTest, DungeonCorruptionIncreases)
{
	// As world state progresses, dungeon should get more corrupted
	WorldState state;
	state.stage = 2; // Mid game

	int corruption = GetDungeonCorruption(state);
	EXPECT_EQ(corruption, 2); // Stage 2 = 2 corruption
}

TEST_F(WorldStateTest, InitialWorldStateIsNormal)
{
	// Initial world state should be normal
	WorldState state = GetWorldState();
	EXPECT_EQ(state.stage, 0);
	EXPECT_FALSE(state.skeletonKingDefeated);
	EXPECT_FALSE(state.enteredCaves);
	EXPECT_FALSE(state.enteredHell);
	EXPECT_FALSE(state.nearDiablo);
}

} // namespace
} // namespace devilution
