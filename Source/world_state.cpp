/**
 * @file world_state.cpp
 *
 * Implementation of world state system for living dungeon.
 */
#include "world_state.h"

#include "quests.h"
#include "tables/objdat.h"

namespace devilution {

WorldState GetWorldState()
{
	WorldState state;

	// Check quest progress
	state.skeletonKingDefeated = Quests[Q_SKELKING]._qactive == QUEST_DONE;

	// Determine stage based on progress
	if (state.skeletonKingDefeated) {
		state.stage = 1;
	}
	if (currlevel >= 9) { // Caves
		state.stage = 2;
		state.enteredCaves = true;
	}
	if (currlevel >= 13) { // Hell
		state.stage = 3;
		state.enteredHell = true;
	}
	if (currlevel >= 15) { // Near Diablo
		state.stage = 4;
		state.nearDiablo = true;
	}

	return state;
}

int GetTristramDarkness(const WorldState &state)
{
	// Each stage adds more darkness
	return state.stage * 2; // 0, 2, 4, 6, 8
}

int GetDungeonCorruption(const WorldState &state)
{
	// Corruption level matches stage
	return state.stage; // 0, 1, 2, 3, 4
}

} // namespace devilution
