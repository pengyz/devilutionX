#include <gtest/gtest.h>

#include "panels/spell_book.hpp"
#include "player.h"
#include "tables/playerdat.hpp"
#include "tables/spelldat.h"

namespace devilution {
namespace {

class SpellUXTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		Players.resize(1);
		MyPlayer = &Players[0];
		LoadSpellData();
		LoadPlayerDataFiles();
	}
};

TEST_F(SpellUXTest, SpellRequirementShown)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;
	player._pMagic = 10;

	// Get spell data for Firebolt (requires 15 magic)
	const SpellData &spellData = GetSpellData(SpellID::Firebolt);

	// Should show current magic and requirement
	std::string requirementText = GetSpellRequirementText(spellData, player);
	EXPECT_NE(requirementText.find("15"), std::string::npos);
	EXPECT_NE(requirementText.find("10"), std::string::npos);
}

TEST_F(SpellUXTest, SpellCanLearnStatus)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;
	player._pMagic = 10;

	// Firebolt requires 15 magic, warrior has 10
	bool canLearn = CanLearnSpell(SpellID::Firebolt, player);
	EXPECT_FALSE(canLearn); // Need 5 more magic

	// With 15 magic, should be able to learn
	player._pMagic = 15;
	canLearn = CanLearnSpell(SpellID::Firebolt, player);
	EXPECT_TRUE(canLearn);
}

TEST_F(SpellUXTest, SorcererCanLearnHighLevelSpell)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Sorcerer;
	player._pMagic = 150;

	// Apocalypse requires 149 magic
	bool canLearn = CanLearnSpell(SpellID::Apocalypse, player);
	EXPECT_TRUE(canLearn);
}

TEST_F(SpellUXTest, WarriorCannotLearnHighLevelSpell)
{
	Player &player = Players[0];
	player._pClass = HeroClass::Warrior;
	player._pMagic = 50; // Warrior max magic is 50

	// Apocalypse requires 149 magic
	bool canLearn = CanLearnSpell(SpellID::Apocalypse, player);
	EXPECT_FALSE(canLearn);
}

} // namespace
} // namespace devilution
