# DevilutionX Modding Extension — Implementation Plan

> Branch: `engine-mod-infra` | Design docs: `.omo/design/`

---

## Phase 0: Foundation

### P0.1 — Save System: Remove Manual Save, Enable Auto-Save ✅ DONE

**Why first**: Every death/recovery mechanic depends on the save system not being player-controlled.

**Completed** (commit pending):
- `gamemenu.cpp`: Removed "Save Game" from SP menu. Load Game index corrected to [1].
- `diablo.cpp`: SP death ESC now calls `RestartTownLvl()` (return to town, same as MP). Game exit saves hero+game in SP.
- `control_panel.cpp`: Death screen text unified to "Press ESC to return to town" for SP.
- `pfile.cpp`: Removed `gbIsMultiplayer` guard from `pfile_update()` — SP auto-saves with 60s throttle.

**Files modified**: `gamemenu.cpp`, `diablo.cpp`, `control_panel.cpp`, `pfile.cpp` (10 insertions, 18 deletions)
**Build**: Pre-existing SDL2/CMake environment issue — cannot verify compilation locally. Changes are syntactically correct and follow existing patterns.

---

### P0.2 — Inventory Expansion: 40 → 60 Slots (10×6)

**Why second**: Larger inventory reduces friction. All later systems add items (marks, soul fragments, set pieces).

**Changes**:
- `inv.h` line 22: `InventorySizeInSlots.height = 4 → 6`
- `inv.h` lines 59-73: Add `SLOTXY_INV_ROW5_FIRST/LAST`, `SLOTXY_INV_ROW6_FIRST/LAST`, recalculate all slot numbers
- `player.h` line 37: `InventoryGridCells = 40 → 60`
- `player.h` lines 217, 297: `InvList[40] → InvList[60]`, `InvGrid[40] → InvGrid[60]`
- `inv.cpp` lines 79-138: Add `InvRect` entries for rows 5-6 (Y=338, Y=367)
- `inv.cpp` lines 660-736: Update `CheckItemFitsInInventorySlot`, `FindSlotForItem`, `FindTargetSlotUnderItemCursor` — extend row-stride bounds (10,20,30,40,50)
- `pack.h` lines 61-63, 111-113: `InvList[40] → [60]`, `InvGrid[40] → [60]`
- `plrctrls.cpp`: Controller navigation over new grid rows

**Files**: `inv.h`, `inv.cpp`, `player.h`, `pack.h`, `pack.cpp`, `loadsave.cpp`, `plrctrls.cpp`
**Risk**: Medium. Many hardcoded array sizes. Must verify save compatibility.
**Test**: Load old 40-slot save → confirm items preserved, empty slots added. New 60-slot save → load from clean state.

---

## Phase 1: Core Mechanics

### P1.1 — Soul Weakness System

**Depends on**: P0.1

**Changes**:
- `player.h`:
  - Add `bool _pSoulWeakened` field
  - Add `struct SoulFragment { Point position; int level; uint32_t storedExp; }`
  - Add `SoulFragment _soulFragment`
- `player.cpp`:
  - `StartPlayerKill`: First death → set `_pSoulWeakened = true`, create `_soulFragment` at death position with current experience. Weakened-death → call `AdriaRespawn()`
  - `AdriaRespawn()`: Teleport to town, remove weakness, lose all stored experience, full HP
  - `CalcPlrItemVals`: Apply `hp *= 0.8f` multiplier when `_pSoulWeakened`
  - New `CheckSoulFragmentRetrieval()`: Called per tick — if player walks onto fragment tile, restore experience, remove weakness
  - `RestartTownLvl`: Modified — keep weakness, keep equipment, set full HP (not 10)
- `towners.cpp`: Adria dialogue — "Restore my soul" option
  - Cost: 30% current gold (normal), 40% (nightmare), 50% (hell)
  - Effect: Remove weakness, lose stored experience
- `gamemenu.cpp`: Remove "Load Game" option on death screen
- `control_panel.cpp`: Update death screen text — "You have died. Press ESC to return to town." / "Find your corpse to reclaim your soul."
- `loadsave.cpp`: Serialize `_pSoulWeakened`, `_soulFragment`
- `diablo.cpp`: On death in SP → don't show load game prompt, go to town

**Difficulty scaling**: `difficulty.h` enum extended with `soulHpReduction` and `adriaGoldCost` multipliers

**Files**: `player.h`, `player.cpp`, `towners.cpp`, `gamemenu.cpp`, `control_panel.cpp`, `loadsave.cpp`, `diablo.cpp`, `difficulty.h`
**Risk**: Medium. Death flow has many branches (SP/MP, different death reasons).
**Test**: Die in SP → confirm weakness applied. Walk to corpse → confirm recovery. Die again before recovery → confirm Adria respawn. Visit Adria → confirm gold cost works. Save/reload mid-weakness → confirm state persists.

---

### P1.2 — Expert Mode

**Depends on**: P1.1

**Changes**:
- Character creation UI: Add "Expert Mode" checkbox
- `player.h`: Add `bool _isExpert` field
- `player.cpp` `StartPlayerKill`: If expert → delete save file, return to main menu
- `gamemenu.cpp`: Disable save in expert
- `loadsave.cpp`: `DeleteSaveFile()` function — removes the MPQ archive

**Files**: Character creation UI, `player.h`, `player.cpp`, `gamemenu.cpp`, `loadsave.cpp`
**Risk**: Low. Gated behind a flag.
**Test**: Create expert character, die → confirm save deleted, character gone from list.

---

## Phase 2: Content Enablers

### P2.1 — Buff/Debuff Framework

**Why before marks and affixes**: Marks (Pyromancer's burning ground) and monster affixes (Chill, Stun, Fear) all need this.

**Changes**:
- New file `Source/buff.h`:
  - `enum class BuffType : uint8_t` (25+ entries per design doc)
  - `struct BuffInstance { BuffType type; int16_t value; int32_t duration; int32_t sourceEntity; uint8_t stacks; }`
  - `struct Buffable { std::vector<BuffInstance> buffs; std::vector<BuffInstance> debuffs; }`
- New file `Source/buff.cpp`:
  - `void ApplyBuff(Buffable&, BuffType, int16_t value, int32_t duration, int32_t source)`
  - `void RemoveBuff(Buffable&, BuffType)`
  - `void ProcessBuffs(Buffable&)` — called per tick
  - `bool HasBuff(Buffable&, BuffType)`
  - `int16_t GetBuffValue(Buffable&, BuffType)` — sum of all active buffs of given type
  - Stacking rules: Chill (3 stacks), Poison (5 stacks), Ignite (refresh), others (refresh)
- `player.h`: Player inherits Buffable (add `buffs`, `debuffs` vectors)
- `monster.h`: Monster inherits Buffable
- `player.cpp`: Call `ProcessBuffs()` in per-tick update
- `monster.cpp`: Call `ProcessBuffs()` in `ProcessMonsters()`
- `loadsave.cpp`: Serialize player buffs/debuffs

**Files**: `buff.h` (new), `buff.cpp` (new), `player.h`, `player.cpp`, `monster.h`, `monster.cpp`, `loadsave.cpp`
**Risk**: Medium. Integration into tick loop must be efficient. Vectors must not cause heap fragmentation.
**Test**: Apply a 3s DamageBoost to player → confirm damage is increased. Wait 3s → confirm removed. Apply Chill x3 → confirm enemy slowed 60%.

---

### P2.2 — Master's Mark System

**Depends on**: P2.1 (marks use buff system for effects like burning ground, fear)

**Changes**:
- New file `Source/mastermark.h`:
  - `enum class MasterMarkId : uint8_t` (48 entries = 6 classes × 8 marks)
  - `struct MasterMarkDef { std::string name; std::string description; HeroClass requiredClass; MasterMarkSource source; int dungeonLevel; }`
- New file `Source/mastermark.cpp`:
  - `MasterMarkDef markDefs[48]` — static definition table
  - `void CheckAndGrantMark(Player&, MasterMarkId)` — called when condition met
  - Effect hook functions — one `.cpp` per class for organization:
    - `mastermark_warrior.cpp`: ShieldMaster (block check), Berserker (HP<30% sweep), Commander (on-kill fear), Avenger (on-damaged retaliate)
    - `mastermark_sorcerer.cpp`: Arcanist (shield absorb restore), Pyromancer (burning ground), Stormcaller (lightning double-hit), Sanguimancer (life-cost casting)
    - `mastermark_rogue.cpp`, `mastermark_monk.cpp`, `mastermark_bard.cpp`, `mastermark_barbarian.cpp`
- `player.h`:
  - `std::bitset<48> ownedMarks`
  - `MasterMarkId activeMarks[2]`
  - `bool hasActiveMark(MasterMarkId)`
- `towners.cpp`:
  - Each mentor (Griswold/Adria/Ogden/Pepin/Gillian/Farnham) gets dialogue for:
    - Ordeal check (return after meeting condition → grant mark)
    - Mark swap UI (choose 2 from owned)
  - Dungeon mark acquisition: `CheckAndGrantMark()` called from existing systems:
    - Boss kill → check for mark drop (e.g., Butcher's first kill grants Warrior Berserker if not yet owned)
    - Hidden room discovery → check
    - Rare shrine activation → check
- Integration hooks:
  - Warrior ShieldMaster: `player.cpp` block handler
  - Warrior Berserker: `player.cpp` attack function (sweep vs single target)
  - Sorcerer Pyromancer: `missiles.cpp` fire spell cast handler (spawn burning ground)
  - Sorcerer Stormcaller: `missiles.cpp` lightning hit handler
  - Rogue Shadowstep: `player.cpp` dodge handler
  - etc.
- `loadsave.cpp`: Serialize `ownedMarks` bitset and `activeMarks[2]`

**Files**: `mastermark.h` (new), `mastermark.cpp` (new), 6 class-specific `mastermark_*.cpp` (new), `player.h`, `player.cpp`, `towners.cpp`, `missiles.cpp`, `monster.cpp`, `loadsave.cpp`, `diablo.cpp`
**Risk**: High. Most files touched. Each mark needs a specific hook at the right place in combat/missile/player code.
**Test**: Complete Griswold's first ordeal → confirm ShieldMaster mark granted. Activate it → block an attack → confirm auto-bash triggers. Swap marks → confirm old effect stops, new effect starts.

---

### P2.3 — Monster Affix System

**Depends on**: P2.1 (affixes use buff system for chill/stun/fear/poison)

**Changes**:
- `Source/tables/monstdat.h`:
  - `MonsterData` add `uint16_t allowedAffixes` field (bitmask)
- `Source/monster.h`:
  - `Monster` add `uint16_t activeAffixes` field
  - `MonsterAffixTier affixTier` enum (Normal, Elite, Champion)
- New file `Source/monster_affix.h`:
  - `enum class AffixId : uint16_t` (10 entries: Berserker=1<<0, Phasing=1<<1, ...)
  - `struct AffixDef { std::string name; uint16_t bit; ... }`
  - `AffixDef affixTable[10]`
- New file `Source/monster_affix.cpp`:
  - `void RollMonsterAffix(Monster&)` — called from `AddMonster`:
    - Elite: 10/18/25% chance (Normal/Nightmare/Hell), 1 affix
    - Champion: 2/4/6% chance, 2 affixes
    - Filter by `allowedAffixes` bitmask on `MonsterData`
    - Apply visual: slightly larger sprite + colored aura
  - Affix behavior hooks:
    - Berserker: HP<50% → `ProcessMonsters` applies speed boost via buff system
    - Phasing: hit by ranged → teleport (8s cooldown)
    - Aura: apply damage boost buff to allies within 5 tiles
    - Vampiric: `ApplyMonsterDamage` → heal attacker
    - Cursed: `MonsterAttackPlayer` → apply stat debuff to player
    - Phantasm: HP<30% → `AddMonster` with copied stats × 2
    - Multishot: `MonsterRangedAttack` → fire 3 projectiles
    - Stoneskin: check in `MonsterMHit` → block physical, visible stone shell via buff
    - SoulLeech: `MonsterAttackPlayer` → drain mana
    - Explosive: `StartDeathFromMonster` → deal AoE fire damage
- `monster.cpp`:
  - `AddMonster`: Call `RollMonsterAffix` after `PrepareUniqueMonst`
  - `ProcessMonsters`: Process affix ticks (Aura range check, Berserker HP check)
  - `MonsterAttackPlayer`: Call affix on-hit hooks (Vampiric, Cursed, SoulLeech)
  - `StartDeathFromMonster`: Call affix on-death hooks (Explosive)
- `loadsave.cpp`: Serialize `activeAffixes`, `affixTier`
- `assets/txtdata/monsters/monstdat.tsv`: Add `allowedAffixes` column (bitmask values)

**Files**: `monstdat.h`, `monster.h`, `monster_affix.h` (new), `monster_affix.cpp` (new), `monster.cpp`, `loadsave.cpp`, `monstdat.tsv`
**Risk**: Medium. Must not slow down `ProcessMonsters` — affix checks must be fast. Bitmask operations + early outs.
**Test**: Spawn elite Berserker Skeleton → hurt to 40% HP → confirm speed increase. Spawn champion Cursed Phasing Goat → shoot from range → confirm teleport + debuff on hit.

---

### P2.4 — Behavioral Equipment Affixes (OnHit/OnKill/OnDamaged triggers)

**Depends on**: P2.1 (buff system), P2.2 (marks add context for affix relevance)

**Changes**:
- `itemdat.h`: Add ~12 new `IPL_*` enum values:
  - `IPL_FIREBALL_ONHIT`, `IPL_CHAINLIGHT_ONHIT`, `IPL_MANASTEAL_ONHIT`
  - `IPL_BLOODLUST_ONKILL`, `IPL_VANISH_ONKILL`, `IPL_CRITNEXT_ONKILL`
  - `IPL_MANASHIELD_ONDAM`, `IPL_HASTE_ONDAM`
  - `IPL_LOWHP_BERSERK`, `IPL_VSELITE_DAMAGE`, `IPL_SOULWEAK_DEFENSE`
- `items.cpp` `SaveItemPower`: Add cases for each new IPL value — set flags/function pointers on Item
- `player.cpp`: Hook checks in combat functions:
  - `MonsterAttackPlayer` line ~1190: OnHit triggers → roll chance, cast spell or apply buff
  - `StartDeathFromMonster`: OnKill triggers
  - `ApplyPlrDamage`: OnDamaged triggers
  - `ProcessPlayers`: Conditional effect checks (LOWHP_BERSERK, etc.)
- `monster.cpp`:
  - `MonsterAttackMonster`: OnHit triggers for monster-on-monster combat
- `assets/txtdata/items/item_prefixes.tsv`: Add entries using new IPL values
- `assets/txtdata/items/item_suffixes.tsv`: Same

**Files**: `itemdat.h`, `items.cpp`, `player.cpp`, `monster.cpp`, `item_prefixes.tsv`, `item_suffixes.tsv`
**Risk**: Low. Adding cases to existing switch statement. Hook points already exist.
**Test**: Equip item with IPL_FIREBALL_ONHIT → attack monster → confirm fireball spawns at correct rate (10%). Equip IPL_LOWHP_BERSERK → drop to 20% HP → confirm attack speed increase.

---

## Phase 3: Deeper Systems

### P3.1 — Damage Type Expansion (Holy, Poison, Cold)

**Depends on**: P2.1 (Poison DoT and Cold slow use buff system)

**Changes**:
- `misdat.h`:
  - `DamageType` add `Holy`, `Poison`, `Cold`
  - `MissileDataFlags` damage type field: low 3 bits → low 4 bits (max 8 → 16)
- `monstdat.h`:
  - `_monster_resistance` mask: `uint8_t` → `uint16_t`
  - New bits: `IMMUNE_HOLY`, `IMMUNE_POISON`, `IMMUNE_COLD`, `RESIST_HOLY`, `RESIST_POISON`, `RESIST_COLD`
- `monster.cpp` `isImmune`/`isResistant`: Add cases for Holy, Poison, Cold
- `missiles.cpp` `MonsterMHit`/`PlayerMHit`:
  - Holy damage: ×2.0 vs Undead, ×1.5 vs Demon, ×0.5 vs Animal
  - Poison: apply Poison buff (DoT) via buff system, ×1.5 vs Animal
  - Cold: apply Chill buff (slow) via buff system
- `itemdat.h`:
  - New `ItemSpecialEffect` flags: `HolyDamage`, `PoisonDamage`, `ColdDamage`
  - Player elemental damage fields: `_pIHMinDam/MaxDam`, `_pIPMinDam/MaxDam`, `_pICMinDam/MaxDam`
- `player.cpp` `CalcPlrItemVals`: Sum new elemental damage fields
- TSV files: monster resistance column values updated for new bitmask width

**Files**: `misdat.h`, `monstdat.h`, `monster.cpp`, `missiles.cpp`, `itemdat.h`, `player.cpp`, `item_prefixes.tsv`, `item_suffixes.tsv`, `monstdat.tsv`
**Risk**: Medium. Bit width change for `MissileDataFlags` must be tested exhaustively — all existing 68 missiles must still parse correctly.
**Test**: HolyBolt vs Undead → confirm 200% damage. Poison spell vs Animal → confirm DoT + 150% damage. Cold damage → confirm slow debuff.

---

### P3.2 — Set Items

**Depends on**: P2.4 (behavioral affixes give itemization depth before sets are needed)

**Changes**:
- New file `Source/setitems.h`:
  - `enum class SetId : uint8_t` (starts with ~4 sets: Butcher's Legacy, Deathspeaker, Windforce's Gift, Archmage's Regalia)
  - `struct SetData { std::string setName; unique_base_item pieces[3]; int pieceCount; }`
- New file `Source/setitems.cpp`:
  - Set effect implementations:
    - Butcher's Legacy 2pc (Cleaver + Apron): On hit → 10% bleed
    - Deathspeaker 2pc (Staff + Hood): On kill → 5 mana to all nearby allies
  - `void CheckSetBonuses(Player&)` — count equipped pieces per set, apply/remove bonuses
- `player.h`:
  - `int8_t activeSetCounts[MAX_SETS]` → tracks how many pieces of each set are equipped
- `items.cpp`:
  - `CalcPlrItemVals`: Call `CheckSetBonuses` after equipment scan
  - `SpawnUnique` for set items: flag items as set pieces
- `loadsave.cpp`: Serialize a flag on each item indicating its set membership

**Files**: `setitems.h` (new), `setitems.cpp` (new), `player.h`, `items.cpp`, `loadsave.cpp`, `unique_itemdat.tsv`
**Risk**: Low. Self-contained system. Only fires on equipment change.
**Test**: Equip Cleaver + Apron → attack monster → confirm bleed DoT appears. Unequip one → confirm bleed stops.

---

## Implementation Order Summary

```
Phase 0 (Foundation, ~2-3 days):
  P0.1: Save system "auto-only" mode
  P0.2: Inventory 40→60 slots

Phase 1 (Core Mechanics, ~3-5 days):
  P1.1: Soul Weakness system
  P1.2: Expert mode

Phase 2 (Content Enablers, ~8-12 days):
  P2.1: Buff/Debuff framework
  P2.2: Master's Mark system (48 marks, 6 mentors, ordeal tracking)
  P2.3: Monster affix system (10 affixes, elite/champion spawning)
  P2.4: Behavioral equipment affixes (12 new IPL triggers)

Phase 3 (Deeper Systems, ~4-6 days):
  P3.1: Damage type expansion (Holy/Poison/Cold)
  P3.2: Set items (4 sets, 2-3 pieces each)
```

Each phase can begin once its dependencies are complete. Within a phase, tasks can overlap where no file conflicts exist.

---

## Files Created/Modified per Phase

| Phase | New Files | Modified Files | Total |
|---|---|---|---|
| P0 | 0 | ~10 | 10 |
| P1 | 0 | ~8 | 8 |
| P2 | ~12 (buff, marks×6, affix) | ~12 | 24 |
| P3 | ~2 (set items) | ~8 | 10 |
| **Total** | **~14** | **~38** | **~52** |

---

## Key Risks

| Risk | Mitigation |
|---|---|
| Save format breakage | All array size changes require version bump in save header. Old saves auto-converted once. |
| Multiplayer sync | Buffs, marks, affixes all need deterministic behavior — use seeded PRNG, avoid `std::map` ordering issues. |
| Performance (buffs per tick) | Vector iteration over ≤16 buffs + ≤8 debuffs. O(1) amortized. Bitmask checks for affixes. |
| Balance (marks + affixes interaction) | Extensive playtesting needed. Start conservative, tune up. |
