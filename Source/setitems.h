#pragma once

#include <cstdint>

namespace devilution {

struct Player;

enum class SetId : uint8_t {
	ButchersLegacy,    // Warrior: Cleaver + Apron + random helm
	Deathspeaker,       // Sorcerer: Staff + Hood + random ring
	WindforcesGift,     // Rogue: Bow + Quiver + random gloves
	ArchmagesRegalia,   // All: Robe + Circlet + random amulet
	COUNT
};

// Number of pieces in each set
constexpr int SetPieceCount[static_cast<size_t>(SetId::COUNT)] = {
	3, // Butcher's Legacy
	3, // Deathspeaker
	3, // Windforce's Gift
	3, // Archmage's Regalia
};

// Piece index constants for clarity
constexpr int ButchersCleaver = 0;
constexpr int ButchersApron = 1;
constexpr int ButchersHelm = 2;

constexpr int DeathspeakerStaff = 0;
constexpr int DeathspeakerHood = 1;
constexpr int DeathspeakerRing = 2;

constexpr int WindforcesBow = 0;
constexpr int WindforcesQuiver = 1;
constexpr int WindforcesGloves = 2;

constexpr int ArchmagesRobe = 0;
constexpr int ArchmagesCirclet = 1;
constexpr int ArchmagesAmulet = 2;

// Called on equipment change to recalculate set bonuses
void CheckSetBonuses(Player &player);

} // namespace devilution
