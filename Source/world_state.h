/**
 * @file world_state.h
 *
 * Interface of world state system for living dungeon.
 */
#pragma once

#include <cstdint>

namespace devilution {

struct WorldState {
	int stage = 0; // 0=normal, 1=alert, 2=tense, 3=fear, 4=despair
	bool skeletonKingDefeated = false;
	bool enteredCaves = false;
	bool enteredHell = false;
	bool nearDiablo = false;
};

/**
 * @brief Get the current world state based on player progress
 * @return Current world state
 */
WorldState GetWorldState();

/**
 * @brief Get the Tristram darkness level based on world state
 * @param state The world state
 * @return Darkness level (0-8)
 */
int GetTristramDarkness(const WorldState &state);

/**
 * @brief Get the dungeon corruption level based on world state
 * @param state The world state
 * @return Corruption level (0-4)
 */
int GetDungeonCorruption(const WorldState &state);

} // namespace devilution
