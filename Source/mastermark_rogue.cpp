#include "mastermark.h"
#include "player.h"

namespace devilution {
namespace RogueMarks {

// =====================================================================
// Marksman — Bow Specialist
// Core: Bow/crossbow +20% dmg. Aim: stand 1s→dmg scales with distance (3%/tile, cap 30%).
// Slot1: A=Multishot (3 arrows), B=Pierce (50%)
// Slot2: A=Longbow (5%/tile, cap 50%), B=Deadly Focus (+25% crit while aiming)
// Slot3: A=Arrow Rain (10 arrows, 30s cd), B=Heartseeker (3s aim→guaranteed crit ×3)
// =====================================================================

static bool HasMarksman(const Player &player) {
	return HasActiveMark(player, MasterMarkId::Marksman);
}

static void MarksmanPerTick(Player &player) {
	if (!HasMarksman(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::Marksman);
	if (state.data2 > 0) state.data2--; // Arrow Rain cooldown
	// data0 = stand ticks, data1 = aim state flag
	if (player._pmode == PM_STAND) {
		state.data0++;
		if (state.data0 >= 60) state.data1 = 1; // enter aim after 1s
	} else {
		state.data0 = 0;
		// Aim state persists; spec says "movement does not break"
	}
}

static int MarksmanRangedDmgBonus(const Player &player, int baseDmg, int distance) {
	if (!HasMarksman(player)) return 0;
	int bonus = baseDmg * 20 / 100;
	auto &state = GetMarkState(player, MasterMarkId::Marksman);
	if (state.data1) {
		int slot2 = GetMarkSlotChoice(player, MasterMarkId::Marksman, 1);
		int perTile = (slot2 == 1) ? 5 : 3;
		int cap = (slot2 == 1) ? 50 : 30;
		int distBonus = std::min(distance * perTile, cap);
		bonus += baseDmg * distBonus / 100;
	}
	return bonus;
}

// =====================================================================
// Shadowstep — Assassin
// Core: Dodge→teleport behind attacker, next attack guaranteed crit. 4s cd.
//        Manual trigger: teleport to target, 8s cd.
// Slot1: A=+15% dodge, B=leave decoy 2s
// Slot2: A=crit 200%+kill reset cd, B=poison DoT on crit
// Slot3: A=can SS twice in 4s, B=+4 tile range + wall bypass
// =====================================================================

static bool HasShadowstep(const Player &player) {
	return HasActiveMark(player, MasterMarkId::Shadowstep);
}

static void ShadowstepPerTick(Player &player) {
	if (!HasShadowstep(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::Shadowstep);
	if (state.data0 > 0) state.data0--; // Passive cd
	if (state.data1 > 0) state.data1--; // Manual cd
	if (state.data2 > 0) state.data2--; // Chain window
}

// =====================================================================
// Precision — Hybrid
// Core: +15% melee crit (ranged half: +7.5%). +20% ranged hit (melee half: +10%)
// Slot1: A=melee crit dmg +30%+ranged hit+10%, B=ranged atk spd+10%+melee hit+10%
// Slot2: A=Weak Spot (crit mark +15% dmg), B=Rhythm (crit→+20% atk spd 3s)
// Slot3: A=Versatility (ranged crit full 15%), B=Execution (ranged→extra arrow, melee→8% lifesteal)
// =====================================================================

static bool HasPrecision(const Player &player) {
	return HasActiveMark(player, MasterMarkId::Precision);
}

static void PrecisionPerTick(Player &player) {
	if (!HasPrecision(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::Precision);
	if (state.data2 > 0) state.data2--; // Rhythm timer
	if (state.data2 <= 0) state.data1 = 0; // Rhythm inactive
}

// === RogueMarks dispatch ===

void OnPlayerAttack(Player & /*player*/, int & /*damage*/, bool /*isMelee*/) {
	// Rogue marks: Precision passive stats handled elsewhere; 
	// Marksman range bonus applied in missile damage calc.
}

void OnPlayerDodge(Player &player) {
	if (!HasShadowstep(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::Shadowstep);
	if (state.data0 > 0) return; // On cooldown
	// TODO: teleport behind attacker, set guaranteed crit, apply slot effects
	state.data0 = 4 * 60;
}

void OnPlayerKill(Player & /*player*/, int /*monsterMaxHp*/) {
	// Rogue marks don't trigger on kill in initial implementation
}

bool OnPlayerFatalDamage(Player & /*player*/) {
	return false;
}

void CheckEffects(Player &player) {
	MarksmanPerTick(player);
	ShadowstepPerTick(player);
	PrecisionPerTick(player);
}

} // namespace RogueMarks
} // namespace devilution
