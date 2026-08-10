#include <gtest/gtest.h>

#include "items.h"
#include "options.h"
#include "stores.h"
#include "tables/itemdat.h"

namespace devilution {
namespace {

class DarkExpeditionScrollBudgetTest : public ::testing::Test {
public:
	static void SetUpTestSuite()
	{
		LoadItemData();
		LoadSpellData();
	}

	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
	}
};

bool WitchStockHasScroll(SpellID spell)
{
	for (const Item &item : WitchItems) {
		if (!item.isEmpty() && item._iMiscId == IMISC_SCROLL && item._iSpell == spell)
			return true;
	}
	return false;
}

TEST_F(DarkExpeditionScrollBudgetTest, WitchStockLacksInfravisionScroll)
{
	// Exclusion in WitchItemOk is deterministic: never present, any round.
	for (int round = 0; round < 40; round++) {
		SpawnWitch(10);
		EXPECT_FALSE(WitchStockHasScroll(SpellID::Infravision)) << "round " << round;
	}
}

TEST_F(DarkExpeditionScrollBudgetTest, WitchStockStillSellsTownPortal)
{
	// The pinned TP scroll must remain available (M2 was cut).
	SpawnWitch(10);
	EXPECT_TRUE(WitchStockHasScroll(SpellID::TownPortal));
}

} // namespace
} // namespace devilution
