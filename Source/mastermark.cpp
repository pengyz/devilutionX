#include "mastermark.h"

#include <algorithm>

#include "player.h"

namespace devilution {

// === Mark Definitions (9 marks) ===
// clang-format off
const MasterMarkDef markDefs[MarkCount] = {
	// ===== Warrior =====
	{
	    N_("Iron Bastion"),
	    N_("Blocking stores damage. Release it in a single blow."),
	    N_("Block stores dmg (cap=100% MaxHP). Release->next blow +130%."),
	    { N_("Fortress: 2%/s decay after 10s idle"), N_("Retribution: release +30% dmg"), N_("Last Stand: fatal->survive 1HP (120s cd)") },
	    { N_("Bulwark: cap +25% (125% MaxHP)"), N_("Shockwave: 2-tile splash (50%)"), N_("Earthshatter: 5-tile AoE at full charge") },
	    HeroClass::Warrior
	},
	{
	    N_("Crimson Brand"),
	    N_("Blood fuels rage. Rage fuels destruction."),
	    N_("Attacks->Rage (20/s). On fatal: rage->HP."),
	    { N_("Frenzy: atk rage x2 (40/s)"), N_("Bloodlust: HP<30%->all x1.5"), N_("Undying: 3s death immune + rage->HP") },
	    { N_("Masochist: dmg taken->half rage"), N_("Deathwish: HP<10%->all x3"), N_("Iron Will: dmg->rage first (5s weak)") },
	    HeroClass::Warrior
	},
	{
	    N_("Arms Master"),
	    N_("Dual two-handers or blade-and-board — your weapons obey you."),
	    N_("Dual wield 2H weapons or 1H+shield. Block penalty halved."),
	    { N_("Titan: dual 2H (atk x1.3)"), N_("Flurry: atk spd x1.5, no block penalty"), N_("Cleave: sweeping AoE attacks") },
	    { N_("Duelist: alt-strike x1.5"), N_("Tempest: dual spd x1.8, counter 100%"), N_("Focus: Duel mode 6s buff") },
	    HeroClass::Warrior
	},
	// ===== Sorcerer =====
	{
	    N_("Sanguimancer"),
	    N_("Mana is an illusion. Blood is the true currency."),
	    N_("Cast->spend HP (1 mana=2 HP). Kill->restore 10% MaxHP."),
	    { N_("Vampiric: restore 15% on kill"), N_("Desperation: HP<30%->1:1 ratio"), N_("Soul Burn: HP<20%->1:1 +40% dmg +20% heal") },
	    { N_("Blood Curse: curse nearby on kill"), N_("Blood Ward: cast->temp HP=50% spent"), N_("Last Rite: fatal->consume Ward survive") },
	    HeroClass::Sorcerer
	},
	{
	    N_("Spellblade"),
	    N_("The staff is not a walking stick. It's a weapon."),
	    N_("Mana Shield toggle. Weapon Enchant: +30% elem dmg. INT->wpn dmg."),
	    { N_("Absorb: mana shield 50%"), N_("Arc Slash: 10% 3-tile AoE"), N_("Mindblade: wpn dmg=INT x0.8") },
	    { N_("Reflect: shield reflects 20%"), N_("Venom: poison DoT 3s, stack x3"), N_("Channel: INT->+0.5% spd, STR->+3% enchant") },
	    HeroClass::Sorcerer
	},
	{
	    N_("Overcharge"),
	    N_("Burn twice as bright. Accept the aftermath."),
	    N_("Activate: 8s x2 cast spd, 0 mana. Then 10s weak (-50% dmg)."),
	    { N_("Combustion: +30% spell dmg during"), N_("Recovery: weakness 5s"), N_("Detonate: end->explode 20% total") },
	    { N_("Endurance: +4s burst (12s)"), N_("Stability: mana-15% (no speed loss)"), N_("Renewal: after weak->full mana, 3s cd") },
	    HeroClass::Sorcerer
	},
	// ===== Rogue =====
	{
	    N_("Marksman"),
	    N_("Distance is armor. Patience is the deadliest arrow."),
	    N_("Bow +20% dmg. Aim 1s->dmg scales with dist (3%/tile, cap 30%)."),
	    { N_("Multishot: 3 arrows"), N_("Longbow: 5%/tile (cap 50%)"), N_("Arrow Rain: 10 arrows (30s cd)") },
	    { N_("Pierce: 50% pierce"), N_("Focus: +25% crit while aiming"), N_("Heartseeker: 3s->x3 crit") },
	    HeroClass::Rogue
	},
	{
	    N_("Shadowstep"),
	    N_("They swing at air. You are already behind them."),
	    N_("Dodge->teleport behind, next atk crit (4s cd). Manual: teleport (8s cd)."),
	    { N_("Evasion: +15% dodge"), N_("Execute: crit 200% + kill=reset cd"), N_("Chain: 2 teleports in 4s") },
	    { N_("Decoy: leave afterimage 2s"), N_("Venom: poison DoT on crit"), N_("Shadow: +4 tile range, wall bypass") },
	    HeroClass::Rogue
	},
	{
	    N_("Precision"),
	    N_("Eyes open. Hands steady. Everything is a target."),
	    N_("+15% melee crit (+7.5% ranged). +20% ranged hit (+10% melee)."),
	    { N_("Marksman: melee crit+30%, ranged hit+10%"), N_("Weak Spot: crit->+15% dmg mark"), N_("Versatility: ranged crit=15%") },
	    { N_("Deadeye: ranged spd+10%, melee hit+10%"), N_("Rhythm: crit->+20% atk spd 3s"), N_("Execution: ranged->extra arrow, melee->8% lifesteal") },
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
