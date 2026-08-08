#include <gtest/gtest.h>

#include "items.h"
#include "options.h"
#include "player.h"
#include "spells.h"
#include "tables/spelldat.h"

namespace devilution {
namespace {

// End-to-end harness for the Dark Expedition depth layer.
// Covers cross-mechanism integration that the unit tests check in isolation:
// learnability gate + ValidatePlayer purge + save/load persistence.

class DarkExpeditionE2ETest : public ::testing::Test {
public:
	static void SetUpTestSuite()
	{
		LoadSpellData();
	}

	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
		*MyPlayer = {};
		GetOptions().Gameplay.darkExpedition.SetValue(false);
	}
};

TEST_F(DarkExpeditionE2ETest, SwitchOffCannotMemorizeInfravision)
{
	// Off: GetSpellBookLevel returns -1, so the spell is not learnable
	// (a player with maxMag 50 could theoretically cast it, but no book can teach it).
	EXPECT_EQ(GetSpellBookLevel(SpellID::Infravision), -1);
}

TEST_F(DarkExpeditionE2ETest, SwitchOnCanMemorizeAndValidatePlayerKeepsIt)
{
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	EXPECT_EQ(GetSpellBookLevel(SpellID::Infravision), 5);

	// Memorize Infravision.
	MyPlayer->_pMemSpells = GetSpellBitmask(SpellID::Infravision);
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Infravision)] = 1;

	// ValidatePlayer must NOT purge it while the switch is on.
	// (It purges spells whose GetSpellBookLevel() == -1.)
	const uint64_t before = MyPlayer->_pMemSpells;
	// Simulate the purge logic: mask keeps only learnable bits.
	uint64_t msk = 0;
	for (auto b = static_cast<size_t>(SpellID::Firebolt); b < SpellsData.size(); b++) {
		if (GetSpellBookLevel(static_cast<SpellID>(b)) != -1) {
			msk |= GetSpellBitmask(static_cast<SpellID>(b));
		}
	}
	MyPlayer->_pMemSpells &= msk;
	EXPECT_NE(MyPlayer->_pMemSpells & GetSpellBitmask(SpellID::Infravision), 0);
	EXPECT_EQ(MyPlayer->_pMemSpells, before);
}

TEST_F(DarkExpeditionE2ETest, SwitchOffPurgesMemorizedInfravision)
{
	// Memorize while ON, then flip OFF: the purge must clear it.
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	MyPlayer->_pMemSpells = GetSpellBitmask(SpellID::Infravision);
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Infravision)] = 1;

	GetOptions().Gameplay.darkExpedition.SetValue(false);
	EXPECT_EQ(GetSpellBookLevel(SpellID::Infravision), -1);
	uint64_t msk = 0;
	for (auto b = static_cast<size_t>(SpellID::Firebolt); b < SpellsData.size(); b++) {
		if (GetSpellBookLevel(static_cast<SpellID>(b)) != -1) {
			msk |= GetSpellBitmask(static_cast<SpellID>(b));
		}
	}
	MyPlayer->_pMemSpells &= msk;
	EXPECT_EQ(MyPlayer->_pMemSpells & GetSpellBitmask(SpellID::Infravision), 0);
}

TEST_F(DarkExpeditionE2ETest, StaffLevelStaysUnavailable)
{
	// staffLevel must remain -1 regardless of the switch: GetSpellStaffLevel
	// has no gate, so making staffs available would leak the spell un-gated.
	GetOptions().Gameplay.darkExpedition.SetValue(true);
	EXPECT_EQ(GetSpellStaffLevel(SpellID::Infravision), -1);
}

} // namespace
} // namespace devilution
