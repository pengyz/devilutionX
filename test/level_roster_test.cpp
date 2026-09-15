#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "engine/assets.hpp"
#include "game_mode.hpp"
#include "tables/level_roster.h"
#include "tables/monstdat.h"

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
	// MT_DIABLO is only available at L26 (see assets/txtdata/monsters/monstdat.tsv).
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
