> **已废弃**：一次性清理任务。光照与法术需求文本的接线已执行，`world_state` 与 `passiveDescription` 的删除已执行；其接线 `GetMonsterActivationRadius` 的计划项已随光照压制撤销而作废。

# Wire Up Skeleton Code — Better D1

## Goal

Connect 7 dead code functions and 1 dead data field to their intended consumers, making the Better D1 systems actually affect gameplay.

## Systems to Wire Up

### 1. Lighting Suppression (3 functions)

**Functions:**
- `GetLightSuppressionMultiplier(int dungeonLevel)` — `lighting.cpp:594`
- `GetEffectiveLightRadius(const Player&, int dungeonLevel)` — `lighting.cpp:620`
- `GetMonsterActivationRadius(const Player&, int dungeonLevel)` — `monster.cpp:5069`

**Current state:** Defined but zero callers. The existing lighting system uses `player._pLightRad` directly.

**Wire-up target:** The `DoVision()` function in `lighting.cpp` uses `player._pLightRad` for vision radius. We need to replace this with `GetEffectiveLightRadius(player, currlevel)` so that deeper levels reduce player vision.

**Effect:** Cathedral (1-4) = normal, Catacombs (5-8) = 90%, Caves (9-12) = 80%, Hell (13-16) = 60%, Crypt (21-24) = 50%. Monsters activate at `vision + 2` tiles, so darker levels = more danger.

**Risk:** If the suppression is too aggressive, players will feel frustrated. The current values (90%/80%/60%/50%) are conservative — equipment bonus is NOT suppressed, only base light radius.

### 2. Spell Requirement Display (2 functions)

**Functions:**
- `GetSpellRequirementText(const SpellData&, const Player&)` — `spell_book.cpp:123`
- `CanLearnSpell(SpellID, const Player&)` — `spell_book.cpp:132`

**Current state:** Defined but zero callers.

**Wire-up target:** `DrawSpellBook()` in `spell_book.cpp` renders spell entries. When hovering over a spell, the tooltip should show the requirement text.

**Effect:** Players see "Need 20 Magic (current 10/50)" when hovering spells they can't learn yet.

**Risk:** Low — purely additive UI information.

### 3. World State (3 functions) — Skeleton Cleanup

**Functions:**
- `GetWorldState()` — `world_state.cpp:13`
- `GetTristramDarkness()` — `world_state.cpp:40`
- `GetDungeonCorruption()` — `world_state.cpp:46`

**Current state:** Defined but zero callers. Header not included anywhere.

**Action:** These functions define state but have no consumers. The NPC dialogue system and visual changes that would consume them don't exist yet. **Delete these functions** — they're dead code that adds confusion. The concepts are preserved in the design docs for future implementation.

**Risk:** None — deleting dead code.

### 4. Passive Description (1 data field)

**Field:** `passiveDescription` in `PlayerData` — `playerdat.hpp:54`

**Current state:** Loaded from `classdat.tsv` but never read by any code.

**Action:** This field was added for the Better D1 class description system but has no consumer yet. **Delete the field and its TSV column** — it's dead data. The concept is preserved in the design docs.

**Risk:** None — deleting dead data.

## Implementation Order

1. Wire up lighting suppression (highest gameplay impact)
2. Wire up spell requirement display (UI improvement)
3. Delete world state skeleton code
4. Delete passiveDescription dead data

## Testing

- Lighting: verify deeper levels are darker, monsters activate outside vision
- Spell requirements: verify hover shows "Need X Magic (current Y/Z)"
- Deletions: verify no build errors, all tests pass
