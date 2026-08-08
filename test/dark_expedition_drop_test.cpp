#include <gtest/gtest.h>

#include "items.h"
#include "options.h"
#include "tables/itemdat.h"

namespace devilution {
namespace {

class DarkExpeditionDropTest : public ::testing::Test {
public:
	static void SetUpTestSuite()
	{
		LoadItemData();
		LoadSpellData();
	}

	void SetUp() override
	{
		GetOptions().Gameplay.darkExpedition.SetValue(false);
	}
};

const ItemData *FindItemByIndex(int index)
{
	return &AllItemsList[index];
}

TEST_F(DarkExpeditionDropTest, SwitchOffAllowsAll)
{
	// Off: no exclusions (red line 13).
	const ItemData &rune = *FindItemByIndex(161); // Rune of Fire
	const ItemData &fireWall = *FindItemByIndex(96); // Scroll of Fire Wall
	EXPECT_TRUE(DarkExpeditionDropOk(rune));
	EXPECT_TRUE(DarkExpeditionDropOk(fireWall));
}

TEST_F(DarkExpeditionDropTest, SwitchOnExcludesRunes)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	for (int idx : { 161, 162, 163, 164, 165 }) { // Runes
		EXPECT_FALSE(DarkExpeditionDropOk(*FindItemByIndex(idx))) << "rune index " << idx;
	}
}

TEST_F(DarkExpeditionDropTest, SwitchOnExcludesDamageScrolls)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	// SCROLLT damage scrolls (dropRate=1): Lightning(95), Fire Wall(98), Inferno(99), Flash(101)...
	const ItemData &lightning = *FindItemByIndex(93);
	const ItemData &fireWall = *FindItemByIndex(96);
	const ItemData &inferno = *FindItemByIndex(97);
	EXPECT_FALSE(DarkExpeditionDropOk(lightning));
	EXPECT_FALSE(DarkExpeditionDropOk(fireWall));
	EXPECT_FALSE(DarkExpeditionDropOk(inferno));
}

TEST_F(DarkExpeditionDropTest, SwitchOnKeepsResurrect)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	// Resurrect is a multiplayer essential — must stay droppable.
	const ItemData &resurrect = *FindItemByIndex(95); // Scroll of Resurrect (dropRate=1)
	EXPECT_TRUE(DarkExpeditionDropOk(resurrect));
}

TEST_F(DarkExpeditionDropTest, SwitchOnKeepsUtilityScrolls)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	// Utility scrolls (SCROLL, not SCROLLT) stay: Identify(96), TownPortal(100).
	const ItemData &identify = *FindItemByIndex(94);
	const ItemData &tp = *FindItemByIndex(98);
	EXPECT_TRUE(DarkExpeditionDropOk(identify));
	EXPECT_TRUE(DarkExpeditionDropOk(tp));
}

TEST_F(DarkExpeditionDropTest, IsItemAvailableUnchangedForRunes)
{
	// The drop filter must NOT touch IsItemAvailable (save/network collateral).
	// In HF, runes remain IsItemAvailable (they just don't drop).
	gbIsHellfire = true;
	for (int idx : { 161, 162, 163, 164, 165 }) {
		EXPECT_TRUE(IsItemAvailable(idx)) << "rune index " << idx << " must stay available in HF";
	}
}

} // namespace
} // namespace devilution
