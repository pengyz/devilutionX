#include <gtest/gtest.h>

#include "ui_test.hpp"
#include "spell_tooltip.h"
#include "tables/spelldat.h"
#include "missiles.h"
#include "spells.h"

namespace devilution {

class SpellTooltipTest : public UITest {
protected:
	static void SetUpTestSuite()
	{
		UITest::SetUpTestSuite();
	}
};

TEST_F(SpellTooltipTest, EvaluateSimpleNumber)
{
	ExprResult result = EvaluateSpellExpr("42", *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result.value, 42);
	EXPECT_FALSE(result.isRange);
}

TEST_F(SpellTooltipTest, EvaluateLvlVariable)
{
	ExprResult result = EvaluateSpellExpr("lvl * 2", *MyPlayer, SpellID::Firebolt, 5);
	EXPECT_EQ(result.value, 10);
}

TEST_F(SpellTooltipTest, EvaluateParVariable)
{
	const SpellData &sd = GetSpellData(SpellID::StoneCurse);
	ExprResult result = EvaluateSpellExpr("par1 + par5", *MyPlayer, SpellID::StoneCurse, 1);
	EXPECT_EQ(result.value, static_cast<int>(sd.sParam[0] + sd.sParam[4]));
}

TEST_F(SpellTooltipTest, EvaluateLnFunction)
{
	const SpellData &sd = GetSpellData(SpellID::StoneCurse);
	ExprResult result = EvaluateSpellExpr("ln(par1, par5)", *MyPlayer, SpellID::StoneCurse, 3);
	int expected = sd.sParam[0] + (3 - 1) * sd.sParam[4];
	EXPECT_EQ(result.value, expected);
}

TEST_F(SpellTooltipTest, EvaluateDamageRange)
{
	DamageRange dr = GetDamageAmt(SpellID::Firebolt, 5);
	ExprResult result = EvaluateSpellExpr("damage", *MyPlayer, SpellID::Firebolt, 5);
	EXPECT_TRUE(result.isRange);
	EXPECT_EQ(result.minValue, dr.min);
	EXPECT_EQ(result.maxValue, dr.max);
}

TEST_F(SpellTooltipTest, EvaluateManaRef)
{
	int expectedMana = GetManaAmount(*MyPlayer, SpellID::Firebolt) >> 6;
	ExprResult result = EvaluateSpellExpr("mana", *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result.value, expectedMana);
}

TEST_F(SpellTooltipTest, EvaluateMathFloor)
{
	ExprResult result = EvaluateSpellExpr("math.floor(7 / 3)", *MyPlayer, SpellID::ChargedBolt, 7);
	EXPECT_EQ(result.value, 2);
}

TEST_F(SpellTooltipTest, EvaluateInvalidExprReturnsZero)
{
	ExprResult result = EvaluateSpellExpr("invalid!!!", *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result.value, 0);
	EXPECT_FALSE(result.isRange);
}

TEST_F(SpellTooltipTest, EvaluateCharLevel)
{
	ExprResult result = EvaluateSpellExpr("charLevel", *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result.value, MyPlayer->getCharacterLevel());
}

TEST_F(SpellTooltipTest, EvaluateMagic)
{
	ExprResult result = EvaluateSpellExpr("magic", *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result.value, MyPlayer->_pMagic);
}

} // namespace devilution
