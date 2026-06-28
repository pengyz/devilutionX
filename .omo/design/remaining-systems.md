# Remaining Engine Systems — Design Proposals

> Status: Proposal Draft · Target: engine-mod-infra

---

## 1. Inventory Expansion — 40 → 60 Slots

### Current State
```
Grid: 10 columns × 4 rows = 40 backpack slots
Equipped: 7 slots (Head, Ring×2, Amulet, Hand×2, Chest)
Belt: 8 slots (consumables)
Total: 55 interactive slots (40+7+8)
```

### Target
```
Grid: 10 columns × 6 rows = 60 backpack slots (+50%)
```

### Why 10×6

- Keeps column count at 10 — avoids recalculating complex row-stride logic in `FindSlotForItem`, `CheckItemFitsInInventorySlot`, `CheckInvPaste`, and `CheckInvCut`
- Adds 2 rows vertically — panel is right-side, vertical space is abundant
- 6 rows = slot numbering continues naturally: rows 5-6 become slots 47-66
- If 6 rows proves too tall for 640×480: 5 rows (10×5=50) is fallback

### All Constants to Change

| Constant | File:Line | Old | New |
|---|---|---|---|
| `InventorySizeInSlots.height` | inv.h:22 | 4 | 6 |
| `InventoryGridCells` | player.h:37 | 40 | 60 |
| `NUM_XY_SLOTS` | inv.h:73 | 55 | 75 |
| `SLOTXY_INV_ROW4_LAST` | inv.h:67 | 46 | 56 |
| `SLOTXY_INV_ROW5_LAST` | inv.h | — | 66 |
| `SLOTXY_INV_ROW6_LAST` | inv.h | — | 76 |
| `SLOTXY_BELT_FIRST` | inv.h:71 | 47 | 67 |
| `SLOTXY_BELT_LAST` | inv.h:72 | 54 | 74 |
| `SLOTXY_INV_LAST` | inv.h:70 | 46 | 66 |

### All Array Sizes to Change

| Array | File:Line | Old | New |
|---|---|---|---|
| `Item InvList[N]` | player.h:217 | 40 | 60 |
| `int8_t InvGrid[N]` | player.h:297 | 40 | 60 |
| `Item InvRect[NUM_XY_SLOTS]` | inv.cpp:79 | 55 | 75 |
| `ItemPack InvList[N]` | pack.h:61 | 40 | 60 |
| `int8_t InvGrid[N]` | pack.h:62 | 40 | 60 |
| `ItemNetPack InvList[N]` | pack.h:111 | 40 | 60 |
| `int8_t InvGrid[N]` | pack.h:112 | 40 | 60 |

### UI: InvRect Additions

New rows extend the existing pattern. Current row 4 ends at Y=309. Each row adds 29px.

```
Row 5: X: 17→278 (step 29), Y: 338
Row 6: X: 17→278 (step 29), Y: 367
```

### Hardcoded Row Logic to Update

The function `CheckItemFitsInInventorySlot` (inv.cpp:660-679) hardcodes `10 * (slotIndex / 10)` for row Y-offset. This stays correct — width unchanged. But bounds checks must allow 60 slots.

`FindSlotForItem` (inv.cpp:688-736): Row-stride constants (10, 20, 30) extend to 40, 50 for new rows.

### Save Compatibility

`PlayerPack::_pNumInv` is `uint8_t` (pack.h:63) — capacity 255. No widening needed for 60. Save format simply serializes more entries.

**Backward compatibility**: Old saves (40-slot) load with 0-filled extra slots. New saves (60-slot) will fail on old builds. Acceptable — this is a one-time migration.

### Belts

Belt stays at 8 slots. No change needed.

---

## 2. Monster Affix System — Priority: HIGH

### Why This Matters

Current state: every Skeleton on dungeon level 5 is identical to every other Skeleton on level 5. The Master's Mark system creates diverse player builds, but monsters provide no diverse challenges to test those builds against.

### Design

```
Normal monster: unchanged

Elite monster: ~15% spawn chance, carries 1 affix
  → Name prefixed: "狂怒的骷髅" (Berserker Skeleton)
  → Slightly larger sprite + colored aura

Champion monster: ~3% spawn chance, carries 2 affixes
  → Name: "诅咒的狂怒骷髅" (Cursed Berserker Skeleton)
  → Visibly distinct (larger sprite + pulsing glow)
```

### Affix Pool

```
Berserker     — HP < 50% → +30% attack speed, +20% move speed, never flees
Phasing       — Hit by ranged attack → teleports next to attacker (8s cooldown)
Aura          — Allies within 5 tiles deal +15% damage. Aura dies with carrier.
Vampiric      — Attacks heal for 30% of damage dealt
Cursed        — On hit: -1 random stat to player (lasts until floor exit)
Phantasm      — HP < 30% → splits into two copies, each at 50% current HP
Multishot     — Ranged attacks fire 3 projectiles in a fan
Stoneskin     — Immune to physical damage for 3s (10s cooldown, visible shell)
Soul Leech    — On hit: steals 10 mana from player
Explosive     — On death: deals 30% of max HP as fire damage in 3-tile radius
```

All affixes are behavioral, not numeric. "Immune to physical for 3s" forces the player to adapt — kite and wait, switch to magic, or focus another target.

### Implementation

```cpp
// MonsterData — add to TSV
struct MonsterData {
    // ...
    uint16_t allowedAffixes;  // bitmask of which affixes this type can roll
};

// Monster — runtime
struct Monster {
    // ...
    uint16_t activeAffixes;   // bitmask of currently active affixes
    MonsterAffixTier affixTier;  // Normal, Elite, Champion
};

// Affix table in new file: Source/monster_affixes.cpp
struct AffixDef {
    std::string name;
    uint16_t bit;
    std::function<void(Monster&)> onSpawn;    // applied once
    std::function<void(Monster&)> onTick;     // per-frame behavior
    std::function<void(Monster&, int damage)> onDamaged;
    std::function<void(Monster&)> onDeath;
};
```

### Difficulty Scaling

| Difficulty | Elite chance | Champion chance |
|---|---|---|
| Normal | 10% | 2% |
| Nightmare | 18% | 4% |
| Hell | 25% | 6% |

### Files Modified

| File | Change |
|---|---|
| `Source/tables/monstdat.h` | Add `allowedAffixes` to MonsterData |
| `Source/monster.h` | Add `activeAffixes`, `affixTier` to Monster struct |
| `Source/monster_affixes.h` | New: AffixDef, AffixId enum, affix table |
| `Source/monster_affixes.cpp` | New: affix behavior implementations |
| `Source/monster.cpp` | Add `RollAffix` call in `AddMonster`, check affixes in combat/damage/death |
| `Source/loadsave.cpp` | Serialize activeAffixes and affixTier |
| `assets/txtdata/monsters/monstdat.tsv` | Add `allowedAffixes` column |

---

## 3. Buff/Debuff Framework — Priority: HIGH

### Why This Matters

Currently DevilutionX has exactly 3 buff types, all hardcoded as `SpellFlag` enum values (Etherealize, RageActive, RageCooldown). Everything else is ad-hoc booleans on the Player/Monster struct.

The Master's Mark system needs buffs (Pyromancer's burning ground). Monster affixes need debuffs (Cursed reduces stats, Chill slows). Future equipment affixes will need both. Without a framework, every new effect requires a new struct field.

### Design

```cpp
// buff_types.h

enum class BuffType : uint8_t {
    // Offensive
    DamageBoost,       // +N% damage dealt
    AttackSpeed,       // +N% attack speed
    CritChance,        // +N% critical hit chance

    // Defensive
    ArmorBoost,        // +N armor
    DamageReduction,   // -N% damage taken
    BlockChance,       // +N% block
    Reflect,           // return N% of damage to attacker

    // Elemental / Damage over time
    Ignite,            // N fire damage per second
    Poison,            // N poison damage per second
    Chill,             // -N% move/attack speed
    Shock,             // next hit: stun for N ticks

    // Crowd Control
    Stun,              // cannot act for N ticks
    Fear,              // flee from source for N ticks
    Taunt,             // forced to attack source for N ticks
    Snare,             // cannot move for N ticks

    // Resource
    ManaRegen,         // +N mana per second
    LifeDrain,         // -N HP per second
    ManaBurn,          // -N mana per second

    // Special
    Invulnerable,      // immune to all damage
    Invisible,         // monsters cannot target
    SoulWeakened,      // our custom death debuff
};

struct BuffInstance {
    BuffType type;
    int16_t value;          // effect magnitude
    int32_t duration;       // remaining game ticks
    int32_t sourceEntity;   // who applied it (player slot, monster index, 0=system)
    uint8_t stacks;         // for stacking buffs (Chill, Poison)
};

// Limits
constexpr int MAX_BUFFS = 16;
constexpr int MAX_DEBUFFS = 8;

struct Buffable {
    std::vector<BuffInstance> buffs;
    std::vector<BuffInstance> debuffs;
};
```

### Processing

```cpp
// Called each game tick for players and monsters
void ProcessBuffs(Buffable &entity) {
    for (auto it = entity.buffs.begin(); it != entity.buffs.end(); ) {
        if (--it->duration <= 0) {
            RemoveBuff(entity, *it);
            it = entity.buffs.erase(it);
        } else {
            TickBuff(entity, *it);
            ++it;
        }
    }
    // Same for debuffs
}
```

### Stacking Rules

| Buff Type | Stacking |
|---|---|
| Chill | Up to 3 stacks (-20/40/60% speed) |
| Poison | Up to 5 stacks (each adds DPS) |
| Ignite | Refresh duration only (no stacking) |
| All others | Refresh duration (same source) or independent (different sources) |

### Integration Points

Player and Monster both implement `Buffable`. Combat functions read buff state:

```cpp
int Player::getCurrentDamage() {
    int base = calculateBaseDamage();
    for (auto &b : buffs) {
        if (b.type == BuffType::DamageBoost) base += base * b.value / 100;
    }
    return base;
}
```

---

## 4. Behavioral Equipment Affixes — Priority: MEDIUM

### Current State

All 85 `IPL_*` effect types (item_effect_type enum) are numeric: +strength, +fire resist, +attack speed, etc. None of them trigger behaviors.

### Proposal: Trigger-Based IPL Effects

Add ~12 new `IPL_*` values that hook into combat events:

```
OnHit triggers (checked in MonsterAttackPlayer / MonsterAttackMonster):
  IPL_FIREBALL_ONHIT      — 10% chance: cast fireball (uses item's spell level)
  IPL_CHAINLIGHT_ONHIT    — 5% chance: cast chain lightning
  IPL_MANASTEAL_ONHIT     — on hit: steal 3 mana
  IPL_LIFESTEAL_ONHIT     — on hit: steal 5% of damage as HP (existing, expand)

OnKill triggers (checked in StartDeathFromMonster):
  IPL_BLOODLUST_ONKILL    — on kill: +20% damage for 5 seconds
  IPL_VANISH_ONKILL       — on kill: become invisible for 3 seconds
  IPL_CRITNEXT_ONKILL     — on kill: next attack guaranteed critical

OnDamaged triggers (checked in ApplyPlrDamage):
  IPL_MANASHIELD_ONDAM    — 10% chance: cast Mana Shield when hit
  IPL_HASTE_ONDAM         — when hit: +20% move speed for 3 seconds

Conditional effects (checked each tick):
  IPL_LOWHP_BERSERK       — HP < 25%: +30% attack speed
  IPL_VSELITE_DAMAGE      — +25% damage vs Elite/Champion monsters
  IPL_SOULWEAK_DEFENSE    — +10% damage reduction while Soul-Weakened
```

### Implementation

Add cases to the existing `SaveItemPower` switch statement (items.cpp:701-1032). Each new case sets a flag or a function pointer on the Item that is checked at the appropriate hook point.

### Files Modified

| File | Change |
|---|---|
| `Source/tables/itemdat.h` | Add new `IPL_*` enum values |
| `Source/items.cpp` | Add cases in `SaveItemPower`, `SaveItemAffix` |
| `Source/player.cpp` | Hook checks in combat/damage functions |
| `Source/monster.cpp` | Hook checks in combat/death functions |
| `assets/txtdata/items/item_prefixes.tsv` | Add new prefix/suffix entries |
| `assets/txtdata/items/item_suffixes.tsv` | Same |

---

## 5. Set Items — Priority: LOW

### Why Low Priority

D1's item pool is small. Set items multiply design surface without a proportional gameplay return. They shine most when the player already has deep itemization — which D1 currently doesn't. Best implemented after behavioral affixes and monster affixes are in.

### Design Sketch

```
2-3 pieces per set.

示例 — 屠夫的遗产 (Butcher's Legacy, Warrior only):
  Butcher's Cleaver (1H Axe, UT: Cleaver drop)
  Bloodstained Apron (Heavy Armor, hidden room in Catacombs)

  2pc: On hit → 10% chance to bleed target (3% HP/sec for 5s)
  3pc: +30% damage vs bleeding targets (reserved for future expansion)

存储: Set数据类似于UniqueItem，但:
  struct SetData {
      std::string setName;
      unique_base_item pieces[3];
      int pieceCount;          // 2 or 3
      std::function<void(Player&)> twoPieceBonus;
      std::function<void(Player&)> threePieceBonus;
  };
```

---

## 6. Damage Type Expansion — Priority: LOW

### Current State

5 types: Physical, Fire, Lightning, Magic, Acid. `MissileDataFlags` uses low 3 bits = max 8 types.

### Proposal

Add 3 new types, use low 4 bits (16 max):

```
New types:
  Holy    — 200% vs Undead, 150% vs Demon, 50% vs Animal
  Poison  — Applies poison DoT, 150% damage vs Animal
  Cold    — Applies Chill debuff through buff framework

Monster resistance: uint8_t → uint16_t
  New bits: IMMUNE_HOLY, IMMUNE_POISON, IMMUNE_COLD
           RESIST_HOLY, RESIST_POISON, RESIST_COLD
```

Extremely straightforward mechanically. Low priority because it enables content rather than creating it — cold/poison/holy only matter once enemies exist that use and resist them.

---

## Recommended Implementation Order

```
Phase 1 (Foundation):
  1. Inventory Expansion (40→60)          ← enables more loot without frustration
  2. Buff/Debuff Framework                ← required by everything below

Phase 2 (Content Enablers):
  3. Monster Affix System                 ← gives Marks a reason to exist
  4. Behavioral Equipment Affixes         ← makes loot interesting

Phase 3 (Polish):
  5. Damage Type Expansion                ← enables new enemy/spell design
  6. Set Items                            ← caps off itemization
```

---

## Design Decisions Log

| Decision | Rationale |
|---|---|
| Inventory: 10×6 not 12×5 | Preserves 10-wide row-stride logic throughout codebase |
| Buff limits: 16 buffs / 8 debuffs | More than enough for all combined systems. Prevents stack explosion. |
| Monster affix: % chance not guaranteed placement | Surprise is D1. Guaranteed elite in room 3 is a pattern. |
| Affixes are bitmask on Monster, not vector of objects | D1's monster array is 200 entries. Bit operations are cheaper than dynamic allocation. |
| Set items deferred | Requires rich item pool first. Chicken-and-egg problem. |
| Damage types deferred | Enables new content but doesn't create it alone. |
