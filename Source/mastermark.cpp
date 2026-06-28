#include "mastermark.h"

#include <algorithm>

#include "player.h"

namespace devilution {

// === Mark Definitions (9 marks) ===
// clang-format off
const MasterMarkDef markDefs[MarkCount] = {
	// Warrior
	{ "Iron Bastion",   "Blocking stores damage. Release it in a single blow.",            HeroClass::Warrior },
	{ "Crimson Brand",  "Blood fuels rage. Rage fuels destruction.",                       HeroClass::Warrior },
	{ "Arms Master",    "Dual two-handers or blade-and-board — your weapons obey you.",    HeroClass::Warrior },
	// Sorcerer
	{ "Sanguimancer",   "Mana is an illusion. Blood is the true currency.",                HeroClass::Sorcerer },
	{ "Spellblade",     "The staff is not a walking stick. It's a weapon.",                HeroClass::Sorcerer },
	{ "Overcharge",     "Burn twice as bright. Accept the aftermath.",                      HeroClass::Sorcerer },
	// Rogue
	{ "Marksman",       "Distance is armor. Patience is the deadliest arrow.",              HeroClass::Rogue },
	{ "Shadowstep",     "They swing at air. You are already behind them.",                  HeroClass::Rogue },
	{ "Precision",      "Eyes open. Hands steady. Everything is a target.",                 HeroClass::Rogue },
};
// clang-format on

// === Mark Management ===

MarkState &GetMarkState(Player &player, MasterMarkId id)
{
	return player.ownedMarks[static_cast<size_t>(id)];
}

const MarkState &GetMarkState(const Player &player, MasterMarkId id)
{
	return player.ownedMarks[static_cast<size_t>(id)];
}

void GrantMark(Player &player, MasterMarkId id)
{
	auto &state = GetMarkState(player, id);
	state.id = id;
	state.isActive = false;
	for (auto &slot : state.slots) {
		slot.socketed = false;
		slot.choice = 0;
	}
}

bool HasMark(const Player &player, MasterMarkId id)
{
	return GetMarkState(player, id).id == id;
}

bool HasActiveMark(const Player &player, MasterMarkId id)
{
	return (player.activeMarks[0] == id) || (player.activeMarks[1] == id);
}

void ActivateMark(Player &player, MasterMarkId id, int activeSlot)
{
	if (activeSlot >= 0 && activeSlot < 2) {
		if (player.activeMarks[activeSlot] != MasterMarkId::COUNT) {
			GetMarkState(player, player.activeMarks[activeSlot]).isActive = false;
		}
		player.activeMarks[activeSlot] = id;
		GetMarkState(player, id).isActive = true;
	}
}

void DeactivateMark(Player &player, int activeSlot)
{
	if (activeSlot >= 0 && activeSlot < 2) {
		if (player.activeMarks[activeSlot] != MasterMarkId::COUNT) {
			GetMarkState(player, player.activeMarks[activeSlot]).isActive = false;
			player.activeMarks[activeSlot] = MasterMarkId::COUNT;
		}
	}
}

void SwapActiveMark(Player &player, MasterMarkId id)
{
	if (!HasMark(player, id))
		return;

	if (HasActiveMark(player, id)) {
		for (int i = 0; i < 2; i++) {
			if (player.activeMarks[i] == id) {
				DeactivateMark(player, i);
				break;
			}
		}
		return;
	}

	for (int i = 0; i < 2; i++) {
		if (player.activeMarks[i] == MasterMarkId::COUNT) {
			ActivateMark(player, id, i);
			return;
		}
	}

	DeactivateMark(player, 0);
	ActivateMark(player, id, 1);
}

uint8_t GetMarkSlotChoice(const Player &player, MasterMarkId id, int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= 3)
		return 0;
	return GetMarkState(player, id).slots[slotIndex].choice;
}

// === Rune Socketing ===

bool SocketRune(Player &player, MasterMarkId id, int slotIndex, uint8_t choice)
{
	if (!HasMark(player, id))
		return false;
	if (slotIndex < 0 || slotIndex >= 3)
		return false;
	if (choice != 1 && choice != 2)
		return false;

	auto &slot = GetMarkState(player, id).slots[slotIndex];
	if (slot.socketed)
		return false; // Already filled, can't change

	slot.socketed = true;
	slot.choice = choice;
	return true;
}

// === Forward-declared class helpers (implemented in per-class .cpp files) ===
namespace WarriorMarks {
	void OnPlayerBlock(Player&, int);
	void OnPlayerAttack(Player&, int&);
	void OnPlayerDamaged(Player&, int&);
	bool OnPlayerFatalDamage(Player&);
	void CheckEffects(Player&);
}
namespace SorcererMarks {
	bool HandleCast(Player&, int);
	void OnPlayerKill(Player&, int);
	bool OnPlayerFatalDamage(Player&);
	int SpellDamageModifier(const Player&);
	void CheckEffects(Player&);
}
namespace RogueMarks {
	void OnPlayerAttack(Player&, int&, bool);
	void OnPlayerDodge(Player&);
	void OnPlayerKill(Player&, int);
	bool OnPlayerFatalDamage(Player&);
	void CheckEffects(Player&);
}

// === Dispatch ===
void MarkOnPlayerBlock(Player &p, int dmg)        { WarriorMarks::OnPlayerBlock(p, dmg); }
void MarkOnPlayerAttack(Player &p, int &d, bool m) { WarriorMarks::OnPlayerAttack(p, d); RogueMarks::OnPlayerAttack(p, d, m); }
void MarkOnPlayerKill(Player &p, int hp)            { SorcererMarks::OnPlayerKill(p, hp); RogueMarks::OnPlayerKill(p, hp); }
void MarkOnPlayerDamaged(Player &p, int &d)         { WarriorMarks::OnPlayerDamaged(p, d); }
bool MarkOnPlayerFatalDamage(Player &p) {
	if (WarriorMarks::OnPlayerFatalDamage(p)) return true;
	if (SorcererMarks::OnPlayerFatalDamage(p)) return true;
	if (RogueMarks::OnPlayerFatalDamage(p)) return true;
	return false;
}
void MarkOnPlayerDodge(Player &p)                   { RogueMarks::OnPlayerDodge(p); }
void MarkOnPlayerSpell(Player &p, int &mana, int &t) { SorcererMarks::HandleCast(p, mana); }
int MarkSpellDamageModifier(const Player &p)         { return SorcererMarks::SpellDamageModifier(p); }
void CheckMarkEffects(Player &p) {
	WarriorMarks::CheckEffects(p);
	SorcererMarks::CheckEffects(p);
	RogueMarks::CheckEffects(p);
}

// === UI ===

std::string GetActiveMarksString(const Player &player)
{
	std::string result;
	for (int i = 0; i < 2; i++) {
		if (player.activeMarks[i] != MasterMarkId::COUNT) {
			if (!result.empty())
				result += ", ";
			result += markDefs[static_cast<size_t>(player.activeMarks[i])].name;
		}
	}
	return result;
}

} // namespace devilution
