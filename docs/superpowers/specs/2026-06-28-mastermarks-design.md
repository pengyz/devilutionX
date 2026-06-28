# Master Marks — Design Specification

> Date: 2026-06-28 | Branch: `engine-mod-infra` | Diablo 1 / DevilutionX

---

## 1. Philosophy

Master Marks are **combat styles that reshape how a class plays**, not passive stat bonuses. Three principles:

1. **Item-driven, not skill-tree-driven.** Runes are dungeon loot. Upgrades require returning to town. Choices are permanent — no respec.
2. **Soft equipment guidance, never hard locks.** No mark requires a specific weapon. But specific weapons gain extra synergy.
3. **Single-player first.** Every mark must enable completing Hell difficulty independently. No "support" marks.

---

## 2. Upgrade System: Three Slots, Nonlinear

Each mark has **3 rune slots**, unlocked in any order by socketing progressively rarer runes found in the dungeon.

| Slot | Rune Rarity | Purpose |
|------|------------|---------|
| Slot 1 | Common (white) | Reinforcement — direction tweak |
| Slot 2 | Rare (blue) | Specialization — distinct path |
| Slot 3 | Legendary (gold) | Transformation — game-changing ultimate |

- **No level requirement.** Slot 3 can be socketed before Slot 1.
- **Permanent once socketed.** No respec. Diablo 1 has no respec.
- **Each slot is a binary choice (A or B).** The player picks one path per slot and lives with it.
- **Slots are independent.** No slot's effect depends on another slot being filled first.

### Rune Acquisition

- **Common (white)**: Unique monster kills, chests (high drop rate), Griswold/Adria sells for gold.
- **Rare (blue)**: Elite/champion drops, hidden rooms, quest rewards, high gold cost from NPCs.
- **Legendary (gold)**: Act bosses, specific dungeon challenges (e.g., kill Diablo on Nightmare+), hidden room in Hell levels only.

### NPC Rune Socketing

Each class's mentor sockets runes:
- **Warrior** → Griswold
- **Sorcerer** → Adria
- **Rogue** → Gillian

Gold cost scales with rune rarity. The NPC dialogue acknowledges the mark being upgraded.

---

## 3. The Nine Marks

### ⚔️ Warrior — Mentor: Griswold

---

#### 3.1 Iron Bastion (Guardian)

> *Blocking is not defense. It's building pressure.*

**Core (Brand)**: Successful blocks generate **Bastion Charge** (stores blocked damage, cap = max HP). Next melee attack releases all charge as bonus damage. Out of combat, charge decays at 5%/s.

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Charge Retention | **Fortress**: Charge decay reduced to 2%/s, begins only after 10s out of combat | **Overflow**: Charge cap +25% (125% max HP) |
| **2 (Blue)** — Release Mode | **Heavy Blow**: Release damage +30%, knocks enemy back 2 tiles | **Splash**: Release hits enemies within 2 tiles of target (50% damage) |
| **3 (Gold)** — Last Stand | **Unbroken**: Fatal damage consumes all charge to survive at 1 HP (120s cooldown) | **Avalanche**: At full charge, manually release a 5-tile AoE shockwave dealing charge value as damage, consumes all charge |

**Soft guidance**: Shield = +50% charge generation speed. One-handers release more often. Two-handers store more per block but block less frequently.

---

#### 3.2 Crimson Brand (Berserker)

> *Your blood is not a resource to hoard. It's a currency to spend.*

**Core (Brand)**: Attacks build **Blood Rage gauge** (cap = max HP). Spend rage for equal amounts of lifesteal, attack speed, and damage bonus. Lower HP → faster rage generation. Rage also acts as a secondary HP bar: incoming damage consumes rage before health.

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Rage Building | **Bloodlust**: Attack-generated rage ×2 | **Adversity**: Taking damage generates rage (damage taken ×0.5) |
| **2 (Blue)** — Low-HP Scaling | **Desperation**: HP < 30% → rage lifesteal/attack/damage ×1.5 | **Bloodthirst**: HP < 10% → all rage effects ×3 |
| **3 (Gold)** — Ultimate Survival | **Undying**: 3s death immunity + rage gen ×2, ends by converting all rage to HP | **Blood Shield**: Damage consumes rage before HP. When rage depletes: 5s weakness (-30% damage/movement) |

**Soft guidance**: Fast weapons build rage faster per hit but hit lighter. Two-handers gain rage more slowly but get more damage per rage point spent.

---

#### 3.3 Arms Master (Weapon Master)

> *A weapon is not a tool. It's an extension of intent.*

This is the warrior's only **numerical-direction** mark, but it unlocks **new weapon stances** rather than flat stat boosts.

**Core (Brand)**: Unlocks two mutually exclusive combat stances.

| Slot | A: Heavy Grip | B: Swift Hands |
|------|---------------|----------------|
| **1 (White)** — Stance Unlock | **Dual two-handed weapons**. Two-handers can now block (block chance halved). Attack speed ×1.3. | **Dual one-handed weapons OR one-handed + shield**. Dual: weapons alternate strikes at avg speed ×1.5, each weapon's on-hit effects trigger independently. Sword+Board: shield bash damage based on weapon damage, blocks knock enemy back 1 tile and trigger weapon counter-strike (50% damage). |
| **2 (Blue)** — Stance Enhancement | Attack speed bonus 1.3→1.5. Block chance penalty removed. | Dual wield speed ×1.5→×1.8. Shield counter-strike damage 50%→100%. |
| **3 (Gold)** — Ultimate Technique | **Sweep**: Swing both weapons simultaneously in a forward fan AoE. Damage = sum of both weapons. If only one target is hit, damage ×1.5. | **Duel**: Activate for 6s. One-handers: +30% crit +30% attack speed. Shield: block chance doubled, every block interrupts enemy actions. 45s cooldown. |

**Soft guidance**: Heavy Grip rewards finding two strong two-handers. Swift Hands rewards collecting diverse one-handers with complementary on-hit effects.

---

### 🔥 Sorcerer — Mentor: Adria

---

#### 3.4 Sanguimancer

> *Mana is an illusion. Blood is the true source.*

**Core (Brand)**: When out of mana, automatically pay HP to cast (1 mana = 2 HP). Killing an enemy restores 10% of that enemy's max HP. HP < 20% → conversion improves to 1:1.

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Recovery | **Blood Tide**: Kill restore increased to 15% max HP | **Touch of Curse**: Blood-paid kills curse enemies within 3 tiles (-15% damage dealt, 3s) |
| **2 (Blue)** — Efficiency | **Sacrifice**: HP < 30% → conversion permanently 1:1 | **Ward**: Blood-paid casts grant temp HP = 50% of HP spent (cap 30% max HP) |
| **3 (Gold)** — Ascension | **Blood God**: HP < 20% → conversion 1:1, spell damage +40%, kill restore 20% max HP | **Immortal**: If temp HP from Ward is active, fatal damage instead consumes all temp HP and survives at 1 HP |

---

#### 3.5 Spellblade

> *The staff is not a walking stick. It's a weapon.*

**Core (Brand)**: Mana Shield becomes a **toggle** (no continuous mana drain; instead, each hit consumes mana equal to absorbed damage). Gain "Enchant Weapon" ability: your current weapon deals bonus elemental damage (fire/electric alternating) based on weapon damage. Weapon damage scales with Intelligence. Strength requirements for weapons are ignored.

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Shield Fortification | **Absorption**: Shield absorb rate 33%→50% | **Reflect**: While shield is active, 20% of absorbed damage rebounds to attacker |
| **2 (Blue)** — Enchant Enhancement | **Elemental Storm**: Enchanted attacks have 10% chance to trigger a 3-tile elemental AoE | **Venom**: Enchant also applies poison DoT (15% weapon dmg/s, 3s, stacks 3×) |
| **3 (Gold)** — Mind Over Body | **Mind Over Might**: Weapon damage = INT × 0.8. All STR requirements waived. | **Dual Discipline**: Per point of INT: +0.5% attack speed. Per point of STR: +3% enchant elemental damage. |

---

#### 3.6 Overcharge

> *Burn bright. Then burn out.*

**Core (Brand)**: Activate "Mana Overcharge": cast speed doubled, zero mana cost, lasts 8s. Ends with 10s of weakness (-50% damage, -30% movement, max mana reduced by 30%). 60s cooldown.

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Overcharge Boost | **Amplify**: During overcharge, spell damage +30% | **Overload**: Overcharge duration +4s |
| **2 (Blue)** — Penalty Mitigation | **Resilience**: Weakness duration 10s→5s | **Temperance**: Max mana reduction 30%→15%, movement speed penalty removed |
| **3 (Gold)** — Ultimate Release | **Annihilation**: Overcharge ends with an explosion (damage = 20% of total damage dealt during overcharge, 4-tile AoE) | **Echo**: 5s after weakness ends: auto-refill mana to full, all cooldowns halved for 3s |

---

### 🗡️ Rogue — Mentor: Gillian

---

#### 3.7 Marksman

> *Distance is armor. Patience is the deadliest arrow.*

**Core (Brand)**: Bow/crossbow damage +20%. After standing still for 1s, enter Aim state (movement does not break it): ranged damage increases with distance (3% per tile, up to 30%).

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Firing Mode | **Multishot**: Fire 3 arrows simultaneously in a fan pattern | **Pierce**: Arrows have 50% chance to pierce through the target |
| **2 (Blue)** — Damage Scaling | **Longbow**: Distance bonus 3%→5% per tile (cap 50%) | **Deadly Focus**: While aiming, crit chance +25% |
| **3 (Gold)** — Ultimate Shot | **Arrow Rain**: Rain 10 arrows on a 5-tile target area (30s cooldown) | **Heartseeker**: After 3s of uninterrupted aim, next shot is guaranteed critical hit with ×3 damage |

---

#### 3.8 Shadowstep

> *They swing at air. You're already behind them.*

**Core (Brand)**: On successful dodge, instantly teleport behind the attacker. Next attack is 100% critical. 4s cooldown. Can be manually triggered even without being attacked (8s cooldown).

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Evasion | **Ghost**: Base dodge chance +15% | **Afterimage**: Teleport leaves a 2s decoy at original position that draws enemy attacks |
| **2 (Blue)** — Backstab | **Assassinate**: Shadowstep crit damage 100%→200%. Kill resets cooldown. | **Venom Blade**: Shadowstep crit applies poison DoT (20% weapon dmg/s, 3s) |
| **3 (Gold)** — Ultimate Mobility | **Shadow Dance**: Can Shadowstep twice within a 4s window | **Abyssal Step**: Shadowstep range +4 tiles, can pass through walls |

---

#### 3.9 Precision

> *Eyes open. Hands steady. Everything is a target.*

This is the rogue's only **numerical-direction** mark, providing hybrid melee/range bonuses without altering core mechanics.

**Core (Brand)**: Melee weapons gain +15% crit chance (ranged gets half: +7.5%). Ranged weapons gain +20% hit chance (melee gets half: +10%).

| Slot | A Path | B Path |
|------|--------|--------|
| **1 (White)** — Attack Refinement | Melee crit damage +30%, ranged hit chance +10% | Ranged attack speed +10%, melee hit chance +10% |
| **2 (Blue)** — Crit Synergy | **Weak Spot**: Critical hits mark the target, all subsequent damage to it +15% (4s) | **Rhythm**: After a critical hit, attack speed +20% for 3s |
| **3 (Gold)** — Universal Mastery | **Versatility**: Melee crit chance +10%. Ranged crit chance penalty removed (full 15%). | **Execution**: Ranged crits fire an extra arrow (50% damage). Melee crits restore HP equal to 8% of damage dealt. |

---

## 4. Non-Functional Requirements

### 4.1 Single-Player First
Every mark must function independently. No mark requires teammates to achieve its core effect. The game is Diablo 1, a single-player game first.

### 4.2 Item-Driven
Runes are items found in the dungeon, not points on a menu. The player discovers marks through exploration, not through a character planner.

### 4.3 Irreversible Choices
Diablo 1 has no respec mechanic. Rune socketing is permanent. This creates weight: the player builds *their* warrior, not *the community's* warrior.

### 4.4 No New UI Paradigms
All interaction happens through existing NPC dialogue. No skill tree panel. No talent screen. Gryswold's "Smith" dialogue, Adria's "Buy/Sell/Recharge" dialogue, Gillian's existing interaction — these are the interfaces.

### 4.5 Dark Gothic Atmosphere
Marks are "branded" onto the character by mentors. Runes are "etched." The vocabulary avoids gamey terms ("talent points," "skill trees," "builds") and leans into the horror-fantasy tone of Diablo 1.

---

## 5. Integration Points

| System | Integration |
|--------|------------|
| `mastermark.h/cpp` | Mark enum (9 marks), slot data structures, rune socketing logic |
| `player.h/cpp` | `activeMarks[2]`, `markSlots[2][3]` per mark, combat hooks |
| `towners.cpp` | Mentor dialogue: mark granting, rune socketing, slot display |
| `monster.cpp` | On-kill hooks for marks that trigger on monster death |
| `missiles.cpp` | Spell cast hooks for marks that modify spells (Overcharge, Sanguimancer) |
| `items.cpp` | Rune item definitions, enchant weapon hook (Spellblade), weapon stat overrides |
| `loadsave.cpp` | Serialization of marks, slots, and rune choices |

---

## 6. Implementation Phases (Referencing engine-mod-infra Plan)

| Phase | What | Status |
|-------|------|--------|
| P0.2 | Inventory 40→60 slots | Not started |
| P1.2 | Expert mode | Not started |
| P2.2 | Master Marks — definition tables, mark management, rune socketing, all 9 mark core effects + 3 slots each | Tables done, effects and slots not started |
| P2.3 | Monster affixes — already complete, no mark changes needed | Complete |
| P2.4+ | Later phases to be specified | Not started |

---

## 7. Open Questions

- **Hellfire classes (Monk, Bard, Barbarian)**: Deferred. Design will follow same principles when these classes are addressed.
- **Rune item icons**: Requires new art assets.
- **Multiplayer balance**: Out of scope for initial implementation. Marks are SP-only initially, MP considerations deferred.

---

*End of specification.*
