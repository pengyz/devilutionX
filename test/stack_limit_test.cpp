#include <gtest/gtest.h>

#include "items.h"
#include "player.h"
#include "tables/itemdat.h"
#include "tables/playerdat.hpp"

namespace devilution {
namespace {

class StackLimitTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

TEST_F(StackLimitTest, WarriorHasHigherPotionStackLimit)
{
	Player &warrior = Players[0];
	warrior._pClass = HeroClass::Warrior;

	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	int stackLimit = GetMaxStackCount(potion, warrior);
	EXPECT_EQ(stackLimit, 8); // Base 5 + 3 warrior bonus
}

TEST_F(StackLimitTest, SorcererHasLowerPotionStackLimit)
{
	Player &sorcerer = Players[0];
	sorcerer._pClass = HeroClass::Sorcerer;

	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	int stackLimit = GetMaxStackCount(potion, sorcerer);
	EXPECT_EQ(stackLimit, 3); // Base 5 - 2 sorcerer penalty
}

TEST_F(StackLimitTest, RogueHasBaseStackLimit)
{
	Player &rogue = Players[0];
	rogue._pClass = HeroClass::Rogue;

	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	int stackLimit = GetMaxStackCount(potion, rogue);
	EXPECT_EQ(stackLimit, 5); // Base 5
}

} // namespace
} // namespace devilution
