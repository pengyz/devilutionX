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
		GetOptions().Gameplay.darkExpedition.SetValue(false);
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

TEST_F(DarkExpeditionScrollBudgetTest, WitchStockHasInfravisionScrollWhenSwitchOff)
{
	// Stock selection is random (17 candidates, ~10 picked); sample enough rounds
	// that a present candidate is overwhelmingly likely to appear at least once.
	bool found = false;
	for (int round = 0; round < 40 && !found; round++) {
		SpawnWitch(10); // level 10 so the iMinMLvl-8 Infravision scroll can appear
		found = WitchStockHasScroll(SpellID::Infravision);
	}
	EXPECT_TRUE(found);
}

TEST_F(DarkExpeditionScrollBudgetTest, WitchStockLacksInfravisionScrollWhenSwitchOn)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	// Exclusion in WitchItemOk is deterministic: never present, any round.
	for (int round = 0; round < 40; round++) {
		SpawnWitch(10);
		EXPECT_FALSE(WitchStockHasScroll(SpellID::Infravision)) << "round " << round;
	}
}

TEST_F(DarkExpeditionScrollBudgetTest, WitchStockStillSellsTownPortalWhenSwitchOn)
{
	// The pinned TP scroll must remain available (M2 was cut).
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	SpawnWitch(10);
	EXPECT_TRUE(WitchStockHasScroll(SpellID::TownPortal));
}

} // namespace
} // namespace devilution
