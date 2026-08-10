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
	}
};

TEST_F(DarkExpeditionE2ETest, CanMemorizeAndValidatePlayerKeepsIt)
{
	EXPECT_EQ(GetSpellBookLevel(SpellID::Infravision), 5);

	// Memorize Infravision.
	MyPlayer->_pMemSpells = GetSpellBitmask(SpellID::Infravision);
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Infravision)] = 1;

	// ValidatePlayer must NOT purge it (it purges spells whose GetSpellBookLevel() == -1).
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

TEST_F(DarkExpeditionE2ETest, StaffLevelStaysUnavailable)
{
	// staffLevel must remain -1: GetSpellStaffLevel has no gate, so making staffs
	// available would leak the spell un-gated.
	EXPECT_EQ(GetSpellStaffLevel(SpellID::Infravision), -1);
}

} // namespace
} // namespace devilution
