# Spell Tooltip System v2 — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace hardcoded spell tooltip logic with a data-driven system (spelldesc.tsv + Lua expressions) so spell display is configured in TSV, not C++.

**Architecture:** A new `spell_tooltip.cpp` module loads presentation config from `spelldesc.tsv`, evaluates Lua expressions for numeric values, and assembles tooltip text. Both `spell_book.cpp` and `spell_list.cpp` call this builder instead of hardcoding display logic.

**Tech Stack:** C++20, Lua 5.4.7 (sol2), Google Test, DataFile/RecordReader TSV parser

**Spec:** `docs/superpowers/specs/2026-07-05-spell-tooltip-system-v2-design.md`

## Global Constraints

- Lua expressions execute in sandboxed environment (no io/os/require/load)
- TSV parser has no quoting support — fields must not contain tabs
- `SpellData` currently has no `par1-8` array — must be added
- `GetDamageAmt(SpellID, int)` returns `DamageRange {int min, int max}` — unchanged
- `GetManaAmount(const Player&, SpellID)` returns raw mana (>>6 for display) — new overload needed
- Tests link against `test_main` (full engine), registered in `CMake/Tests.cmake`
- Use `UITest` fixture from `test/ui_test.hpp` for tests needing a Player object

---

### Task 1: Add sParam[8] to SpellData + Update Loader

**Files:**
- Modify: `Source/tables/spelldat.h:226-267` — add `uint32_t sParam[8]` to SpellData struct
- Modify: `Source/tables/spelldat.cpp:230-259` — read 8 new columns in LoadSpellData
- Modify: `txtdata/spells/spelldat.tsv` — add param1-param8 columns
- Modify: `mods/Hellfire/txtdata/spells/spelldat.tsv` — add param1-param8 columns

**Interfaces:**
- Produces: `SpellData.sParam[8]` — accessible by spell_tooltip.cpp and Lua module

- [ ] **Step 1: Add sParam to SpellData struct**

In `Source/tables/spelldat.h`, after `sStaffMax` (line ~261), add:

```cpp
uint32_t sParam[8];
```

- [ ] **Step 2: Initialize sParam in AddNullSpell**

In `Source/tables/spelldat.cpp`, in `AddNullSpell()` (lines 21-33), ensure the null spell has zeroed params. Since `SpellData` is default-constructed and `uint32_t` zero-initializes in aggregate init, verify the existing `SpellsData.emplace_back()` path zero-initializes. If not, add explicit zeroing.

- [ ] **Step 3: Read param columns in LoadSpellData**

In `Source/tables/spelldat.cpp`, after `reader.readString("description", ...)` (line 256), add:

```cpp
for (int i = 0; i < 8; i++) {
    reader.readOptionalInt(fmt::format("param{}", i + 1), item.sParam[i]);
}
```

Note: `readOptionalInt` leaves the value unchanged if the field is empty. Initialize `item.sParam` to zeros before reading (or rely on default zero-init).

Actually, since the params are at the END of the TSV (after description), the RecordReader reads fields sequentially. We need to add the columns after `description` in the TSV. But RecordReader reads in order, so we must read them in order too.

Revised approach — add params BEFORE description in TSV (so reading order matches). Or, since description is currently the last column, add params after it and read after description.

- [ ] **Step 4: Update spelldat.tsv — add param columns**

Add 8 new columns after `description` in both TSV files. Header:

```
id	name	soundId	bookCost10	staffCost10	manaCost	flags	bookLevel	staffLevel	minIntelligence	missiles	manaMultiplier	minMana	staffMin	staffMax	description	param1	param2	param3	param4	param5	param6	param7	param8
```

For each spell, set param values. Initial values (others default to 0):

| Spell | param1 | param5 | Notes |
|-------|--------|--------|-------|
| Firebolt | 0 | 0 | Uses GetDamageAmt directly |
| StoneCurse | 8 | 5 | Duration formula: par1 + (lvl-1)*par5 |
| ChargedBolt | 3 | 0 | Base bolt count: par1 + floor(lvl/3) |
| ManaShield | 0 | 0 | Uses expression `50 + lvl*3` directly |
| Others | 0 | 0 | Most spells use damage/mana refs |

- [ ] **Step 5: Build and run spelldat tests**

```bash
cmake --build build_vs18 --target spelldat_test -j
cd build_vs18 && ctest -R spelldat_test -V
```

Expected: All 3 existing spelldat tests pass (DescriptionLoaded, SpellCount, EmptyDescriptionIsValid).

- [ ] **Step 6: Commit**

```bash
git add Source/tables/spelldat.h Source/tables/spelldat.cpp txtdata/spells/spelldat.tsv mods/Hellfire/txtdata/spells/spelldat.tsv
git commit -m "feat(data): add sParam[8] array to SpellData for generic spell parameters"
```

---

### Task 2: Create spelldesc.tsv Data Files

**Files:**
- Create: `txtdata/spells/spelldesc.tsv` — Diablo spell presentation config
- Create: `mods/Hellfire/txtdata/spells/spelldesc.tsv` — Hellfire spell presentation config

**Interfaces:**
- Produces: TSV data consumed by `LoadSpellDescData()` in Task 4

- [ ] **Step 1: Create Diablo spelldesc.tsv**

Create `txtdata/spells/spelldesc.tsv` with all 36 Diablo spells. Each spell must have at least: `level_display` + `mana` + `text` (description). Spells with damage get `damage_range`/`heal_range`. Spells with meaningful level scaling get `upgrade` section rows.

```
spellId	section	priority	formatType	expression	textKey	formulaText
Firebolt	desc	1	level_display
Firebolt	desc	2	damage_range	damage	Damage
Firebolt	desc	3	mana	mana	Mana
Firebolt	desc	4	text
Firebolt	upgrade	1	damage_delta	damage	Damage
Firebolt	upgrade	2	mana_delta	mana	Mana
Healing	desc	1	level_display
Healing	desc	2	heal_range	damage	Heals
Healing	desc	3	mana	mana	Mana
Healing	desc	4	text
Healing	upgrade	1	heal_delta	damage	Heals
Healing	upgrade	2	mana_delta	mana	Mana
Lightning	desc	1	level_display
Lightning	desc	2	damage_range	damage	Damage
Lightning	desc	3	mana	mana	Mana
Lightning	desc	4	text
Lightning	upgrade	1	damage_delta	damage	Damage
Lightning	upgrade	2	mana_delta	mana	Mana
Flash	desc	1	level_display
Flash	desc	2	damage_range	damage	Damage
Flash	desc	3	mana	mana	Mana
Flash	upgrade	1	damage_delta	damage	Damage
Flash	upgrade	2	mana_delta	mana	Mana
Identify	desc	1	level_display
Identify	desc	2	mana	mana	Mana
Identify	desc	3	text
FireWall	desc	1	level_display
FireWall	desc	2	damage_range	damage	Damage
FireWall	desc	3	mana	mana	Mana
FireWall	upgrade	1	damage_delta	damage	Damage
FireWall	upgrade	2	mana_delta	mana	Mana
TownPortal	desc	1	level_display
TownPortal	desc	2	mana	mana	Mana
TownPortal	desc	3	text
StoneCurse	desc	1	level_display
StoneCurse	desc	2	mana	mana	Mana
StoneCurse	desc	3	value_single	par1 + (lvl - 1) * par5	Duration (frames)
StoneCurse	upgrade	1	mana_delta	mana	Mana
StoneCurse	upgrade	2	value_delta	par1 + (lvl - 1) * par5	Duration
Infravision	desc	1	level_display
Infravision	desc	2	mana	mana	Mana
Phasing	desc	1	level_display
Phasing	desc	2	mana	mana	Mana
ManaShield	desc	1	level_display
ManaShield	desc	2	mana	mana	Mana
ManaShield	upgrade	1	mana_delta	mana	Mana
Fireball	desc	1	level_display
Fireball	desc	2	damage_range	damage	Damage
Fireball	desc	3	mana	mana	Mana
Fireball	upgrade	1	damage_delta	damage	Damage
Fireball	upgrade	2	mana_delta	mana	Mana
Guardian	desc	1	level_display
Guardian	desc	2	damage_range	damage	Damage
Guardian	desc	3	mana	mana	Mana
Guardian	upgrade	1	damage_delta	damage	Damage
Guardian	upgrade	2	mana_delta	mana	Mana
ChainLightning	desc	1	level_display
ChainLightning	desc	2	damage_range	damage	Damage
ChainLightning	desc	3	mana	mana	Mana
ChainLightning	upgrade	1	damage_delta	damage	Damage
ChainLightning	upgrade	2	mana_delta	mana	Mana
FlameWave	desc	1	level_display
FlameWave	desc	2	damage_range	damage	Damage
FlameWave	desc	3	mana	mana	Mana
FlameWave	upgrade	1	damage_delta	damage	Damage
FlameWave	upgrade	2	mana_delta	mana	Mana
DoomSerpents	desc	1	level_display
DoomSerpents	desc	2	mana	mana	Mana
BloodRitual	desc	1	level_display
BloodRitual	desc	2	mana	mana	Mana
Nova	desc	1	level_display
Nova	desc	2	damage_range	damage	Damage
Nova	desc	3	mana	mana	Mana
Nova	upgrade	1	damage_delta	damage	Damage
Nova	upgrade	2	mana_delta	mana	Mana
Invisibility	desc	1	level_display
Invisibility	desc	2	mana	mana	Mana
Inferno	desc	1	level_display
Inferno	desc	2	damage_range	damage	Damage
Inferno	desc	3	mana	mana	Mana
Inferno	upgrade	1	damage_delta	damage	Damage
Inferno	upgrade	2	mana_delta	mana	Mana
Golem	desc	1	level_display
Golem	desc	2	damage_range	damage	Damage
Golem	desc	3	mana	mana	Mana
Rage	desc	1	level_display
Rage	desc	2	mana	mana	Mana
Teleport	desc	1	level_display
Teleport	desc	2	mana	mana	Mana
Apocalypse	desc	1	level_display
Apocalypse	desc	2	damage_range	damage	Damage
Apocalypse	desc	3	mana	mana	Mana
Etherealize	desc	1	level_display
Etherealize	desc	2	mana	mana	Mana
ItemRepair	desc	1	level_display
ItemRepair	desc	2	mana	mana	Mana
ItemRepair	warning	1	special		Warning: reduces max durability!
StaffRecharge	desc	1	level_display
StaffRecharge	desc	2	mana	mana	Mana
TrapDisarm	desc	1	level_display
TrapDisarm	desc	2	mana	mana	Mana
Elemental	desc	1	level_display
Elemental	desc	2	damage_range	damage	Damage
Elemental	desc	3	mana	mana	Mana
Elemental	upgrade	1	damage_delta	damage	Damage
Elemental	upgrade	2	mana_delta	mana	Mana
ChargedBolt	desc	1	level_display
ChargedBolt	desc	2	damage_range	damage	Damage
ChargedBolt	desc	3	mana	mana	Mana
ChargedBolt	desc	4	value_single	par1 + math.floor(lvl / 3)	Bolts
ChargedBolt	upgrade	1	damage_delta	damage	Damage
ChargedBolt	upgrade	2	value_delta	par1 + math.floor(lvl / 3)	Bolts
HolyBolt	desc	1	level_display
HolyBolt	desc	2	damage_range	damage	Damage
HolyBolt	desc	3	mana	mana	Mana
HolyBolt	upgrade	1	damage_delta	damage	Damage
HolyBolt	upgrade	2	mana_delta	mana	Mana
Resurrect	desc	1	level_display
Resurrect	desc	2	mana	mana	Mana
Telekinesis	desc	1	level_display
Telekinesis	desc	2	mana	mana	Mana
HealOther	desc	1	level_display
HealOther	desc	2	heal_range	damage	Heals
HealOther	desc	3	mana	mana	Mana
HealOther	upgrade	1	heal_delta	damage	Heals
HealOther	upgrade	2	mana_delta	mana	Mana
BloodStar	desc	1	level_display
BloodStar	desc	2	damage_range	damage	Damage
BloodStar	desc	3	mana	mana	Mana
BloodStar	upgrade	1	damage_delta	damage	Damage
BloodStar	upgrade	2	mana_delta	mana	Mana
BoneSpirit	desc	1	level_display
BoneSpirit	desc	2	special		Dmg: 1/3 target hp
BoneSpirit	desc	3	mana	mana	Mana
```

- [ ] **Step 2: Create Hellfire spelldesc.tsv**

Copy the Diablo file and add the 16 Hellfire-specific spells (Mana, Magi, Jester, LightningWall, Immolation, Warp, Reflect, Berserk, RingOfFire, Search, RuneOfFire, RuneOfLight, RuneOfNova, RuneOfImmolation, RuneOfStone). Each with at least `level_display` + `mana` + optional `text`.

- [ ] **Step 3: Commit**

```bash
git add txtdata/spells/spelldesc.tsv mods/Hellfire/txtdata/spells/spelldesc.tsv
git commit -m "feat(data): add spelldesc.tsv for all 52 spells"
```

---

### Task 3: Create Lua Spell Module

**Files:**
- Create: `Source/lua/modules/spells.hpp`
- Create: `Source/lua/modules/spells.cpp`
- Modify: `Source/lua/lua_global.cpp` — register module
- Modify: `Source/CMakeLists.txt` — add source file

**Interfaces:**
- Produces: `LuaSpellsModule(sol::state_view&)` — registered as `devilutionx.spells`
- Produces: Lua context variables `lvl`, `par1-8`, `damage{min,max}`, `mana`, `charLevel`, `magic`, `ln(a,b)`

- [ ] **Step 1: Create spells.hpp**

```cpp
#pragma once

#include <sol/sol.hpp>

namespace devilution {

sol::table LuaSpellsModule(sol::state_view &lua);

} // namespace devilution
```

- [ ] **Step 2: Create spells.cpp**

```cpp
#include "lua/modules/spells.hpp"

#include <sol/sol.hpp>

#include "lua/metadoc.hpp"

namespace devilution {

sol::table LuaSpellsModule(sol::state_view &lua)
{
    sol::table table = lua.create_table();
    // The module is a placeholder — actual expression evaluation
    // happens via sol::state::safe_script with a custom environment.
    // This module exposes helper functions if needed by mods.
    return table;
}

} // namespace devilution
```

- [ ] **Step 3: Register in lua_global.cpp**

Add include at top (around line 35):
```cpp
#include "lua/modules/spells.hpp"
```

Add to `commonPackages` table in `LuaInitialize()` (around line 315):
```cpp
"devilutionx.spells", LuaSpellsModule(lua),
```

- [ ] **Step 4: Add to CMakeLists.txt**

In `Source/CMakeLists.txt`, add to the lua/modules source list (around line 110):
```cmake
  lua/modules/spells.cpp
```

- [ ] **Step 5: Build to verify compilation**

```bash
cmake --build build_vs18 --target devilutionx -j 2>&1 | tail -5
```

Expected: Build succeeds with no errors.

- [ ] **Step 6: Commit**

```bash
git add Source/lua/modules/spells.hpp Source/lua/modules/spells.cpp Source/lua/lua_global.cpp Source/CMakeLists.txt
git commit -m "feat(lua): add spells module for tooltip expression evaluation"
```

---

### Task 4: Implement EvaluateSpellExpr (Layer 1 — TDD)

**Files:**
- Create: `Source/spell_tooltip.h`
- Create: `Source/spell_tooltip.cpp`
- Create: `test/spell_tooltip_test.cpp`
- Modify: `CMake/Tests.cmake` — add test
- Modify: `Source/CMakeLists.txt` — add source

**Interfaces:**
- Produces: `ExprResult EvaluateSpellExpr(const std::string &expr, const Player &player, SpellID spell, int level)`
- Consumes: `GetDamageAmt(SpellID, int)`, `GetManaAmount(const Player&, SpellID)`, `GetSpellData(SpellID)`

- [ ] **Step 1: Write failing tests for EvaluateSpellExpr**

Create `test/spell_tooltip_test.cpp`:

```cpp
#include <gtest/gtest.h>

#include "ui_test.hpp"
#include "spell_tooltip.h"
#include "tables/spelldat.h"
#include "missiles.h"
#include "spells.h"

namespace devilution {

class SpellTooltipTest : public UITest {
protected:
    static void SetUpTestSuite()
    {
        UITest::SetUpTestSuite();
    }
};

TEST_F(SpellTooltipTest, EvaluateSimpleNumber)
{
    ExprResult result = EvaluateSpellExpr("42", *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result.value, 42);
    EXPECT_FALSE(result.isRange);
}

TEST_F(SpellTooltipTest, EvaluateLvlVariable)
{
    ExprResult result = EvaluateSpellExpr("lvl * 2", *MyPlayer, SpellID::Firebolt, 5);
    EXPECT_EQ(result.value, 10);
}

TEST_F(SpellTooltipTest, EvaluateParVariable)
{
    const SpellData &sd = GetSpellData(SpellID::StoneCurse);
    ExprResult result = EvaluateSpellExpr("par1 + par5", *MyPlayer, SpellID::StoneCurse, 1);
    EXPECT_EQ(result.value, static_cast<int>(sd.sParam[0] + sd.sParam[4]));
}

TEST_F(SpellTooltipTest, EvaluateLnFunction)
{
    const SpellData &sd = GetSpellData(SpellID::StoneCurse);
    ExprResult result = EvaluateSpellExpr("ln(par1, par5)", *MyPlayer, SpellID::StoneCurse, 3);
    int expected = sd.sParam[0] + (3 - 1) * sd.sParam[4];
    EXPECT_EQ(result.value, expected);
}

TEST_F(SpellTooltipTest, EvaluateDamageRange)
{
    DamageRange dr = GetDamageAmt(SpellID::Firebolt, 5);
    ExprResult result = EvaluateSpellExpr("damage", *MyPlayer, SpellID::Firebolt, 5);
    EXPECT_TRUE(result.isRange);
    EXPECT_EQ(result.minValue, dr.min);
    EXPECT_EQ(result.maxValue, dr.max);
}

TEST_F(SpellTooltipTest, EvaluateManaRef)
{
    int expectedMana = GetManaAmount(*MyPlayer, SpellID::Firebolt) >> 6;
    ExprResult result = EvaluateSpellExpr("mana", *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result.value, expectedMana);
}

TEST_F(SpellTooltipTest, EvaluateMathFloor)
{
    ExprResult result = EvaluateSpellExpr("math.floor(7 / 3)", *MyPlayer, SpellID::ChargedBolt, 7);
    EXPECT_EQ(result.value, 2);
}

TEST_F(SpellTooltipTest, EvaluateInvalidExprReturnsZero)
{
    ExprResult result = EvaluateSpellExpr("invalid!!!", *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result.value, 0);
    EXPECT_FALSE(result.isRange);
}

TEST_F(SpellTooltipTest, EvaluateCharLevel)
{
    ExprResult result = EvaluateSpellExpr("charLevel", *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result.value, MyPlayer->getCharacterLevel());
}

TEST_F(SpellTooltipTest, EvaluateMagic)
{
    ExprResult result = EvaluateSpellExpr("magic", *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result.value, MyPlayer->_pMagic);
}

} // namespace devilution
```

- [ ] **Step 2: Add test to CMake/Tests.cmake**

Add `spell_tooltip_test` to the `tests` list (after `spelldat_test` at line 51):

```cmake
  spell_tooltip_test
```

- [ ] **Step 3: Build test to verify it fails (RED)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j 2>&1 | tail -10
```

Expected: Compilation error — `spell_tooltip.h` not found, `EvaluateSpellExpr` not defined.

- [ ] **Step 4: Create spell_tooltip.h with declarations**

```cpp
#pragma once

#include <string>
#include <cstdint>

#include "player.h"
#include "tables/spelldat.h"

namespace devilution {

struct ExprResult {
    int value;
    int minValue;
    int maxValue;
    bool isRange;
};

ExprResult EvaluateSpellExpr(const std::string &expr, const Player &player, SpellID spell, int level);

} // namespace devilution
```

- [ ] **Step 5: Create spell_tooltip.cpp with minimal implementation**

```cpp
#include "spell_tooltip.h"

#include <string>

#include <sol/sol.hpp>

#include "lua/lua_global.hpp"
#include "missiles.h"
#include "spells.h"
#include "utils/log.hpp"

namespace devilution {

ExprResult EvaluateSpellExpr(const std::string &expr, const Player &player, SpellID spell, int level)
{
    ExprResult result { 0, 0, 0, false };

    sol::state &lua = GetLuaState();
    sol::table ctx = lua.create_table();

    ctx["lvl"] = level;
    ctx["charLevel"] = player.getCharacterLevel();
    ctx["magic"] = player._pMagic;
    ctx["mana"] = GetManaAmount(player, spell) >> 6;

    auto [min, max] = GetDamageAmt(spell, level);
    ctx["damage"] = lua.create_table_with("min", min, "max", max);

    const SpellData &sd = GetSpellData(spell);
    for (int i = 0; i < 8; i++)
        ctx[fmt::format("par{}", i + 1)] = sd.sParam[i];

    int lvl = level;
    ctx.set_function("ln", [lvl](int a, int b) { return a + (lvl - 1) * b; });

    sol::environment env(lua, sol::create, lua.globals());
    // Restrict to safe math functions
    env["math"] = lua.create_table_with(
        "floor", [](double x) { return static_cast<int>(std::floor(x)); },
        "ceil", [](double x) { return static_cast<int>(std::ceil(x)); },
        "min", [](int a, int b) { return std::min(a, b); },
        "max", [](int a, int b) { return std::max(a, b); });

    // Copy context vars into environment
    for (auto &[k, v] : ctx)
        env[k] = v;

    sol::protected_function_result pfr = lua.safe_script(expr, env);
    if (!pfr.valid()) {
        sol::error err = pfr;
        LogError("SpellExpr eval error for '{}': {}", expr, err.what());
        return result;
    }

    if (pfr.get_type() == sol::type::table) {
        sol::table t = pfr;
        result.minValue = t.get_or("min", 0);
        result.maxValue = t.get_or("max", 0);
        result.value = result.minValue;
        result.isRange = true;
    } else if (pfr.get_type() == sol::type::number) {
        result.value = pfr.get<int>();
    }

    return result;
}

} // namespace devilution
```

- [ ] **Step 6: Add spell_tooltip.cpp to CMakeLists.txt**

In `Source/CMakeLists.txt`, add to the source list:

```cmake
  spell_tooltip.cpp
```

- [ ] **Step 7: Build and run tests (GREEN)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
cd build_vs18 && ctest -R spell_tooltip_test -V
```

Expected: All 10 tests pass.

- [ ] **Step 8: Commit**

```bash
git add Source/spell_tooltip.h Source/spell_tooltip.cpp test/spell_tooltip_test.cpp CMake/Tests.cmake Source/CMakeLists.txt
git commit -m "feat(tooltip): implement EvaluateSpellExpr with Lua binding (Layer 1 TDD)"
```

---

### Task 5: Implement LoadSpellDescData (Layer 2 — TDD)

**Files:**
- Modify: `Source/spell_tooltip.h` — add data types and load function
- Modify: `Source/spell_tooltip.cpp` — implement loading
- Modify: `test/spell_tooltip_test.cpp` — add Layer 2 tests

**Interfaces:**
- Produces: `tl::expected<void, std::string> LoadSpellDescData()`
- Produces: `std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section)`

- [ ] **Step 1: Add data types to spell_tooltip.h**

```cpp
enum class DescFormat : uint8_t {
    DamageRange, HealRange, ValueSingle, ValueDelta,
    Mana, ManaDelta, Text, Special, LevelDisplay,
    HealDelta,  // for heal spell upgrade preview
};

enum class DescSection : uint8_t { Desc, Upgrade, Warning };

struct SpellDescLine {
    SpellID spellId;
    DescSection section;
    uint8_t priority;
    DescFormat format;
    std::string expression;
    std::string textKey;
    std::string formulaText;
};

// Forward declare global storage
extern std::vector<SpellDescLine> SpellDescLines;

tl::expected<void, std::string> LoadSpellDescData();
std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section);
```

- [ ] **Step 2: Write failing tests for data loading**

Add to `test/spell_tooltip_test.cpp`:

```cpp
TEST_F(SpellTooltipTest, LoadSpellDescDataSucceeds)
{
    auto result = LoadSpellDescData();
    ASSERT_TRUE(result.has_value()) << result.error();
}

TEST_F(SpellTooltipTest, DescLinesExistForFirebolt)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
    ASSERT_GE(lines.size(), 2u); // at least damage + mana
}

TEST_F(SpellTooltipTest, UpgradeLinesExistForFirebolt)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Upgrade);
    EXPECT_FALSE(lines.empty());
}

TEST_F(SpellTooltipTest, UnknownSpellReturnsEmpty)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Invalid, DescSection::Desc);
    EXPECT_TRUE(lines.empty());
}

TEST_F(SpellTooltipTest, LinesOrderedByPriority)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
    for (size_t i = 1; i < lines.size(); i++) {
        EXPECT_GE(lines[i]->priority, lines[i - 1]->priority);
    }
}

TEST_F(SpellTooltipTest, UtilitySpellHasManaLine)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::TownPortal, DescSection::Desc);
    bool hasMana = false;
    for (const auto *line : lines) {
        if (line->format == DescFormat::Mana) hasMana = true;
    }
    EXPECT_TRUE(hasMana);
}
```

- [ ] **Step 3: Build test to verify it fails (RED)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
```

Expected: Compilation error — `LoadSpellDescData` not defined.

- [ ] **Step 4: Implement LoadSpellDescData and GetSpellDescLines**

Add to `spell_tooltip.cpp`:

```cpp
#include "data/file.hpp"
#include "data/record_reader.hpp"

namespace devilution {

std::vector<SpellDescLine> SpellDescLines;

namespace {

DescFormat ParseDescFormat(std::string_view value)
{
    if (value == "damage_range") return DescFormat::DamageRange;
    if (value == "heal_range") return DescFormat::HealRange;
    if (value == "value_single") return DescFormat::ValueSingle;
    if (value == "value_delta") return DescFormat::ValueDelta;
    if (value == "mana") return DescFormat::Mana;
    if (value == "mana_delta") return DescFormat::ManaDelta;
    if (value == "text") return DescFormat::Text;
    if (value == "special") return DescFormat::Special;
    if (value == "level_display") return DescFormat::LevelDisplay;
    if (value == "heal_delta") return DescFormat::HealDelta;
    app_fatal("Unknown DescFormat: {}", value);
    return DescFormat::Text;
}

DescSection ParseDescSection(std::string_view value)
{
    if (value == "desc") return DescSection::Desc;
    if (value == "upgrade") return DescSection::Upgrade;
    if (value == "warning") return DescSection::Warning;
    app_fatal("Unknown DescSection: {}", value);
    return DescSection::Desc;
}

SpellID ParseSpellIdForDesc(std::string_view value)
{
    // Reuse the same parsing as spelldat.cpp
    // Map string name to SpellID enum
    // (implementation mirrors ParseSpellId from spelldat.cpp)
    // ... full if-chain or lookup table
}

} // namespace

tl::expected<void, std::string> LoadSpellDescData()
{
    constexpr std::string_view filename = "txtdata\\spells\\spelldesc.tsv";
    SpellDescLines.clear();

    DataFile dataFile;
    auto loadResult = DataFile::load(filename);
    if (!loadResult.has_value()) {
        return tl::make_unexpected(fmt::format("Failed to load {}: {}", filename, loadResult.error()));
    }
    dataFile = std::move(*loadResult);

    dataFile.skipHeaderOrDie(filename);

    for (DataFileRecord record : dataFile) {
        RecordReader reader { record, filename };

        SpellDescLine line;

        // spellId
        std::string spellIdStr;
        reader.readString("spellId", spellIdStr);
        line.spellId = ParseSpellIdForDesc(spellIdStr);

        // section
        std::string sectionStr;
        reader.readString("section", sectionStr);
        line.section = ParseDescSection(sectionStr);

        // priority
        reader.readInt("priority", line.priority);

        // formatType
        std::string formatStr;
        reader.readString("formatType", formatStr);
        line.format = ParseDescFormat(formatStr);

        // expression
        reader.readString("expression", line.expression);

        // textKey
        reader.readString("textKey", line.textKey);

        // formulaText (optional — may be empty)
        reader.readString("formulaText", line.formulaText);

        SpellDescLines.push_back(std::move(line));
    }

    return {};
}

std::vector<const SpellDescLine *> GetSpellDescLines(SpellID spell, DescSection section)
{
    std::vector<const SpellDescLine *> result;
    for (const auto &line : SpellDescLines) {
        if (line.spellId == spell && line.section == section)
            result.push_back(&line);
    }
    std::sort(result.begin(), result.end(),
        [](const SpellDescLine *a, const SpellDescLine *b) { return a->priority < b->priority; });
    return result;
}

} // namespace devilution
```

- [ ] **Step 5: Build and run tests (GREEN)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
cd build_vs18 && ctest -R spell_tooltip_test -V
```

Expected: All Layer 1 + Layer 2 tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/spell_tooltip.h Source/spell_tooltip.cpp test/spell_tooltip_test.cpp
git commit -m "feat(tooltip): implement LoadSpellDescData + GetSpellDescLines (Layer 2 TDD)"
```

---

### Task 6: Implement FormatDescLine (Layer 3 — TDD)

**Files:**
- Modify: `Source/spell_tooltip.h` — add FormatDescLine declaration
- Modify: `Source/spell_tooltip.cpp` — implement FormatDescLine
- Modify: `test/spell_tooltip_test.cpp` — add Layer 3 tests

**Interfaces:**
- Produces: `std::string FormatDescLine(const SpellDescLine &line, const Player &player, SpellID spell, int level)`
- Consumes: `EvaluateSpellExpr()`, `GetSpellData()`

- [ ] **Step 1: Write failing tests for FormatDescLine**

Add to `test/spell_tooltip_test.cpp`:

```cpp
TEST_F(SpellTooltipTest, FormatDamageRange)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
    const SpellDescLine *dmgLine = nullptr;
    for (const auto *l : lines) {
        if (l->format == DescFormat::DamageRange) { dmgLine = l; break; }
    }
    ASSERT_NE(dmgLine, nullptr);
    std::string result = FormatDescLine(*dmgLine, *MyPlayer, SpellID::Firebolt, 5);
    EXPECT_THAT(result, testing::HasSubstr("Damage"));
    EXPECT_THAT(result, testing::HasSubstr("-"));
}

TEST_F(SpellTooltipTest, FormatHealRange)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Healing, DescSection::Desc);
    const SpellDescLine *healLine = nullptr;
    for (const auto *l : lines) {
        if (l->format == DescFormat::HealRange) { healLine = l; break; }
    }
    ASSERT_NE(healLine, nullptr);
    std::string result = FormatDescLine(*healLine, *MyPlayer, SpellID::Healing, 3);
    EXPECT_THAT(result, testing::HasSubstr("Heals"));
    EXPECT_THAT(result, testing::HasSubstr("-"));
}

TEST_F(SpellTooltipTest, FormatMana)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Desc);
    const SpellDescLine *manaLine = nullptr;
    for (const auto *l : lines) {
        if (l->format == DescFormat::Mana) { manaLine = l; break; }
    }
    ASSERT_NE(manaLine, nullptr);
    std::string result = FormatDescLine(*manaLine, *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_THAT(result, testing::HasSubstr("Mana"));
    int expectedMana = GetManaAmount(*MyPlayer, SpellID::Firebolt) >> 6;
    EXPECT_THAT(result, testing::HasSubstr(std::to_string(expectedMana)));
}

TEST_F(SpellTooltipTest, FormatSpecial)
{
    SpellDescLine line;
    line.format = DescFormat::Special;
    line.textKey = "Dmg: 1/3 target hp";
    line.expression = "";
    std::string result = FormatDescLine(line, *MyPlayer, SpellID::BoneSpirit, 1);
    EXPECT_EQ(result, "Dmg: 1/3 target hp");
}

TEST_F(SpellTooltipTest, FormatLevelDisplay)
{
    SpellDescLine line;
    line.format = DescFormat::LevelDisplay;
    line.expression = "";
    std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 5);
    EXPECT_THAT(result, testing::HasSubstr("Level"));
    EXPECT_THAT(result, testing::HasSubstr("5"));
    EXPECT_THAT(result, testing::HasSubstr("15"));
}

TEST_F(SpellTooltipTest, FormatValueDelta)
{
    LoadSpellDescData();
    auto lines = GetSpellDescLines(SpellID::Firebolt, DescSection::Upgrade);
    const SpellDescLine *manaDelta = nullptr;
    for (const auto *l : lines) {
        if (l->format == DescFormat::ManaDelta) { manaDelta = l; break; }
    }
    ASSERT_NE(manaDelta, nullptr);
    std::string result = FormatDescLine(*manaDelta, *MyPlayer, SpellID::Firebolt, 5);
    EXPECT_THAT(result, testing::HasSubstr("\xe2\x86\x92")); // → UTF-8
}

TEST_F(SpellTooltipTest, FormatTextShowsDescription)
{
    SpellDescLine line;
    line.format = DescFormat::Text;
    line.expression = "";
    line.textKey = "";
    std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result, GetSpellData(SpellID::Firebolt).sDescription);
}

TEST_F(SpellTooltipTest, FormatTextFromTextKey)
{
    SpellDescLine line;
    line.format = DescFormat::Text;
    line.expression = "";
    line.textKey = "Custom text here";
    std::string result = FormatDescLine(line, *MyPlayer, SpellID::Firebolt, 1);
    EXPECT_EQ(result, "Custom text here");
}
```

- [ ] **Step 2: Build test to verify it fails (RED)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
```

Expected: `FormatDescLine` not defined.

- [ ] **Step 3: Implement FormatDescLine**

Add to `spell_tooltip.cpp`:

```cpp
std::string FormatDescLine(const SpellDescLine &line, const Player &player, SpellID spell, int level)
{
    switch (line.format) {
    case DescFormat::LevelDisplay:
        return fmt::format("Level {:d} / {:d}", level, MaxSpellLevel);

    case DescFormat::Special:
        return line.textKey;

    case DescFormat::Text:
        if (!line.textKey.empty())
            return line.textKey;
        return std::string(GetSpellData(spell).sDescription);

    case DescFormat::Mana: {
        int mana = EvaluateSpellExpr(line.expression.empty() ? "mana" : line.expression, player, spell, level).value;
        return fmt::format("{:s}: {:d}", line.textKey, mana);
    }

    case DescFormat::ManaDelta: {
        int curMana = EvaluateSpellExpr(line.expression.empty() ? "mana" : line.expression, player, spell, level).value;
        int nextMana = EvaluateSpellExpr(line.expression.empty() ? "mana" : line.expression, player, spell, level + 1).value;
        return fmt::format("{:s}: {:d} \xe2\x86\x92 {:d}", line.textKey, curMana, nextMana);
    }

    case DescFormat::DamageRange: {
        ExprResult r = EvaluateSpellExpr(line.expression, player, spell, level);
        if (r.isRange)
            return fmt::format("{:s}: {:d} - {:d}", line.textKey, r.minValue, r.maxValue);
        return fmt::format("{:s}: {:d}", line.textKey, r.value);
    }

    case DescFormat::HealRange: {
        ExprResult r = EvaluateSpellExpr(line.expression, player, spell, level);
        if (r.isRange)
            return fmt::format("{:s}: {:d} - {:d}", line.textKey, r.minValue, r.maxValue);
        return fmt::format("{:s}: {:d}", line.textKey, r.value);
    }

    case DescFormat::ValueSingle: {
        ExprResult r = EvaluateSpellExpr(line.expression, player, spell, level);
        return fmt::format("{:s}: {:d}", line.textKey, r.value);
    }

    case DescFormat::ValueDelta: {
        ExprResult cur = EvaluateSpellExpr(line.expression, player, spell, level);
        ExprResult next = EvaluateSpellExpr(line.expression, player, spell, level + 1);
        return fmt::format("{:s}: {:d} \xe2\x86\x92 {:d}", line.textKey, cur.value, next.value);
    }

    case DescFormat::HealDelta: {
        ExprResult cur = EvaluateSpellExpr(line.expression, player, spell, level);
        ExprResult next = EvaluateSpellExpr(line.expression, player, spell, level + 1);
        if (cur.isRange && next.isRange)
            return fmt::format("{:s}: {:d}-{:d} \xe2\x86\x92 {:d}-{:d}", line.textKey, cur.minValue, cur.maxValue, next.minValue, next.maxValue);
        return fmt::format("{:s}: {:d} \xe2\x86\x92 {:d}", line.textKey, cur.value, next.value);
    }
    }
    return "";
}
```

Note: `MaxSpellLevel` is a constant (15). Verify its declaration in the codebase.

- [ ] **Step 4: Build and run tests (GREEN)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
cd build_vs18 && ctest -R spell_tooltip_test -V
```

Expected: All Layer 1 + 2 + 3 tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/spell_tooltip.h Source/spell_tooltip.cpp test/spell_tooltip_test.cpp
git commit -m "feat(tooltip): implement FormatDescLine for all format types (Layer 3 TDD)"
```

---

### Task 7: Implement BuildSpellTooltip (Layer 4 — TDD)

**Files:**
- Modify: `Source/spell_tooltip.h` — add builder declarations
- Modify: `Source/spell_tooltip.cpp` — implement builders
- Modify: `test/spell_tooltip_test.cpp` — add Layer 4 tests

**Interfaces:**
- Produces: `SpellTooltip BuildSpellTooltip(const Player &player, SpellID spell)`
- Produces: `SpellTooltip BuildSpellListTooltip(const Player &player, SpellID spell)`

- [ ] **Step 1: Write failing tests for BuildSpellTooltip**

Add to `test/spell_tooltip_test.cpp`:

```cpp
TEST_F(SpellTooltipTest, BuildTooltipTitle)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
    EXPECT_FALSE(tooltip.title.empty());
    EXPECT_THAT(tooltip.title, testing::HasSubstr("Firebolt"));
}

TEST_F(SpellTooltipTest, BuildTooltipHasLines)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
    EXPECT_GE(tooltip.lines.size(), 2u);
}

TEST_F(SpellTooltipTest, BuildTooltipIncludesUpgrade)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::Firebolt);
    bool hasArrow = false;
    for (const auto &line : tooltip.lines) {
        if (line.find("\xe2\x86\x92") != std::string::npos) hasArrow = true;
    }
    EXPECT_TRUE(hasArrow);
}

TEST_F(SpellTooltipTest, BuildListTooltipNoUpgrade)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellListTooltip(*MyPlayer, SpellID::Firebolt);
    for (const auto &line : tooltip.lines) {
        EXPECT_EQ(line.find("\xe2\x86\x92"), std::string::npos) << "Upgrade line in list tooltip: " << line;
    }
}

TEST_F(SpellTooltipTest, UtilitySpellNoDamageLine)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::TownPortal);
    for (const auto &line : tooltip.lines) {
        EXPECT_EQ(line.find("Damage"), std::string::npos) << "Damage line for utility spell: " << line;
    }
}

TEST_F(SpellTooltipTest, BoneSpiritSpecialLine)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::BoneSpirit);
    bool hasSpecial = false;
    for (const auto &line : tooltip.lines) {
        if (line.find("1/3 target hp") != std::string::npos) hasSpecial = true;
    }
    EXPECT_TRUE(hasSpecial);
}

TEST_F(SpellTooltipTest, ItemRepairWarningLine)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellTooltip(*MyPlayer, SpellID::ItemRepair);
    bool hasWarning = false;
    for (const auto &line : tooltip.lines) {
        if (line.find("reduces max durability") != std::string::npos) hasWarning = true;
    }
    EXPECT_TRUE(hasWarning);
}

TEST_F(SpellTooltipTest, BuildListTooltipTitle)
{
    LoadSpellDescData();
    auto tooltip = BuildSpellListTooltip(*MyPlayer, SpellID::Firebolt);
    EXPECT_FALSE(tooltip.title.empty());
    // Should contain spell name
    EXPECT_THAT(tooltip.title, testing::HasSubstr("Firebolt"));
}
```

- [ ] **Step 2: Build test to verify it fails (RED)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
```

Expected: `BuildSpellTooltip` not defined.

- [ ] **Step 3: Implement BuildSpellTooltip and BuildSpellListTooltip**

Add to `spell_tooltip.cpp`:

```cpp
SpellTooltip BuildSpellTooltip(const Player &player, SpellID spell)
{
    const SpellData &sd = GetSpellData(spell);
    const int level = player.GetSpellLevel(spell);

    SpellTooltip tooltip;
    tooltip.title = pgettext("spell", sd.sNameText);

    // desc section
    for (const auto *line : GetSpellDescLines(spell, DescSection::Desc)) {
        if (level == 0 && line->format != DescFormat::LevelDisplay && line->format != DescFormat::Text)
            continue; // Skip numeric lines at level 0
        if (level == 0 && line->format == DescFormat::LevelDisplay) {
            tooltip.lines.push_back(_("Spell Level 0 - Unusable"));
            continue;
        }
        tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
    }

    // upgrade section (only if level > 0 and < max)
    if (level > 0 && level < MaxSpellLevel) {
        auto upgradeLines = GetSpellDescLines(spell, DescSection::Upgrade);
        if (!upgradeLines.empty()) {
            tooltip.lines.push_back(_("Next Level:"));
            for (const auto *line : upgradeLines) {
                tooltip.lines.push_back("  " + FormatDescLine(*line, player, spell, level));
            }
        }
    }

    // warning section
    for (const auto *line : GetSpellDescLines(spell, DescSection::Warning)) {
        tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
    }

    return tooltip;
}

SpellTooltip BuildSpellListTooltip(const Player &player, SpellID spell)
{
    const SpellData &sd = GetSpellData(spell);
    const int level = player.GetSpellLevel(spell);

    SpellTooltip tooltip;
    tooltip.title = pgettext("spell", sd.sNameText);

    // Determine spell type for title
    // (caller can override title if needed)

    // desc section only — no upgrade
    for (const auto *line : GetSpellDescLines(spell, DescSection::Desc)) {
        if (level == 0 && line->format != DescFormat::LevelDisplay && line->format != DescFormat::Text)
            continue;
        if (level == 0 && line->format == DescFormat::LevelDisplay) {
            tooltip.lines.push_back(_("Spell Level 0 - Unusable"));
            continue;
        }
        tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
    }

    // warning section
    for (const auto *line : GetSpellDescLines(spell, DescSection::Warning)) {
        tooltip.lines.push_back(FormatDescLine(*line, player, spell, level));
    }

    return tooltip;
}
```

Note: `MaxSpellLevel` constant (15). Verify it exists or define it. `pgettext` and `_` are from `utils/language.h`.

- [ ] **Step 4: Build and run tests (GREEN)**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
cd build_vs18 && ctest -R spell_tooltip_test -V
```

Expected: All 30+ tests pass across all 4 layers.

- [ ] **Step 5: Commit**

```bash
git add Source/spell_tooltip.h Source/spell_tooltip.cpp test/spell_tooltip_test.cpp
git commit -m "feat(tooltip): implement BuildSpellTooltip + BuildSpellListTooltip (Layer 4 TDD)"
```

---

### Task 8: Add GetManaAmount Override + Fix const_cast

**Files:**
- Modify: `Source/spells.h` — add overload declaration
- Modify: `Source/spells.cpp` — implement overload
- Modify: `Source/spell_tooltip.cpp` — use new overload in EvaluateSpellExpr

**Interfaces:**
- Produces: `int GetManaAmount(const Player &player, SpellID spell, int overrideSpellLevel)`

- [ ] **Step 1: Add overload declaration to spells.h**

After existing `GetManaAmount` declaration (line 26):

```cpp
int GetManaAmount(const Player &player, SpellID sn, int overrideSpellLevel);
```

- [ ] **Step 2: Implement overload in spells.cpp**

Add before the existing `GetManaAmount` (line 103):

```cpp
int GetManaAmount(const Player &player, SpellID sn, int overrideSpellLevel)
{
    int ma;
    int adj = 0;
    const int sl = std::max(overrideSpellLevel - 1, 0);

    if (sl > 0) {
        adj = sl * GetSpellData(sn).sManaAdj;
    }
    if (sn == SpellID::Firebolt) {
        adj /= 2;
    }
    if (sn == SpellID::Resurrect && sl > 0) {
        adj = sl * (GetSpellData(SpellID::Resurrect).sManaCost / 8);
    }

    if (sn == SpellID::Healing || sn == SpellID::HealOther) {
        ma = (GetSpellData(SpellID::Healing).sManaCost + 2 * player.getCharacterLevel() - adj);
    } else if (GetSpellData(sn).sManaCost == 255) {
        ma = (player._pMaxManaBase >> 6) - adj;
    } else {
        ma = (GetSpellData(sn).sManaCost - adj);
    }

    ma = std::max(ma, 0);
    ma <<= 6;

    if (gbIsHellfire && player._pClass == HeroClass::Sorcerer) {
        ma /= 2;
    } else if (player._pClass == HeroClass::Rogue || player._pClass == HeroClass::Monk || player._pClass == HeroClass::Bard) {
        ma -= ma / 4;
    }

    if (GetSpellData(sn).sMinMana > ma >> 6) {
        ma = GetSpellData(sn).sMinMana << 6;
    }

    return ma;
}
```

Then make the existing overload delegate:

```cpp
int GetManaAmount(const Player &player, SpellID sn)
{
    return GetManaAmount(player, sn, player.GetSpellLevel(sn));
}
```

- [ ] **Step 3: Update EvaluateSpellExpr to use override for mana_delta**

In `spell_tooltip.cpp`, in `EvaluateSpellExpr`, the `mana` context variable should use the level parameter:

```cpp
ctx["mana"] = GetManaAmount(player, spell, level) >> 6;
```

This ensures that when `FormatDescLine` calls `EvaluateSpellExpr(expr, player, spell, level+1)` for mana_delta, the mana is computed at the correct level.

- [ ] **Step 4: Build and run all spell_tooltip tests**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j
cd build_vs18 && ctest -R spell_tooltip_test -V
```

Expected: All tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/spells.h Source/spells.cpp Source/spell_tooltip.cpp
git commit -m "feat(spells): add GetManaAmount overload with overrideSpellLevel"
```

---

### Task 9: Integrate into spell_book.cpp

**Files:**
- Modify: `Source/panels/spell_book.cpp` — replace hardcoded hover logic

**Interfaces:**
- Consumes: `BuildSpellTooltip()`, `SpellTooltip` from spell_tooltip.h

- [ ] **Step 1: Add include**

At top of `spell_book.cpp`, add:

```cpp
#include "spell_tooltip.h"
```

Remove `#include <cstdio>` (no longer needed).

- [ ] **Step 2: Replace hardcoded hover logic**

In `DrawSpellBook()`, replace the hover detection block (lines 196-267 in current code) with:

```cpp
// Hover detection for floating tooltip
if (!hoveredSpellFound && IsValidSpell(sn) && (spl & GetSpellBitmask(sn)) != 0) {
    const Point panelPos = GetPanelPosition(UiPanels::Spell);
    const Rectangle entryRect = { panelPos + Displacement { 11, yp + textPaddingTop }, { SpellBookDescription.width, SpellBookDescription.height } };
    if (entryRect.contains(MousePosition)) {
        hoveredSpellFound = true;
        SpellTooltip tooltip = BuildSpellTooltip(player, sn);
        FloatingInfoString = pgettext("spell", GetSpellData(sn).sNameText);
        for (const auto &line : tooltip.lines)
            AddInfoBoxString(line);
    }
}
```

- [ ] **Step 3: Remove debug fprintf**

Remove all `fprintf(stderr, "[DBG]...")` lines from spell_book.cpp.

- [ ] **Step 4: Build**

```bash
cmake --build build_vs18 --target devilutionx -j 2>&1 | tail -5
```

Expected: Build succeeds.

- [ ] **Step 5: Manual verification**

Run the game, open spell book, hover over spells. Verify:
- Firebolt shows: Level, Damage, Mana
- StoneCurse shows: Level, Mana, Duration
- TownPortal shows: Level, Mana
- BoneSpirit shows: "Dmg: 1/3 target hp"
- ItemRepair shows warning
- Upgrade preview shows "→" delta for leveled spells

- [ ] **Step 6: Commit**

```bash
git add Source/panels/spell_book.cpp
git commit -m "refactor(spell_book): replace hardcoded hover with BuildSpellTooltip"
```

---

### Task 10: Integrate into spell_list.cpp

**Files:**
- Modify: `Source/panels/spell_list.cpp` — replace hardcoded switch-case

**Interfaces:**
- Consumes: `BuildSpellListTooltip()`, `SpellTooltip` from spell_tooltip.h

- [ ] **Step 1: Add include**

At top of `spell_list.cpp`, add:

```cpp
#include "spell_tooltip.h"
```

Remove `#include <cstdio>` (no longer needed).

- [ ] **Step 2: Replace hardcoded switch-case**

In `DrawSpellList()`, replace the spell type switch (lines 150-197 in current code) with:

```cpp
if (spellListItem.isSelected) {
    uint8_t spellColor = PAL16_GRAY + 5;
    switch (spellListItem.type) {
    case SpellType::Skill:
        spellColor = PAL16_YELLOW - 46;
        PrintSBookSpellType(out, spellListItem.location, _("Skill"), spellColor);
        break;
    case SpellType::Spell:
        if (!myPlayer.isOnLevel(0)) spellColor = PAL16_BLUE + 5;
        PrintSBookSpellType(out, spellListItem.location, _("Spell"), spellColor);
        break;
    case SpellType::Scroll:
        if (!myPlayer.isOnLevel(0)) spellColor = PAL16_RED - 59;
        PrintSBookSpellType(out, spellListItem.location, _("Scroll"), spellColor);
        break;
    case SpellType::Charges:
        if (!myPlayer.isOnLevel(0)) spellColor = PAL16_ORANGE + 5;
        PrintSBookSpellType(out, spellListItem.location, _("Staff"), spellColor);
        break;
    case SpellType::Invalid:
        break;
    }

    SpellTooltip tooltip = BuildSpellListTooltip(myPlayer, spellId);
    FloatingInfoString = tooltip.title;
    for (const auto &line : tooltip.lines)
        AddInfoBoxString(line);

    // Scroll count (keep existing logic)
    if (spellListItem.type == SpellType::Scroll) {
        const int scrollCount = c_count_if(InventoryAndBeltPlayerItemsRange { myPlayer }, [spellId](const Item &item) {
            return item.isScrollOf(spellId);
        });
        AddInfoBoxString(fmt::format(fmt::runtime(ngettext("{:d} Scroll", "{:d} Scrolls", scrollCount)), scrollCount));
    }
    // Staff charges (keep existing logic)
    if (spellListItem.type == SpellType::Charges) {
        int charges = myPlayer.InvBody[INVLOC_HAND_LEFT]._iCharges;
        AddInfoBoxString(fmt::format(fmt::runtime(ngettext("{:d} Charge", "{:d} Charges", charges)), charges));
    }

    // Hotkey (keep existing logic)
    std::optional<std::string_view> fullHotkeyName = GetHotkeyName(spellId, spellListItem.type);
    if (fullHotkeyName) {
        AddInfoBoxString(fmt::format(fmt::runtime(_("Spell Hotkey {:s}")), *fullHotkeyName));
    }
}
```

Note: Keep the spell type color/icon rendering (PrintSBookSpellType) — only replace the tooltip content logic.

- [ ] **Step 3: Remove debug fprintf**

Remove all `fprintf(stderr, "[DBG]...")` lines from spell_list.cpp.

- [ ] **Step 4: Build**

```bash
cmake --build build_vs18 --target devilutionx -j 2>&1 | tail -5
```

Expected: Build succeeds.

- [ ] **Step 5: Manual verification**

Run the game, hover spells in bottom bar. Verify:
- All spell types show damage/mana info (not just name)
- Skill type now shows damage/mana (was only "{name} Skill")
- Scroll/Charge counts still display
- Hotkey info still displays
- No debug output in stderr

- [ ] **Step 6: Commit**

```bash
git add Source/panels/spell_list.cpp
git commit -m "refactor(spell_list): replace hardcoded switch-case with BuildSpellListTooltip"
```

---

### Task 11: Final Cleanup + Full Test Suite

**Files:**
- Modify: `Source/panels/spell_book.cpp` — final cleanup
- Modify: `Source/panels/spell_list.cpp` — final cleanup
- Modify: `.gitignore` — fix trailing newline

- [ ] **Step 1: Verify no remaining debug output**

```bash
grep -rn "fprintf.*stderr.*DBG" Source/panels/spell_book.cpp Source/panels/spell_list.cpp
```

Expected: No matches.

- [ ] **Step 2: Verify no remaining const_cast in spell_book.cpp**

```bash
grep -n "const_cast" Source/panels/spell_book.cpp
```

Expected: No matches.

- [ ] **Step 3: Fix .gitignore trailing newline**

Ensure `.gitignore` ends with a newline character.

- [ ] **Step 4: Run full test suite**

```bash
cd build_vs18 && ctest -V 2>&1 | tail -30
```

Expected: All tests pass, including spell_tooltip_test and spelldat_test.

- [ ] **Step 5: Final commit**

```bash
git add -A
git commit -m "chore: cleanup debug logs, const_cast hack, fix .gitignore"
```
