#include <gtest/gtest.h>

#include <fstream>
#include <set>
#include <vector>
#include <utility>
#include <iterator>
#include <cstdio>
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
// These guards are about the *shipped source* tables, so the reader opens the source tree
// explicitly: BasePath() resolves to the build directory at test time, and the build copy is
// not used here at all (it happens to be identical today, but it is not what we are guarding).
// If the source table cannot be opened the test SKIPs rather than reading anything else.
std::set<std::string> ReadAffixFamiliesFromTsv(const std::string &relativePath, bool &opened)
{
	opened = false;
	std::set<std::string> families;
	std::ifstream file(paths::BasePath() + "../" + relativePath);
	if (!file.is_open())
		return families; // no fallback: a missing source tree must SKIP, never read the build copy
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



namespace {
// FNV-1a (64-bit) over the raw bytes. This is a change detector, not a cryptographic digest:
// any single-byte edit to a protected table changes the value.
uint64_t Fnv1a64(const std::vector<char> &data)
{
	uint64_t hash = 0xcbf29ce484222325ULL;
	for (const unsigned char byte : data) {
		hash ^= byte;
		hash *= 0x100000001b3ULL;
	}
	return hash;
}

// Reads the generated golden list (test/fixtures/pressure/immutable_tables.fnv64). Regenerate
// with: python3 - <<'PY' ... FNV-1a over the three tables ... PY  (see the ledger entry).
std::vector<std::pair<std::string, std::string>> ReadGoldenList()
{
	std::vector<std::pair<std::string, std::string>> entries;
	std::ifstream file(paths::BasePath() + "../test/fixtures/pressure/immutable_tables.fnv64");
	if (!file.is_open())
		return entries;
	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back(); // the golden list is CRLF; without this the path never opens
		if (line.empty() || line[0] == '#')
			continue;
		const size_t sep = line.find("  ");
		if (sep == std::string::npos)
			continue;
		entries.emplace_back(line.substr(0, sep), line.substr(sep + 2));
	}
	return entries;
}
} // namespace

TEST(PressureShapesTest, ImmutableTablesUnchanged)
{
	// (g): the numeric, drop and price tables must not change as part of this leg.
	const auto golden = ReadGoldenList();
	ASSERT_GE(golden.size(), 3u) << "golden list unexpectedly small; the check would be vacuous";
	size_t checked = 0;
	for (const auto &[expected, relativePath] : golden) {
		std::ifstream file(paths::BasePath() + "../" + relativePath, std::ios::binary);
		if (!file.is_open())
			continue;
		const std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		ASSERT_GT(data.size(), 0u) << relativePath << " read as empty";
		char actual[17];
		std::snprintf(actual, sizeof(actual), "%016llx", static_cast<unsigned long long>(Fnv1a64(data)));
		EXPECT_EQ(std::string(actual), expected)
		    << relativePath << " changed; if intentional, regenerate the golden list consciously";
		checked++;
	}
	if (checked == 0)
		GTEST_SKIP() << "no protected table was available on disk";
}

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
	// Exact goldens measured from the source tables on 2026-09-18. A loose bound cannot tell a
	// stale or partial table from the real one (the build copy was identical), so this is an
	// equality: any change here is a conscious data edit that must update this line.
	ASSERT_EQ(prefixes.size(), 18u) << "prefix family count changed; update this golden consciously";
	ASSERT_EQ(suffixes.size(), 32u) << "suffix family count changed; update this golden consciously";

	// Premise 1: no acid resistance family exists (verified 2026-09-18); if one appears, the
	// "answerable demand" arithmetic in appendix D must be re-derived.
	EXPECT_EQ(prefixes.count("ACIDRES") + suffixes.count("ACIDRES"), 0u)
	    << "an acid resistance family appeared; re-derive the guard thresholds";

	// Premise 2, stated precisely (the spec's original wording "families intersect curses is
	// empty" is false: the shipped tables do contain curse families such as LIGHT_CURSE). The
	// checkable claim is that the counter pool contains none of them, with the curse set derived
	// from the data rather than hardcoded.
	std::set<std::string> curseFamilies;
	for (const std::string &family : prefixes)
		if (family.ends_with("_CURSE"))
			curseFamilies.insert(family);
	for (const std::string &family : suffixes)
		if (family.ends_with("_CURSE"))
			curseFamilies.insert(family);
	ASSERT_GT(curseFamilies.size(), 0u)
	    << "no curse families found at all; the parse or the table shape changed";
	const std::set<std::string> counterPool { "FIRERES", "LIGHTRES", "MAGICRES", "ALLRES", "LIGHT" };
	for (const std::string &family : counterPool) {
		EXPECT_EQ(curseFamilies.count(family), 0u) << family << " is a curse and must not be a counter";
		EXPECT_EQ(prefixes.count(family) + suffixes.count(family), 1u)
		    << family << " is missing from the shipped affix tables (rename or removal?)";
	}
}


