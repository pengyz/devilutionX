/**
 * @file spell_book_hover_test.cpp
 *
 * Tests for spell book hover tooltip and upgrade preview.
 *
 * Covers:
 *  - GetDamageAmt returning increasing values with spell level
 *  - BoneSpirit special-case damage display
 *  - Utility spells returning -1 for damage
 */

#include <gtest/gtest.h>

#include "ui_test.hpp"

#include "missiles.h"
#include "player.h"
#include "tables/spelldat.h"

namespace devilution {
namespace {

/**
 * @brief Test fixture for spell book hover tooltip tests.
 *
 * Inherits from UITest which provides a fully-initialised single-player game
 * with a level-25 Warrior so that GetDamageAmt (which requires MyPlayer) works.
 */
class SpellBookHoverTest : public UITest {
};

TEST_F(SpellBookHoverTest, UpgradePreviewFormat)
{
	// Verify GetDamageAmt returns expected values for Firebolt
	const auto [min1, max1] = GetDamageAmt(SpellID::Firebolt, 1);
	const auto [min2, max2] = GetDamageAmt(SpellID::Firebolt, 2);

	EXPECT_GT(min2, min1); // Damage should increase with level
	EXPECT_GT(max2, max1);
}

TEST_F(SpellBookHoverTest, BoneSpiritFixedEffect)
{
	// BoneSpirit returns {-1, -1} from GetDamageAmt; the special
	// "Dmg: 1/3 target hp" text is handled by GetSpellPowerText.
	const auto [min, max] = GetDamageAmt(SpellID::BoneSpirit, 1);
	EXPECT_EQ(min, -1);
	EXPECT_EQ(max, -1);
}

TEST_F(SpellBookHoverTest, UtilitySpellNoDamage)
{
	// Utility spells like TownPortal should not show damage
	const auto [min, max] = GetDamageAmt(SpellID::TownPortal, 1);
	EXPECT_EQ(min, -1); // -1 indicates no damage
	EXPECT_EQ(max, -1);
}

TEST_F(SpellBookHoverTest, HealingScalesWithLevel)
{
	// Healing should increase with spell level
	const auto [min1, max1] = GetDamageAmt(SpellID::Healing, 1);
	const auto [min2, max2] = GetDamageAmt(SpellID::Healing, 2);

	EXPECT_GT(min2, min1);
	EXPECT_GT(max2, max1);
}

} // namespace
} // namespace devilution
