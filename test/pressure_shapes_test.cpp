#include <gtest/gtest.h>

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include "drlg_test.hpp"
#include "tables/level_roster.h"
#include "tables/monstdat.h"
#include "utils/paths.h"

using namespace devilution;

namespace {
// The two premises the pressure-shape guard thresholds rest on (spec appendix D / section 6.1b):
//  - D1 has no acid resistance family at all, and
//  - the counter pool contains no curse family.
// They are read from the shipped affix tables, and the test skips (rather than passes
// vacuously) when those tables are not present on disk.
// BasePath() resolves to the build directory at test time, which holds a stale copy of the
// data. A guard about the *shipped* tables must read the source tree, so the source-relative
// path is tried first (build/../assets/...), and the build copy only as a fallback.
std::set<std::string> ReadAffixFamiliesFromTsv(const std::string &relativePath, bool &opened)
{
	opened = false;
	std::set<std::string> families;
	std::ifstream file(paths::BasePath() + "../" + relativePath);
	if (!file.is_open())
		file.open(paths::BasePath() + relativePath);
	if (!file.is_open())
		return families;
	opened = true;
	std::string line;
	bool first = true;
	while (std::getline(file, line)) {
		if (first) { // header
			first = false;
			continue;
		}
		if (line.empty())
			continue;
		const size_t tab = line.find('\t');
		if (tab == std::string::npos)
			continue;
		const size_t second = line.find('\t', tab + 1);
		if (second == std::string::npos)
			continue;
		families.insert(line.substr(tab + 1, second - tab - 1));
	}
	return families;
}
} // namespace

TEST(PressureShapesTest, Level16HasNoSquadParameterRow)
{
	// Preamble copied from test/level_roster_baseline_test.cpp:535-544 (the production order):
	// PrefPath -> TestInitGame -> LoadMonsterData -> LoadLevelRoster. Calling LoadLevelRoster()
	// alone aborts with "roster row names unknown monster id" because monster data is not loaded.
	paths::SetPrefPath(paths::BasePath() + "test/fixtures/");
	TestInitGame();
	LoadMonsterData();
	LoadLevelRoster();
	ASSERT_NE(GetLevelRosterParams(1), nullptr) << "roster parameters were not loaded; the checks below would be vacuous";

	// Author decision (default (a), 2026-09-18): level 16 is the boss floor and GetLevelMTypes()
	// returns early there (Source/monster.cpp:3561-3566), so a parameter row would be dead data.
	//
	// The *falsifiable* half reads the source table: adding a 16 row there must fail this test.
	// The runtime half below documents the same invariant, but it cannot be falsified by editing
	// data files (LoadLevelRoster() reads the packed/unpacked copy, not the source tree), so it
	// is not the guard of record.
	{
		std::ifstream paramsFile(paths::BasePath() + "../assets/txtdata/monsters/level_roster_params.tsv");
		if (paramsFile.is_open()) {
			std::string line;
			bool hasLevel16 = false;
			while (std::getline(paramsFile, line)) {
				if (line.rfind("16\t", 0) == 0)
					hasLevel16 = true;
			}
			EXPECT_FALSE(hasLevel16) << "L16 gained a parameter row in the source table; review the early return in GetLevelMTypes() first";
		} else {
			GTEST_SKIP() << "source parameter table not available on disk";
		}
	}
	EXPECT_EQ(GetLevelRosterParams(16), nullptr);
}

TEST(PressureShapesTest, CounterPoolPremisesHold)
{
	bool prefixesOpened = false;
	bool suffixesOpened = false;
	const std::set<std::string> prefixes = ReadAffixFamiliesFromTsv("assets/txtdata/items/item_prefixes.tsv", prefixesOpened);
	const std::set<std::string> suffixes = ReadAffixFamiliesFromTsv("assets/txtdata/items/item_suffixes.tsv", suffixesOpened);
	if (!prefixesOpened || !suffixesOpened)
		GTEST_SKIP() << "shipped affix tables not available on disk";
	// Non-vacuity: a parse that yields nothing would make every check below meaningless.
	// Bounds are the measured source values (prefixes 18, suffixes 32) minus margin; a parse
	// that yields less means we are reading the wrong (stale, build-directory) copy.
	ASSERT_GE(prefixes.size(), 15u) << "affix parse produced too few families; reading a stale copy?";
	ASSERT_GE(suffixes.size(), 25u) << "affix parse produced too few families; reading a stale copy?";

	// Premise 1: no acid resistance family exists (verified 2026-09-18); if one appears, the
	// "answerable demand" arithmetic in appendix D must be re-derived.
	EXPECT_EQ(prefixes.count("ACIDRES") + suffixes.count("ACIDRES"), 0u)
	    << "an acid resistance family appeared; re-derive the guard thresholds";

	// Premise 2: the counter pool used by the guard is exactly these families and none of them
	// is a curse (curses lower a stat, they are not counterplay).
	const std::set<std::string> counterPool { "FIRERES", "LIGHTRES", "MAGICRES", "ALLRES", "LIGHT" };
	for (const std::string &family : counterPool) {
		EXPECT_EQ(family.find("_CURSE"), std::string::npos) << family;
		EXPECT_EQ(prefixes.count(family) + suffixes.count(family), 1u)
		    << family << " is missing from the shipped affix tables (rename or removal?)";
	}
}
