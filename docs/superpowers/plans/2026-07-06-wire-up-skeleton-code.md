# Wire Up Skeleton Code — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Connect 7 dead code functions and 1 dead data field to their intended consumers.

**Architecture:** Wire existing functions into call sites, delete dead code, update TSV data.

**Tech Stack:** C++, TSV data files

## Global Constraints

- MSVC Build Tools 18 (vcvarsall.bat required for builds)
- Build: `cmake --build build_vs18 --target <target> -j`
- Test: `cd build_vs18 && ./spell_tooltip_test.exe`
- Game launch: `cd build_vs18 && start ./devilutionx.exe`

---

### Task 1: Wire Up Lighting Suppression

**Files:**
- Modify: `Source/lighting.cpp` — call site for `GetEffectiveLightRadius`
- Modify: `Source/lighting.h` — ensure declarations are public
- Modify: `Source/monster.cpp` — call site for `GetMonsterActivationRadius`

**Interfaces:**
- Consumes: `GetLightSuppressionMultiplier(int dungeonLevel)` — returns float (0.0-1.0)
- Consumes: `GetEffectiveLightRadius(const Player&, int dungeonLevel)` — returns int
- Consumes: `GetMonsterActivationRadius(const Player&, int dungeonLevel)` — returns int
- Produces: Modified vision radius for players, modified activation radius for monsters

The existing `DoVision` function receives `radius` as a parameter from its caller. The caller is in `Source/scrollrt.cpp` or similar. We need to find where `player._pLightRad` is passed as the vision radius and replace it with `GetEffectiveLightRadius(player, currlevel)`.

- [ ] **Step 1: Find the call site**

Search for where `player._pLightRad` or `_pLightRad` is used as a vision/light radius argument. The key call site is where `DoVision` or `DoLighting` is called with the player's light radius.

```bash
grep -n "_pLightRad" Source/lighting.cpp Source/scrollrt.cpp Source/diablo.cpp Source/player.cpp
```

Expected: Find the line(s) where `_pLightRad` is passed to vision/lighting functions.

- [ ] **Step 2: Replace with GetEffectiveLightRadius**

At the call site, replace `player._pLightRad` with `GetEffectiveLightRadius(player, currlevel)`. Add `#include "lighting.h"` if not already present.

Example change:
```cpp
// Before:
DoVision(player.position.tile, player._pLightRad, ...);

// After:
DoVision(player.position.tile, GetEffectiveLightRadius(player, currlevel), ...);
```

- [ ] **Step 3: Build and verify**

```bash
cmake --build build_vs18 --target devilutionx -j 2>&1 | tail -5
```

Expected: Build succeeds.

- [ ] **Step 4: Commit**

```bash
git add Source/lighting.cpp
git commit -m "feat: wire up lighting suppression for deeper dungeon levels"
```

---

### Task 2: Wire Up Spell Requirement Display

**Files:**
- Modify: `Source/panels/spell_book.cpp` — add requirement text to spell tooltip
- Modify: `Source/spell_tooltip.cpp` — add a new DescSource for spell requirements

**Interfaces:**
- Consumes: `GetSpellRequirementText(const SpellData&, const Player&)` — returns std::string
- Consumes: `CanLearnSpell(SpellID, const Player&)` — returns bool
- Produces: Additional tooltip line showing spell requirements

The spell tooltip is built in `BuildSpellTooltip()` in `spell_tooltip.cpp`. We need to add a new line at the end of the tooltip that shows the requirement text when the player cannot learn the spell.

- [ ] **Step 1: Add requirement display to BuildSpellTooltip**

In `Source/spell_tooltip.cpp`, in `BuildSpellTooltip()`, after the desc section and before the upgrade section, add a check: if the player cannot learn the spell (based on magic requirement), show the requirement text.

```cpp
// After desc section, before upgrade section
if (!CanLearnSpell(spell, player)) {
    tooltip.lines.emplace_back(GetSpellRequirementText(sd, player), UiFlags::ColorRed);
}
```

This requires adding `#include "panels/spell_book.hpp"` to spell_tooltip.cpp.

- [ ] **Step 2: Build and verify**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j 2>&1 | tail -5
cd build_vs18 && ./spell_tooltip_test.exe 2>&1 | tail -5
```

Expected: Build succeeds, all tests pass.

- [ ] **Step 3: Commit**

```bash
git add Source/spell_tooltip.cpp
git commit -m "feat: show spell requirement text in tooltip for unlearnable spells"
```

---

### Task 3: Delete World State Skeleton Code

**Files:**
- Delete: `Source/world_state.cpp`
- Delete: `Source/world_state.h`
- Modify: `Source/CMakeLists.txt` — remove world_state.cpp from build

**Interfaces:**
- Removes: `GetWorldState()`, `GetTristramDarkness()`, `GetDungeonCorruption()`

- [ ] **Step 1: Remove from CMakeLists.txt**

In `Source/CMakeLists.txt`, find and remove the line that adds `world_state.cpp` to the source list.

- [ ] **Step 2: Delete the files**

```bash
rm Source/world_state.cpp Source/world_state.h
```

- [ ] **Step 3: Build and verify**

```bash
cmake --build build_vs18 --target devilutionx -j 2>&1 | tail -5
```

Expected: Build succeeds (no other code references these files).

- [ ] **Step 4: Commit**

```bash
git add -A Source/world_state.*
git commit -m "chore: remove dead world_state skeleton code (no consumers)"
```

---

### Task 4: Delete passiveDescription Dead Data

**Files:**
- Modify: `Source/tables/playerdat.hpp` — remove passiveDescription field
- Modify: `Source/tables/playerdat.cpp` — remove reader.readString line
- Modify: `assets/txtdata/classes/classdat.tsv` — remove passiveDescription column
- Modify: `txtdata/classes/classdat.tsv` — remove passiveDescription column
- Modify: `mods/Hellfire/txtdata/classes/classdat.tsv` — remove passiveDescription column (if exists)

**Interfaces:**
- Removes: `PlayerData::passiveDescription` field

- [ ] **Step 1: Remove field from struct**

In `Source/tables/playerdat.hpp`, remove:
```cpp
/* Class Passive Skill Description */
std::string passiveDescription;
```

- [ ] **Step 2: Remove reader call**

In `Source/tables/playerdat.cpp`, in `LoadClassDatFromFile`, remove:
```cpp
reader.readString("passiveDescription", playerData.passiveDescription);
```

- [ ] **Step 3: Remove column from TSV files**

Remove the `passiveDescription` column (the last column) from:
- `assets/txtdata/classes/classdat.tsv`
- `txtdata/classes/classdat.tsv` (if exists separately)

The header row should change from:
```
className	folderName	portrait	inv	passiveDescription
```
to:
```
className	folderName	portrait	inv
```

And each data row should have the last column removed.

- [ ] **Step 4: Build and verify**

```bash
cmake --build build_vs18 --target spell_tooltip_test -j 2>&1 | tail -5
cd build_vs18 && ./spell_tooltip_test.exe 2>&1 | tail -5
```

Expected: Build succeeds, all tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/tables/playerdat.hpp Source/tables/playerdat.cpp assets/txtdata/classes/classdat.tsv txtdata/classes/classdat.tsv
git commit -m "chore: remove dead passiveDescription field (no consumers)"
```
