#include "mastermark.h"

#include <algorithm>
#include <cstdint>

#include "player.h"

namespace devilution {

// clang-format off
const MasterMarkDef markDefs[static_cast<size_t>(MasterMarkId::COUNT)] = {
	// Warrior — 4 mentor ordeals + 4 dungeon
	{ "Shield Master",  "A shield is not a wall. It is a weapon.",                             HeroClass::Warrior,   MasterMarkSource::MentorOrdeal, 0 },
	{ "Berserker",      "When death is closest, rage burns brightest.",                        HeroClass::Warrior,   MasterMarkSource::MentorOrdeal, 0 },
	{ "Commander",      "One falls. The rest remember fear.",                                  HeroClass::Warrior,   MasterMarkSource::MentorOrdeal, 0 },
	{ "Avenger",        "Every blow against you is a debt.",                                   HeroClass::Warrior,   MasterMarkSource::MentorOrdeal, 0 },
	{ "Juggernaut",     "While moving toward an enemy, next attack ignores 50% armor.",        HeroClass::Warrior,   MasterMarkSource::Dungeon,      0 },
	{ "Sentinel",       "Standing still for 1.5s grants +30% block chance until movement.",    HeroClass::Warrior,   MasterMarkSource::Dungeon,      0 },
	{ "Executioner",    "Attacks against stunned or feared enemies deal double damage.",       HeroClass::Warrior,   MasterMarkSource::Dungeon,      0 },
	{ "Iron Will",      "Cannot be knocked back. Bleed/poison duration halved.",               HeroClass::Warrior,   MasterMarkSource::Dungeon,      0 },

	// Sorcerer — 4 mentor ordeals + 4 dungeon
	{ "Arcanist",       "The shield drinks. What it drinks, it gives back.",                   HeroClass::Sorcerer,  MasterMarkSource::MentorOrdeal, 0 },
	{ "Pyromancer",     "Fire does not strike and fade. Fire follows.",                        HeroClass::Sorcerer,  MasterMarkSource::MentorOrdeal, 0 },
	{ "Stormcaller",    "The lightning does not wander. It chooses.",                          HeroClass::Sorcerer,  MasterMarkSource::MentorOrdeal, 0 },
	{ "Sanguimancer",   "Life is mana. Mana is life. The boundary is illusion.",               HeroClass::Sorcerer,  MasterMarkSource::MentorOrdeal, 0 },
	{ "Frostborn",      "Cold attacks slow enemies. Slow stacks up to 3 times.",               HeroClass::Sorcerer,  MasterMarkSource::Dungeon,      0 },
	{ "Ley Weaver",     "Standing still for 2s makes next spell cost 50% less mana.",         HeroClass::Sorcerer,  MasterMarkSource::Dungeon,      0 },
	{ "Echo Mage",      "Killing an enemy with a spell auto-recasts at 40% power.",           HeroClass::Sorcerer,  MasterMarkSource::Dungeon,      0 },
	{ "Voidcaller",     "Mana below 20% grants +30% spell damage but no regeneration.",        HeroClass::Sorcerer,  MasterMarkSource::Dungeon,      0 },

	// Rogue — 4 mentor ordeals + 4 dungeon
	{ "Deadeye",        "Patience is the deadliest arrow. Stand still for 2s for a guaranteed crit.", HeroClass::Rogue, MasterMarkSource::MentorOrdeal, 0 },
	{ "Trapsmith",      "The ground beneath them is your weapon. Enhanced trap types.",       HeroClass::Rogue,     MasterMarkSource::MentorOrdeal, 0 },
	{ "Shadowstep",     "They swing at air. You are already behind them.",                    HeroClass::Rogue,     MasterMarkSource::MentorOrdeal, 0 },
	{ "Ricochet",       "One arrow. Three corpses. Arrows bounce to nearest enemy.",          HeroClass::Rogue,     MasterMarkSource::MentorOrdeal, 0 },
	{ "Predator",       "Attacking from behind deals +40% damage and causes bleed.",          HeroClass::Rogue,     MasterMarkSource::Dungeon,      0 },
	{ "Wind Walker",    "Dodging grants +20% movement speed for 2 seconds.",                  HeroClass::Rogue,     MasterMarkSource::Dungeon,      0 },
	{ "Venomancer",     "Critical hits apply poison. Poisoned enemies take +15% trap damage.", HeroClass::Rogue,    MasterMarkSource::Dungeon,      0 },
	{ "Ghost",          "Stand still for 3s to become invisible until you move or attack.",   HeroClass::Rogue,     MasterMarkSource::Dungeon,      0 },

	// Monk — 4 mentor ordeals + 4 dungeon
	{ "Iron Palm",      "Unarmed combo strikes: 3rd hit is guaranteed crit + knockback.",     HeroClass::Monk,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Serenity",       "3s without damage makes next ability free.",                         HeroClass::Monk,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Chi Wave",       "Killing an enemy releases a healing wave to you and allies.",        HeroClass::Monk,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Earth Stance",   "Cannot be knocked back. Standing still grants +50% armor.",           HeroClass::Monk,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Flowing Water",  "Dodging doubles your next attack speed.",                            HeroClass::Monk,      MasterMarkSource::Dungeon,      0 },
	{ "Inner Fire",     "Mana depleted grants +40% unarmed damage.",                          HeroClass::Monk,      MasterMarkSource::Dungeon,      0 },
	{ "Karma",          "Stored damage taken is released on next attack +50%.",               HeroClass::Monk,      MasterMarkSource::Dungeon,      0 },
	{ "Transcendence",  "Death spawns you with Chi Wave already active.",                     HeroClass::Monk,      MasterMarkSource::Dungeon,      0 },

	// Bard — 4 mentor ordeals + 4 dungeon
	{ "Warsong",        "Every 4th hit makes nearby enemies take +20% damage for 3s.",        HeroClass::Bard,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Lament",         "Killing an enemy slows nearby enemies by 40% for 2s.",                HeroClass::Bard,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Echosong",       "Spells echo at 30% power toward nearest enemy after 2s.",            HeroClass::Bard,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Hymn of Respite","Standing still regenerates HP for nearby allies.",                   HeroClass::Bard,      MasterMarkSource::MentorOrdeal, 0 },
	{ "Crescendo",      "Consecutive hits on same target: +10% damage per hit (max +50%).",  HeroClass::Bard,      MasterMarkSource::Dungeon,      0 },
	{ "Dissonance",     "Switching targets within 1s stuns the new target for 1s.",           HeroClass::Bard,      MasterMarkSource::Dungeon,      0 },
	{ "Coda",           "Enemy below 20% HP: next attack deals triple damage (30s cd).",      HeroClass::Bard,      MasterMarkSource::Dungeon,      0 },
	{ "Overture",       "First attack in combat fears nearby enemies for 1s.",                 HeroClass::Bard,      MasterMarkSource::Dungeon,      0 },

	// Barbarian — 4 mentor ordeals + 4 dungeon
	{ "Blood Rage",     "Each kill grants +5% damage, stacks up to 5 times (5s duration).",   HeroClass::Barbarian, MasterMarkSource::MentorOrdeal, 0 },
	{ "Unchained",      "Cannot be stunned/frozen/feared. CC instead increases damage +20%.", HeroClass::Barbarian, MasterMarkSource::MentorOrdeal, 0 },
	{ "Sunder",         "Attacks ignore 30% enemy armor. +10% per consecutive hit (max 60%).", HeroClass::Barbarian, MasterMarkSource::MentorOrdeal, 0 },
	{ "Warcry",         "Activate to taunt all enemies in 4 tiles for 3s. -25% damage taken.", HeroClass::Barbarian, MasterMarkSource::MentorOrdeal, 0 },
	{ "Thick Skin",     "Damage taken reduced by 1 per nearby enemy (max 8).",                HeroClass::Barbarian, MasterMarkSource::Dungeon,      0 },
	{ "Momentum",       "Moving 2s continuously makes next attack double damage + knockback.", HeroClass::Barbarian, MasterMarkSource::Dungeon,      0 },
	{ "Last Stand",     "HP below 15%: +40% attack speed and death immunity for 3s.",         HeroClass::Barbarian, MasterMarkSource::Dungeon,      0 },
	{ "Trophy Hunter",  "Killing a unique monster grants +15% damage and speed for 60s.",     HeroClass::Barbarian, MasterMarkSource::Dungeon,      0 },
};
// clang-format on

void GrantMark(Player &player, MasterMarkId id)
{
	player.ownedMarks.set(static_cast<size_t>(id));
}

bool HasMark(const Player &player, MasterMarkId id)
{
	return player.ownedMarks.test(static_cast<size_t>(id));
}

bool HasActiveMark(const Player &player, MasterMarkId id)
{
	return (player.activeMarks[0] == id) || (player.activeMarks[1] == id);
}

void ActivateMark(Player &player, MasterMarkId id, int slot)
{
	if (slot >= 0 && slot < 2) {
		player.activeMarks[slot] = id;
	}
}

void DeactivateMark(Player &player, int slot)
{
	if (slot >= 0 && slot < 2) {
		player.activeMarks[slot] = MasterMarkId::COUNT;
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

int GetShieldArmor(const Player &player)
{
	int ac = 0;
	if (player.InvBody[INVLOC_HAND_LEFT]._itype == ItemType::Shield)
		ac += player.InvBody[INVLOC_HAND_LEFT]._iAC;
	if (player.InvBody[INVLOC_HAND_RIGHT]._itype == ItemType::Shield)
		ac += player.InvBody[INVLOC_HAND_RIGHT]._iAC;
	return ac;
}

void CheckMarkEffects(Player &player)
{
	(void)player;
}

} // namespace devilution
