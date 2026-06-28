# Soul Weakness — Death & Resurrection System

> Status: Design Complete · Target: engine-mod-infra

## Design Philosophy

D1's original "save anywhere, reload on death" system undermines its own tension. The unknown is D1's core asset — but manual saves let players scout doors risk-free, die, and reload with perfect knowledge.

We replace this with a **soul-based consequence system** where death creates a temporary debuff rather than a binary pass/fail. The player is never locked out — they are **weakened but capable**, with multiple paths to recovery, each carrying its own trade-off.

---

## The Soul Weakness State

```
┌──────────────────────────────────────────────────────┐
│                   DEATH FLOW                          │
│                                                      │
│  Alive ───death───→ Soul-Weakened                    │
│                       │                              │
│                       ├─ Recover body → Alive        │
│                       │     + regain all XP           │
│                       │                              │
│                       ├─ Die again → Alive           │
│                       │     respawn at Adria          │
│                       │     lose ALL accumulated XP   │
│                       │                              │
│                       └─ Visit Adria → Alive         │
│                             pay 30% gold              │
│                             lose ALL accumulated XP   │
└──────────────────────────────────────────────────────┘
```

### What Death Takes

| Aspect | Effect |
|---|---|
| **Max HP** | Reduced by 20% (normal), 25% (nightmare), 30% (hell) |
| **Current HP** | Proportionally adjusted to the new maximum |
| **Equipment** | Fully retained — all attributes, magic effects, and stats |
| **Spells** | Fully retained — all learned spells and levels |
| **Inventory** | Fully retained — all items remain in backpack |
| **Gold** | Fully retained (no coin drop on death) |

### What Death Leaves Behind

A **Soul Fragment** at the location of death, containing:

- All experience earned since the character's last level-up
- Visual: a faint violet flame on the ground tile
- Persistent across save/load

### Soul-Weakened State Indicators

- Character portrait gains a violet glow around its edges
- HP bar displays a visible notch showing the reduction
- Status text: "Soul-Weakened" on the character sheet

---

## Recovery Paths

### Path 1: Retrieval (跑尸 — Body Run)

```
Walk to the Soul Fragment → automatic absorption
  → Soul-Weakness removed
  → All stored experience restored
  → No gold cost
  → No further penalty
```

**Design intent**: The "honor" path. You accept the risk and reclaim what's yours through effort. Rewards player knowledge (you know the route) and persistence.

### Path 2: Second Death (自动兜底)

```
Die while Soul-Weakened → respawn at Adria in Tristram
  → Soul-Weakness automatically removed
  → ALL accumulated experience (from first death through second death) lost permanently
  → No gold cost
```

**Design intent**: The "safety valve." If the player is truly stuck — died in an unreachable location, or can't survive the run-back — the system self-resolves. The cost is total experience loss from the entire death episode. No stacking corpses, no unpayable debts.

### Path 3: Adria's Restoration (女巫召回)

```
Visit Adria → "Restore my soul"
  → Soul-Weakness removed
  → ALL accumulated experience lost permanently
  → Cost: 30% of current gold (normal), 40% (nightmare), 50% (hell)
  → Minimum cost: 0 gold (Adria does not reject the destitute)
```

**Design intent**: The "convenience" path. Pay to skip the run-back. The percentage-based cost means punishment scales with wealth — a rich character in the endgame pays more, keeping the decision meaningful forever.

---

## Difficulty Scaling

| Difficulty | HP Reduction | Adria's Fee |
|---|---|---|
| Normal | -20% Max HP | 30% gold |
| Nightmare | -25% Max HP | 40% gold |
| Hell | -30% Max HP | 50% gold |

Higher difficulties make death progressively more punishing, incentivizing body retrieval over paid restoration.

---

## Class-Adaptive Impact

The same 20% reduction hits different classes differently, creating natural class-specific tension without explicit per-class rules:

| Class | Normal HP | Weakened HP | Experience |
|---|---|---|---|
| Warrior | 200 | 160 (-40) | Still tanky. Can absorb the hit. Run-back is annoying but safe. |
| Rogue | 120 | 96 (-24) | Noticeably fragile. Ranged attacks keep you safer during run-back. |
| Sorcerer | 100 | 80 (-20) | Very fragile. Each monster is a threat. Strong motivation to recover or pay. |
| Monk | 150 | 120 (-30) | Moderate impact. Staff reach helps during run-back. |
| Bard | 120 | 96 (-24) | Similar to Rogue. Sword reach provides some safety. |
| Barbarian | 200 | 160 (-40) | Like Warrior. Can bulldoze back to the corpse. |

**The system automatically makes Sorcerers think twice before pushing deeper, without a single Sorcerer-specific line of code.**

---

## Expert Mode

```
Expert Mode:
  → Death = character deletion
  → No Soul-Weakness
  → No Adria restoration
  → No corpse retrieval
  → Save file deleted on death
```

The Soul-Weakness system only applies to normal play. Expert mode is pure permadeath — as in Diablo 2's Hardcore.

---

## Save System Change

To complement the Soul-Weakness system, manual saving is removed from single-player:

| Before | After |
|---|---|
| Player can save anytime | Save button disabled |
| Death → reload save | Death → Soul-Weakness (see above) |
| World rolls back on death | World persists on death |
| System autosaves on level transitions | Same (existing behavior) |
| System autosaves on exit | Same (existing behavior) |

The player can **quit and resume**, but cannot **save-scum**. This removes the "try, die, reload, know what's behind the door" loop that trivializes D1's tension.

---

## Implementation Mapping

| Component | File(s) | Change |
|---|---|---|
| Soul-Weakness flag | `player.h` | Add `bool _pSoulWeakened` field |
| HP reduction logic | `player.cpp` `CalcPlrItemVals` | Apply multiplier after stat calc |
| Soul Fragment data | `player.cpp` | New `struct SoulFragment { Point position; int level; uint32_t storedExp; }` |
| Death entry point | `player.cpp` `StartPlayerKill` | Branch: first death → set weakened + drop fragment; weakened death → Adria respawn |
| Fragment retrieval | `player.cpp` | New `CheckSoulFragmentRetrieval()` — called each tick when player moves |
| Adria dialogue | `towners.cpp` | New option: "Restore my soul" with gold check |
| Save/load | `loadsave.cpp` | Serialize `_pSoulWeakened`, `_soulFragment` position and experience |
| Portrait visual | UI / render code | Violet border when `_pSoulWeakened` is true |
| HP bar visual | UI code | Notch indicator on HP bar |
| Save button | `gamemenu.cpp` | Disable "Save Game" option in single-player |
| Expert mode | Character creation UI | Flag passed to game init; death handler checks and deletes save |

---

## Design Decisions Log

| Decision | Rationale |
|---|---|
| HP reduction over equipment loss | Equipment loss = can't play. HP reduction = can play but hurts |
| Percentage-based gold cost | Keeps punishment relevant across entire game lifecycle |
| Second death auto-resolves | No stacking punishment. No unpayable debts. |
| Experience loss, not level loss | Losing progress within a level hurts. Losing actual levels = rage-quit. |
| No Soul Fragment stacking | One fragment at a time. Old fragment overwrites on new death. |
| Adria as the soul NPC | Lore-appropriate. She already deals in magic and the occult. |
| Expert mode bypasses Soul-Weakness | Expert mode should be simpler (death = over), not layered with intermediate states |

---

## What We Explicitly Rejected

| Rejected Idea | Why |
|---|---|
| Equipment drop on death | Makes the game unplayable. 10 HP naked = can't kill anything. |
| Gold-only penalty | Gold is nearly worthless in late-game D1. Not a real punishment. |
| Soul retrieval with checkpoint/bonfire system | Requires level redesign. Breaks D1's random generation and continuous descent. |
| Unified soul-as-experience-and-currency (Dark Souls model) | Requires bonfire checkpoints for the loop to work. Without them, run-backs are too unpredictable. |
| Monster respawn on death | Punishes exploration satisfaction. D1's "I cleared this area" feeling is precious. |
| Nioh 2's guardian spirit loss | Too complex to map cleanly to D1's spell/class system without creating class imbalance. |
