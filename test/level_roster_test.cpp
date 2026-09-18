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
		// F2 (task 1 fix review round 1): every two-arg ValidateLevelRoster() call in this
		// suite relies on that overload's default `maxLevel = MaxValidatedDungeonLevel()`,
		// which reads this global (see level_roster.h/.cpp) and resolves to 16 when false,
		// 24 when true. This binary never otherwise sets gbIsHellfire, so today's default
		// happens to read false - but that is an ACCIDENT of initialization order, not a
		// property this suite asserts. Pin it explicitly so the default maxLevel these 42+
		// calls implicitly depend on stays 16 by DECLARATION, not by whichever other test
		// binary/global happened to run first; a later change that flips gbIsHellfire
		// elsewhere in the process must not silently change what these calls validate.
		gbIsHellfire = false;
		LoadMonsterData();
	}

	// Bonus guard for the pin above: if any future test in this suite ever flips
	// gbIsHellfire (directly, or indirectly via code under test) without restoring it,
	// every LATER two-arg ValidateLevelRoster() call in the same run would silently start
	// validating against maxLevel 24 instead of 16. Catch that drift right after it would
	// happen instead of leaving it to show up as an unrelated assertion failure elsewhere.
	void TearDown() override
	{
		EXPECT_FALSE(gbIsHellfire) << "LevelRosterTest pins gbIsHellfire = false in "
		                              "SetUpTestSuite() so the default maxLevel of the "
		                              "two-arg ValidateLevelRoster() calls in this suite "
		                              "stays 16; some test in this run left it true";
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
	// At level 14 the RangedTurret candidates (MT_SUCCUBUS, MT_SNOWWICH, MT_HLSPWN,
	// MT_MAGISTR, MT_COUNSLR) number well above 1, but the asymmetric cap for L13-15
	// (content density contract, 2026-09-16) limits the ranged classes to 1 while the
	// non-ranged classes are uncapped. A RangedTurret floor of 2 must be rejected.
	const std::vector<LevelRosterEntry> entries { { 14, MT_VTEXLRD, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params {
		{ 14, 6000, 2, { { BehaviorClass::RangedTurret, 2 } } },
	};
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("class floor"), std::string::npos);
	EXPECT_NE(error->find("caps"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsThreeSameClassCoreMembersUnderTheB1CapAtL10)
{
	// R29: core members are pre-added before the sampling loop, so they bypass the B1
	// cap entirely. A table listing 3 RangedKite cores at level 10 would therefore ship a
	// level that violates the <= 2 guarantee no matter what the sampler does, and the
	// acceptance test could not catch it (the realised roster is the input). The only place
	// this can be blocked is load time, so validation must reject it here.
	const std::vector<LevelRosterEntry> entries {
		{ 10, MT_BMAGMA, LevelRosterRole::Core, false },
		{ 10, MT_WMAGMA, LevelRosterRole::Core, false },
		{ 10, MT_RSTORM, LevelRosterRole::Core, false },
	};
	const std::vector<LevelRosterParams> params { { 10, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value()) << "3 RangedKite cores at L10 must be rejected";
	EXPECT_NE(error->find("core roster has 3"), std::string::npos) << *error;
	EXPECT_NE(error->find("RangedKite"), std::string::npos) << *error;
	EXPECT_NE(error->find("level 10"), std::string::npos) << *error;
	EXPECT_NE(error->find("cap for that level and class is 2"), std::string::npos) << *error;

	// Prove the rejection is driven by the count against the cap, not by these three types
	// being unacceptable on their own: dropping to exactly the cap must pass.
	const std::vector<LevelRosterEntry> atCap { entries[0], entries[1] };
	EXPECT_FALSE(ValidateLevelRoster(atCap, params).has_value());
}

TEST_F(LevelRosterTest, ValidationRejectsThreeSameClassCoreMembersInSpawnModeToo)
{
	// The core-vs-cap check sits before the spawn-mode relaxation, because the cap guarantee
	// is about the shipped table, not about which availability rules are in force. The
	// vehicle is the RangedTurret class, capped at 1 in L13-15 since the density contract.
	const std::vector<LevelRosterEntry> entries {
		{ 14, MT_SUCCUBUS, LevelRosterRole::Core, false },
		{ 14, MT_HLSPWN, LevelRosterRole::Core, false },
		{ 14, MT_MAGISTR, LevelRosterRole::Core, false },
	};
	const std::vector<LevelRosterParams> params { { 14, 6000, 2, {} } };

	gbIsSpawn = true;
	const auto error = ValidateLevelRoster(entries, params);
	gbIsSpawn = false;

	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("core roster has 3"), std::string::npos) << *error;
	EXPECT_NE(error->find("RangedTurret"), std::string::npos) << *error;
}

// Phase A2 task 1: the roster table is a SINGLE table extended to L24, so a non-Hellfire
// install parses the L17-24 rows too. Those rows name monsters that only the hf overlay's
// monstdat.tsv makes available (base monstdat.tsv ships them as availability=Never), so
// validating them unconditionally would make the shipped table fatal on every Diablo-only
// install - i.e. refuse to start. Per-level checks are therefore scoped to the levels the
// active game mode can actually reach, while global/structural checks stay unscoped.
//
// MT_UNRAV is the vehicle: base monstdat.tsv has it as `Never` with window 17-18, and the
// hf overlay re-ships the same row as available. Under the base data loaded by this suite it
// is therefore unavailable at L17 no matter what, which is exactly the pre-A2 fatal.
TEST_F(LevelRosterTest, ValidationSkipsLevelsAboveTheActiveLevelRange)
{
	// A Diablo-only install (maxLevel 16) must accept the table: L17's row is unreachable
	// data there, not a defect.
	const std::vector<LevelRosterEntry> entries {
		{ 1, MT_WSKELAX, LevelRosterRole::Core, false },
		{ 17, MT_UNRAV, LevelRosterRole::Core, false },
	};
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, { { BehaviorClass::Melee, 2 } } },
		{ 17, 6000, 2, {} },
	};

	const auto scoped = ValidateLevelRoster(entries, params, 16);
	EXPECT_FALSE(scoped.has_value())
	    << "L17 rows must not be validated when the active range stops at 16: " << scoped.value_or("");
}

TEST_F(LevelRosterTest, ValidationStillChecksLevelsInsideTheActiveLevelRange)
{
	// The other direction, and the reason the scoping is not just "ignore L17+": once the
	// range really does include L17 (a Hellfire install, maxLevel 24), the same row must be
	// held to the full per-level check. Under this suite's base monstdat MT_UNRAV is
	// availability=Never, so it is genuinely unavailable at L17 and must be fatal.
	const std::vector<LevelRosterEntry> entries {
		{ 1, MT_WSKELAX, LevelRosterRole::Core, false },
		{ 17, MT_UNRAV, LevelRosterRole::Core, false },
	};
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, { { BehaviorClass::Melee, 2 } } },
		{ 17, 6000, 2, {} },
	};

	const auto error = ValidateLevelRoster(entries, params, 24);
	ASSERT_TRUE(error.has_value()) << "L17 must be validated when the active range reaches it";
	EXPECT_NE(error->find("not available"), std::string::npos) << *error;
	EXPECT_NE(error->find("level 17"), std::string::npos) << *error;
}

TEST_F(LevelRosterTest, GlobalChecksIgnoreTheActiveLevelRange)
{
	// The scoping must not become a blanket "skip everything about L17+": structural defects
	// are properties of the TABLE, independent of whether the level is reachable, and a
	// mis-scoped implementation that wrapped these in the level guard would ship them
	// unchecked. Each case below is a global check applied to an OUT-OF-RANGE level
	// (maxLevel 16) and must still be rejected.

	// Duplicate (level, monster_id) entry rows.
	{
		const std::vector<LevelRosterEntry> entries {
			{ 20, MT_UNRAV, LevelRosterRole::Core, false },
			{ 20, MT_UNRAV, LevelRosterRole::Tail, false },
		};
		const std::vector<LevelRosterParams> params { { 20, 6000, 2, {} } };
		const auto error = ValidateLevelRoster(entries, params, 16);
		ASSERT_TRUE(error.has_value());
		EXPECT_NE(error->find("more than one roster row"), std::string::npos) << *error;
	}
	// Duplicate params rows for the same level.
	{
		const std::vector<LevelRosterEntry> entries { { 20, MT_UNRAV, LevelRosterRole::Core, false } };
		const std::vector<LevelRosterParams> params { { 20, 6000, 2, {} }, { 20, 6000, 2, {} } };
		const auto error = ValidateLevelRoster(entries, params, 16);
		ASSERT_TRUE(error.has_value());
		EXPECT_NE(error->find("more than one params row"), std::string::npos) << *error;
	}
	// Non-positive max_image.
	{
		const std::vector<LevelRosterEntry> entries { { 20, MT_UNRAV, LevelRosterRole::Core, false } };
		const std::vector<LevelRosterParams> params { { 20, 0, 2, {} } };
		const auto error = ValidateLevelRoster(entries, params, 16);
		ASSERT_TRUE(error.has_value());
		EXPECT_NE(error->find("non-positive max_image"), std::string::npos) << *error;
	}
	// Negative tail_draw.
	{
		const std::vector<LevelRosterEntry> entries { { 20, MT_UNRAV, LevelRosterRole::Core, false } };
		const std::vector<LevelRosterParams> params { { 20, 6000, -1, {} } };
		const auto error = ValidateLevelRoster(entries, params, 16);
		ASSERT_TRUE(error.has_value());
		EXPECT_NE(error->find("negative tail_draw"), std::string::npos) << *error;
	}
	// squad_chance above 100 / squad_size above 3 / squads enabled with size 0.
	{
		const std::vector<LevelRosterEntry> entries { { 20, MT_UNRAV, LevelRosterRole::Core, false } };
		const std::vector<LevelRosterParams> tooMuchChance { { 20, 6000, 2, {}, 101, 1, true } };
		const auto chanceError = ValidateLevelRoster(entries, tooMuchChance, 16);
		ASSERT_TRUE(chanceError.has_value());
		EXPECT_NE(chanceError->find("squad_chance above 100"), std::string::npos) << *chanceError;

		const std::vector<LevelRosterParams> tooBigSquad { { 20, 6000, 2, {}, 30, 4, true } };
		const auto sizeError = ValidateLevelRoster(entries, tooBigSquad, 16);
		ASSERT_TRUE(sizeError.has_value());
		EXPECT_NE(sizeError->find("squad_size above 3"), std::string::npos) << *sizeError;

		const std::vector<LevelRosterParams> zeroSquad { { 20, 6000, 2, {}, 30, 0, true } };
		const auto zeroError = ValidateLevelRoster(entries, zeroSquad, 16);
		ASSERT_TRUE(zeroError.has_value());
		EXPECT_NE(zeroError->find("squad_size 0"), std::string::npos) << *zeroError;
	}
	// The BehaviorClass::Count sentinel in a class floor.
	{
		const std::vector<LevelRosterEntry> entries { { 20, MT_UNRAV, LevelRosterRole::Core, false } };
		const std::vector<LevelRosterParams> params { { 20, 6000, 2, { { BehaviorClass::Count, 1 } } } };
		const auto error = ValidateLevelRoster(entries, params, 16);
		ASSERT_TRUE(error.has_value());
		EXPECT_NE(error->find("sentinel"), std::string::npos) << *error;
	}
	// An unknown monster id (out-of-bounds type) would let downstream code index past
	// MonstersData, so it must be rejected regardless of reachability.
	{
		const std::vector<LevelRosterEntry> entries {
			{ 20, static_cast<_monster_id>(MonstersData.size() + 5), LevelRosterRole::Core, false },
		};
		const std::vector<LevelRosterParams> params { { 20, 6000, 2, {} } };
		const auto error = ValidateLevelRoster(entries, params, 16);
		ASSERT_TRUE(error.has_value());
		EXPECT_NE(error->find("unknown monster id"), std::string::npos) << *error;
	}
}

TEST_F(LevelRosterTest, CoreNonEmptyAndCoreVsCapAreScopedToTheActiveLevelRange)
{
	// core-non-empty and core-vs-cap are per-LEVEL checks (§4.4.4 counts cores for a given
	// level), so they follow the level scope. Out of range they must not fire; in range they
	// must. Pinning both directions keeps the split honest in both directions.
	const std::vector<LevelRosterEntry> tailOnly { { 20, MT_UNRAV, LevelRosterRole::Tail, false } };
	const std::vector<LevelRosterParams> params20 { { 20, 6000, 2, {} } };
	EXPECT_FALSE(ValidateLevelRoster(tailOnly, params20, 16).has_value())
	    << "an unreachable level's missing core is unreachable data, not a defect";
	const auto inRange = ValidateLevelRoster(tailOnly, params20, 24);
	ASSERT_TRUE(inRange.has_value());
	EXPECT_NE(inRange->find("no core roster members"), std::string::npos) << *inRange;

	// core-vs-cap: three RangedTurret cores at L14 exceed that class's cap of 1 (the
	// non-ranged classes are uncapped in L13-15). Out of range (maxLevel 13) the level is
	// unreachable; in range it must still be rejected.
	const std::vector<LevelRosterEntry> threeMelee {
		{ 14, MT_SUCCUBUS, LevelRosterRole::Core, false },
		{ 14, MT_HLSPWN, LevelRosterRole::Core, false },
		{ 14, MT_MAGISTR, LevelRosterRole::Core, false },
	};
	const std::vector<LevelRosterParams> params14 { { 14, 6000, 2, {} } };
	EXPECT_FALSE(ValidateLevelRoster(threeMelee, params14, 13).has_value());
	const auto capError = ValidateLevelRoster(threeMelee, params14, 14);
	ASSERT_TRUE(capError.has_value());
	EXPECT_NE(capError->find("core roster has 3"), std::string::npos) << *capError;
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

// ---------------------------------------------------------------------------
// Task 3 (spec 2026-09-15-level-rosters-design 4.3.3): the squad columns.
//
// Validation rules, and why they are shaped this way rather than "all three
// fields always in range":
//   - squad_chance is a percentage, so > 100 is meaningless -> reject.
//   - squad_size feeds PlaceGroup's `num`, and the spec caps a core squad at a
//     leader plus at most 3 minions -> > 3 is rejected unconditionally.
//   - squad_size == 0 is only a defect on a level that actually rolls squads.
//     With squad_chance == 0 the level never enters the squad branch, so 0 is
//     the correct "squads off" spelling and must stay legal; rejecting it
//     unconditionally would outlaw every squad-free level (and every
//     LevelRosterParams built without squad columns, e.g. the pre-Task-3 rows
//     the other cases in this file construct). So the rejection is conditional:
//     squad_chance > 0 with squad_size == 0 would roll a squad it can never
//     populate, leaving a leader whose packSize is 0.
// ---------------------------------------------------------------------------

TEST_F(LevelRosterTest, ValidationRejectsAnOutOfRangeSquadChance)
{
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	params[0].squadChance = 101;
	params[0].squadSize = 2;
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("squad_chance"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAnOutOfRangeSquadSize)
{
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	params[0].squadChance = 30;
	params[0].squadSize = 4;
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("squad_size"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationRejectsAZeroSquadSizeWhenSquadsAreEnabled)
{
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	params[0].squadChance = 30;
	params[0].squadSize = 0;
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("squad_size"), std::string::npos);
}

TEST_F(LevelRosterTest, ValidationAcceptsAZeroSquadSizeWhenSquadsAreDisabled)
{
	// squad_chance = 0 is the "squads off" spelling: the sampling loop never
	// enters the squad branch, so squad_size carries no meaning and 0 is valid.
	// This is the complement of the case above; without it, nothing pins the
	// rejection as CONDITIONAL and an unconditional "squad_size >= 1" check
	// would pass the test above while breaking every squad-free level.
	const std::vector<LevelRosterEntry> entries { { 1, MT_WSKELAX, LevelRosterRole::Core, false } };
	std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	params[0].squadChance = 0;
	params[0].squadSize = 0;
	EXPECT_FALSE(ValidateLevelRoster(entries, params).has_value());
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

// Base fixture: LoadCoreArchives()/LoadMonsterData() only need to run once per binary (they are
// expensive and idempotent-enough for repeated calls to just log a benign "already registered"
// warning), so they stay in SetUpTestSuite(). AssetsPath is process-global mutable state that a
// single test's body previously mutated directly; that made the two tests below only pass in a
// specific run order (e.g. under --gtest_filter='LevelRosterFixtureLoadTest.LoadsTheShipped*'
// alone, or under --gtest_shuffle) since nothing ever restored the original path. Each concrete
// fixture below now saves/restores AssetsPath per-test in SetUp()/TearDown(), mirroring
// HasLooseLogicAssetsTest in test/assets_test.cpp, so the two tests are independent of each
// other and of execution order.
class LevelRosterFixtureLoadTestBase : public ::testing::Test {
protected:
	static void SetUpTestSuite()
	{
		LoadCoreArchives();
		gbIsSpawn = false;
		LoadMonsterData();
	}

	void SetUp() override
	{
		savedAssetsPath_ = paths::AssetsPath();
	}

	void TearDown() override
	{
		paths::SetAssetsPath(savedAssetsPath_);
	}

private:
	std::string savedAssetsPath_;
};

class LevelRosterFixtureLoadTest : public LevelRosterFixtureLoadTestBase {
protected:
	void SetUp() override
	{
		LevelRosterFixtureLoadTestBase::SetUp();
		paths::SetAssetsPath(paths::BasePath() + "test/fixtures/");
	}
};

class LevelRosterShippedLoadTest : public LevelRosterFixtureLoadTestBase {
protected:
	void SetUp() override
	{
		LevelRosterFixtureLoadTestBase::SetUp();
		paths::SetAssetsPath(paths::BasePath() + "assets/");
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
	const auto wskelax = std::find_if(level1.begin(), level1.end(), [](const LevelRosterEntry &e) { return e.type == MT_WSKELAX; });
	const auto nzombie = std::find_if(level1.begin(), level1.end(), [](const LevelRosterEntry &e) { return e.type == MT_NZOMBIE; });
	ASSERT_NE(wskelax, level1.end());
	ASSERT_NE(nzombie, level1.end());
	// Both rows are `core	-` in the fixture TSV (see test/fixtures/txtdata/monsters/
	// level_rosters_interleaved.tsv): role Core, allow_unique_boost false.
	EXPECT_EQ(wskelax->role, LevelRosterRole::Core);
	EXPECT_FALSE(wskelax->allowUniqueBoost);
	EXPECT_EQ(nzombie->role, LevelRosterRole::Core);
	EXPECT_FALSE(nzombie->allowUniqueBoost);
}

TEST_F(LevelRosterFixtureLoadTest, ParamsCarrySquadColumns)
{
	// The three squad columns are read positionally by RecordReader, so this
	// drives the real loader against the fixture table rather than constructing
	// LevelRosterParams by hand: a column swap or an off-by-one in the read
	// order would be invisible to a hand-built struct.
	//
	// The fixture's two rows deliberately differ in every squad column
	// (L1 "40 3 true", L2 "0 0 false") so no assertion below can be satisfied
	// by a loader that writes the same value into both rows, and squad_leashed
	// is pinned in BOTH polarities - a parser that ignored the column and left
	// the field at its default would pass one row and fail the other.
	LoadLevelRosterFromFiles(
	    "txtdata\\monsters\\level_rosters_interleaved.tsv",
	    "txtdata\\monsters\\level_roster_params_interleaved.tsv");

	const LevelRosterParams *params1 = GetLevelRosterParams(1);
	ASSERT_NE(params1, nullptr);
	EXPECT_EQ(params1->squadChance, 40);
	EXPECT_EQ(params1->squadSize, 3);
	EXPECT_TRUE(params1->squadLeashed);
	// The pre-existing columns must still land in their own fields: reading three
	// more columns shifts nothing if the order is right, and this catches it if not.
	EXPECT_EQ(params1->maxImage, 6000);
	EXPECT_EQ(params1->tailDraw, 2);

	const LevelRosterParams *params2 = GetLevelRosterParams(2);
	ASSERT_NE(params2, nullptr);
	EXPECT_EQ(params2->squadChance, 0);
	EXPECT_EQ(params2->squadSize, 0);
	EXPECT_FALSE(params2->squadLeashed);
}

// The squad_leashed parser accepts true/false and 1/0 and REJECTS everything else, on the
// grounds that an unrecognised spelling is a mis-typed column rather than a falsy value: a
// silent default would turn a typo into "squads are unleashed on this level", i.e. a behaviour
// change nothing reports. That claim had no case pinning it, so a later "just default it"
// simplification would have gone unnoticed. Both cases below feed a table that is otherwise
// completely valid, so the fatal can only come from this column.
//
// Death-test shape follows the ParseClassFloors cases above: the loader reaches the fatal path
// through RecordReader::failOnError -> DataFile::reportFatalFieldError -> app_fatal, which exits
// with status 1. The matched text is the parser's OWN message ("Invalid squad_leashed value"),
// not just the generic "Invalid value ... for squad_leashed" that reportFatalFieldError prints
// for any rejected field, so the case pins this parser rather than merely "the load failed".
TEST_F(LevelRosterFixtureLoadTest, LoadingRejectsANonBooleanSquadLeashedSpelling)
{
	// "yes" is a plausible boolean spelling that the column deliberately does not accept.
	EXPECT_EXIT(LoadLevelRosterFromFiles(
	                "txtdata\\monsters\\level_rosters_interleaved.tsv",
	                "txtdata\\monsters\\level_roster_params_squad_leashed_typo.tsv"),
	    ::testing::ExitedWithCode(1), "Invalid squad_leashed value");
}

TEST_F(LevelRosterFixtureLoadTest, LoadingRejectsAnOutOfRangeNumericSquadLeashed)
{
	// 1/0 are accepted because the params table is numeric elsewhere; that must not slide into
	// "any integer is truthy", which is how a mis-shifted column would read.
	EXPECT_EXIT(LoadLevelRosterFromFiles(
	                "txtdata\\monsters\\level_rosters_interleaved.tsv",
	                "txtdata\\monsters\\level_roster_params_squad_leashed_outofrange.tsv"),
	    ::testing::ExitedWithCode(1), "Invalid squad_leashed value");
}

TEST_F(LevelRosterShippedLoadTest, LoadsTheShippedRosterAndValidatesIt)
{
	LoadLevelRoster();

	// Real values from the shipped tables (assets/txtdata/monsters/level_roster_params.tsv row
	// "9	16000	3	RangedKite=1"): assert the parsed numbers rather than just non-null/non-empty,
	// so swapping the maxImage/tailDraw columns (both valid positive ints) would be caught.
	const LevelRosterParams *params9 = GetLevelRosterParams(9);
	ASSERT_NE(params9, nullptr);
	EXPECT_EQ(params9->maxImage, 16000);
	EXPECT_EQ(params9->tailDraw, 3);

	const std::span<const LevelRosterEntry> level9 = GetLevelRoster(9);
	EXPECT_FALSE(level9.empty());
	// assets/txtdata/monsters/level_rosters.tsv has "9	MT_BMAGMA	core	-": role Core,
	// allow_unique_boost false. Pin this specific row's parsed fields, not just the roster's
	// non-emptiness.
	const auto bmagma = std::find_if(level9.begin(), level9.end(), [](const LevelRosterEntry &e) { return e.type == MT_BMAGMA; });
	ASSERT_NE(bmagma, level9.end());
	EXPECT_EQ(bmagma->role, LevelRosterRole::Core);
	EXPECT_FALSE(bmagma->allowUniqueBoost);
}

TEST_F(LevelRosterShippedLoadTest, ShippedParamsEnableSquadsOnEveryParameterisedLevel)
{
	// Task 3 ships squad columns for L1-15 (the params table has no L16 row by
	// design - see level_roster.h). The scatter loop's squad branch is gated on
	// squadChance > 0 && squadSize > 0, so a level shipping 0 in either column
	// would silently never form a squad; that is the failure this pins.
	LoadLevelRoster();

	for (uint8_t level = 1; level <= 15; level++) {
		const LevelRosterParams *params = GetLevelRosterParams(level);
		ASSERT_NE(params, nullptr) << "level " << static_cast<int>(level) << " must have a params row";
		EXPECT_GT(params->squadChance, 0)
		    << "level " << static_cast<int>(level) << " ships squad_chance 0, so its squad branch is dead";
		EXPECT_LE(params->squadChance, 100) << "level " << static_cast<int>(level);
		EXPECT_GE(params->squadSize, 1)
		    << "level " << static_cast<int>(level) << " ships squad_size 0 with squads enabled";
		EXPECT_LE(params->squadSize, 3) << "level " << static_cast<int>(level);
	}
}
