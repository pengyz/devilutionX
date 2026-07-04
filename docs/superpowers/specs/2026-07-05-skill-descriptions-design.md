# Skill Description & Upgrade Preview System

## Problem

Diablo 1's skill descriptions are too vague. Players cannot see:
- What a skill actually does (no description text)
- How spell damage/healing scales with level
- Side effects of skills (e.g., Repair reduces max durability)
- What the next skill level would improve

## Design

### Data Layer

**`SpellData` struct** — add `std::string sDescription` field.

**`spelldat.tsv`** — add `description` column (optional, empty string = no description shown).

**Load logic** (`LoadSpellData`): read new column after `staffMax`. No breaking change — old TSV files without the column are not supported by the new reader, but both Diablo and Hellfire TSV files are updated atomically with this change.

### Display Layer

**No layout changes to existing UI.** All new information is shown via the floating tooltip (`FloatingInfoString`).

#### Spell List (bottom action bar, `spell_list.cpp`)

When hovering over a spell/skill in the bottom bar:
```
Firebolt (Spell)
Level: 3
Damage: 12-24    Mana: 6
Fires a bolt of fire at a single target.    ← description via pgettext("spell_description", ...)
```

Implementation: after the type/level/charges info is added to `FloatingInfoString`, append description if non-empty.

#### Spell Book (`spell_book.cpp`)

When hovering over a spell entry in the spell book:
```
Firebolt
Level: 3 / 15
Damage: 12-24    Mana: 6
Next level: 16-30  Mana: 5  ▲              ← upgrade preview
Fires a bolt of fire...                     ← description
```

Upgrade preview:
- If `currentLevel < MaxSpellLevel (15)`: show `GetDamageAmt(spell, level+1)` vs current
- Show change arrows: `▲` = improvement, `▼` = regression
- If `currentLevel >= MaxSpellLevel`: no preview shown

Special cases:
- **ItemRepair**: `"Warning: reduces max durability!"` appended below description
- **Healing/HealOther**: show healing range instead of damage range
- **BoneSpirit**: "Dmg: 1/3 target hp"
- **Utility skills** (no damage): no damage line shown

### Hover Detection (Spell Book)

Each spell entry rectangle is `SpellBookDescription {250, 43}` starting at `{11, yp + 43}` relative to the spell panel. Iterate through entries during `DrawSpellBook`, check `entryRect.contains(MousePosition)`, and set `FloatingInfoString` + `ComparisonInfoString` for the hovered spell.

### Translation

All description text is wrapped with `pgettext("spell_description", ...)` to allow context-specific gettext translations. The .po files will need new entries with `msgctxt "spell_description" msgid "Fires a bolt..."`.

### Testing

| Test | What it verifies |
|---|---|
| `spelldat_test.cpp` | TSV loading with description column, empty description = no crash |
| `spell_list_test.cpp` | Description text appears in FloatingInfoString |
| `spell_book_hover_test.cpp` | Hover detection + upgrade preview text correct |

## Files Changed

| File | Change |
|---|---|
| `Source/tables/spelldat.h` | Add `std::string sDescription` |
| `Source/tables/spelldat.cpp` | Read `description` column |
| `assets/txtdata/spells/spelldat.tsv` | Add column + English descriptions |
| `mods/Hellfire/txtdata/spells/spelldat.tsv` | Add column + English descriptions |
| `Source/panels/spell_list.cpp` | Append description on hover |
| `Source/panels/spell_book.cpp` | Hover detection + upgrade preview |
| `test/spelldat/spelldat_test.cpp` | New: TSV loading tests |
| `CMake/Tests.cmake` | Register new tests |
