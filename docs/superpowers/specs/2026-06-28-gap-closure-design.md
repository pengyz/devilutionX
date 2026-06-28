# Engine-Mod-Infra Gap Closure — Design Spec

> Date: 2026-06-28 | Branch: `engine-mod-infra`

## 1. Summary

Close 5 data-path and logic gaps found in review. All fixes are code + TSV
data; no new architecture. Tests added for each gap proving end-to-end
functionality (item generation → affix application, save → load round-trip,
set piece equipping → bonus activation).

## 2. The Gaps

| # | Gap | Root Cause | Fix Strategy |
|---|-----|-----------|-------------|
| 1 | New IPL values unreachable by affix system | 12 IPL names missing from string→enum map in `itemdat.cpp` | Add mapping entries |
| 2 | Holy/Poison/Cold damage has no equipment source | Player lacks damage fields; no IPL values to confer them | Add 6 fields + 3 IPL + TSV columns |
| 3 | Set bonuses are dead code | `CountEquippedSetPieces` returns 0; no item-to-set mapping | Add `setId`/`setPiece` TSV columns; parse in loader |
| 4 | `_iProcFlags` lost on save/load | Field not serialized in SaveItem/LoadItemData | Add serialization |
| 5 | Existing uniques can't use new features | No TSV mechanism to assign proc flags / set IDs | Add `procFlags`/`procChance` TSV columns |

## 3. Design

### 3.1 Gap 1 — String-to-Enum Mapping

**File**: `Source/tables/itemdat.cpp`

Add 12 entries after the `IPL_LIFETOMANA` mapping:

```
FIREBALL_ONHIT → IPL_FIREBALL_ONHIT
CHAINLIGHT_ONHIT → IPL_CHAINLIGHT_ONHIT
MANASTEAL_ONHIT → IPL_MANASTEAL_ONHIT
LIFESTEAL_ONHIT → IPL_LIFESTEAL_ONHIT
FROSTNOVA_ONDAM → IPL_FROSTNOVA_ONDAM
CONFUSE_ONHIT → IPL_CONFUSE_ONHIT
BLOODLUST_ONKILL → IPL_BLOODLUST_ONKILL
VANISH_ONKILL → IPL_VANISH_ONKILL
CRITNEXT_ONKILL → IPL_CRITNEXT_ONKILL
MANASHIELD_ONDAM → IPL_MANASHIELD_ONDAM
HASTE_ONDAM → IPL_HASTE_ONDAM
THORNS_ONDAM → IPL_THORNS_ONDAM
```

**TSV columns**: `item_suffixes.tsv` — add 12 new suffix rows referencing the new IPL names.

**Test**: Load suffix TSV → verify at least one suffix references a behavioral IPL → simulate item creation with that suffix → verify `_iProcFlags` nonzero.

### 3.2 Gap 2 — Elemental Damage Fields

**New Player fields** (in `player.h`, after existing `_pILMaxDam`):

```cpp
int _pIHMinDam;  // Holy
int _pIHMaxDam;
int _pIPMinDam;  // Poison
int _pIPMaxDam;
int _pICMinDam;  // Cold
int _pICMaxDam;
```

All default to 0. Packed/unpacked in `pack.cpp`, serialized in `loadsave.cpp`.

**New IPL values** (in `itemdat.h`):

```cpp
IPL_HOLYDAM,    // item confers Holy damage
IPL_POISONDAM,  // item confers Poison damage
IPL_COLDDAM,    // item confers Cold damage
```

**IPL → item field mapping** (in `items.cpp` `SaveItemPower`):

```cpp
case IPL_HOLYDAM:
    item._iHMinDam = power.param1;
    item._iHMaxDam = power.param2;
    break;
// etc.
```

**TSV columns**: `item_prefixes.tsv` and `item_suffixes.tsv` — add rows for Holy/Fire/Cold damage prefixes/suffixes. Unique items that make thematic sense (Lightsabre → Holy, Inferno → Fire already exists, Ice Shank → Cold) updated via `unique_itemdat.tsv` `procFlags` column.

**CalcPlrItemVals**: sum `_iHMinDam`→`_pIHMinDam` etc., following the pattern used for fire/lightning.

**Test**: Create item with `IPL_HOLYDAM` → equip → verify `player._pIHMinDam > 0`.

### 3.3 Gap 3 — Set Items

**TSV columns added to `unique_itemdat.tsv`**:

```
setId    setPiece
-1       -1        ← default for non-set items
0        0         ← ButchersLegacy piece 0 (The Butcher's Cleaver)
...
```

**Parse change** (`itemdat.cpp` `LoadUniqueItemDatFromFile`):
Read `setId` and `setPiece` columns; write to `item._iSetId` and `item._iSetPiece`.

**New Item fields** (`items.h`):

```cpp
int8_t _iSetId = -1;    // -1 = not a set piece, 0-3 = SetId
int8_t _iSetPiece = -1;  // piece index within set
```

**Serialization**: append to end of `SaveItem` / `LoadItemData`.

**Logic fix** (`setitems.cpp` `CountEquippedSetPieces`):
Replace stub with: iterate `player.InvBody`, count items where `_iSetId == setId`.

**ApplySetBonus** already wired in `CalcPlrItemVals` via `CheckSetBonuses`.

**Test**: Equip 2 set pieces → call `CheckSetBonuses` → verify bonus buff applied. Unequip one → verify bonus removed.

### 3.4 Gap 4 — Proc Flag Serialization

**New Item field** (`items.h`):

```cpp
uint8_t _iProcChance = 0;  // 0-100 trigger probability, or reflect %
```

**SaveItem** — add after `_iLMaxDam`:

```cpp
file.WriteLE<uint16_t>(item._iProcFlags);
file.WriteLE<uint8_t>(item._iProcChance);
```

**LoadItemData** — add corresponding reads.

**SaveItemPower update**: OnKill effects set `_iProcChance = 100` (always trigger). OnHit/OnDamaged effects set `_iProcChance` based on item level (5-15%). THORNS stores reflect %.

**CheckEquipmentProcsOnHit** — replace hardcoded `RandomInt(100) < N` with reading `item._iProcChance`.

**Test**: Set `_iProcFlags` + `_iProcChance` on item → save → load → verify both fields preserved.

### 3.5 Gap 5 — Unique Enhancement via TSV

**TSV columns added to `unique_itemdat.tsv`**:

```
procFlags    procChance
0            0          ← default
FIREBALL_ONHIT,MANASTEAL_ONHIT    10    ← bitmask, comma-separated
HOLYDAM      0          ← non-probabilistic (0 means "not a proc, just an elemental flag")
```

`procFlags` uses the IPL string name (same namespace as power0-power5 columns; the proc is separate from the power system — it's a behavioral modifier, not an item power). Multiple flags separated by commas. Parsed into `_iProcFlags` bitmask.

**Parse change** (`itemdat.cpp`): read `procFlags` and `procChance` columns. If `procFlags` non-empty, parse comma-separated IPL names via the same string→enum mapper.

**Which uniques get what (initial pass)**:

| Unique | procFlags | procChance | Reason |
|--------|-----------|-----------|--------|
| Griswold's Edge | FIREBALL_ONHIT | 10 | Already has fire theme |
| Shadowhawk | LIFESTEAL_ONHIT | 8 | Already has STEALLIFE |
| The Grandfather | CRITNEXT_ONKILL | 100 | Legendary sword |
| Veil of Steel | THORNS_ONDAM | 15 | Steel thematic |
| Arkaine's Valor | MANASHIELD_ONDAM | 12 | Valor = warding |

**Test**: Load a unique item with `procFlags` set → verify `_iProcFlags` nonzero → equip → attack → verify proc triggers.

## 4. Test Strategy

Each gap gets one "closed-loop" test proving the full data path:

| Gap | Test |
|-----|------|
| 1 | Load suffix TSV → find behavioral IPL → generate item → verify `_iProcFlags` |
| 2 | Create item with `IPL_HOLYDAM` → equip → verify `player._pIHMinDam > 0` |
| 3 | Equip 2 set pieces → `CheckSetBonuses` → verify buff → unequip → verify removed |
| 4 | Set proc flags → save item → load item → verify flags preserved |
| 5 | Load unique with `procFlags` TSV → verify `_iProcFlags` → equip → attack monster → proc fires |

Tests go in `test/gap_closure_test.cpp` and `test/set_items_test.cpp`.

## 5. File Change Summary

| File | Change |
|------|--------|
| `itemdat.h` | 3 new IPL_HOLYDAM/IPL_POISONDAM/IPL_COLDDAM |
| `itemdat.cpp` | 12 string mappings + parse `setId`/`setPiece`/`procFlags`/`procChance` columns |
| `items.h` | `_iSetId`, `_iSetPiece`, `_iProcChance` fields |
| `items.cpp` | SaveItemPower cases for new IPLs; CalcPlrItemVals holy/poison/cold accum |
| `player.h` | 6 elemental damage fields |
| `pack.h/cpp` | Pack/unpack new player fields |
| `loadsave.cpp` | Serialize new Item/Player fields |
| `setitems.cpp` | Real `CountEquippedSetPieces` implementation |
| `item_suffixes.tsv` | 12 behavioral affix rows + 3 elemental rows |
| `item_prefixes.tsv` | 1-2 elemental prefix rows |
| `unique_itemdat.tsv` | `setId`/`setPiece`/`procFlags`/`procChance` columns + 5 enhanced uniques |
| `test/gap_closure_test.cpp` | 5 closed-loop tests |
