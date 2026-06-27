#pragma once

#include <bitset>
#include <cstdint>
#include <string>
#include <vector>

#include "tables/playerdat.hpp"

namespace devilution {
struct Player;



enum class MasterMarkId : uint8_t {
	// Warrior (8)
	ShieldMaster,
	Berserker,
	Commander,
	Avenger,
	Juggernaut,
	Sentinel,
	Executioner,
	IronWill,
	// Sorcerer (8)
	Arcanist,
	Pyromancer,
	Stormcaller,
	Sanguimancer,
	Frostborn,
	LeyWeaver,
	EchoMage,
	Voidcaller,
	// Rogue (8)
	Deadeye,
	Trapsmith,
	Shadowstep,
	Ricochet,
	Predator,
	WindWalker,
	Venomancer,
	Ghost,
	// Monk (8)
	IronPalm,
	Serenity,
	ChiWave,
	EarthStance,
	FlowingWater,
	InnerFire,
	Karma,
	Transcendence,
	// Bard (8)
	Warsong,
	Lament,
	Echosong,
	HymnOfRespite,
	Crescendo,
	Dissonance,
	Coda,
	Overture,
	// Barbarian (8)
	BloodRage,
	Unchained,
	Sunder,
	Warcry,
	ThickSkin,
	Momentum,
	LastStand,
	TrophyHunter,

	COUNT
};

enum class MasterMarkSource : uint8_t {
	MentorOrdeal,
	Dungeon,
};

struct MasterMarkDef {
	const char *name;
	const char *description;
	HeroClass requiredClass;
	MasterMarkSource source;
	int dungeonLevel; // 0 = mentor ordeal, 1-16 = dungeon level
};

extern const MasterMarkDef markDefs[static_cast<size_t>(MasterMarkId::COUNT)];

// Mark state management
void GrantMark(Player &player, MasterMarkId id);
bool HasMark(const Player &player, MasterMarkId id);
bool HasActiveMark(const Player &player, MasterMarkId id);
void ActivateMark(Player &player, MasterMarkId id, int slot);
void DeactivateMark(Player &player, int slot);

constexpr size_t MarkCount = static_cast<size_t>(MasterMarkId::COUNT);

// Ordeal tracking
struct OrdealState {
	MasterMarkId markId;
	int progress;
	bool completed;
};

// Mark effect hooks — called from combat code
void CheckMarkEffects(Player &player);

// Get shield armor for ShieldMaster bash
int GetShieldArmor(const Player &player);

// Check if an ordeal is completed and grant mark
void CheckAndGrantOrdealMarks(Player &player);

// Swap a player's active marks — deactivates both, activates newest owned
void SwapActiveMark(Player &player, MasterMarkId id);

} // namespace devilution
