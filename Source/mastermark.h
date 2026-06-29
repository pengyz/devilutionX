#pragma once

#include <cstdint>
#include <string>

#include "tables/playerdat.hpp"

namespace devilution {
struct Player;

// === Mark Identifiers (9 marks, 3 per class) ===

enum class MasterMarkId : uint8_t {
	// Warrior (3)
	IronBastion,
	CrimsonBrand,
	ArmsMaster,
	// Sorcerer (3)
	Sanguimancer,
	Spellblade,
	Overcharge,
	// Rogue (3)
	Marksman,
	Shadowstep,
	Precision,

	COUNT // = 9
};

// === Rune System ===

enum class RuneRarity : uint8_t {
	Common,    // White  — slot 1
	Rare,      // Blue   — slot 2
	Legendary, // Gold   — slot 3
};

// Each mark has 3 rune slots, each with a binary choice
struct MarkSlot {
	RuneRarity requiredRune;
	bool socketed = false;
	uint8_t choice = 0; // 0 = empty, 1 = path A, 2 = path B
};

// Per-mark runtime state (4 generic data fields for mark-specific use)
struct MarkState {
	MasterMarkId id = MasterMarkId::COUNT;
	MarkSlot slots[3];
	bool isActive = false;

	// Mark-specific runtime data (interpreted per-mark)
	int32_t data0 = 0;
	int32_t data1 = 0;
	int32_t data2 = 0;
	int32_t data3 = 0;
};

// === Mark Definition ===

struct MasterMarkDef {
	const char *name;
	const char *description;
	const char *coreMechanic;
	const char *slotDescA[3];
	const char *slotDescB[3];
	HeroClass requiredClass;
};

extern const MasterMarkDef markDefs[static_cast<size_t>(MasterMarkId::COUNT)];
constexpr size_t MarkCount = static_cast<size_t>(MasterMarkId::COUNT);

// === Mark Management ===

void GrantMark(Player &player, MasterMarkId id);
bool HasMark(const Player &player, MasterMarkId id);
bool HasActiveMark(const Player &player, MasterMarkId id);
void ActivateMark(Player &player, MasterMarkId id, int activeSlot);
void DeactivateMark(Player &player, int activeSlot);
void SwapActiveMark(Player &player, MasterMarkId id);

// Returns the MarkState for a given mark on a player
MarkState &GetMarkState(Player &player, MasterMarkId id);
const MarkState &GetMarkState(const Player &player, MasterMarkId id);

// Returns which path (0=empty, 1=A, 2=B) the player chose for a given slot on a given mark
uint8_t GetMarkSlotChoice(const Player &player, MasterMarkId id, int slotIndex);

// === Rune Socketing ===

// Socket a rune into a mark slot, permanently locking the choice.
// Returns false if the rune rarity doesn't match or slot is already filled.
bool SocketRune(Player &player, MasterMarkId id, int slotIndex, uint8_t choice);

// === Combat Hooks ===

// Called per tick for active marks
void CheckMarkEffects(Player &player);

// Called on successful block
void MarkOnPlayerBlock(Player &player, int blockedDamage);

// Called before melee attack damage calculation
void MarkOnPlayerAttack(Player &player, int &damage, bool isMelee);

// Called when player kills a monster
void MarkOnPlayerKill(Player &player, int monsterMaxHp);

// Called when player takes damage (before HP subtraction)
// Returns true if the mark prevented death
bool MarkOnPlayerDamaged(Player &player, int &damage);

// Called when player dodges an attack
void MarkOnPlayerDodge(Player &player);

// Called before spell cast (modifies mana cost and cast speed)
void MarkOnPlayerSpell(Player &player, int &manaCost, int &castTime);

// Called to compute spell damage modifier
int MarkSpellDamageModifier(const Player &player);

// Called when player is about to die from fatal damage
// Returns true if death was prevented by a mark
bool MarkOnPlayerFatalDamage(Player &player);

// === UI ===
std::string GetActiveMarksString(const Player &player);

} // namespace devilution
