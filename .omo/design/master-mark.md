# Master's Mark — Class Specialization System

> Status: Design Complete · Target: engine-mod-infra

## Design Philosophy

D1's character progression is entirely external — you find better equipment, you read better spell books, but your class identity never deepens. A Warrior is a Warrior from level 1 to Diablo. This is the core weakness of D1's build system.

Master's Marks fix this. They are **not talents you click in a menu.** They are **rules you discover in the dungeon.** Each Mark permanently changes how a specific ability or mechanic works — not with numbers, but with behavior. A Sorcerer with Pyromancer is a completely different experience from a Sorcerer with Stormcaller, even with the same equipment.

### Core Rules

- Each class has **8 Marks** total
- **4 Marks** come from the class Mentor in Tristram (quest/ordeal rewards)
- **4 Marks** are found in the dungeon (hidden rooms, rare shrines, specific boss drops)
- A player can **activate 2 Marks at a time**
- Marks can be swapped freely at the Mentor in Tristram
- Marks have **no quality, no levels, no stats, no upgrading**
- You either have a Mark, or you don't

### Design Principle

> A Mark changes what your character *does*, not how much. The best Mark creates a sentence that wasn't true before: "When I block, I counterattack." "My fire spells leave burning ground." "Killing an enemy terrifies its allies."

---

## Mentor System

Each class has a mentor in Tristram. These are existing NPCs — no new characters needed. The mentor gives 4 ordeals. Each ordeal rewards one Mark.

The ordeals are **not quest log entries.** They are conditions the game silently tracks. Return to the mentor after fulfilling the condition to receive the Mark.

```
┌──────────────┬───────────┬────────────────────────────────┐
│    Class     │  Mentor   │  Tristram Role                  │
├──────────────┼───────────┼────────────────────────────────┤
│  Warrior     │ Griswold  │  The Blacksmith                 │
│  Rogue       │ Ogden     │  The Tavern Keeper              │
│  Sorcerer    │ Adria     │  The Witch                      │
│  Monk        │ Pepin     │  The Healer                     │
│  Bard        │ Gillian   │  The Barmaid                    │
│  Barbarian   │ Farnham   │  The Drunk                      │
└──────────────┴───────────┴────────────────────────────────┘
```

---

## Warrior — Marks of the Battlemaster

**Mentor: Griswold the Blacksmith**

Griswold was once a soldier. He sees in you the discipline and fury that makes a true warrior.

```
┌─────────────────────────────────────────────────────────────┐
│ Mark 1 — Shield Master (盾牌专精)                            │
│                                                             │
│  "A shield is not a wall. It is a weapon."                  │
│                                                             │
│  On successful block:                                       │
│    → Auto-deliver a shield bash to the attacker             │
│    → Knockback + brief stun (0.5s)                          │
│    → Bash damage = 30% of shield armor value                │
│                                                             │
│  Mentor's Ordeal 1: "Survive Cathedral level 3 without      │
│  equipping a weapon. Your shield must be your blade."       │
│  (Solo-kill 20 enemies on dlvl 3 while weapon slot is empty)│
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 2 — Berserker (狂战士)                                  │
│                                                             │
│  "When death is closest, rage burns brightest."              │
│                                                             │
│  While HP < 30%:                                            │
│    → Attacks become sweeping (hits all enemies in arc)      │
│    → Each enemy hit restores 3% of maximum HP               │
│                                                             │
│  Mentor's Ordeal 2: "Kill the Butcher. No healing potions." │
│  (Defeat Butcher without consuming a single healing potion) │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 3 — Commander (指挥官)                                  │
│                                                             │
│  "One falls. The rest remember fear."                       │
│                                                             │
│  On killing an enemy:                                       │
│    → Enemies within 2 tiles are Feared for 2 seconds        │
│    → Feared enemies take +15% damage from all sources       │
│                                                             │
│  Mentor's Ordeal 3: "Find the Skeleton King's Crown."       │
│  (Complete the Leoric quest line)                           │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 4 — Avenger (复仇者)                                    │
│                                                             │
│  "Every blow against you is a debt."                        │
│                                                             │
│  On being hit:                                              │
│    → Return 50% of damage to the attacker                   │
│  Passive:                                                   │
│    → Can block elemental projectiles (Fire/Lightning)       │
│                                                             │
│  Mentor's Ordeal 4: "The cursed must avenge themselves.     │
│  Die in the Catacombs, then kill the creature that slew     │
│  you."                                                      │
│  (Return to corpse and kill the monster that killed you)    │
└─────────────────────────────────────────────────────────────┘
```

### Dungeon Marks (Warrior)

```
Mark 5 — Juggernaut (攻城锤)
  While moving toward an enemy → next attack ignores 50% armor

Mark 6 — Sentinel (哨兵)
  Standing still for 1.5s → gain +30% block chance until movement

Mark 7 — Executioner (刽子手)
  Attacks against stunned or feared enemies deal double damage

Mark 8 — Iron Will (钢铁意志)
  Cannot be knocked back. Bleed/poison duration halved.
```

### Notable Warrior Builds

| Build | Marks | Playstyle |
|---|---|---|
| Bulwark | Shield Master + Sentinel | Immovable wall. Block → bash. Stand → block more. |
| Berserker | Berserker + Juggernaut | Low-HP death machine. Sweep → heal → keep sweeping. |
| Warlord | Commander + Executioner | Kill one → fear many → execute the terrified. |
| Blood Debt | Avenger + Berserker | Get hit → retaliate. Low HP → sweep + heal. High risk. |
| Vanguard | Shield Master + Commander | Block stun one, kill another → fear explosion. |

---

## Sorcerer — Marks of the Arcanist

**Mentor: Adria the Witch**

Adria knows that magic is not a tool — it is a living thing that reshapes its wielder.

```
┌─────────────────────────────────────────────────────────────┐
│ Mark 1 — Arcanist (奥术师)                                   │
│                                                             │
│  "The shield drinks. What it drinks, it gives back."        │
│                                                             │
│  Mana Shield:                                               │
│    → Duration doubled                                        │
│    → Absorption increased by 50%                             │
│    → Damage absorbed restores 20% of that amount as mana    │
│                                                             │
│  Mentor's Ordeal 1: "Absorb 1000 damage with Mana Shield    │
│  in a single expedition."                                   │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 2 — Pyromancer (烈焰行者)                               │
│                                                             │
│  "Fire does not strike and fade. Fire follows."             │
│                                                             │
│  After casting any Fire spell:                              │
│    → Leave burning ground at your feet for 3 seconds        │
│    → Burning ground moves with you (not fixed position)     │
│    → Burning ground deals minor damage + ignites enemies    │
│                                                             │
│  Mentor's Ordeal 2: "Kill 50 enemies with fire. The final   │
│  kill must be from burning ground, not direct flame."       │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 3 — Stormcaller (风暴使者)                              │
│                                                             │
│  "The lightning does not wander. It chooses."               │
│                                                             │
│  Lightning spells:                                          │
│    → No longer chain to secondary targets                   │
│    → Instead: strikes the primary target twice in 0.3s      │
│    → Second strike deals +20% damage                        │
│                                                             │
│  Mentor's Ordeal 3: "Strike a single enemy with lightning   │
│  20 times during a single expedition."                      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 4 — Sanguimancer (血法师)                               │
│                                                             │
│  "Life is mana. Mana is life. The boundary is illusion."    │
│                                                             │
│  Healing spell:                                             │
│    → Costs life instead of mana                             │
│    → Base healing doubled                                   │
│  Firebolt:                                                  │
│    → Can be cast with life instead of mana (no penalty)     │
│                                                             │
│  Mentor's Ordeal 4: "Reach 1 HP in combat, then reach       │
│  full HP again without using a potion."                     │
└─────────────────────────────────────────────────────────────┘
```

### Dungeon Marks (Sorcerer)

```
Mark 5 — Frostborn (冰脉)
  Cold attacks slow enemies. Slow stacks up to 3 times (-20/40/60% speed).

Mark 6 — Ley Weaver (地脉编织者)
  Standing still for 2s → next spell costs 50% less mana

Mark 7 — Echo Mage (回响法师)
  After killing an enemy with a spell → automatically recast same spell
  at 40% power toward nearest enemy (once per 8 seconds)

Mark 8 — Voidcaller (虚空召唤者)
  Mana below 20% → all spell damage +30%, but you cannot regenerate
  mana naturally while below threshold
```

### Notable Sorcerer Builds

| Build | Marks | Playstyle |
|---|---|---|
| Immortal | Arcanist + Sanguimancer | Shield absorbs → restores mana. Life casts heals. Perpetual motion. |
| Walking Fire | Pyromancer + Arcanist | Shield up, walk into packs with burning ground. |
| Bolt Sniper | Stormcaller + Ley Weaver | Stand still → cheap lightning → blast single targets. |
| Blood Pyre | Sanguimancer + Pyromancer | Life-fireball everything. Burning ground heals through kills. |
| Frost Storm | Frostborn + Stormcaller | Slow with frost → double-strike with lightning. |

---

## Rogue — Marks of the Huntress

**Mentor: Ogden the Tavern Keeper**

Ogden was an adventurer in his youth. His tavern walls are lined with trophies he never speaks about. For a Rogue, he breaks his silence.

```
┌─────────────────────────────────────────────────────────────┐
│ Mark 1 — Deadeye (狙击手)                                    │
│                                                             │
│  "Patience is the deadliest arrow."                         │
│                                                             │
│  Stand still for 2 seconds:                                 │
│    → Next arrow is a guaranteed critical hit                │
│    → Critical damage bonus increases with time spent still  │
│      (up to +50% after 5 seconds)                            │
│                                                             │
│  Mentor's Ordeal 1: "Land 20 critical hits in a single      │
│  expedition."                                               │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 2 — Trapsmith (陷阱大师)                                │
│                                                             │
│  "The ground beneath them is your weapon."                  │
│                                                             │
│  Replaces standard trap with 3 types:                       │
│    Spike Trap: damage + bleed                               │
│    Poison Cloud: damage over time in 3x3 area               │
│    Frost Trap: slow enemies in 3x3 area                     │
│  Maximum 2 traps active at once.                            │
│                                                             │
│  Mentor's Ordeal 2: "Kill 15 enemies with traps."           │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 3 — Shadowstep (暗影步)                                 │
│                                                             │
│  "They swing at air. You are already behind them."           │
│                                                             │
│  On successful dodge:                                       │
│    → Instantly teleport behind the attacker                 │
│    → Next attack within 1 second is guaranteed to hit       │
│                                                             │
│  Mentor's Ordeal 3: "Dodge 5 attacks in a row without       │
│  taking damage."                                            │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ Mark 4 — Ricochet (弹射箭)                                   │
│                                                             │
│  "One arrow. Three corpses."                                │
│                                                             │
│  Arrows on hit:                                             │
│    → Bounce to the nearest enemy within 5 tiles              │
│    → Up to 2 bounces                                        │
│    → Each bounce: -30% damage from previous                 │
│                                                             │
│  Mentor's Ordeal 4: "Kill 3 enemies with a single arrow."   │
└─────────────────────────────────────────────────────────────┘
```

### Dungeon Marks (Rogue)

```
Mark 5 — Predator (狩猎者)
  Attacking an enemy from behind deals +40% damage and causes bleed

Mark 6 — Wind Walker (风行者)
  Dodging grants +20% movement speed for 2 seconds

Mark 7 — Venomancer (毒师)
  Critical hits apply poison (damage over time). Poisoned enemies
  take +15% damage from traps.

Mark 8 — Ghost (幽灵)
  After standing still for 3 seconds → become invisible to enemies
  until you move or attack
```

### Notable Rogue Builds

| Build | Marks | Playstyle |
|---|---|---|
| Sniper | Deadeye + Ghost | Go invisible → position → wait → critical headshot. |
| Trapper | Trapsmith + Venomancer | Lay traps → poison spreads → everything dies in the zone. |
| Assassin | Shadowstep + Predator | Dodge behind → backstab for 140%. Hit and run. |
| Ricochet | Ricochet + Wind Walker | Keep moving, kite packs, arrows bounce through crowds. |
| Ambush | Ghost + Deadeye | Complete invisibility into devastating alpha strike. |

---

## Monk — Marks of the Ascetic

**Mentor: Pepin the Healer**

Pepin knows that the body is a vessel. The Monk's path is to master it.

```
┌─────────────────────────────────────────────────────────────┐
│ Mark 1 — Iron Palm (铁掌)                                    │
│  Unarmed attacks gain +1 combo strike per hit (up to 3).    │
│  3rd strike: guaranteed critical + knockback.               │
│                                                             │
│ Mark 2 — Serenity (宁静)                                     │
│  After 3 seconds without taking damage → next ability costs  │
│  no mana. Stacks with Ley Weaver.                            │
│                                                             │
│ Mark 3 — Chi Wave (气波)                                     │
│  Killing an enemy releases a healing wave (heals you and     │
│  allies within 3 tiles for 10% max HP).                     │
│                                                             │
│ Mark 4 — Earth Stance (大地之势)                              │
│  Cannot be knocked back. Standing still → armor +50%.        │
│  Moving breaks stance.                                      │
└─────────────────────────────────────────────────────────────┘

### Dungeon Marks (Monk)

```
Mark 5 — Flowing Water (流水)
  Dodging → next attack speed doubled

Mark 6 — Inner Fire (内火)
  Mana depleted → unarmed damage +40%

Mark 7 — Karma (业)
  Damage taken is stored. Next attack releases stored damage +50%.

Mark 8 — Transcendence (超脱)
  Death (Soul-Weakened state) → spawn with Chi Wave already active
```

---

## Bard — Marks of the Skald

**Mentor: Gillian the Barmaid**

Gillian listens to every tale told in Ogden's tavern. She knows which stories carry power.

```
┌─────────────────────────────────────────────────────────────┐
│ Mark 1 — Warsong (战歌)                                      │
│  Attacking plays a rhythm. Every 4th hit → nearby enemies   │
│  take +20% damage for 3 seconds.                            │
│                                                             │
│ Mark 2 — Lament (哀歌)                                       │
│  Killing an enemy → nearby enemies are slowed by 40% for 2s  │
│  and deal -15% damage.                                      │
│                                                             │
│ Mark 3 — Echosong (回响之歌)                                 │
│  Each spell cast echoes 2 seconds later at 30% power         │
│  toward the nearest enemy.                                  │
│                                                             │
│ Mark 4 — Hymn of Respite (休憩之诗)                           │
│  Standing still → slowly regenerate HP for all nearby allies │
│  (including yourself).                                      │
└─────────────────────────────────────────────────────────────┘

### Dungeon Marks (Bard)

```
Mark 5 — Crescendo (渐强)
  Consecutive hits on the same target: +10% damage per hit (max +50%)

Mark 6 — Dissonance (不协和音)
  Switching targets within 1s → stun the new target for 1s

Mark 7 — Coda (终章)
  Enemy below 20% HP → your next attack deals triple damage (30s cooldown)

Mark 8 — Overture (序曲)
  First attack in each combat → all nearby enemies are Feared for 1s
```

---

## Barbarian — Marks of the Berserk

**Mentor: Farnham the Drunk**

Everyone in Tristram thinks Farnham is a broken man. They don't know he once stood at the gates of Hell itself.

```
┌─────────────────────────────────────────────────────────────┐
│ Mark 1 — Blood Rage (血怒)                                   │
│  Each kill → +5% damage, stacks up to 5 times.              │
│  Stacks last 5 seconds. Killing refreshes duration.          │
│                                                             │
│ Mark 2 — Unchained (挣脱)                                    │
│  Cannot be stunned, frozen, or feared.                      │
│  Crowd control effects instead increase your damage by 20%   │
│  for their duration.                                        │
│                                                             │
│ Mark 3 — Sunder (破甲)                                       │
│  Attacks ignore 30% of enemy armor.                         │
│  Attacking the same target repeatedly increases ignore       │
│  by +10% per hit (max 60%). Switching targets resets.       │
│                                                             │
│ Mark 4 — Warcry (战吼)                                       │
│  Activate (costs 20% current HP, 30s cooldown):              │
│    → All enemies in 4 tiles are Taunted (forced to attack    │
│      you) for 3 seconds.                                     │
│    → During taunt, you take -25% damage from all sources.    │
└─────────────────────────────────────────────────────────────┘

### Dungeon Marks (Barbarian)

```
Mark 5 — Thick Skin (厚皮)
  Damage taken reduced by 1 per nearby enemy (max 8)

Mark 6 — Momentum (冲势)
  Moving continuously for 2s → next attack deals double damage and knocks back

Mark 7 — Last Stand (背水一战)
  HP below 15% → gain +40% attack speed and immunity to death for 3 seconds
  (once per expedition)

Mark 8 — Trophy Hunter (猎头者)
  Killing a Unique monster → gain +15% damage and speed for 60 seconds
```

---

## Implementation

### Data Structure

```cpp
// MasterMark.h

enum class MasterMarkId : uint8_t {
    // Warrior
    ShieldMaster, Berserker, Commander, Avenger,
    Juggernaut, Sentinel, Executioner, IronWill,
    // Sorcerer
    Arcanist, Pyromancer, Stormcaller, Sanguimancer,
    Frostborn, LeyWeaver, EchoMage, Voidcaller,
    // Rogue
    Deadeye, Trapsmith, Shadowstep, Ricochet,
    Predator, WindWalker, Venomancer, Ghost,
    // Monk
    IronPalm, Serenity, ChiWave, EarthStance,
    FlowingWater, InnerFire, Karma, Transcendence,
    // Bard
    Warsong, Lament, Echosong, HymnOfRespite,
    Crescendo, Dissonance, Coda, Overture,
    // Barbarian
    BloodRage, Unchained, Sunder, Warcry,
    ThickSkin, Momentum, LastStand, TrophyHunter,

    COUNT
};

struct MasterMarkDef {
    std::string name;
    std::string description;
    HeroClass requiredClass;      // CLASS_WARRIOR, etc.
    MasterMarkSource source;      // MENTOR_ORDEAL or DUNGEON
    int dungeonLevel;             // 0 for mentor marks
};
```

### Player State

```cpp
struct Player {
    // ...
    std::bitset<static_cast<size_t>(MasterMarkId::COUNT)> ownedMarks;
    MasterMarkId activeMarks[2];
    // ...
};
```

### Mark Activation

```cpp
// Active marks are checked at relevant hook points.

// Example: Shield Master — checked in block handler
void OnBlock(Player &player, Monster &attacker) {
    if (player.hasActiveMark(MasterMarkId::ShieldMaster)) {
        int bashDamage = player.GetShieldArmor() * 0.3f;
        attacker.takePhysicalDamage(bashDamage);
        attacker.applyKnockback(player.direction);
        attacker.applyStun(30);  // 30 frames = 0.5s
    }
}

// Example: Berserker — checked in attack handler
bool ShouldSweep(Player &player) {
    return player.hasActiveMark(MasterMarkId::Berserker)
        && player.hpPercent() < 0.3f;
}
```

### New Files

| File | Purpose |
|---|---|
| `Source/mastermark.h` | Mark ID enum, MarkDef struct, source enum |
| `Source/mastermark.cpp` | Mark definitions table, helper functions |
| `Source/mastermark_warrior.cpp` | Warrior mark callbacks |
| `Source/mastermark_sorcerer.cpp` | Sorcerer mark callbacks |
| `Source/mastermark_rogue.cpp` | Rogue mark callbacks |
| ... | One per class |

### Existing File Modifications

| File | Change |
|---|---|
| `Source/player.h` | Add `ownedMarks` bitset, `activeMarks[2]`, `hasActiveMark()` |
| `Source/player.cpp` | Mark check hooks in combat/defense/spell functions |
| `Source/towners.cpp` | Mentor dialogue options (ordeal check + mark grant) |
| `Source/loadsave.cpp` | Serialize ownedMarks and activeMarks |
| `Source/diablo.cpp` | Initialize mark definitions on game start |

---

## Design Decisions Log

| Decision | Rationale |
|---|---|
| No rarity/quality/level on marks | Prevents "mark farming." A Mark is a binary discovery. |
| 8 per class, 2 active | 28 combinations per class. Deep enough for replayability. |
| 4 mentor + 4 dungeon | Mentor marks provide direction. Dungeon marks reward exploration. |
| No mark trading between classes | Each class's marks are balanced within that class's power curve. |
| Free swapping at mentor | Encourages experimentation. "I wonder if Pyromancer + Frostborn works..." |
| Existing NPCs as mentors | No new characters. Deepens existing town lore. Griswold as warrior mentor is perfect. Farnham as barbarian mentor gives the "drunk" a redemption arc. |
| Behavioral changes only | No "+10% fire damage." No "+5 strength." The phrase "my lightning chains to enemies" becomes "my lightning strikes the same enemy twice." |
