#pragma once

#include <expected>
#include <string>

#include "engine/clx_sprite.hpp"
#include "engine/surface.hpp"
#include "tables/spelldat.h"

namespace devilution {

struct Player;

std::expected<void, std::string> InitSpellBook();
void FreeSpellBook();
void CheckSBook();
void DrawSpellBook(const Surface &out);

/**
 * @brief Get the requirement text for a spell
 * @param spellData The spell data
 * @param player The player to check requirements for
 * @return Formatted requirement text
 */
std::string GetSpellRequirementText(const SpellData &spellData, const Player &player);

/**
 * @brief Check if a player can learn a spell
 * @param spell The spell ID
 * @param player The player to check
 * @return True if the player can learn the spell
 */
bool CanLearnSpell(SpellID spell, const Player &player);

} // namespace devilution
