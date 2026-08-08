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



TEST_F(ConsumeScrollTest, FallbackPathConsumesOneFromStack)
{
	// spellFrom=0 forces the fallback path (hotkey / spell-slot cast).
	// A stack of 3 in the inventory must drop to 2, not vanish.
	Item scroll {};
	scroll._itype = ItemType::Misc;
	scroll._iMiscId = IMISC_SCROLL;
	scroll._iSpell = SpellID::Firebolt;
	scroll._iStackCount = 3;
	MyPlayer->InvList[0] = scroll;
	MyPlayer->_pNumInv = 1;

	MyPlayer->executedSpell.spellId = SpellID::Firebolt;
	MyPlayer->executedSpell.spellFrom = 0; // fallback path
	ConsumeScroll(*MyPlayer);

	EXPECT_EQ(MyPlayer->InvList[0]._iStackCount, 2);
	EXPECT_EQ(MyPlayer->_pNumInv, 1); // item stays, stack decremented
}

TEST_F(ConsumeScrollTest, FallbackPathRemovesExhaustedStack)
{
	Item scroll {};
	scroll._itype = ItemType::Misc;
	scroll._iMiscId = IMISC_SCROLL;
	scroll._iSpell = SpellID::Firebolt;
	scroll._iStackCount = 1;
	MyPlayer->InvList[0] = scroll;
	MyPlayer->_pNumInv = 1;

	MyPlayer->executedSpell.spellId = SpellID::Firebolt;
	MyPlayer->executedSpell.spellFrom = 0;
	ConsumeScroll(*MyPlayer);

	EXPECT_TRUE(MyPlayer->InvList[0].isEmpty());
	EXPECT_EQ(MyPlayer->_pNumInv, 0); // exhausted stack removed
}
} // namespace
} // namespace devilution
