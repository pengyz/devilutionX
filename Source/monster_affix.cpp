#include "monster_affix.h"

#include <algorithm>
#include <cstdint>

#include "buff.h"
#include "engine/point.hpp"
#include "engine/random.hpp"
#include "missiles.h"
#include "monster.h"
#include "player.h"
#include "tables/misdat.h"

namespace devilution {

const AffixDef affixTable[10] = {
	{ "Berserker", static_cast<uint16_t>(AffixId::Berserker) },
	{ "Phasing", static_cast<uint16_t>(AffixId::Phasing) },
	{ "Aura", static_cast<uint16_t>(AffixId::Aura) },
	{ "Vampiric", static_cast<uint16_t>(AffixId::Vampiric) },
	{ "Cursed", static_cast<uint16_t>(AffixId::Cursed) },
	{ "Phantasm", static_cast<uint16_t>(AffixId::Phantasm) },
	{ "Multishot", static_cast<uint16_t>(AffixId::Multishot) },
	{ "Stoneskin", static_cast<uint16_t>(AffixId::Stoneskin) },
	{ "SoulLeech", static_cast<uint16_t>(AffixId::SoulLeech) },
	{ "Explosive", static_cast<uint16_t>(AffixId::Explosive) },
};

namespace {

bool HasAffix(const Monster &monster, AffixId id)
{
	return (monster.activeAffixes & static_cast<uint16_t>(id)) != 0;
}

} // namespace

void RestoreMonsterAffixes(Monster &monster, _difficulty difficulty)
{
	if (monster.isUnique() || monster.isPlayerMinion())
		return;

	int eliteChance = 10;
	int championChance = 2;

	if (difficulty == DIFF_NIGHTMARE) {
		eliteChance = 18;
		championChance = 4;
	} else if (difficulty == DIFF_HELL) {
		eliteChance = 25;
		championChance = 6;
	}

	SplitMix32 rng(monster.rndItemSeed);
	const int roll = static_cast<int>(rng.next() % 100);

	if (roll < championChance) {
		monster.affixTier = MonsterAffixTier::Champion;
		int first = static_cast<int>(rng.next() % 10);
		int second = static_cast<int>(rng.next() % 9);
		if (second >= first) ++second;
		monster.activeAffixes = affixTable[first].bit | affixTable[second].bit;
	} else if (roll < championChance + eliteChance) {
		monster.affixTier = MonsterAffixTier::Elite;
		monster.activeAffixes = affixTable[rng.next() % 10].bit;
	}
}

void RollMonsterAffix(Monster &monster, _difficulty difficulty)
{
	if (monster.isUnique() || monster.isPlayerMinion())
		return;

	int eliteChance = 10;
	int championChance = 2;

	if (difficulty == DIFF_NIGHTMARE) {
		eliteChance = 18;
		championChance = 4;
	} else if (difficulty == DIFF_HELL) {
		eliteChance = 25;
		championChance = 6;
	}

	const int roll = GenerateRnd(100);

	if (roll < championChance) {
		monster.affixTier = MonsterAffixTier::Champion;
		int first = GenerateRnd(10);
		int second = GenerateRnd(9);
		if (second >= first) ++second;
		monster.activeAffixes = affixTable[first].bit | affixTable[second].bit;
	} else if (roll < championChance + eliteChance) {
		monster.affixTier = MonsterAffixTier::Elite;
		monster.activeAffixes = affixTable[GenerateRnd(10)].bit;
	}
}

void ProcessMonsterAffixes(Monster &monster)
{
	if (monster.affixTier == MonsterAffixTier::Normal)
		return;

	const uint16_t affixes = monster.activeAffixes;

	if (affixes == 0)
		return;

	if (HasAffix(monster, AffixId::Berserker)) {
		if (monster.hitPoints < monster.maxHitPoints / 2 && monster.hitPoints > 0) {
			monster.buffable.Remove(BuffType::AttackSpeed);
			monster.buffable.Apply(BuffType::AttackSpeed, 30, 2, 0);
		}
	}

	if (HasAffix(monster, AffixId::Aura)) {
		for (size_t i = 0; i < MaxMonsters; i++) {
			Monster &other = Monsters[i];
			if (&other == &monster || other.isInvalid || other.mode == MonsterMode::Death)
				continue;
			if (other.position.tile.WalkingDistance(monster.position.tile) <= 5) {
				other.buffable.Remove(BuffType::DamageBoost);
				other.buffable.Apply(BuffType::DamageBoost, 15, 2, 0);
			}
		}
	}

	if (monster.affixTimers[0] > 0)
		monster.affixTimers[0]--;
	if (monster.affixTimers[1] > 0)
		monster.affixTimers[1]--;
}

void OnAffixMonsterAttack(Monster &monster, Player &player, int damage)
{
	if (monster.affixTier == MonsterAffixTier::Normal)
		return;

	if (HasAffix(monster, AffixId::Vampiric)) {
		const int heal = damage * 30 / 100;
		monster.hitPoints = std::min(monster.hitPoints + heal, monster.maxHitPoints);
	}

	if (HasAffix(monster, AffixId::Cursed)) {
		if (&player == MyPlayer) {
			switch (GenerateRnd(4)) {
			case 0:
				ModifyPlrStr(player, -1);
				break;
			case 1:
				ModifyPlrMag(player, -1);
				break;
			case 2:
				ModifyPlrDex(player, -1);
				break;
			case 3:
				ModifyPlrVit(player, -1);
				break;
			}
		}
	}

	if (HasAffix(monster, AffixId::SoulLeech)) {
		if (&player == MyPlayer) {
			player._pMana = std::max(0, player._pMana - 10);
			player._pManaBase = std::max(0, player._pManaBase - 10);
		}
	}
}

void OnAffixMonsterDamaged(Monster &monster, int damage)
{
	if (monster.affixTier == MonsterAffixTier::Normal)
		return;

	if (HasAffix(monster, AffixId::Stoneskin) && monster.affixTimers[0] == 0) {
		monster.buffable.Apply(BuffType::Invulnerable, 1, 60, 0);
		monster.affixTimers[0] = 200;
	}

	if (HasAffix(monster, AffixId::Phantasm) && (monster.affixFlags & 1) == 0) {
		if (monster.hitPoints < monster.maxHitPoints * 30 / 100 && monster.hitPoints > 0) {
			Point spawnPos = monster.position.tile;
			for (int d = 0; d < 8; d++) {
				Point pos = monster.position.tile + static_cast<Direction>(d);
				if (IsTileAvailable(monster, pos)) {
					spawnPos = pos;
					break;
				}
			}
			if (spawnPos != monster.position.tile) {
				Monster *copy = AddMonster(spawnPos, monster.direction, monster.levelType, true);
				if (copy != nullptr) {
					copy->maxHitPoints = monster.maxHitPoints / 2;
					copy->hitPoints = copy->maxHitPoints;
				}
			}
			monster.affixFlags |= 1;
		}
	}
}

void OnAffixMonsterDeath(Monster &monster)
{
	if (!HasAffix(monster, AffixId::Explosive))
		return;

	AddMissile(
	    monster.position.tile,
	    monster.position.tile,
	    Direction::South,
	    MissileID::Fireball,
	    TARGET_BOTH,
	    monster,
	    0,
	    0);
}

void OnAffixMonsterRangedHit(Monster &monster, Point attackerPosition)
{
	if (!HasAffix(monster, AffixId::Phasing))
		return;
	if (monster.affixTimers[1] > 0)
		return;
	if (monster.mode == MonsterMode::Death)
		return;

	for (int d = 0; d < 8; d++) {
		Point pos = attackerPosition + static_cast<Direction>(d);
		if (IsTileAvailable(monster, pos)) {
			M_ClearSquares(monster);
			monster.position.tile = pos;
			monster.position.future = pos;
			monster.position.old = pos;
			monster.occupyTile(pos, false);
			monster.affixTimers[1] = 160;
			return;
		}
	}
}

} // namespace devilution
