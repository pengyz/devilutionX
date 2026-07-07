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

} // namespace
} // namespace devilution
