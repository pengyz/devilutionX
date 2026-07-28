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

	static void TearDownTestSuite()
	{
		SpellDescLines.clear();
		UITest::TearDownTestSuite();
	}
};

// ---- Layer 2: Data Loading ----

TEST_F(SpellTooltipTest, LoadSpellDescDataSucceeds)
{
	auto result = LoadSpellDescData();
	ASSERT_TRUE(result.has_value()) << result.error();
}

TEST_F(SpellTooltipTest, DescLinesExistForFirebolt)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
	ASSERT_GE(lines.size(), 2u);
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

TEST_F(SpellTooltipTest, ManaShieldHasAbsorbLine)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::ManaShield, DescSection::Desc);
	bool hasAbsorb = false;
	for (const auto *line : lines) {
		if (line->source == DescSource::Absorb) hasAbsorb = true;
	}
	EXPECT_TRUE(hasAbsorb);
}

TEST_F(SpellTooltipTest, GuardianHasLifetimeLine)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::Guardian, DescSection::Desc);
	bool hasLife = false;
	for (const auto *line : lines) {
		if (line->source == DescSource::GuardianLife) hasLife = true;
	}
	EXPECT_TRUE(hasLife);
}

// ---- Layer 3: Formatting ----

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
	line.source = DescSource::None;
	line.textKey = "Dmg: 1/3 target hp";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::BoneSpirit, 1);
	EXPECT_EQ(result, "Dmg: 1/3 target hp");
}

TEST_F(SpellTooltipTest, FormatLevelDisplay)
{
	SpellDescLine line;
	line.format = DescFormat::LevelDisplay;
	line.source = DescSource::None;
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
	EXPECT_THAT(result, testing::HasSubstr("->"));
}

TEST_F(SpellTooltipTest, FormatTextShowsDescription)
{
	SpellDescLine line;
	line.format = DescFormat::Text;
	line.source = DescSource::None;
	line.textKey = "";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result, GetSpellData(SpellID::Firebolt).sDescription);
}

TEST_F(SpellTooltipTest, FormatTextFromTextKey)
{
	SpellDescLine line;
	line.format = DescFormat::Text;
	line.source = DescSource::None;
	line.textKey = "Custom text here";
	std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 1);
	EXPECT_EQ(result, "Custom text here");
}

TEST_F(SpellTooltipTest, FormatManaShieldAbsorb)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::ManaShield, DescSection::Desc);
	const SpellDescLine *absorbLine = nullptr;
	for (const auto *l : lines) {
		if (l->source == DescSource::Absorb) { absorbLine = l; break; }
	}
	ASSERT_NE(absorbLine, nullptr);
	std::string result = FormatDescLine(*absorbLine, *MyPlayer, SpellID::ManaShield, 3);
	EXPECT_THAT(result, testing::HasSubstr("absorbs"));
	EXPECT_THAT(result, testing::HasSubstr("%"));
}

TEST_F(SpellTooltipTest, FormatStoneCurseDuration)
{
	LoadSpellDescData();
	auto lines = GetSpellDescLines(SpellID::StoneCurse, DescSection::Desc);
	const SpellDescLine *durLine = nullptr;
	for (const auto *l : lines) {
		if (l->source == DescSource::Duration) { durLine = l; break; }
	}
	ASSERT_NE(durLine, nullptr);
	std::string result = FormatDescLine(*durLine, *MyPlayer, SpellID::StoneCurse, 5);
	EXPECT_THAT(result, testing::HasSubstr("Duration"));
	// level 5: min(5+6, 15) * 16 / 20 = 8 seconds
	EXPECT_THAT(result, testing::HasSubstr("8"));
}

// ---- Layer 4: BuildSpellTooltip / BuildSpellListTooltip ----

TEST_F(SpellTooltipTest, BuildTooltipTitle)
{
	LoadSpellDescData();
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
	EXPECT_FALSE(tooltip.title.empty());
	EXPECT_THAT(tooltip.title, testing::HasSubstr("Firebolt"));
}

TEST_F(SpellTooltipTest, BuildTooltipHasLines)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Firebolt)] = 5;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
	EXPECT_GE(tooltip.lines.size(), 2u);
}

TEST_F(SpellTooltipTest, BuildTooltipLevelZeroShowsUnusable)
{
	LoadSpellDescData();
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
	bool hasUnusable = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("Unusable") != std::string::npos) hasUnusable = true;
	}
	EXPECT_TRUE(hasUnusable) << "Level 0 tooltip should contain 'Unusable'";
}

TEST_F(SpellTooltipTest, BuildTooltipIncludesUpgrade)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Firebolt)] = 5;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
	bool hasArrow = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("->") != std::string::npos) hasArrow = true;
	}
	EXPECT_TRUE(hasArrow) << "Tooltip at level 5 should include upgrade arrows";
}

TEST_F(SpellTooltipTest, BuildListTooltipNoUpgrade)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Firebolt)] = 5;
	auto tooltip = BuildSpellListTooltip(*MyPlayer, SpellID::Firebolt, SpellType::Spell);
	for (const auto &[text, color] : tooltip.lines) {
		EXPECT_EQ(text.find("->"), std::string::npos) << "Upgrade line in list tooltip: " << text;
	}
}

TEST_F(SpellTooltipTest, UtilitySpellNoDamageLine)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::TownPortal)] = 3;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::TownPortal);
	for (const auto &[text, color] : tooltip.lines) {
		EXPECT_EQ(text.find("Damage"), std::string::npos) << "Damage line for utility spell: " << text;
	}
}

TEST_F(SpellTooltipTest, BoneSpiritSpecialLine)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::BoneSpirit)] = 3;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::BoneSpirit);
	bool hasSpecial = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("1/3 target HP") != std::string::npos) hasSpecial = true;
	}
	EXPECT_TRUE(hasSpecial) << "BoneSpirit tooltip should contain special '1/3 target HP'";
}

TEST_F(SpellTooltipTest, ItemRepairWarningLine)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::ItemRepair)] = 1;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::ItemRepair);
	bool hasWarning = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("reduces max durability") != std::string::npos) hasWarning = true;
	}
	EXPECT_TRUE(hasWarning) << "ItemRepair tooltip should contain durability warning";
}

TEST_F(SpellTooltipTest, BuildListTooltipTitle)
{
	LoadSpellDescData();
	auto tooltip = BuildSpellListTooltip(*MyPlayer, SpellID::Firebolt, SpellType::Spell);
	EXPECT_FALSE(tooltip.title.empty());
	EXPECT_THAT(tooltip.title, testing::HasSubstr("Firebolt"));
}

TEST_F(SpellTooltipTest, BuildListTooltipIncludesWarning)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::ItemRepair)] = 1;
	auto tooltip = BuildSpellListTooltip(*MyPlayer, SpellID::ItemRepair, SpellType::Spell);
	bool hasWarning = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("reduces max durability") != std::string::npos) hasWarning = true;
	}
	EXPECT_TRUE(hasWarning) << "List tooltip should include warnings";
}

TEST_F(SpellTooltipTest, ManaShieldTooltipShowsAbsorb)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::ManaShield)] = 3;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::ManaShield);
	bool hasAbsorb = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("absorbs") != std::string::npos) hasAbsorb = true;
	}
	EXPECT_TRUE(hasAbsorb) << "ManaShield tooltip should show absorption info";
}

TEST_F(SpellTooltipTest, GuardianTooltipShowsDamage)
{
	LoadSpellDescData();
	MyPlayer->_pSplLvl[static_cast<size_t>(SpellID::Guardian)] = 5;
	auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Guardian);
	bool hasDamage = false;
	for (const auto &[text, color] : tooltip.lines) {
		if (text.find("Damage") != std::string::npos) hasDamage = true;
	}
	EXPECT_TRUE(hasDamage) << "Guardian tooltip should show damage info";
}

} // namespace devilution
