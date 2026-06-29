#include "mastermark.h"

#include <algorithm>

#include "player.h"

namespace devilution {

// === Mark Definitions (9 marks) ===
// clang-format off
const MasterMarkDef markDefs[MarkCount] = {
	// ===== Warrior =====
	{
	    "Iron Bastion",
	    "Blocking stores damage. Release it in a single blow.",
	    "Block stores dmg (cap=100% MaxHP). Release->next blow +130%.",
	    { "Fortress: 2%/s decay after 10s idle", "Retribution: release +30% dmg", "Last Stand: fatal->survive 1HP (120s cd)" },
	    { "Bulwark: cap +25% (125% MaxHP)", "Shockwave: 2-tile splash (50%)", "Earthshatter: 5-tile AoE at full charge" },
	    HeroClass::Warrior
	},
	{
	    "Crimson Brand",
	    "Blood fuels rage. Rage fuels destruction.",
	    "Attacks->Rage (20/s). On fatal: rage->HP.",
	    { "Frenzy: atk rage x2 (40/s)", "Bloodlust: HP<30%->all x1.5", "Undying: 3s death immune + rage->HP" },
	    { "Masochist: dmg taken->half rage", "Deathwish: HP<10%->all x3", "Iron Will: dmg->rage first (5s weak)" },
	    HeroClass::Warrior
	},
	{
	    "Arms Master",
	    "Dual two-handers or blade-and-board — your weapons obey you.",
	    "Dual wield 2H weapons or 1H+shield. Block penalty halved.",
	    { "Titan: dual 2H (atk x1.3)", "Flurry: atk spd x1.5, no block penalty", "Cleave: sweeping AoE attacks" },
	    { "Duelist: alt-strike x1.5", "Tempest: dual spd x1.8, counter 100%", "Focus: Duel mode 6s buff" },
	    HeroClass::Warrior
	},
	// ===== Sorcerer =====
	{
	    "Sanguimancer",
	    "Mana is an illusion. Blood is the true currency.",
	    "Cast->spend HP (1 mana=2 HP). Kill->restore 10% MaxHP.",
	    { "Vampiric: restore 15% on kill", "Desperation: HP<30%->1:1 ratio", "Soul Burn: HP<20%->1:1 +40% dmg +20% heal" },
	    { "Blood Curse: curse nearby on kill", "Blood Ward: cast->temp HP=50% spent", "Last Rite: fatal->consume Ward survive" },
	    HeroClass::Sorcerer
	},
	{
	    "Spellblade",
	    "The staff is not a walking stick. It's a weapon.",
	    "Mana Shield toggle. Weapon Enchant: +30% elem dmg. INT->wpn dmg.",
	    { "Absorb: mana shield 50%", "Arc Slash: 10% 3-tile AoE", "Mindblade: wpn dmg=INT x0.8" },
	    { "Reflect: shield reflects 20%", "Venom: poison DoT 3s, stack x3", "Channel: INT->+0.5% spd, STR->+3% enchant" },
	    HeroClass::Sorcerer
	},
	{
	    "Overcharge",
	    "Burn twice as bright. Accept the aftermath.",
	    "Activate: 8s x2 cast spd, 0 mana. Then 10s weak (-50% dmg).",
	    { "Combustion: +30% spell dmg during", "Recovery: weakness 5s", "Detonate: end->explode 20% total" },
	    { "Endurance: +4s burst (12s)", "Stability: mana-15% (no speed loss)", "Renewal: after weak->full mana, 3s cd" },
	    HeroClass::Sorcerer
	},
	// ===== Rogue =====
	{
	    "Marksman",
	    "Distance is armor. Patience is the deadliest arrow.",
	    "Bow +20% dmg. Aim 1s->dmg scales with dist (3%/tile, cap 30%).",
	    { "Multishot: 3 arrows", "Longbow: 5%/tile (cap 50%)", "Arrow Rain: 10 arrows (30s cd)" },
	    { "Pierce: 50% pierce", "Focus: +25% crit while aiming", "Heartseeker: 3s->x3 crit" },
	    HeroClass::Rogue
	},
	{
	    "Shadowstep",
	    "They swing at air. You are already behind them.",
	    "Dodge->teleport behind, next atk crit (4s cd). Manual: teleport (8s cd).",
	    { "Evasion: +15% dodge", "Execute: crit 200% + kill=reset cd", "Chain: 2 teleports in 4s" },
	    { "Decoy: leave afterimage 2s", "Venom: poison DoT on crit", "Shadow: +4 tile range, wall bypass" },
	    HeroClass::Rogue
	},
	{
	    "Precision",
	    "Eyes open. Hands steady. Everything is a target.",
	    "+15% melee crit (+7.5% ranged). +20% ranged hit (+10% melee).",
	    { "Marksman: melee crit+30%, ranged hit+10%", "Weak Spot: crit->+15% dmg mark", "Versatility: ranged crit=15%" },
	    { "Deadeye: ranged spd+10%, melee hit+10%", "Rhythm: crit->+20% atk spd 3s", "Execution: ranged->extra arrow, melee->8% lifesteal" },
	    HeroClass::Rogue
	},
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
bool MarkOnPlayerDamaged(Player &p, int &d)         { WarriorMarks::OnPlayerDamaged(p, d); return false; }
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
