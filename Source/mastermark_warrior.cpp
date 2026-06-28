#include "mastermark.h"
#include "player.h"

namespace devilution {
namespace WarriorMarks {

// =====================================================================
// Iron Bastion — Guardian
// Slot1: A=decay 2%/s after 10s, B=cap +25%
// Slot2: A=release +30% dmg, B=2-tile splash (50%)
// Slot3: A=fatal→survive 1HP (120s cd), B=full charge→5-tile AoE shockwave
// =====================================================================

static bool HasIronBastion(const Player &player) {
	return HasActiveMark(player, MasterMarkId::IronBastion);
}

static int GetBastionCap(const Player &player) {
	int cap = player._pMaxHP;
	if (GetMarkSlotChoice(player, MasterMarkId::IronBastion, 0) == 2)
		cap = cap * 125 / 100;
	return cap;
}

static int GetBastionDecayRate(const Player &player) {
	return (GetMarkSlotChoice(player, MasterMarkId::IronBastion, 0) == 1) ? 2 : 5;
}

static void IronBastionOnBlock(Player &player, int blockedDamage) {
	if (!HasIronBastion(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::IronBastion);
	int cap = GetBastionCap(player);
	state.data0 = std::min(state.data0 + blockedDamage, cap);
}

static void IronBastionOnAttack(Player &player, int &damage) {
	if (!HasIronBastion(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::IronBastion);
	int charge = state.data0;
	if (charge <= 0) return;
	int slot2 = GetMarkSlotChoice(player, MasterMarkId::IronBastion, 1);
	if (slot2 == 1) damage += charge * 130 / 100;
	else damage += charge;
	state.data0 = 0;
}

static bool IronBastionOnFatal(Player &player) {
	if (!HasIronBastion(player)) return false;
	auto &state = GetMarkState(player, MasterMarkId::IronBastion);
	if (GetMarkSlotChoice(player, MasterMarkId::IronBastion, 2) != 1) return false;
	if (state.data0 <= 0 || state.data2 > 0) return false;
	state.data0 = 0;
	player._pHitPoints = 1;
	state.data2 = 120 * 60;
	return true;
}

static void IronBastionPerTick(Player &player) {
	if (!HasIronBastion(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::IronBastion);
	if (state.data2 > 0) state.data2--;

	int decayRate = GetBastionDecayRate(player);
	if (decayRate == 2 && state.data1 < 10 * 60) {
		state.data1++; // 10s timer for Fortress
	} else if (state.data1 >= 10 * 60 || decayRate == 5) {
		bool inCombat = (player._pmode >= PM_ATTACK && player._pmode <= PM_WALK3);
		if (!inCombat)
			state.data0 = std::max(0, state.data0 - (state.data0 * decayRate / 100));
	}
}

// =====================================================================
// Crimson Brand — Berserker
// Slot1: A=attack rage ×2, B=damage taken → rage (×0.5)
// Slot2: A=HP<30% → all effects ×1.5, B=HP<10% → ×3
// Slot3: A=3s death immunity + rage→HP, B=damage→rage first (5s weakness)
// =====================================================================

static bool HasCrimsonBrand(const Player &player) {
	return HasActiveMark(player, MasterMarkId::CrimsonBrand);
}

static void CrimsonBrandOnAttack(Player &player, int &damage) {
	if (!HasCrimsonBrand(player)) return;
	(void)damage;
	auto &state = GetMarkState(player, MasterMarkId::CrimsonBrand);
	int gen = 20;
	if (GetMarkSlotChoice(player, MasterMarkId::CrimsonBrand, 0) == 1) gen *= 2;
	state.data0 = std::min(state.data0 + gen, player._pMaxHP);
}

static void CrimsonBrandOnDamaged(Player &player, int damage) {
	if (!HasCrimsonBrand(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::CrimsonBrand);
	if (GetMarkSlotChoice(player, MasterMarkId::CrimsonBrand, 0) == 2)
		state.data0 = std::min(state.data0 + damage / 2, player._pMaxHP);

	if (GetMarkSlotChoice(player, MasterMarkId::CrimsonBrand, 2) == 2) {
		if (state.data0 > 0) {
			int absorbed = std::min(state.data0, damage);
			state.data0 -= absorbed;
			if (state.data0 <= 0)
				state.data2 = 5 * 60;
		}
	}
}

static bool CrimsonBrandOnFatal(Player &player) {
	if (!HasCrimsonBrand(player)) return false;
	auto &state = GetMarkState(player, MasterMarkId::CrimsonBrand);
	if (GetMarkSlotChoice(player, MasterMarkId::CrimsonBrand, 2) != 1) return false;
	if (state.data1 > 0) return false;
	state.data1 = 3 * 60;
	return true;
}

static void CrimsonBrandPerTick(Player &player) {
	if (!HasCrimsonBrand(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::CrimsonBrand);
	if (state.data1 > 0) {
		state.data1--;
		if (state.data1 <= 0) {
			player._pHitPoints = std::min(player._pHitPoints + state.data0, player._pMaxHP);
			state.data0 = 0;
		}
	}
	if (state.data2 > 0) state.data2--;
}

// =====================================================================
// Arms Master — Weapon Master
// Slot1: A=dual 2H (block half, atk×1.3), B=dual 1H alt-strike ×1.5
// Slot2: A=speed 1.5, block penalty gone; B=dual speed 1.8, counter 100%
// Slot3: A=Sweep AoE, B=Duel 6s buff
// =====================================================================

static bool HasArmsMaster(const Player &player) {
	return HasActiveMark(player, MasterMarkId::ArmsMaster);
}

static void ArmsMasterPerTick(Player &player) {
	if (!HasArmsMaster(player)) return;
	auto &state = GetMarkState(player, MasterMarkId::ArmsMaster);
	if (state.data2 > 0) state.data2--;
	if (state.data1 > 0) state.data1--;
}

// === WarriorMarks dispatch ===

void OnPlayerBlock(Player &player, int blockedDamage) {
	IronBastionOnBlock(player, blockedDamage);
}

void OnPlayerAttack(Player &player, int &damage) {
	IronBastionOnAttack(player, damage);
	CrimsonBrandOnAttack(player, damage);
}

void OnPlayerDamaged(Player &player, int &damage) {
	CrimsonBrandOnDamaged(player, damage);
}

bool OnPlayerFatalDamage(Player &player) {
	if (IronBastionOnFatal(player)) return true;
	if (CrimsonBrandOnFatal(player)) return true;
	return false;
}

void CheckEffects(Player &player) {
	IronBastionPerTick(player);
	CrimsonBrandPerTick(player);
	ArmsMasterPerTick(player);
}

} // namespace WarriorMarks
} // namespace devilution
