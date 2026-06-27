#pragma once

#include <cstdint>
#include <string_view>

#include "engine/point.hpp"
#include "levels/gendung.h"

namespace devilution {

struct Monster;
struct Player;

enum class AffixId : uint16_t {
	Berserker  = 1 << 0,
	Phasing    = 1 << 1,
	Aura       = 1 << 2,
	Vampiric   = 1 << 3,
	Cursed     = 1 << 4,
	Phantasm   = 1 << 5,
	Multishot  = 1 << 6,
	Stoneskin  = 1 << 7,
	SoulLeech  = 1 << 8,
	Explosive  = 1 << 9,
};

enum class MonsterAffixTier : uint8_t {
	Normal,
	Elite,
	Champion,
};

struct AffixDef {
	std::string_view name;
	uint16_t bit;
};

extern const AffixDef affixTable[10];

void RollMonsterAffix(Monster &monster, _difficulty difficulty);
void RestoreMonsterAffixes(Monster &monster, _difficulty difficulty);
void ProcessMonsterAffixes(Monster &monster);
void OnAffixMonsterAttack(Monster &monster, Player &player, int damage);
void OnAffixMonsterDamaged(Monster &monster, int damage);
void OnAffixMonsterDeath(Monster &monster);
void OnAffixMonsterRangedHit(Monster &monster, Point attackerPosition);

} // namespace devilution
