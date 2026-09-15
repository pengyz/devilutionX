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
 */
void LoadLevelRoster();

std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level);
const LevelRosterParams *GetLevelRosterParams(uint8_t level);

/**
 * @brief Validates a candidate roster against the currently loaded MonstersData/UniqueMonstersData.
 * @return An error description, or an empty optional when the roster is valid.
 */
std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params);

} // namespace devilution
