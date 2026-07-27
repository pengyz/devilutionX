> **分类**：Base　**状态**：已实施　**评判基准**：`2026-07-27-better-d1-design-charter.md`

# Skill Description System v2 — Complete & Aligned

## Problem

The v1 implementation (2026-07-05) added skill descriptions but has gaps:
- TSV descriptions incomplete: only 22/37 Diablo spells, 6/50 Hellfire spells have descriptions
- Upgrade preview format doesn't match spec: missing Mana delta, uses verbose `→` format
- ItemRepair warning hardcoded in English, not translated
- Spell book hover missing Mana cost display
- Description appears after hotkey info in spell list, should be before

## Design

### Data Layer — TSV Completion

**Diablo TSV** (`assets/txtdata/spells/spelldat.tsv`):

Complete descriptions for all learnable spells (bookLevel > 0 or obtainable via staff/quest):
- FireWall, TownPortal, StoneCurse, Phasing, ManaShield, ChainLightning, FlameWave, Elemental, ChargedBolt, HolyBolt, Resurrect, Telekinesis, HealOther, BloodStar, BoneSpirit

Leave empty (unobtainable cut content):
- DoomSerpents, BloodRitual, Invisibility (bookLevel=-1, staffLevel=-1, marked `// unused`)

Already complete (no change):
- Firebolt, Healing, Lightning, Flash, Identify, Infravision, Fireball, Guardian, Nova, Inferno, Golem, Rage, Teleport, Apocalypse, Etherealize, ItemRepair, StaffRecharge, TrapDisarm

**Hellfire TSV** (`mods/Hellfire/txtdata/spells/spelldat.tsv`):

Complete descriptions for all Hellfire-exclusive spells:
- LightningWall, Immolation, Warp, Reflect, Berserk, RingOfFire, Search, RuneOfFire, RuneOfLight, RuneOfNova, RuneOfImmolation, RuneOfStone, Mana, Magi, Jester

Inherit Diablo descriptions for shared spells (copy from Diablo TSV).

### Display Layer — Upgrade Preview

**Format**: `Next: {newMin}({deltaMin})-{newMax}({deltaMax})    Mana: {newMana} ({deltaMana})`

Example for Firebolt at level 3 (current: Damage 12-24, Mana 6; next: Damage 16-30, Mana 5):
```
Firebolt
Level: 3 / 15
Damage: 12-24    Mana: 6
Next: 16(+4)-30(+6)    Mana: 5 (-1)
Fires a bolt of fire at a single target.
```

**Classification logic**:

1. **Damage spells** (GetDamageAmt returns non-1): Show `Damage: min-max` + `Next: ...` with deltas
2. **Healing spells** (Healing/HealOther): Show `Heals: min-max` + `Next: ...` with deltas
3. **BoneSpirit**: Show `Dmg: 1/3 target hp` (fixed effect, no upgrade preview)
4. **Utility spells** (GetDamageAmt returns -1): No damage line, show only `Mana: X (delta)` if mana changes

**Delta calculation**:
- minDelta = nextMin - currentMin
- maxDelta = nextMax - currentMax
- manaDelta = nextMana - currentMana
- Display as signed integer: `+4`, `-1`, `+0`

**When to show**:
- Only when `currentLevel > 0 && currentLevel < MaxSpellLevel (15)`
- At max level: no preview shown
- At level 0: show "Unusable" (existing behavior)

### Bug Fixes

**3.1 ItemRepair warning translation**
- Current: `AddInfoBoxString(_("Warning: reduces max durability!"));`
- Fix: Already uses `_()`, verify it's in the translation template

**3.2 Spell book hover Mana display**
- Add `Mana: X` line after Level line in hover tooltip
- Use `GetManaAmount(player, sn) >> 6` (same as existing spell book display)

**3.3 Spell list description position**
- Move description append before hotkey info block
- Current order: type → level → charges → hotkey → description
- New order: type → level → charges → description → hotkey

**3.4 Spell book hover detection**
- Verify `entryRect` uses `textPaddingTop` correctly
- Test hover area alignment with visual spell entries

### Testing

**spelldat_test.cpp** (extend existing):
- Test: all learnable Diablo spells have non-empty description
- Test: all learnable Hellfire spells have non-empty description
- Test: cut content spells (DoomSerpents/BloodRitual/Invisibility) have empty description

**spell_list_test.cpp** (new):
- Test: hover shows description in FloatingInfoString
- Test: description appears before hotkey info

**spell_book_hover_test.cpp** (new):
- Test: hover detection works for valid spells
- Test: upgrade preview format matches `Next: X(+d)-Y(+d)    Mana: Z (d)`
- Test: utility spells show only Mana delta
- Test: BoneSpirit shows `Dmg: 1/3 target hp`
- Test: ItemRepair shows warning

## Files Changed

| File | Change |
|---|---|
| `assets/txtdata/spells/spelldat.tsv` | Add descriptions for 15 Diablo spells |
| `mods/Hellfire/txtdata/spells/spelldat.tsv` | Add descriptions for ~40 Hellfire spells + inherit Diablo |
| `Source/panels/spell_book.cpp` | Upgrade preview format, Mana display, delta calculation |
| `Source/panels/spell_list.cpp` | Move description before hotkey |
| `test/spelldat_test.cpp` | Extend: description coverage tests |
| `test/spell_list_test.cpp` | New: description display tests |
| `test/spell_book_hover_test.cpp` | New: hover + upgrade preview tests |
| `CMake/Tests.cmake` | Register new test files |

## Non-Goals

- No font changes (arrows not needed, using `(+d)` format instead)
- No layout changes to existing UI
- No new translation strings beyond what's already in the codebase
- No changes to spell mechanics or balance

## Success Criteria

1. All learnable spells in both Diablo and Hellfire have English descriptions
2. Upgrade preview shows correct deltas for damage, healing, and mana
3. All bug fixes verified by tests
4. No regression in existing spell UI behavior
