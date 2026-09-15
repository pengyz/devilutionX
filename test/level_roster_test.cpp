#include <gtest/gtest.h>

#include <algorithm>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "engine/assets.hpp"
#include "game_mode.hpp"
#include "tables/level_roster.h"
#include "tables/monstdat.h"
#include "utils/paths.h"

using namespace devilution;

namespace {

class LevelRosterTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadCoreArchives();
		gbIsSpawn = false;
		LoadMonsterData();
	}
};

} // namespace

TEST_F(LevelRosterTest, ValidationAcceptsAWellFormedRoster)
{
	const std::vector<LevelRosterEntry> entries {
		{ 1, MT_WSKELAX, LevelRosterRole::Core, false },
		{ 1, MT_NZOMBIE, LevelRosterRole::Tail, false },
	};
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, { { BehaviorClass::Melee, 2 } } },
	};
	EXPECT_FALSE(ValidateLevelRoster(entries, params).has_value());
}

TEST_F(LevelRosterTest, ValidationRejectsAMonsterUnavailableAtThatLevel)
{
	// MT_DIABLO has availability=Never in monstdat.tsv (not merely excluded by its
	// level window), so it is rejected at any level under the full retail check. The
	// level-window rejection path is covered separately by the MT_WSKELAX@L3 case below.
	const std::vector<LevelRosterEntry> entries { { 1, MT_DIABLO, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("not available"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAnInvalidRangeRow)
{
	// MT_WSKELAX is only available on levels 1-2; placing it at level 3 must be rejected.
	const std::vector<LevelRosterEntry> entries { { 3, MT_WSKELAX, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 3, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("not available"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAnUnsatisfiableClassFloor)
{
	// No level's candidate pool has 99 Boss-class monsters available.
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, { { BehaviorClass::Boss, 99 } } },
	};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("class floor"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAUniqueBaseWithoutTheWhitelist)
{
	// MT_NGOATMC is Gharbad the Weak's base type at level 4 (unique_monstdat.tsv).
	const std::vector<LevelRosterEntry> rejectedEntries { { 4, MT_NGOATMC, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 4, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(rejectedEntries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("allow_unique_boost"), std::string::npos);

	const std::vector<LevelRosterEntry> allowedEntries { { 4, MT_NGOATMC, LevelRosterRole::Core, true } };
	EXPECT_FALSE(ValidateLevelRoster(allowedEntries, params).has_value());
}

TEST_F(LevelRosterTest, ValidationRelaxesInSpawnMode)
{
	// MT_DIABLO is Never-available in the full retail check (only unlocked via a dedicated
	// path, not the normal availability window), so this table is rejected under retail.
	const std::vector<LevelRosterEntry> entries { { 1, MT_DIABLO, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };

	gbIsSpawn = true;
	EXPECT_FALSE(ValidateLevelRoster(entries, params).has_value());

	// Prove the branch is actually taken, not that retail checks were silently disabled:
	// the same table must still be rejected once we switch back to retail.
	gbIsSpawn = false;
	EXPECT_TRUE(ValidateLevelRoster(entries, params).has_value());
}

TEST_F(LevelRosterTest, ValidationRejectsLevelWithNoCoreInSpawnMode)
{
	// Level 1 has roster params but only a tail member, no core member.
	const std::vector<LevelRosterEntry> entries { { 1, MT_NZOMBIE, LevelRosterRole::Tail, false } };
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };

	gbIsSpawn = true;
	const auto error = ValidateLevelRoster(entries, params);
	gbIsSpawn = false;

	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("core"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAClassFloorThatExceedsTheB1CapAtL10)
{
	// At level 10, RangedKite candidates (MT_BACID, MT_RSTORM, MT_YMAGMA, MT_BMAGMA,
	// MT_WMAGMA, MT_STORM) number well above 2, but the B1 cap for L9-12 limits
	// RangedKite to 2. A floor of 3 must be rejected even though the raw pool is bigger.
	const std::vector<LevelRosterEntry> entries { { 10, MT_BACID, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 10, 6000, 2, { { BehaviorClass::RangedKite, 3 } } },
	};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("class floor"), std::string::npos);
	EXPECT_NE(error->find("caps"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationAcceptsAClassFloorAtTheB1CapAtL10)
{
	// A floor of exactly 2 must pass: the raw candidate pool is bigger than 2, but the
	// cap of 2 is still satisfied.
	const std::vector<LevelRosterEntry> entries { { 10, MT_BACID, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 10, 6000, 2, { { BehaviorClass::RangedKite, 2 } } },
	};
	EXPECT_FALSE(ValidateLevelRoster(entries, params).has_value());
}

TEST_F(LevelRosterTest, ValidationRejectsAClassFloorThatExceedsTheB1CapAtL14)
{
	// At level 14, Melee candidates (MT_VTEXLRD, MT_BALROG, MT_RSNAKE, MT_BSNAKE,
	// MT_NBLACK, MT_RTBLACK, MT_BTBLACK, MT_RBLACK) number well above 2, but the B1 cap
	// for L13-16 limits any single class to 2. A floor of 3 must be rejected.
	const std::vector<LevelRosterEntry> entries { { 14, MT_VTEXLRD, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 14, 6000, 2, { { BehaviorClass::Melee, 3 } } },
	};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("class floor"), std::string::npos);
	EXPECT_NE(error->find("caps"), std::string::npos);
}

TEST_F(LevelRosterTest, SortRosterByLevelThenFindLevelRosterKeepsAllMembersOfAnInterleavedLevel)
{
	// FindLevelRoster() assumes same-level entries are contiguous. SortRosterByLevel()
	// must sort by level first so interleaved TSV rows (level 1, 2, 1) don't silently drop
	// the second level-1 batch, and must be stable so within-level file order survives.
	std::vector<LevelRosterEntry> entries {
		{ 1, MT_WSKELAX, LevelRosterRole::Core, false },
		{ 2, MT_SNEAK, LevelRosterRole::Core, false },
		{ 1, MT_NZOMBIE, LevelRosterRole::Core, false },
	};

	SortRosterByLevel(entries);
	const std::span<const LevelRosterEntry> level1 = FindLevelRoster(entries, 1);

	ASSERT_EQ(level1.size(), 2u);
	EXPECT_EQ(level1[0].type, MT_WSKELAX);
	EXPECT_EQ(level1[1].type, MT_NZOMBIE);
}

TEST_F(LevelRosterTest, ValidationRejectsAnOutOfRangeMonsterIdInSpawnMode)
{
	// Spawn mode relaxes availability/unique-whitelist/floors checks, but must still share
	// the type-existence bounds check. MT_INVALID (-1) is legally parsed by enum_cast
	// (magic_enum's customized range for _monster_id includes it), so it could otherwise
	// smuggle an out-of-bounds MonstersData index through the relaxed branch.
	const std::vector<LevelRosterEntry> entries { { 1, MT_INVALID, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };

	gbIsSpawn = true;
	const auto error = ValidateLevelRoster(entries, params);
	gbIsSpawn = false;

	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("unknown monster id"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAParamsLevelWithNoCoreInRetailMode)
{
	// A level with a params row but no roster entries at all (so no core member) must be
	// rejected even outside spawn mode: the core-non-empty check now runs in both modes.
	const std::vector<LevelRosterEntry> entries {};
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("no core"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAnEntryLevelWithNoCoreAndNoParamsRow)
{
	// A level with a roster entry (tail only, no params row at all) but no core member
	// must be rejected: the check is driven by (entries union params), not just params rows.
	const std::vector<LevelRosterEntry> entries { { 1, MT_NZOMBIE, LevelRosterRole::Tail, false } };
	const std::vector<LevelRosterParams> params {};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("no core"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsANonPositiveMaxImage)
{
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 1, 0, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("max_image"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsANegativeTailDraw)
{
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 1, 6000, -1, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("tail_draw"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsTheClassFloorCountSentinel)
{
	// BehaviorClass::Count is a sentinel value, not a real category, but magic_enum's
	// enum_cast would otherwise resolve the string "Count" as if it were a valid class.
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, { { BehaviorClass::Count, 1 } } },
	};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("Count"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsADuplicateParamsLevel)
{
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, {} },
		{ 1, 9000, 3, {} },
	};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("more than one"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsADuplicateEntryLevelAndMonsterId)
{
	// Same (level, monster_id) pair appearing twice must be rejected even though each row
	// individually is a valid, available roster entry.
	const std::vector<LevelRosterEntry> entries {
		{ 1, MT_WSKELAX, LevelRosterRole::Core, false },
		{ 1, MT_WSKELAX, LevelRosterRole::Tail, false },
	};
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("more than one"), std::string::npos);
}

TEST(ParseClassFloorsTest, ParsesTwoValidEntries)
{
	// BehaviorClass has no enumerator literally named "Ranged"; the two real ranged
	// categories are RangedTurret/RangedKite, so both positive entries here use real names.
	const auto result = ParseClassFloors("Melee=2,RangedTurret=1");
	const std::vector<std::pair<BehaviorClass, uint8_t>> expected {
		{ BehaviorClass::Melee, 2 },
		{ BehaviorClass::RangedTurret, 1 },
	};
	EXPECT_EQ(result, expected);
}

TEST(ParseClassFloorsTest, EmptyValueYieldsAnEmptyResult)
{
	EXPECT_TRUE(ParseClassFloors("").empty());
}

TEST(ParseClassFloorsTest, FatalsOnAnEntryMissingTheEqualsSign)
{
	EXPECT_EXIT(ParseClassFloors("Melee2"), ::testing::ExitedWithCode(1), "missing");
}

TEST(ParseClassFloorsTest, FatalsOnAnUnknownClassName)
{
	EXPECT_EXIT(ParseClassFloors("Ranged=1"), ::testing::ExitedWithCode(1), "unknown BehaviorClass");
}

TEST(ParseClassFloorsTest, FatalsOnANonNumericFloor)
{
	EXPECT_EXIT(ParseClassFloors("Melee=abc"), ::testing::ExitedWithCode(1), "bad integer");
}

TEST(ParseClassFloorsTest, FatalsOnAnEmptyFloorValue)
{
	// "Melee=" has an '=' but nothing after it: from_chars must fail to parse the floor.
	EXPECT_EXIT(ParseClassFloors("Melee="), ::testing::ExitedWithCode(1), "bad integer");
}

namespace {

class LevelRosterFixtureLoadTest : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadCoreArchives();
		gbIsSpawn = false;
		LoadMonsterData();
		paths::SetAssetsPath(paths::BasePath() + "/test/fixtures/");
	}
};

} // namespace

TEST_F(LevelRosterFixtureLoadTest, LoadingAnInterleavedFixtureGathersAllMembersOfALevel)
{
	// The fixture's rows are deliberately out of level order (L1, L2, L1) to prove the
	// production loader (not just SortRosterByLevel() in isolation) still collects every
	// level-1 row via LoadLevelRosterFromFiles() -> GetLevelRoster().
	LoadLevelRosterFromFiles(
	    "txtdata\\monsters\\level_rosters_interleaved.tsv",
	    "txtdata\\monsters\\level_roster_params_interleaved.tsv");

	const std::span<const LevelRosterEntry> level1 = GetLevelRoster(1);
	ASSERT_EQ(level1.size(), 2u);
	EXPECT_TRUE(std::any_of(level1.begin(), level1.end(), [](const LevelRosterEntry &e) { return e.type == MT_WSKELAX; }));
	EXPECT_TRUE(std::any_of(level1.begin(), level1.end(), [](const LevelRosterEntry &e) { return e.type == MT_NZOMBIE; }));
}

TEST_F(LevelRosterFixtureLoadTest, LoadsTheShippedRosterAndValidatesIt)
{
	// Reset back to the real assets directory (the fixture test above pointed AssetsPath at
	// test/fixtures/) before exercising the production LoadLevelRoster() entry point.
	paths::SetAssetsPath(paths::BasePath() + "assets/");
	LoadLevelRoster();

	const std::span<const LevelRosterEntry> level9 = GetLevelRoster(9);
	EXPECT_FALSE(level9.empty());
	EXPECT_NE(GetLevelRosterParams(9), nullptr);
}
