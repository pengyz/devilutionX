/**
 * @file level_roster.h
 *
 * Interface of the per-level monster roster tables (types, loader, validation).
 */
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tables/monstdat.h"

namespace devilution {

enum class LevelRosterRole : uint8_t {
	Core,
	Tail,
};

struct LevelRosterEntry {
	uint8_t level;
	_monster_id type;
	LevelRosterRole role;
	bool allowUniqueBoost;
};

struct LevelRosterParams {
	uint8_t level;
	int maxImage;
	int tailDraw;
	std::vector<std::pair<BehaviorClass, uint8_t>> classFloors;
	/**
	 * Percent chance (0-100) that a core type drawn by the scatter loop is placed as a
	 * squad (one leader plus `squadSize` minions of another core type) instead of as a
	 * plain same-type group. 0 disables squads for the level entirely.
	 */
	uint8_t squadChance = 0;
	/**
	 * Number of minions requested for a squad, 1-3. Only meaningful when squadChance > 0;
	 * validation rejects 0 in that case, because a squad roll that can place no minion
	 * would leave a leader with packSize 0.
	 */
	uint8_t squadSize = 0;
	/**
	 * true: minions are leashed to the leader (setLeader + packSize + the 4-tile leash).
	 * false (spec 4.3.4 fallback): the leader is still passed to PlaceGroup, so minions
	 * are still seeded next to it, but no leash/packSize/regroup semantics are applied.
	 */
	bool squadLeashed = true;
};

/**
 * @brief Loads and validates the shipped level roster tables. Fatal on any validation failure.
 *
 * Must be called after LoadMonsterData(): validation reads MonstersData/UniqueMonstersData,
 * which LoadMonsterData() populates.
 *
 * Each call clears and rebuilds the internal Entries/Params storage, so any std::span or
 * pointer previously returned by GetLevelRoster()/GetLevelRosterParams() is invalidated the
 * moment this runs again; callers must not retain those views across a reload.
 */
void LoadLevelRoster();

/**
 * @brief Loads and validates the level roster tables from the given file paths (instead of the
 * shipped `txtdata\monsters\level_rosters.tsv`/`level_roster_params.tsv`). Fatal on any
 * validation failure.
 *
 * `LoadLevelRoster()` calls this with the shipped paths; tests use it directly against
 * `test/fixtures/` tables so this has a production caller.
 */
void LoadLevelRosterFromFiles(std::string_view rosterFile, std::string_view paramsFile);

/**
 * @brief Parses a `class_floors` cell (e.g. "Melee=2,Ranged=1") into class/floor pairs.
 *
 * Fatal (`app_fatal`) on a malformed entry: missing '=', an unknown BehaviorClass name, or a
 * non-integer floor. An empty `value` yields an empty result (no floors for that level).
 */
std::vector<std::pair<BehaviorClass, uint8_t>> ParseClassFloors(std::string_view value);

/**
 * @brief Stable-sorts `entries` by level, preserving each level's relative file order.
 *
 * GetLevelRoster()/FindLevelRoster() rely on same-level rows being physically contiguous,
 * which the source TSV does not guarantee, so LoadLevelRoster() calls this before use.
 */
void SortRosterByLevel(std::span<LevelRosterEntry> entries);

/**
 * @brief Returns the contiguous run of rows for `level` from an already sorted table.
 *
 * `entries` must already be sorted by level (see SortRosterByLevel()); otherwise rows for
 * `level` that are not contiguous will not all be included.
 *
 * The returned span is a view into `entries` and shares its lifetime: it is only valid as long
 * as the backing storage is not reallocated, resized, or destroyed. In particular, a span
 * returned from GetLevelRoster() (which is backed by LoadLevelRoster()'s internal storage) is
 * invalidated by the next LoadLevelRoster()/LoadLevelRosterFromFiles() call; callers must not
 * retain it across a reload.
 */
std::span<const LevelRosterEntry> FindLevelRoster(std::span<const LevelRosterEntry> entries, uint8_t level);

/**
 * This function does not filter by monster availability (spawn/retail/hellfire): under spawn
 * data most core rows are `availability=Retail` and therefore unavailable, so a caller that
 * samples from this roster (Task 3's sampling loop) must apply its own availability filter
 * against the currently loaded MonstersData before treating a returned entry as usable; a
 * non-empty span here is not itself a guarantee of spawn-mode availability.
 *
 * Level 16 is a documented exception (spec 4.2.7, R31): GetLevelMTypes() hardcodes L16's types
 * and returns before the roster pre-add runs, so `level_rosters.tsv`'s L16 rows are
 * REGISTRATION ONLY - they record which monsters the level fields for the validator and for
 * later phases, and never influence sampling. `level_roster_params.tsv` deliberately has no L16
 * row for the same reason: a params row there would be unreachable table data. The TSV format
 * carries no comment syntax, so that fact is recorded here rather than beside the rows.
 */
std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level);
const LevelRosterParams *GetLevelRosterParams(uint8_t level);

/**
 * @brief Validates a candidate roster against the currently loaded MonstersData/UniqueMonstersData.
 * @return An error description, or an empty optional when the roster is valid.
 */
std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params);

/**
 * B1 sampling cap for this level's behaviour mix (monster.cpp:3513-3531).
 * Task 3's sampling loop must use this too, so the two cannot drift.
 */
uint8_t BehaviorClassCapForLevel(uint8_t level, BehaviorClass cls);

} // namespace devilution
