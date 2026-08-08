#include <gtest/gtest.h>

#include "items.h"
#include "inv.h"
#include "player.h"

namespace devilution {
namespace {

class ConsumeScrollTest : public ::testing::Test {
public:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
		*MyPlayer = {};
	}
};

TEST_F(ConsumeScrollTest, StackOfThreeConsumesOne)
{
	// Stack of 3 scrolls: using one leaves 2 (fixes whole-stack removal bug).
	Item item;
	item._iMiscId = IMISC_SCROLL;
	item._iSpell = SpellID::Firebolt;
	item._iStackCount = 3;

	int removeCalls = 0;
	ConsumeOneOrRemove(item, [&removeCalls]() { removeCalls++; });

	EXPECT_EQ(item._iStackCount, 2);
	EXPECT_EQ(removeCalls, 0); // whole-item removal not called
}

TEST_F(ConsumeScrollTest, SingleScrollRemovesWhole)
{
	Item item;
	item._iMiscId = IMISC_SCROLL;
	item._iSpell = SpellID::Firebolt;
	item._iStackCount = 1;

	int removeCalls = 0;
	ConsumeOneOrRemove(item, [&removeCalls]() { removeCalls++; });

	EXPECT_EQ(removeCalls, 1); // exhausted stack removes the item
}

TEST_F(ConsumeScrollTest, StackOfFiveConsumesOne)
{
	Item item;
	item._iMiscId = IMISC_SCROLL;
	item._iSpell = SpellID::TownPortal;
	item._iStackCount = 5;

	int removeCalls = 0;
	ConsumeOneOrRemove(item, [&removeCalls]() { removeCalls++; });

	EXPECT_EQ(item._iStackCount, 4);
	EXPECT_EQ(removeCalls, 0);
}

} // namespace
} // namespace devilution
