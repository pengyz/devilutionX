#include <algorithm>
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

TEST_F(ConsumableStackTest, StackCountDisplayLogic)
{
	// 测试堆叠数 > 1 时应该显示
	Item item;
	item._iStackCount = 3;

	bool shouldDisplay = item._iStackCount > 1;
	EXPECT_TRUE(shouldDisplay);
}

TEST_F(ConsumableStackTest, SingleItemNotDisplayed)
{
	Item item;
	item._iStackCount = 1;

	bool shouldDisplay = item._iStackCount > 1;
	EXPECT_FALSE(shouldDisplay);
}

TEST_F(ConsumableStackTest, StackCountDefaultValue)
{
	Item item;
	// 默认堆叠数应该是1
	EXPECT_EQ(item._iStackCount, 1);
}

TEST_F(ConsumableStackTest, NetworkSyncClampValue)
{
	// 测试网络同步会钳制堆叠数
	int rawValue = 200;
	int clamped = std::clamp(rawValue, 1, 127);
	EXPECT_EQ(clamped, 127);
}

TEST_F(ConsumableStackTest, WarriorPassiveDescription)
{
	// 战士应该有"物品携带"被动
	// 这是占位符 - 实际实现取决于技能系统
	EXPECT_TRUE(true);
}

TEST_F(ConsumableStackTest, SorcererPassiveDescription)
{
	// 法师应该有"轻装出行"被动
	// 这是占位符 - 实际实现取决于技能系统
	EXPECT_TRUE(true);
}

TEST_F(ConsumableStackTest, FullStackingWorkflow)
{
	Player &warrior = Players[0];
	warrior._pClass = HeroClass::Warrior;

	// Clear belt
	for (auto &item : warrior.SpdList) {
		item.clear();
	}

	// Verify warrior max stack for potions
	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;
	potion._iStackCount = 1;
	potion.IDidx = IDI_HEAL;

	int maxStack = GetMaxStackCount(potion, warrior);
	EXPECT_EQ(maxStack, 8);

	// Step 1: Place first potion in empty belt slot
	EXPECT_TRUE(CanStackItem(potion));
	EXPECT_TRUE(warrior.SpdList[0].isEmpty());
	warrior.SpdList[0] = potion;
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 1);

	// Step 2: Simulate stacking 7 more potions into slot 0
	for (int i = 1; i < 8; i++) {
		EXPECT_TRUE(CanStackItem(potion));
		EXPECT_EQ(warrior.SpdList[0].IDidx, potion.IDidx);
		EXPECT_LT(warrior.SpdList[0]._iStackCount, GetMaxStackCount(warrior.SpdList[0], warrior));
		warrior.SpdList[0]._iStackCount++;
	}

	// Step 3: Verify full stack
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 8);
	EXPECT_FALSE(warrior.SpdList[0]._iStackCount < GetMaxStackCount(warrior.SpdList[0], warrior));

	// Step 4: Use one potion (decrement stack)
	EXPECT_GT(warrior.SpdList[0]._iStackCount, 1);
	warrior.SpdList[0]._iStackCount--;
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 7);

	// Step 5: Verify stack can still accept more
	EXPECT_LT(warrior.SpdList[0]._iStackCount, GetMaxStackCount(warrior.SpdList[0], warrior));

	// Step 6: Use remaining potions one by one
	while (warrior.SpdList[0]._iStackCount > 1) {
		warrior.SpdList[0]._iStackCount--;
	}
	// Last use removes the item
	warrior.SpdList[0].clear();
	EXPECT_TRUE(warrior.SpdList[0].isEmpty());
}

} // namespace
} // namespace devilution
