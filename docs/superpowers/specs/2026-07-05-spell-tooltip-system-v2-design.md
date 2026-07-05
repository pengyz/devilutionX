# Spell Tooltip System v2: Data-Driven Skill Descriptions

## Problem

The v1 implementation (commit f2ed740e9) added `SpellData.sDescription` and basic tooltip rendering, but:

- **36/36 Diablo spells have empty descriptions** in spelldat.tsv — no information displayed
- **45/52 total spells lack descriptions** — only 7 Hellfire spells have text
- **Display logic is hardcoded** in spell_book.cpp (~80 lines) and spell_list.cpp (~50 lines) — adding/modifying a spell's tooltip requires C++ changes
- **Spell list bottom bar shows no damage/mana** for any spell type
- **Skill type shows only "{name} Skill"** — zero numeric info
- **23 utility spells show nothing** (GetDamageAmt returns {-1,-1})
- **const_cast hack** for mana upgrade preview (temporarily modifies const player data)
- **Debug fprintf statements** left in production code
- **No structured upgrade preview** — current implementation uses raw `->` arrows

## Goal

A data-driven spell tooltip system inspired by D2's SkillDesc.txt architecture, where tooltip content is configured in TSV files and rendered by a centralized builder. Changes to spell display require TSV edits, not C++ recompilation.

## Design Principles

1. **D2-aligned architecture**: Separate mechanism layer (spelldat.tsv) from presentation layer (spelldesc.tsv)
2. **Pragmatic scope**: Use Lua 5.4.7 (already embedded) as expression engine instead of building a custom formula parser
3. **TDD-driven**: All core logic (expression evaluation, formatting, assembly) tested before implementation
4. **Single source of truth**: One spelldesc.tsv drives both spell_book and spell_list tooltips

## Architecture

```
spelldat.tsv (mechanism)     spelldesc.tsv (presentation)
┌──────────────────┐        ┌──────────────────────────────┐
│ id, name, mana   │        │ spellId  section  expression │
│ flags, missiles  │◄──────►│ format   textKey  priority   │
│ bookLvl, staffLvl│        │ ...52 spells x ~3-5 lines    │
│ description      │        └──────────────┬───────────────┘
└──────────────────┘                       │
                                           ▼
                                 ┌──────────────────────┐
                                 │ spell_tooltip.cpp    │
                                 │                      │
                                 │ LoadSpellDescData()  │
                                 │ EvaluateSpellExpr()  │ ← Lua 5.4.7
                                 │ FormatDescLine()     │
                                 │ BuildSpellTooltip()  │
                                 │ BuildSpellListTooltip│
                                 └──────────┬───────────┘
                                            │
                               ┌────────────┴────────────┐
                               ▼                         ▼
                        spell_book.cpp            spell_list.cpp
                        (upgrade preview)         (structured stats)
```

## spelldesc.tsv Format

### Columns

| Column | Type | Description |
|--------|------|-------------|
| spellId | string | SpellID name (must match spelldat.tsv) |
| section | enum | `desc` / `upgrade` / `warning` |
| priority | int | Display order (lower = first) |
| formatType | enum | Output format (see below) |
| expression | string | Lua expression for computing value (empty = use textKey only) |
| textKey | string | Display label (e.g. "Damage") or fixed text (for special/text types) |
| formulaText | string | (Optional) Alt-key formula display text |

### Section Enum

| Section | Display in spell_book | Display in spell_list |
|---------|----------------------|----------------------|
| `desc` | Yes | Yes |
| `upgrade` | Yes (LOL-style "Next Level:") | No |
| `warning` | Yes | Yes |

### formatType Enum

| formatType | Input | Output Example |
|------------|-------|----------------|
| `damage_range` | expression → min, max | "Damage: 12 - 24" |
| `heal_range` | expression → min, max | "Heals: 30 - 60" |
| `value_single` | expression → int | "Duration (frames): 150" |
| `value_delta` | expression at lvl and lvl+1 | "Duration: 150 → 175" |
| `mana` | mana at current level | "Mana: 6" |
| `mana_delta` | mana at lvl and lvl+1 | "Mana: 6 → 5" |
| `text` | sDescription or textKey | (flavor text) |
| `special` | none | textKey as-is (e.g. "Dmg: 1/3 target hp") |
| `level_display` | none | "Level 5 / 15" |

### Example TSV Rows

```tsv
spellId	section	priority	formatType	expression	textKey	formulaText
Firebolt	desc	1	level_display			Level {cur} / {max}
Firebolt	desc	2	damage_range	damage	Damage	Magic/8 + Level + 1
Firebolt	desc	3	mana	mana	Mana	Base - Level * 0.5
Firebolt	desc	4	text			Fires a bolt of fire at a single target.
Firebolt	upgrade	1	damage_delta	damage	Damage
Firebolt	upgrade	2	mana_delta	mana	Mana
Healing	desc	1	level_display			Level {cur} / {max}
Healing	desc	2	heal_range	damage	Heals
Healing	desc	3	mana	mana	Mana
Healing	desc	4	text			Restores some of the caster's health.
TownPortal	desc	1	level_display			Level {cur} / {max}
TownPortal	desc	2	mana	mana	Mana
TownPortal	desc	3	text			Opens a portal to the town.
StoneCurse	desc	1	level_display			Level {cur} / {max}
StoneCurse	desc	2	mana	mana	Mana
StoneCurse	desc	3	value_single	par1 + (lvl - 1) * par5	Duration (frames)
StoneCurse	upgrade	1	mana_delta	mana	Mana
StoneCurse	upgrade	2	value_delta	par1 + (lvl - 1) * par5	Duration
BoneSpirit	desc	1	level_display			Level {cur} / {max}
BoneSpirit	desc	2	special		Dmg: 1/3 target hp
BoneSpirit	desc	3	mana	mana	Mana
ItemRepair	desc	1	mana	mana	Mana
ItemRepair	warning	1	text		Warning: reduces max durability!
ChargedBolt	desc	1	level_display			Level {cur} / {max}
ChargedBolt	desc	2	damage_range	damage	Damage
ChargedBolt	desc	3	mana	mana	Mana
ChargedBolt	desc	4	value_single	par1 + math.floor(lvl / 3)	Bolts
ChargedBolt	upgrade	1	damage_delta	damage	Damage
ChargedBolt	upgrade	2	value_delta	par1 + math.floor(lvl / 3)	Bolts
ManaShield	desc	1	level_display			Level {cur} / {max}
ManaShield	desc	2	value_single	50 + lvl * 3	Absorb %
ManaShield	desc	3	mana	mana	Mana
ManaShield	upgrade	1	value_delta	50 + lvl * 3	Absorb %
ManaShield	upgrade	2	mana_delta	mana	Mana
```

Note: For `damage_range`/`heal_range`/`damage_delta`, the expression `"damage"` returns the `damage` table `{min, max}` from the Lua context. The formatter extracts both values. For other formatTypes, the expression returns a single int.

All 52 spells (36 Diablo + 16 Hellfire) must have desc rows with at least: level_display + mana + description text. Upgrade rows are optional (only for spells with meaningful level scaling).

## Lua Expression Engine

### Context Variables (available in expressions)

| Variable | Type | Source |
|----------|------|--------|
| `lvl` | int | `player.GetSpellLevel(spell)` |
| `par1`..`par8` | int | `SpellData` parameters |
| `damage` | table `{min, max}` | `GetDamageAmt(spell, lvl)` |
| `mana` | int | `GetManaAmount(player, spell) >> 6` |
| `charLevel` | int | `player._pLevel` |
| `magic` | int | `player._pMagic` |

### Helper Functions

| Function | Definition | Example |
|----------|------------|---------|
| `ln(a, b)` | `a + (lvl - 1) * b` | `ln(par1, par5)` at lvl5 = par1 + 4*par5 |

### Allowed Lua Standard Library

Only `math.floor`, `math.ceil`, `math.min`, `math.max`. No `io`, `os`, `require`, `load`, `dofile`, or metatable manipulation. Expressions execute in a sandboxed environment.

### Expression Return Type

Most expressions return a single `int`. For `damage_range` and `heal_range`, the expression must evaluate to a Lua table `{min=N, max=N}`. The formatter extracts both values.

Implementation: `EvaluateSpellExpr` checks the Lua return type — if it's a table, extract `min` and `max` fields; if it's a number, return as single int. Internally represented as:

```cpp
struct ExprResult {
    int value;      // single value (for value_single, mana, etc.)
    int minValue;   // for damage_range, heal_range, damage_delta
    int maxValue;   // for damage_range, heal_range, damage_delta
    bool isRange;   // true if result was a table {min, max}
};
```

For `damage_range`/`heal_range`, the expression is typically just `"damage"` (the table itself) or `"{min=X, max=Y}"` (an explicit table literal).

For `value_delta`/`mana_delta`, the formatter calls the expression at `lvl` and `lvl+1`, producing two single-int results.

### Error Handling

Invalid expressions log a warning and return 0. Tooltip displays "Error" for that line. No crash, no assertion failure.

## SpellTooltipBuilder API

```cpp
// spell_tooltip.h

enum class DescFormat : uint8_t {
    DamageRange, HealRange, ValueSingle, ValueDelta,
    Mana, ManaDelta, Text, Special, LevelDisplay,
};

enum class DescSection : uint8_t { Desc, Upgrade, Warning };

struct ExprResult {
    int value;      // single value (for value_single, mana, etc.)
    int minValue;   // for range types (damage_range, heal_range)
    int maxValue;   // for range types (damage_range, heal_range)
    bool isRange;   // true if result was a Lua table {min, max}
};

struct SpellDescLine {
    SpellID spellId;
    DescSection section;
    uint8_t priority;
    DescFormat format;
    std::string expression;   // Lua expression (empty = use textKey)
    std::string textKey;      // Label or fixed text
    std::string formulaText;  // Alt-key formula display (optional)
};

struct SpellTooltip {
    std::string title;
    std::vector<std::string> lines;
};

tl::expected<void, std::string> LoadSpellDescData();
std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section);
ExprResult EvaluateSpellExpr(const std::string &expr, const Player &player, SpellID spell, int level);
std::string FormatDescLine(const SpellDescLine &line, const Player &player, SpellID spell, int level);
SpellTooltip BuildSpellTooltip(const Player &player, SpellID spell);
SpellTooltip BuildSpellListTooltip(const Player &player, SpellID spell);
```

### BuildSpellTooltip Output (spell book)

```
Title: "Firebolt"
Lines:
  "Level 5 / 15"
  "Damage: 12 - 24"
  "Mana: 6"
  "Fires a bolt of fire at a single target."
  "Next Level:"
  "  Damage: 12-24 → 16-30"
  "  Mana: 6 → 5"
```

### BuildSpellListTooltip Output (bottom bar)

```
Title: "Firebolt Spell"
Lines:
  "Spell Level 5"
  "Damage: 12 - 24"
  "Mana: 6"
  "Fires a bolt of fire at a single target."
```

No upgrade section. Spell list also appends hotkey info after the tooltip (handled by spell_list.cpp, not the builder).

## GetManaAmount Override

New overload to fix the const_cast hack:

```cpp
// spells.h
int GetManaAmount(const Player &player, SpellID spell, int overrideSpellLevel);
```

Same logic as existing `GetManaAmount` but reads spellLevel from parameter instead of `player._pSplLvl`. The existing overload delegates to this one: `return GetManaAmount(player, spell, player.GetSpellLevel(spell))`.

## Rendering Integration

### spell_book.cpp

Replace ~60 lines of hardcoded hover logic with:

```cpp
if (entryRect.contains(MousePosition)) {
    hoveredSpellFound = true;
    SpellTooltip tooltip = BuildSpellTooltip(player, sn);
    FloatingInfoString = tooltip.title;
    for (const auto &line : tooltip.lines)
        AddInfoBoxString(line);
}
```

### spell_list.cpp

Replace ~50 lines of switch-case with:

```cpp
if (spellListItem.isSelected) {
    SpellTooltip tooltip = BuildSpellListTooltip(myPlayer, spellId);
    FloatingInfoString = tooltip.title;
    for (const auto &line : tooltip.lines)
        AddInfoBoxString(line);
    // hotkey stays as-is, appended after
}
```

### Alt Key Formula Display (deferred)

The `formulaText` column in spelldesc.tsv stores formula descriptions. When Alt is held, FormatDescLine returns formulaText instead of the numeric value. This is a post-MVP feature — the column exists in TSV but rendering can be added later.

## Cleanup

- Remove `fprintf(stderr, "[DBG]...")` from spell_book.cpp and spell_list.cpp
- Remove `const_cast` hack in spell_book.cpp (replaced by GetManaAmount overload)
- Remove `#include <cstdio>` from spell_book.cpp and spell_list.cpp

## Testing Strategy

### Layer 1: Expression Evaluation (unit tests)

```
// Single value expressions
EvaluateSpellExpr("42", player, spell, 1).value → 42
EvaluateSpellExpr("lvl * 2", player, spell, 5).value → 10
EvaluateSpellExpr("par1 + par2", player, spell, 1).value → par1 + par2
EvaluateSpellExpr("ln(par1, par5)", player, spell, 3).value → par1 + 2*par5
EvaluateSpellExpr("mana", player, spell, 1).value → GetManaAmount(player, spell) >> 6
EvaluateSpellExpr("math.floor(lvl / 3)", player, spell, 7).value → 2

// Range expression (returns table {min, max})
EvaluateSpellExpr("damage", player, spell, 5).isRange → true
EvaluateSpellExpr("damage", player, spell, 5).minValue → GetDamageAmt(spell, 5).min
EvaluateSpellExpr("damage", player, spell, 5).maxValue → GetDamageAmt(spell, 5).max

// Error handling
EvaluateSpellExpr("invalid!!!", player, spell, 1).value → 0 (no crash)
```

### Layer 2: Data Loading (unit tests)

```
LoadSpellDescData() succeeds
GetSpellDescLines(Firebolt, Desc) returns >= 2 lines
GetSpellDescLines(Firebolt, Upgrade) returns >= 1 line
GetSpellDescLines(Invalid, Desc) returns empty
Lines within a section are ordered by priority
```

### Layer 3: Formatting (unit tests)

```
FormatDescLine(DamageRange, "damage", "Damage", spell, 5)
  → contains "Damage:" and "-"
FormatDescLine(HealRange, "damage", "Heals", spell, 3)
  → contains "Heals:" and "-"
FormatDescLine(ValueSingle, "par1 + (lvl-1) * par5", "Duration", spell, 5)
  → contains "Duration:" and numeric value
FormatDescLine(ValueDelta, "mana", "Mana", spell, 5)
  → contains "→"
FormatDescLine(Mana, "mana", "Mana", spell, 1)
  → contains "Mana:" and numeric value
FormatDescLine(Special, "", "Dmg: 1/3 target hp", spell, 1)
  → equals "Dmg: 1/3 target hp"
FormatDescLine(Text, "", "", spell, 1)
  → equals sDescription
FormatDescLine(LevelDisplay, "", "", spell, 5)
  → contains "Level" and "5" and "15"
```

### Layer 4: Assembly (unit tests)

```
BuildSpellTooltip title is non-empty
BuildSpellTooltip lines.size() >= 2
BuildSpellTooltip includes upgrade lines (contains "→")
BuildSpellListTooltip has no upgrade lines
Utility spell (TownPortal) tooltip has no "Damage:" line
Level 0 spell shows "Unusable"
```

### Layer 5: Rendering (manual verification)

- Hover over each spell in spell book → correct tooltip
- Hover over each spell in bottom bar → correct tooltip
- Verify upgrade preview shows delta for damage/mana/other variables
- Verify utility spells show mana + description
- Verify Skill type shows damage/mana info
- Verify no debug output in stderr

## File Changes

### New Files

| File | Purpose |
|------|---------|
| `txtdata/spells/spelldesc.tsv` | Diablo spell presentation config |
| `mods/Hellfire/txtdata/spells/spelldesc.tsv` | Hellfire spell presentation config |
| `Source/spell_tooltip.h` | Builder API declarations |
| `Source/spell_tooltip.cpp` | Core logic: load, evaluate, format, assemble |
| `Source/lua/modules/spells.lua` | Lua context registration for spell expressions |
| `test/spell_tooltip_test.cpp` | TDD tests (Layers 1-4) |

### Modified Files

| File | Change |
|------|--------|
| `Source/panels/spell_book.cpp` | Replace ~60 lines hardcoded with 10-line builder call; remove debug logs |
| `Source/panels/spell_list.cpp` | Replace ~50 lines switch-case with 8-line builder call; remove debug logs |
| `Source/spells.h` | Add `GetManaAmount` overload with `overrideSpellLevel` param |
| `Source/spells.cpp` | Implement `GetManaAmount` overload |
| `Source/lua/lua_global.cpp` | Register spells module |
| `Source/tables/spelldat.h` | Expose par1-8 array in SpellData (if not already present) |
| `CMakeLists.txt` | Add spell_tooltip.cpp to build |
| `.gitignore` | Keep `.omo/` entry, add trailing newline |

### Unchanged Files

| File | Reason |
|------|--------|
| `Source/missiles.cpp` | GetDamageAmt unchanged — expression references it via `damage.min/max` |
| `Source/tables/spelldat.cpp` | LoadSpellData unchanged |
| `Source/control/control_infobox.cpp` | FloatingInfoString rendering unchanged |

## TDD Implementation Order

```
1.  Write Layer 1 tests (expression evaluation)     → RED
2.  Implement EvaluateSpellExpr (Lua binding)        → GREEN
3.  Write Layer 2 tests (data loading)               → RED
4.  Implement LoadSpellDescData + GetSpellDescLines   → GREEN
5.  Write Layer 3 tests (formatting)                 → RED
6.  Implement FormatDescLine                         → GREEN
7.  Write Layer 4 tests (assembly)                   → RED
8.  Implement BuildSpellTooltip + BuildSpellListTooltip → GREEN
9.  Refactor spell_book.cpp integration              → manual verify
10. Refactor spell_list.cpp integration              → manual verify
11. Cleanup: const_cast hack + debug logs            → full test suite
12. Write spelldesc.tsv for all 52 spells            → manual verify
```

## Out of Scope (Deferred)

- **Alt key formula display**: TSV column exists, rendering deferred
- **Chinese translation .po files**: Independent task
- **Synergy system** (D2 dsc3 section): High complexity, separate feature
- **Spell list upgrade preview**: By design, only shown in spell book
