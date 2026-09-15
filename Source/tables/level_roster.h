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
};

/**
 * @brief Loads and validates the shipped level roster tables. Fatal on any validation failure.
 *
 * Not wired to any production call site yet (see Task 2b).
 *
 * Each call clears and rebuilds the internal Entries/Params storage, so any std::span or
 * pointer previously returned by GetLevelRoster()/GetLevelRosterParams() is invalidated the
 * moment this runs again; callers must not retain those views across a reload.
 */
void LoadLevelRoster();

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
