#include <gmock/gmock.h>
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

TEST_F(SpellTooltipTest, LoadSpellDescDataSucceeds)
{
	auto result = LoadSpellDescData();
	ASSERT_TRUE(result.has_value()) << result.error();
}

TEST_F(SpellTooltipTest, DescLinesExistForFirebolt)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
	ASSERT_GE(lines.size(), 2u); // at least damage + mana
}

TEST_F(SpellTooltipTest, UpgradeLinesExistForFirebolt)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Upgrade);
	EXPECT_FALSE(lines.empty());
}

TEST_F(SpellTooltipTest, UnknownSpellReturnsEmpty)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Invalid, DescSection::Desc);
	EXPECT_TRUE(lines.empty());
}

TEST_F(SpellTooltipTest, LinesOrderedByPriority)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
	for (size_t i = 1; i < lines.size(); i++) {
		EXPECT_GE(lines[i]->priority, lines[i - 1]->priority);
	}
}

TEST_F(SpellTooltipTest, UtilitySpellHasManaLine)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::TownPortal, DescSection::Desc);
	bool hasMana = false;
	for (const auto *line : lines) {
		if (line->format == DescFormat::Mana) hasMana = true;
	}
	EXPECT_TRUE(hasMana);
}

TEST_F(SpellTooltipTest, FormatDamageRange)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
	const SpellDescLine *dmgLine = nullptr;
	for (const auto *l : lines) {
		if (l->format == DescFormat::DamageRange) { dmgLine = l; break; }
	}
	ASSERT_NE(dmgLine, nullptr);
	std::string result = FormatDescLine(*dmgLine, *MyPlayer, SpellID::Firebolt, 5);
	EXPECT_THAT(result, testing::HasSubstr("Damage"));
	EXPECT_THAT(result, testing::HasSubstr("-"));
}

TEST_F(SpellTooltipTest, FormatHealRange)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Healing, DescSection::Desc);
	const SpellDescLine *healLine = nullptr;
	for (const auto *l : lines) {
		if (l->format == DescFormat::HealRange) { healLine = l; break; }
	}
	ASSERT_NE(healLine, nullptr);
	std::string result = FormatDescLine(*healLine, *MyPlayer, SpellID::Healing, 3);
	EXPECT_THAT(result, testing::HasSubstr("Heals"));
	EXPECT_THAT(result, testing::HasSubstr("-"));
}

TEST_F(SpellTooltipTest, FormatMana)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
	const SpellDescLine *manaLine = nullptr;
	for (const auto *l : lines) {
		if (l->format == DescFormat::Mana) { manaLine = l; break; }
	}
	ASSERT_NE(manaLine, nullptr);
	std::string result = FormatDescLine(*manaLine, *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_THAT(result, testing::HasSubstr("Mana"));
	int expectedMana = GetManaAmount(*MyPlayer, SpellID::Firebolt) >> 6;
	EXPECT_THAT(result, testing::HasSubstr(std::to_string(expectedMana)));
}

TEST_F(SpellTooltipTest, FormatSpecial)
{
	SpellDescLine line;
	line.format = DescFormat::Special;
	line.textKey = "Dmg: 1/3 target hp";
	line.expression = "";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::BoneSpirit, 1);
	EXPECT_EQ(result, "Dmg: 1/3 target hp");
}

TEST_F(SpellTooltipTest, FormatLevelDisplay)
{
	SpellDescLine line;
	line.format = DescFormat::LevelDisplay;
	line.expression = "";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 5);
	EXPECT_THAT(result, testing::HasSubstr("Level"));
	EXPECT_THAT(result, testing::HasSubstr("5"));
	EXPECT_THAT(result, testing::HasSubstr("15"));
}

TEST_F(SpellTooltipTest, FormatValueDelta)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Upgrade);
	const SpellDescLine *manaDelta = nullptr;
	for (const auto *l : lines) {
		if (l->format == DescFormat::ManaDelta) { manaDelta = l; break; }
	}
	ASSERT_NE(manaDelta, nullptr);
	std::string result = FormatDescLine(*manaDelta, *MyPlayer, SpellID::Firebolt, 5);
	EXPECT_THAT(result, testing::HasSubstr("\xe2\x86\x92")); // -> UTF-8
}

TEST_F(SpellTooltipTest, FormatTextShowsDescription)
{
	SpellDescLine line;
	line.format = DescFormat::Text;
	line.expression = "";
	line.textKey = "";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result, GetSpellData(SpellID::Firebolt).sDescription);
}

TEST_F(SpellTooltipTest, FormatTextFromTextKey)
{
	SpellDescLine line;
	line.format = DescFormat::Text;
	line.expression = "";
	line.textKey = "Custom text here";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result, "Custom text here");
}

} // namespace devilution
