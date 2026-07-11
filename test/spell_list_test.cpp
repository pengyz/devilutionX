/**
 * @file spell_list_test.cpp
 *
 * Tests for spell list description display.
 *
 * Covers:
 *  - Spell descriptions being populated from data
 *  - Description content validity (non-empty, non-whitespace)
 */

#include <gtest/gtest.h>

#include "ui_test.hpp"

#include "tables/spelldat.h"

namespace devilution {
namespace {

/**
 * @brief Test fixture for spell list display tests.
 *
 * Inherits from UITest which provides a fully-initialised single-player game
 * with loaded spell data so that GetSpellData works.
 */
class SpellListTest : public UITest {
};

TEST_F(SpellListTest, DescriptionExists)
{
	// Verify Firebolt has a description
	const SpellData &firebolt = GetSpellData(SpellID::Firebolt);
	EXPECT_FALSE(firebolt.sDescription.empty());
}

TEST_F(SpellListTest, DescriptionIsNotEmpty)
{
	// Verify description is not just whitespace
	const SpellData &firebolt = GetSpellData(SpellID::Firebolt);
	EXPECT_FALSE(firebolt.sDescription.empty());
	EXPECT_NE(firebolt.sDescription.find_first_not_of(" \t\n"), std::string::npos);
}

} // namespace
} // namespace devilution
