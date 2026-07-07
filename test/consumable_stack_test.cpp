#include <gtest/gtest.h>

#include "items.h"
#include "player.h"

namespace devilution {
namespace {

class ConsumableStackTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

TEST_F(ConsumableStackTest, ItemHasStackCount)
{
	Item item;
	item._itype = ItemType::Misc;
	item._iMiscId = IMISC_HEAL;
	item._iStackCount = 1;

	EXPECT_EQ(item._iStackCount, 1);
}

TEST_F(ConsumableStackTest, DefaultStackCountIsOne)
{
	Item item;
	item._itype = ItemType::Misc;
	item._iMiscId = IMISC_HEAL;

	EXPECT_EQ(item._iStackCount, 1);
}

TEST_F(ConsumableStackTest, HealthPotionCanStack)
{
	Item item;
	item._itype = ItemType::Misc;
	item._iMiscId = IMISC_HEAL;

	EXPECT_TRUE(CanStackItem(item));
}

TEST_F(ConsumableStackTest, ManaPotionCanStack)
{
	Item item;
	item._itype = ItemType::Misc;
	item._iMiscId = IMISC_MANA;

	EXPECT_TRUE(CanStackItem(item));
}

TEST_F(ConsumableStackTest, ScrollCanStack)
{
	Item item;
	item._itype = ItemType::Misc;
	item._iMiscId = IMISC_SCROLL;

	EXPECT_TRUE(CanStackItem(item));
}

TEST_F(ConsumableStackTest, EquipmentCannotStack)
{
	Item item;
	item._itype = ItemType::Sword;

	EXPECT_FALSE(CanStackItem(item));
}

TEST_F(ConsumableStackTest, QuestItemCannotStack)
{
	Item item;
	item._itype = ItemType::Misc;
	item._iMiscId = IMISC_FULLHEAL;

	EXPECT_FALSE(CanStackItem(item));
}

TEST_F(ConsumableStackTest, WarriorHasHigherPotionStackLimit)
{
	Player &warrior = Players[0];
	warrior._pClass = HeroClass::Warrior;

	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	int maxStack = GetMaxStackCount(potion, warrior);
	EXPECT_EQ(maxStack, 8); // Base 5 + 3 warrior bonus
}

TEST_F(ConsumableStackTest, SorcererHasLowerPotionStackLimit)
{
	Player &sorcerer = Players[0];
	sorcerer._pClass = HeroClass::Sorcerer;

	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	int maxStack = GetMaxStackCount(potion, sorcerer);
	EXPECT_EQ(maxStack, 3); // Base 5 - 2 sorcerer penalty
}

TEST_F(ConsumableStackTest, RogueHasBasePotionStackLimit)
{
	Player &rogue = Players[0];
	rogue._pClass = HeroClass::Rogue;

	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	int maxStack = GetMaxStackCount(potion, rogue);
	EXPECT_EQ(maxStack, 5); // Base 5
}

TEST_F(ConsumableStackTest, WarriorHasHigherScrollStackLimit)
{
	Player &warrior = Players[0];
	warrior._pClass = HeroClass::Warrior;

	Item scroll;
	scroll._itype = ItemType::Misc;
	scroll._iMiscId = IMISC_SCROLL;

	int maxStack = GetMaxStackCount(scroll, warrior);
	EXPECT_EQ(maxStack, 4); // Base 3 + 1 warrior bonus
}

TEST_F(ConsumableStackTest, ScrollBaseStackLimit)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Rogue;

	Item scroll;
	scroll._itype = ItemType::Misc;
	scroll._iMiscId = IMISC_SCROLL;

	int maxStack = GetMaxStackCount(scroll, player);
	EXPECT_EQ(maxStack, 3); // Base 3
}

TEST_F(ConsumableStackTest, NonStackableItemReturnsOne)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;

	Item sword;
	sword._itype = ItemType::Sword;

	int maxStack = GetMaxStackCount(sword, player);
	EXPECT_EQ(maxStack, 1);
}

} // namespace
} // namespace devilution
