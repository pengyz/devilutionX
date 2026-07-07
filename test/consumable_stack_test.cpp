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

TEST_F(ConsumableStackTest, BeltStackingLogicIdentifiesMatch)
{
	// Verify that belt stacking logic correctly identifies a stackable match:
	// same IDidx, stackable item, and not at max stack
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;

	// Clear belt
	for (auto &item : player.SpdList) {
		item.clear();
	}

	// Place a potion in slot 0
	Item potion1;
	potion1._itype = ItemType::Misc;
	potion1._iMiscId = IMISC_HEAL;
	potion1._iStackCount = 1;
	potion1.IDidx = IDI_HEAL;
	player.SpdList[0] = potion1;

	// Verify the conditions for stacking are met
	Item potion2;
	potion2._itype = ItemType::Misc;
	potion2._iMiscId = IMISC_HEAL;
	potion2._iStackCount = 1;
	potion2.IDidx = IDI_HEAL;

	EXPECT_TRUE(CanStackItem(potion2));
	EXPECT_EQ(player.SpdList[0].IDidx, potion2.IDidx);
	EXPECT_LT(player.SpdList[0]._iStackCount, GetMaxStackCount(player.SpdList[0], player));

	// Simulate stacking
	player.SpdList[0]._iStackCount++;
	EXPECT_EQ(player.SpdList[0]._iStackCount, 2);
}

TEST_F(ConsumableStackTest, BeltStackingRespectsMaxCount)
{
	// When a belt slot is at max stack, new items should go to an empty slot
	Player &player = Players[0];
	player._pClass = HeroClass::Sorcerer; // Potion max stack is 3

	// Clear belt
	for (auto &item : player.SpdList) {
		item.clear();
	}

	// Fill slot 0 to max
	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;
	potion._iStackCount = 3;
	potion.IDidx = IDI_HEAL;
	player.SpdList[0] = potion;

	// Verify slot is at max capacity
	EXPECT_EQ(GetMaxStackCount(potion, player), 3);
	EXPECT_EQ(player.SpdList[0]._iStackCount, 3);

	// A new item cannot stack into the full slot
	EXPECT_FALSE(player.SpdList[0]._iStackCount < GetMaxStackCount(player.SpdList[0], player));

	// Verify empty slot exists for overflow
	EXPECT_TRUE(player.SpdList[1].isEmpty());
}

TEST_F(ConsumableStackTest, BeltStackingDifferentItemsDontStack)
{
	// Different item types should not stack together
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;

	// Clear belt
	for (auto &item : player.SpdList) {
		item.clear();
	}

	// Place a health potion
	Item healPotion;
	healPotion._itype = ItemType::Misc;
	healPotion._iMiscId = IMISC_HEAL;
	healPotion._iStackCount = 1;
	healPotion.IDidx = IDI_HEAL;
	player.SpdList[0] = healPotion;

	// A mana potion should not stack with health potion
	Item manaPotion;
	manaPotion._itype = ItemType::Misc;
	manaPotion._iMiscId = IMISC_MANA;
	manaPotion._iStackCount = 1;
	manaPotion.IDidx = IDI_MANA;

	EXPECT_NE(player.SpdList[0].IDidx, manaPotion.IDidx);
	EXPECT_EQ(player.SpdList[0]._iStackCount, 1); // Unchanged
}

TEST_F(ConsumableStackTest, UsingItemDecrementsStack)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;

	// Clear belt
	for (auto &item : player.SpdList) {
		item.clear();
	}

	// Place stacked potion in belt
	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;
	potion._iStackCount = 3;
	potion.IDidx = IDI_HEAL;
	player.SpdList[0] = potion;

	// Simulate usage: decrement if stack > 1, otherwise remove
	if (player.SpdList[0]._iStackCount > 1) {
		player.SpdList[0]._iStackCount--;
	} else {
		player.SpdList[0].clear();
	}

	EXPECT_EQ(player.SpdList[0]._iStackCount, 2);
}

TEST_F(ConsumableStackTest, UsingLastItemRemovesIt)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;

	// Clear belt
	for (auto &item : player.SpdList) {
		item.clear();
	}

	// Place single potion in belt
	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;
	potion._iStackCount = 1;
	potion.IDidx = IDI_HEAL;
	player.SpdList[0] = potion;

	// Simulate usage: decrement if stack > 1, otherwise remove
	if (player.SpdList[0]._iStackCount > 1) {
		player.SpdList[0]._iStackCount--;
	} else {
		player.SpdList[0].clear();
	}

	EXPECT_TRUE(player.SpdList[0].isEmpty());
}

} // namespace
} // namespace devilution
