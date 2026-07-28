/**
 * @file spelldat_test.cpp
 *
 * Tests for spell data loading from TSV, including description column.
 */

#include <gtest/gtest.h>

#include "ui_test.hpp"

#include "tables/spelldat.h"

namespace devilution {

class SpelldatTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadSpellData();
	}
};

TEST_F(SpelldatTest, DescriptionLoaded)
{
	const SpellData &firebolt = GetSpellData(SpellID::Firebolt);
	EXPECT_FALSE(firebolt.sDescription.empty());
	EXPECT_NE(firebolt.sDescription.find("bolt of fire"), std::string::npos);
}

TEST_F(SpelldatTest, SpellCount)
{
	// Diablo has 37 spells + Null entry at index 0
	EXPECT_GE(SpellsData.size(), 37u);
}

TEST_F(SpelldatTest, EmptyDescriptionIsValid)
{
	// Null spell (index 0) has no TSV entry, description should be empty
	const SpellData &nullSpell = GetSpellData(SpellID::Null);
	EXPECT_TRUE(nullSpell.sDescription.empty());
}

TEST_F(SpelldatTest, AllLearnableSpellsHaveDescriptions)
{
	// Diablo learnable spells (bookLevel > 0 or obtainable)
	const SpellID learnableSpells[] = {
		SpellID::Firebolt, SpellID::Healing, SpellID::Lightning,
		SpellID::Flash, SpellID::Identify, SpellID::FireWall,
		SpellID::TownPortal, SpellID::StoneCurse, SpellID::Infravision,
		SpellID::Phasing, SpellID::ManaShield, SpellID::Fireball,
		SpellID::Guardian, SpellID::ChainLightning, SpellID::FlameWave,
		SpellID::Nova, SpellID::Inferno, SpellID::Golem,
		SpellID::Rage, SpellID::Teleport, SpellID::Apocalypse,
		SpellID::Etherealize, SpellID::ItemRepair, SpellID::StaffRecharge,
		SpellID::TrapDisarm, SpellID::Elemental, SpellID::ChargedBolt,
		SpellID::HolyBolt, SpellID::Resurrect, SpellID::Telekinesis,
		SpellID::HealOther, SpellID::BloodStar, SpellID::BoneSpirit,
	};

	for (const auto &spellId : learnableSpells) {
		const SpellData &sd = GetSpellData(spellId);
		EXPECT_FALSE(sd.sDescription.empty())
		    << "Spell " << static_cast<int>(spellId) << " (" << sd.sNameText << ") has empty description";
	}
}

TEST_F(SpelldatTest, CutContentSpellsHaveEmptyDescriptions)
{
	// These spells are unused cut content and should have empty descriptions
	const SpellID cutContentSpells[] = {
		SpellID::DoomSerpents,
		SpellID::BloodRitual,
		SpellID::Invisibility,
	};

	for (const auto &spellId : cutContentSpells) {
		const SpellData &sd = GetSpellData(spellId);
		EXPECT_TRUE(sd.sDescription.empty())
		    << "Cut content spell " << static_cast<int>(spellId) << " (" << sd.sNameText << ") should have empty description";
	}
}

} // namespace devilution
