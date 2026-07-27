> **分类**：Base　**状态**：已实施　**评判基准**：`2026-07-27-better-d1-design-charter.md`

# Spec: Floating Info UI + Skill Bar

**Date**: 2026-06-29
**Branch**: `feature/qol-upgrades`
**Status**: Approved

---

## Overview

Remove the legacy fixed-position info panel (`InfoBoxRect`, 288×64 text area in the main UI panel), migrate all information display to floating tooltips (Diablo 2 style), repurpose the freed panel space with a spell skill bar, level/difficulty display, and expanded belt.

## Motivation

- The fixed info panel feels dated — Diablo 2 established the floating tooltip pattern as standard
- DevilutionX already introduced floating tooltips for inventory/stash/store items via `FloatingInfoString`
- 18 additional info display paths still use `InfoString` (old panel) — these need migration
- The freed 288×64 pixel space can host a spell quickbar (D2-style skill icons) and level info

## Architecture

### New Files

| File | Purpose |
|------|---------|
| `Source/qol/floating_info.cpp/hpp` | Generic floating tooltip engine extracted from `control_infobox.cpp` |
| `Source/qol/skill_bar.cpp/hpp` | Spell quickbar (4 slots mapping F5-F8) |
| `Source/panels/level_info.cpp/hpp` | Level name + difficulty display |
| `test/floating_info_test.cpp` | Floating box unit tests |
| `test/skill_bar_test.cpp` | Quickbar unit tests |
| `test/info_migration_test.cpp` | InfoString → FloatingInfoString migration tests |

### Modified Files

| File | Change |
|------|--------|
| `control/control_infobox.cpp` | Rewrite: all paths use `FloatingInfoString` via `SetFloatingInfo()` |
| `control/control.hpp` | Remove `InfoBoxRect`, add `SetFloatingInfo()` / `FloatingPosition` declarations |
| `engine/render/scrollrt.cpp` | Remove `DrawInfoBox()` call; add `DrawFloatingInfoBox()`, `DrawSkillBar()`, `DrawLevelInfo()` |
| `control/control_panel.cpp` | Remove `DrawInfoBox()` call |
| `items.cpp` | `GetItemStr()` → use `FloatingInfoString` |
| `objects.cpp` | `GetObjectStr()` → use `FloatingInfoString` |
| `monster.cpp` | `PrintMonstHistory()` / `PrintUniqueHistory()` → use `FloatingInfoString` |
| `levels/trigs.cpp` | Stair/portal text → use `FloatingInfoString` |
| `cursor.cpp` | Town portal / red portal text → use `FloatingInfoString` |
| `quests.cpp` | Quest portal text → use `FloatingInfoString` |

### Unchanged Files (existing floating paths)

- `inv.cpp` — already sets both `InfoString` and `FloatingInfoString`
- `qol/stash.cpp` — already sets both
- `qol/visual_store.cpp` — already sets both

## Floating Tooltip Engine

### Interface

```cpp
enum class FloatingPosition {
    Cursor,          // Follows mouse cursor
    WorldEntity,     // World-space entity (monster/item/NPC/stair)
    PanelElement,    // Panel-space element (button/slot)
};

void SetFloatingInfo(std::string_view text, UiFlags color,
                     FloatingPosition pos, Point anchor);
```

### Positioning Per Scenario

| Scenario | Position Mode | Anchor | Offset |
|----------|---------------|--------|--------|
| Inventory/stash/store items | `PanelElement` | Item icon center | Existing logic |
| Main panel buttons | `PanelElement` | Button center | Above |
| Spell quickbar slots | `PanelElement` | Icon center | Above |
| Ground items | `WorldEntity` | Item world→screen coord | Above item |
| Monsters/NPCs | `WorldEntity` | Monster head position | Same as `monhealthbar` |
| Players | `WorldEntity` | Player head | Above |
| Stairs/portals | `WorldEntity` | Entrance world coord | Above |
| Held item | `Cursor` | Cursor position | Bottom-right |
| Spell select button | `PanelElement` | Button center | Above |

### Screen Clamping

Reuse existing `ClampAboveOrBelow()` and `std::clamp()` for horizontal bounds. Floating box must never extend beyond screen edges.

### Content Construction

All info lines are built via `AddInfoBoxString()`, which now writes to `FloatingInfoString` by default:

```cpp
void AddInfoBoxString(std::string_view str, bool floatingBox = true);
```

Functions like `PrintUniqueHistory()`, `PrintMonstHistory()`, `GetItemStr()`, `GetObjectStr()` internally call `AddInfoBoxString` — they require zero modification.

## Info Display Migration Map

26 total display scenarios. 5 already floating, 18 need migration, 3 become obsolete (panel buttons show floating instead).

### Already Floating (no change)

| # | Scenario | File |
|---|----------|------|
| 1 | Inventory item hover | `inv.cpp` |
| 2 | Store tab buttons | `visual_store.cpp` |
| 3 | Store repair buttons | `visual_store.cpp` |
| 4 | Store items | `visual_store.cpp` |
| 5 | Stash items | `stash.cpp` |

### Migrate to WorldEntity Mode

| # | Scenario | File |
|---|----------|------|
| 6 | Ground item hover | `items.cpp` → `control_infobox.cpp` |
| 7 | Object hover (chest/door/etc) | `objects.cpp` → `control_infobox.cpp` |
| 8 | Monster hover (dungeon) | `monster.cpp` → `control_infobox.cpp` |
| 9 | NPC hover (town) | `control_infobox.cpp` |
| 10 | Player hover | `control_infobox.cpp` |
| 11 | Stair down to dungeon | `trigs.cpp` |
| 12 | Warp to catacombs/caves/hell/etc | `trigs.cpp` |
| 13 | Stair up | `trigs.cpp` |
| 14 | Town portal (other player's) | `cursor.cpp` |
| 15 | Red portal | `cursor.cpp` |
| 16 | Quest portal | `quests.cpp` |

### Migrate to Cursor Mode

| # | Scenario | File |
|---|----------|------|
| 17 | Holding gold | `control_infobox.cpp` |
| 18 | Holding unusable item | `control_infobox.cpp` |
| 19 | Holding usable item | `control_infobox.cpp` |

### Migrate to PanelElement Mode

| # | Scenario | File |
|---|----------|------|
| 20 | Main panel button hover | `control_infobox.cpp` |
| 21 | Spell select button hover | `control_infobox.cpp` |
| 22 | Player portrait hover | `control_infobox.cpp` |
| 23 | XP bar hover | `control_infobox.cpp` |

## Panel Layout: Repurposing InfoBoxRect

### Before (InfoBoxRect = {177, 46, 288×64})

```
┌──────────────────────────────────────────────────┐
│             [ Legacy Info Text Area ]            │
│            "Requirements not met"                │
└──────────────────────────────────────────────────┘
```

### After (three sub-zones)

```
┌──────────────────────────────────────────────────────────────────┐
│  Main Panel (640×128)                                            │
│                                                                  │
│  [Level Info] [── Spell Quickbar (4 slots) ──] [Existing Belt]  │
│  "Caves Lv5"   [🔥][💚][⚡][🌀]                    [1][2][3][4]...│
│  "Nightmare"                                                    │
└──────────────────────────────────────────────────────────────────┘
```

| Zone | Width | Content |
|------|-------|---------|
| Left (level info) | 60px | Level name + difficulty, 2 lines |
| Center (quickbar) | 140px | 4 spell slots, evenly spaced |
| Right (belt) | remainder | Existing belt, no layout change |

## Spell Quickbar

### Data Model

```cpp
struct SkillSlot {
    SpellID spellId;       // Bound spell (SPELL_INVALID = empty)
    SpellType spellType;   // Skill / Spell / Scroll / Charges
    bool isActive;         // Currently selected ready-spell
    int charges;           // Remaining scroll/charge count
};
```

Source: `MyPlayer->_pSplHotKey[0..3]` and `MyPlayer->_pRSpell` (no new data — purely visual reflection of existing F5-F8 bindings).

### Rendering

1. For each slot, if `spellId != INVALID`: draw spell icon from spell icon resource
2. If `isActive`: draw gold highlight border
3. If empty: draw dim background frame
4. Bottom-right of each icon: small charge count number (for Scroll/Charges types)
5. Hover → `SetFloatingInfo(spell details, PanelElement, icon center)`

### Interaction

- Click slot → `SetSpell(player, slot.spellId, slot.spellType)` (equivalent to pressing F5-F8)
- Drag spellbook onto empty slot → bind that spell (equivalent to Shift+F5)

## Level Info Bar

### Data Sources

```cpp
const char* LevelNames[] = {"Tristram", "Catacombs", "Caves", "Hell", "Hive", "Crypt"};
const char* DifficultyNames[] = {"Normal", "Nightmare", "Hell"};
```

### Display Rules

| Context | Line 1 | Line 2 |
|---------|--------|--------|
| Town | "Tristram" | (none) |
| Dungeon | "Catacombs Level 3" | "Nightmare" |
| Multiplayer | Same as dungeon | No difficulty line shown |

### Rendering

Two lines, `GameFont12` font, `UiFlags::ColorButtonface`. Line height 12px + 2px spacing. No animation on level transitions (loading screen handles the transition).

## Implementation Phases

### Phase 1: Floating Tooltip Engine (`qol/floating_info`)
- New file, zero dependencies on other changes
- `SetFloatingInfo()`, `GetFloatingInfoRect()`, clamping logic
- Unit tests first

### Phase 2: Migrate InfoString → FloatingInfoString
- File-by-file: `items.cpp`, `objects.cpp`, `monster.cpp`, `trigs.cpp`, `cursor.cpp`, `quests.cpp`
- Each file gets a corresponding test case verifying content parity
- `control_infobox.cpp` rewritten last (depends on all helpers)

### Phase 3: Remove Old Info Panel
- Remove `InfoBoxRect` constant
- Remove `PrintInfo()` function
- Remove `DrawInfoBox()` calls from `scrollrt.cpp` and `control_panel.cpp`
- Panic if any `InfoString` assignment is found at compile time

### Phase 4: Spell Quickbar
- New `qol/skill_bar.cpp/hpp`
- Unit tests for SkillSlot model, charge display, empty state
- Render integration in `scrollrt.cpp`

### Phase 5: Level Info Bar
- New `panels/level_info.cpp/hpp`
- Unit tests for level name mapping, difficulty text, town special case, multiplayer mode

### Phase 6: Integration & Regression
- End-to-end: hover monster → floating box at monster head
- End-to-end: hover panel button → floating box at button
- End-to-end: click quickbar slot → spell changes
- Verify `DrawInfoBox` never called (compile-time check)

## Test Strategy

### Unit Tests (Phase 1)

```cpp
// FloatingInfoEngineTest
CursorMode_AnchorNearMouse      // Box appears near cursor
ClampOffScreenRightEdge         // Box doesn't exceed screen width
ClampOffScreenBottomEdge        // Box doesn't exceed screen height
WorldEntity_ConvertsToScreen    // World coords → screen coords
PanelElement_UsesPanelOffset    // Panel element anchored correctly
MultiLine_HasCorrectHeight      // Height accounts for all lines
EmptyString_EmptyRect           // Empty text → zero-size rect
```

### Migration Tests (Phase 2)

```cpp
// InfoMigrationTest
GetItemStr_GroundGold_FormatsGoldPieces    // "42 gold pieces"
GetItemStr_GroundItem_ShowsNameAndColor    // Item name + quality color
GetObjectStr_TrappedChest_ShowsWarning     // "Trapped Chest" in red
GetObjectStr_DisabledShrine_ShowsDisabled  // "(disabled)" suffix
PrintMonstHistory_LowKills_NoResistance    // <15 kills → no resist info
PrintMonstHistory_HighKills_ShowsHP        // ≥30 kills → HP range shown
PrintUniqueHistory_ShowsResistancesIfAny   // Resist/immune info when applicable
ForceTownTrig_UpStairs_FormatCorrect        // "Up to level 4"
ForceTownTrig_Town_ShowsTown               // "Up to town"
```

### Quickbar Tests (Phase 4)

```cpp
// SkillBarTest
Slot_EmptyWhenNoBinding            // Unbound slot → empty
Slot_HasIconWhenBound              // Bound slot → icon rendered
Slot_HighlightedWhenActive         // Current ready spell → gold border
Slot_ChargeNumberForScroll         // Scroll type → "3" overlay
Slot_ChargeNumberForStaffCharges   // Charges type → "12" overlay
Slot_NoChargeNumberForLearnedSpell // Skill/Spell type → no number
Slot_ClickSetsReadySpell           // Click → SetSpell() called
```

### Integration Tests (Phase 6)

```cpp
// IntegrationTest
MonsterHover_FloatingBoxAtMonsterHead    // World entity positioning
ButtonHover_FloatingBoxAtButton          // Panel element positioning  
QuickbarClick_SpellChanges               // Click → spell ready changes
DrawInfoBox_NeverCalled                  // Compile-time assertion
```

## Edge Cases & Error Handling

| Scenario | Behavior |
|----------|----------|
| Spell slot unbound | Empty slot with dim border |
| Scroll with 0 charges | Gray icon + "Depleted" tooltip |
| Staff with 0 charges | Gray icon + "Depleted" tooltip |
| Level info in town | "Tristram", no difficulty line |
| Level info in multiplayer | Level name only, no difficulty |
| Target entity off-screen | No floating box rendered |
| Floating text is empty | No box rendered |
| Unknown monster (first encounter, <15 kills) | Name only, no kill count |
| Window loses focus | PauseOnFocusLoss applies; floating boxes not re-rendered |
| Headless mode | All floating/quickbar rendering skipped |

## Mode Compatibility

| Mode | All Features |
|------|-------------|
| Single player | ✓ Full |
| Multiplayer | ✓ Full (floating shows other player info) |
| Hellfire | ✓ Quickbar compatible with HF spells (Search, Reflect, etc.) |
| Headless | ✓ All drawing skipped |
| Gamepad | ✓ Quickbar supports gamepad focus navigation |

## Render Order

```
1. World terrain (floor, walls, decorations)
2. Monsters / NPCs
3. Ground items
4. Monster health bars (existing qol)
5. World entity floating boxes (NEW - monsters, items, objects, players)
6. Main panel (inventory, belt, buttons, stash)
7. Spell quickbar (NEW)
8. Level info bar (NEW)
9. Panel element floating boxes (NEW - buttons, slots)
10. Mouse cursor
```

This ensures floating boxes appear above all other UI elements with correct z-ordering.

## Non-Goals

- Adding new spell shortcut slots beyond F5-F8
- Item quick-use slots (belt handles this)
- Right-click skill separation (D2 dual-skill model)
- Any gameplay mechanic changes — display only
