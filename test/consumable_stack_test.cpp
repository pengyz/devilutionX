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

} // namespace
} // namespace devilution
