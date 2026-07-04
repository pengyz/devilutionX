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

} // namespace devilution
