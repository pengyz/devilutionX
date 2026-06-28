#include "mastermark.h"
#include "player.h"

namespace devilution {
namespace SorcererMarks {

// =====================================================================
// Sanguimancer — Blood Mage
// Core: When out of mana, pay HP to cast (1 mana = 2 HP). Kill → restore 10% max HP.
// Slot1: A=restore 15%, B=curse nearby enemies on blood kill
// Slot2: A=HP<30% → conversion 1:1, B=blood cast grants temp HP=50% of HP spent
// Slot3: A=HP<20% → 1:1 +40% dmg +20% restore, B=fatal→consume temp HP survive
// =====================================================================

bool HasSanguimancer(const Player &player) {
	return HasActiveMark(player, MasterMarkId::Sanguimancer);
}

bool SanguimancerHandleCast(Player &player, int manaCost) {
	if (!HasSanguimancer(player)) return false;
	if (player._pMana >= manaCost) return false;

	int ratio = 2; // HP per mana
	int slot2 = GetMarkSlotChoice(player, MasterMarkId::Sanguimancer, 0);
	if (slot2 == 1 && player._pHitPoints < player._pMaxHP * 30 / 100) ratio = 1;
	int slot3 = GetMarkSlotChoice(player, MasterMarkId::Sanguimancer, 2);
	if (slot3 == 1 && player._pHitPoints < player._pMaxHP * 20 / 100) ratio = 1;

	int hpCost = manaCost * ratio;
	if (player._pHitPoints <= hpCost) return false;
	player._pHitPoints -= hpCost;
	player._pMana += manaCost;
	return true;
}

void SanguimancerOnKill(Player &player, int monsterMaxHp) {
	if (!HasSanguimancer(player)) return;
	int slot1 = GetMarkSlotChoice(player, MasterMarkId::Sanguimancer, 0);
	int pct = (slot1 == 1) ? 15 : 10;
	int slot3 = GetMarkSlotChoice(player, MasterMarkId::Sanguimancer, 2);
	if (slot3 == 1 && player._pHitPoints < player._pMaxHP * 20 / 100) pct = 20;
	int restore = monsterMaxHp * pct / 100;
	player._pHitPoints = std::min(player._pHitPoints + restore, player._pMaxHP);
}

bool SanguimancerOnFatal(Player &player) {
	if (!HasSanguimancer(player)) return false;
	if (GetMarkSlotChoice(player, MasterMarkId::Sanguimancer, 2) != 2) return false;
	auto &state = GetMarkState(player, MasterMarkId::Sanguimancer);
	if (state.data0 <= 0) return false; // data0 = temp HP (Ward)
	player._pHitPoints = 1;
	state.data0 = 0;
	return true;
}

int SanguimancerSpellDmgMod(const Player &player) {
	if (!HasSanguimancer(player)) return 0;
	if (GetMarkSlotChoice(player, MasterMarkId::Sanguimancer, 2) != 1) return 0;
	if (player._pHitPoints < player._pMaxHP * 20 / 100) return 40;
	return 0;
}

// =====================================================================
// Spellblade — Melee Sorcerer
// Core: Mana Shield toggle (no drain, each hit costs mana=absorbed dmg).
//        Weapon enchant: bonus elemental dmg (fire/lightning alt).
//        Weapon dmg from INT, ignore STR req.
// Slot1: A=absorb 50%, B=reflect 20%
// Slot2: A=10% chance 3-tile AoE, B=poison DoT 3s stack 3×
// Slot3: A=weapon dmg=INT×0.8, B=INT→+0.5% atk spd, STR→+3% enchant dmg
// =====================================================================

bool HasSpellblade(const Player &player) {
	return HasActiveMark(player, MasterMarkId::Spellblade);
}

int SpellbladeEnchantBonus(const Player &player, int baseDmg) {
	if (!HasSpellblade(player)) return 0;
	auto &state = GetMarkState(player, MasterMarkId::Spellblade);
	if (state.data0 <= 0) return 0; // Enchant active timer
	int bonus = baseDmg * 30 / 100;
	int slot3 = GetMarkSlotChoice(player, MasterMarkId::Spellblade, 2);
	if (slot3 == 2) bonus += bonus * player._pStrength * 3 / 100;
	return bonus;
}

void SpellbladeApplyEnchant(Player &player) {
	auto &state = GetMarkState(player, MasterMarkId::Spellblade);
	state.data0 = 60 * 60; // 60s enchant
	state.data1 = (state.data1 == 0) ? 1 : 0; // Toggle fire/lightning
}

void SpellbladePerTick(Player &player) {
	auto &state = GetMarkState(player, MasterMarkId::Spellblade);
	if (state.data0 > 0) state.data0--;
}

// =====================================================================
// Overcharge — Burst Mage
// Core: Activate: 8s ×2 cast speed, 0 mana cost. Then 10s weakness (-50% dmg, mana-30%).
// Slot1: A=+30% spell dmg during, B=+4s duration
// Slot2: A=weakness 5s, B=mana-15% (no speed penalty)
// Slot3: A=end explodes (20% total dmg), B=after weakness: auto-refill mana, halved cooldowns 3s
// =====================================================================

bool HasOvercharge(const Player &player) {
	return HasActiveMark(player, MasterMarkId::Overcharge);
}

void OverchargeActivate(Player &player) {
	if (!HasOvercharge(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::Overcharge);
	if (state.data2 > 0 || state.data0 > 0 || state.data1 > 0) return;
	state.data0 = 8 * 60; // Burst 8s
	int slot1 = GetMarkSlotChoice(player, MasterMarkId::Overcharge, 0);
	if (slot1 == 2) state.data0 += 4 * 60;
}

bool OverchargeModifyCast(Player &player, int &manaCost, int &castTime) {
	if (!HasOvercharge(player)) return false;
	auto &state = GetMarkState(player, MasterMarkId::Overcharge);
	if (state.data0 <= 0) return false;
	manaCost = 0;
	castTime /= 2;
	return true;
}

int OverchargeSpellDmgMod(const Player &player) {
	if (!HasOvercharge(player)) return 0;
	auto &state = GetMarkState(player, MasterMarkId::Overcharge);
	if (state.data0 <= 0) return 0;
	return (GetMarkSlotChoice(player, MasterMarkId::Overcharge, 0) == 1) ? 30 : 0;
}

int OverchargeWeaknessDmgMod(const Player &player) {
	if (!HasOvercharge(player)) return 0;
	auto &state = GetMarkState(player, MasterMarkId::Overcharge);
	return (state.data1 > 0) ? -50 : 0;
}

void OverchargePerTick(Player &player) {
	if (!HasOvercharge(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::Overcharge);
	if (state.data2 > 0) state.data2--; // Cooldown
	if (state.data0 > 0) { // Burst active
		state.data0--;
		if (state.data0 <= 0) { // Burst ends → weakness
			int slot2 = GetMarkSlotChoice(player, MasterMarkId::Overcharge, 1);
			state.data1 = (slot2 == 1) ? 5 * 60 : 10 * 60;
			state.data2 = 60 * 60; // 60s cooldown after weakness
		}
	} else if (state.data1 > 0) { // Weakness active
		state.data1--;
		if (state.data1 <= 0) { // Weakness over
			if (GetMarkSlotChoice(player, MasterMarkId::Overcharge, 2) == 2) {
				player._pMana = player._pMaxMana;
			}
		}
	}
}

// === SorcererMarks dispatch ===

bool HandleCast(Player &player, int manaCost) {
	return SanguimancerHandleCast(player, manaCost);
}

void OnPlayerKill(Player &player, int monsterMaxHp) {
	SanguimancerOnKill(player, monsterMaxHp);
}

bool OnPlayerFatalDamage(Player &player) {
	return SanguimancerOnFatal(player);
}

int SpellDamageModifier(const Player &player) {
	int mod = SanguimancerSpellDmgMod(player);
	mod += OverchargeSpellDmgMod(player);
	mod += OverchargeWeaknessDmgMod(player);
	return mod;
}

void CheckEffects(Player &player) {
	SpellbladePerTick(player);
	OverchargePerTick(player);
}

} // namespace SorcererMarks
} // namespace devilution
