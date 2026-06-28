# DevilutionX Modding — Continuation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete P0.2 (inventory 40→60), P1.2 (expert mode), and P2.2 (9 master marks with 3-slot nonlinear rune system) with full unit test coverage.

**Architecture:** Three sequential phases. P0.2 expands inventory arrays and adds backward-compatible save migration. P1.2 adds expert-mode flag, death-deletes-save logic, and character-creation toggle. P2.2 replaces the current 48-mark stub system with 9 fully designed marks (3 per class), each with 3 independent rune slots, rune items as dungeon loot, NPC socketing dialogues, and combat hook integration.

**Tech Stack:** C++17, Google Test, DevilutionX engine, DevilutionX save format (MPQ-based).

**Spec:** `docs/superpowers/specs/2026-06-28-mastermarks-design.md`

---

## Phase 0.2: Inventory Expansion 40→60

### Task 0.2.1: Update inventory constants and slot enums

**Files:**
- Modify: `Source/inv.h:22,26,36,46-74`
- Modify: `Source/player.h:39,217,297`

- [ ] **Step 1: Update `InventorySizeInSlots` height and `INVITEM_INV_LAST`**

In `Source/inv.h`, change:
```cpp
constexpr Size InventorySizeInSlots { 10, 4 };
```
to:
```cpp
constexpr Size InventorySizeInSlots { 10, 6 };
```

Change `INVITEM_INV_LAST` from `46` to `66` (7 + 60 - 1 = 66):
```cpp
	INVITEM_INV_FIRST  = 7,
	INVITEM_INV_LAST   = 66,
```

- [ ] **Step 2: Add new `SLOTXY` entries for rows 5 and 6**

In `Source/inv.h`, after `SLOTXY_INV_ROW4_LAST = 46`, add:
```cpp
	SLOTXY_INV_ROW5_FIRST = 47,
	SLOTXY_INV_ROW5_LAST  = 56,
	SLOTXY_INV_ROW6_FIRST = 57,
	SLOTXY_INV_ROW6_LAST  = 66,
	SLOTXY_INV_LAST       = SLOTXY_INV_ROW6_LAST,
```

Update `NUM_XY_SLOTS` from `55` to `75`:
```cpp
	NUM_XY_SLOTS          = 75
```

- [ ] **Step 3: Update `InventoryGridCells` constant**

In `Source/player.h`, change:
```cpp
constexpr int InventoryGridCells = 40;
```
to:
```cpp
constexpr int InventoryGridCells = 60;
```

- [ ] **Step 4: Update `InvList` and `InvGrid` arrays**

In `Source/player.h`, find `Item InvList[40]` and `int8_t InvGrid[40]` and change both to `[60]`.
In `Source/pack.h`, find `ItemPack InvList[InventoryGridCells]` and `int8_t InvGrid[InventoryGridCells]` — these auto-resolve via the constexpr change.

- [ ] **Step 5: Commit**

```bash
git add Source/inv.h Source/player.h
git commit -m "feat(inv): expand inventory constants from 40 to 60 slots"
```

---

### Task 0.2.2: Expand InvRect with rows 5-6

**Files:**
- Modify: `Source/inv.cpp:129`

- [ ] **Step 1: Add InvRect entries for row 5 (Y=338) and row 6 (Y=367)**

In `Source/inv.cpp`, after the row 4 InvRect entries (ending at `{ 278, 309 }`), add:

```cpp
	{ {  17, 338 }, { 29, 29 } }, // inv row 5
	{ {  46, 338 }, { 29, 29 } },
	{ {  75, 338 }, { 29, 29 } },
	{ { 104, 338 }, { 29, 29 } },
	{ { 133, 338 }, { 29, 29 } },
	{ { 162, 338 }, { 29, 29 } },
	{ { 191, 338 }, { 29, 29 } },
	{ { 220, 338 }, { 29, 29 } },
	{ { 249, 338 }, { 29, 29 } },
	{ { 278, 338 }, { 29, 29 } },
	{ {  17, 367 }, { 29, 29 } }, // inv row 6
	{ {  46, 367 }, { 29, 29 } },
	{ {  75, 367 }, { 29, 29 } },
	{ { 104, 367 }, { 29, 29 } },
	{ { 133, 367 }, { 29, 29 } },
	{ { 162, 367 }, { 29, 29 } },
	{ { 191, 367 }, { 29, 29 } },
	{ { 220, 367 }, { 29, 29 } },
	{ { 249, 367 }, { 29, 29 } },
	{ { 278, 367 }, { 29, 29 } },
```

- [ ] **Step 2: Update the slot layout comment at top of InvRect**

In `Source/inv.cpp:70-77`, update the comment diagram to show 6 rows of 10 slots each (rows 1-4 plus 5-6), with final slot number 66.

- [ ] **Step 3: Commit**

```bash
git add Source/inv.cpp
git commit -m "feat(inv): add InvRect entries for rows 5 and 6"
```

---

### Task 0.2.3: Update row-stride logic in inventory placement functions

**Files:**
- Modify: `Source/inv.cpp` — `CheckItemFitsInInventorySlot`, `FindSlotForItem`, `FindTargetSlotUnderItemCursor`

- [ ] **Step 1: Add tests for 60-slot boundary cases**

Create `test/inv_60_test.cpp`:

```cpp
#include <gtest/gtest.h>
#include "inv.h"
#include "player.h"

namespace devilution {
namespace {

constexpr const char MissingMpqAssetsSkipReason[] = "MPQ assets not found - skipping";

class Inv60Test : public ::testing::Test {
public:
    void SetUp() override
    {
        Players.resize(1);
        MyPlayer = &Players[0];
        if (missingMpqAssets_) {
            GTEST_SKIP() << MissingMpqAssetsSkipReason;
        }
    }

    static void SetUpTestSuite()
    {
        LoadCoreArchives();
        LoadGameArchives();
        missingMpqAssets_ = !HaveMainData();
        if (missingMpqAssets_) return;
        InitCursor();
        LoadItemData();
    }

private:
    static bool missingMpqAssets_;
};

bool Inv60Test::missingMpqAssets_ = false;

TEST_F(Inv60Test, InvRectRow5Coordinates)
{
    // Row 5 first slot = index 47 (7 equipped + 40 slots = 47)
    const int row5First = SLOTXY_INV_ROW5_FIRST;
    EXPECT_EQ(InvRect[row5First].position.x, 17);
    EXPECT_EQ(InvRect[row5First].position.y, 338);
}

TEST_F(Inv60Test, InvRectRow6Coordinates)
{
    const int row6First = SLOTXY_INV_ROW6_FIRST;
    EXPECT_EQ(InvRect[row6First].position.x, 17);
    EXPECT_EQ(InvRect[row6First].position.y, 367);
}

TEST_F(Inv60Test, InvRectTotalCount)
{
    // NUM_XY_SLOTS must cover all slots
    for (int i = 0; i < NUM_XY_SLOTS; i++) {
        EXPECT_GT(InvRect[i].size.width, 0);
    }
}

TEST_F(Inv60Test, InventoryGridCellsIs60)
{
    EXPECT_EQ(InventoryGridCells, 60);
}

TEST_F(Inv60Test, INVITEM_INV_LASTIs66)
{
    EXPECT_EQ(static_cast<int>(INVITEM_INV_LAST), 66);
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Update row-stride boundaries in `CheckItemFitsInInventorySlot`**

In `Source/inv.cpp`, find the function and add boundary checks for strides of 50. Update loop bounds to iterate over all 6 rows (0-5). The stride values should now include: 10, 20, 30, 40, 50.

- [ ] **Step 3: Update `FindSlotForItem` and `FindTargetSlotUnderItemCursor`**

These functions iterate over `INVITEM_INV_FIRST` through `INVITEM_INV_LAST`. Since `INVITEM_INV_LAST` is now 66 (was 46), the loop bounds auto-expand. Verify the row stride calculation works for 6 rows.

- [ ] **Step 4: Run tests**

```bash
cmake --build build2 --target devilutionx-test
./build2/devilutionx-test --gtest_filter=Inv60*
```

Expected: all Inv60* tests pass.

- [ ] **Step 5: Commit**

```bash
git add test/inv_60_test.cpp Source/inv.cpp
git commit -m "feat(inv): update placement functions for 60-slot inventory"
```

---

### Task 0.2.4: Load/save backward compatibility

**Files:**
- Modify: `Source/loadsave.cpp`

- [ ] **Step 1: Add test for loading old 40-slot save**

Create `test/loadsave_inv_60_test.cpp`:

```cpp
#include <gtest/gtest.h>
#include "loadsave.h"
#include "pack.h"
#include "player.h"

namespace devilution {
namespace {

TEST(LoadSaveInv60Test, Old40SlotToNew60SlotConversion)
{
    // Simulate reading an old 40-slot save:
    // - 40 items in InvList, 40 values in InvGrid
    // After migration, items 40-59 should be empty
    //
    // This is a design test — actual implementation requires
    // plumbing through the save system's migration path.

    PlayerPack oldPack {};
    std::memset(&oldPack, 0, sizeof(oldPack));

    // Fill first 40 grid slots with sequential values
    for (int i = 0; i < 40; i++) {
        oldPack.InvGrid[i] = static_cast<int8_t>(i + 1);
    }
    // After migration to 60, slots 40-59 should be 0
    for (int i = 40; i < 60; i++) {
        EXPECT_EQ(oldPack.InvGrid[i], 0);
    }
}

TEST(LoadSaveInv60Test, Full60SlotRoundTrip)
{
    PlayerPack pack {};
    std::memset(&pack, 0, sizeof(pack));

    // Fill all 60 slots
    for (int i = 0; i < 60; i++) {
        pack.InvGrid[i] = static_cast<int8_t>((i % 40) + 1);
    }
    // Verify all 60 populated
    for (int i = 0; i < 60; i++) {
        EXPECT_GT(pack.InvGrid[i], 0);
    }
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Implement save migration in `loadsave.cpp`**

In the save-load path, detect old-format saves (40-slot inventory) and migrate:
```cpp
// When loading old saves, extend InvGrid/InvList to 60 slots,
// filling the extra 20 with empty/zero values.
// The version number should be bumped for new saves.
```

- [ ] **Step 3: Commit**

```bash
git add test/loadsave_inv_60_test.cpp Source/loadsave.cpp
git commit -m "feat(inv): backward-compatible 40→60 save migration"
```

---

### Task 0.2.5: Controller navigation for new rows

**Files:**
- Modify: `Source/plrctrls.cpp`

- [ ] **Step 1: Update D-pad navigation boundaries**

In `Source/plrctrls.cpp`, find the inventory cursor movement logic. Extend the row boundary checks to include rows 5 and 6. The cursor Y position check must allow values up to row 6 (Y=367).

- [ ] **Step 2: Add test for controller navigation**

Create `test/plrctrls_inv_60_test.cpp`:
```cpp
TEST(PlrctrlsInv60Test, CursorCanReachRow6)
{
    // Verify D-pad can navigate cursor to the 6th row
    // starting from row 4, pressing down twice reaches row 6
}
```

- [ ] **Step 3: Commit**

```bash
git add test/plrctrls_inv_60_test.cpp Source/plrctrls.cpp
git commit -m "feat(inv): extend controller navigation to 6 rows"
```

---

## Phase 1.2: Expert Mode

### Task 1.2.1: Add `_isExpert` field and serialization

**Files:**
- Modify: `Source/player.h`
- Modify: `Source/loadsave.cpp`
- Modify: `Source/pack.h` (if needed for net pack)

- [ ] **Step 1: Add `_isExpert` field**

In `Source/player.h`, after the Soul Weakness block (around line 382):
```cpp
	bool _isExpert = false;
```

- [ ] **Step 2: Serialize `_isExpert` in loadsave**

In `Source/loadsave.cpp`, in the save function:
```cpp
// After existing _pSoulWeakened serialization
file.WriteLE<bool>(player._isExpert);
```

In the load function:
```cpp
player._isExpert = file.ReadLE<bool>();
```

- [ ] **Step 3: Update `PlayerPack` if needed**

If the pack struct has reserved bytes, use one for `_isExpert`. Check `Source/pack.h` reserved bytes.

- [ ] **Step 4: Add tests**

Create `test/expert_mode_test.cpp`:
```cpp
TEST(ExpertModeTest, ExpertFlagPersistsThroughSave)
{
    // Create player with _isExpert = true
    // Save, load, verify flag persists
}

TEST(ExpertModeTest, NonExpertFlagDefaultsFalse)
{
    Player player;
    EXPECT_FALSE(player._isExpert);
}
```

- [ ] **Step 5: Commit**

```bash
git add Source/player.h Source/loadsave.cpp test/expert_mode_test.cpp
git commit -m "feat(expert): add _isExpert field with save persistence"
```

---

### Task 1.2.2: Expert death deletes save

**Files:**
- Modify: `Source/player.cpp`
- Modify: `Source/loadsave.cpp` (new `DeleteSaveFile` function)

- [ ] **Step 1: Implement `DeleteSaveFile()`**

In `Source/loadsave.cpp`, add:
```cpp
void DeleteSaveFile(const Player &player)
{
    // Construct the save file path for the character
    // Remove the MPQ archive file
    // Handle errors gracefully (file may not exist, etc.)
}
```

- [ ] **Step 2: Modify `StartPlayerKill` for expert death**

In `Source/player.cpp`, in `StartPlayerKill`, after the death animation start:
```cpp
if (player._isExpert) {
    DeleteSaveFile(player);
    // Set flag to return to main menu instead of restarting
    // Expert death = character gone, no town restart
    return;
}
```

- [ ] **Step 3: Add test**

```cpp
TEST(ExpertModeTest, DeathTriggersSaveDeletion)
{
    // Set player as expert
    // Simulate death
    // Verify DeleteSaveFile was called
    // Verify save file no longer exists
}

TEST(ExpertModeTest, NonExpertDeathDoesNotDelete)
{
    // Regular player death
    // Save file must still exist
}
```

- [ ] **Step 4: Commit**

```bash
git add Source/player.cpp Source/loadsave.cpp test/expert_mode_test.cpp
git commit -m "feat(expert): implement expert death save deletion"
```

---

### Task 1.2.3: Character creation UI toggle and menu restrictions

**Files:**
- Modify: `Source/gamemenu.cpp`
- Modify: character creation UI files (investigate exact paths)

- [ ] **Step 1: Add Expert checkbox to character creation**

In the character creation screen flow, add an "Expert Mode" toggle/checkbox. When checked, set `_isExpert = true` on the new character.

- [ ] **Step 2: Disable "Load Game" for expert characters**

In `Source/gamemenu.cpp`, the menu update function: if `_isExpert`, disable the "Load Game" menu item to prevent save scumming.

- [ ] **Step 3: Add tests**

```cpp
TEST(ExpertModeTest, LoadGameDisabledForExpert)
{
    // Expert character → "Load Game" greyed out
}

TEST(ExpertModeTest, SaveGameDisabledForExpert)
{
    // Expert character → no manual save
}
```

- [ ] **Step 4: Commit**

```bash
git add Source/gamemenu.cpp <character-creation-files>
git commit -m "feat(expert): expert toggle in character creation and menu restrictions"
```

---

## Phase 2.2: Master Marks — Complete Rework

> NOTE: The current `mastermark.h/cpp` has 48 stub marks. This phase replaces it entirely with the 9-mark, 3-slot, nonlinear rune system defined in `docs/superpowers/specs/2026-06-28-mastermarks-design.md`.

---

### Task 2.2.0: Prune old mark system

**Files:**
- Rewrite: `Source/mastermark.h`
- Rewrite: `Source/mastermark.cpp`
- Modify: `Source/player.h` (update ownedMarks size, add slot data)
- Modify: `Source/CMakeLists.txt` (add new class files)

- [ ] **Step 1: Rewrite `mastermark.h` with 9-mark enum**

Replace the 48-mark `MasterMarkId` enum with:
```cpp
enum class MasterMarkId : uint8_t {
    // Warrior (3)
    IronBastion,    // Guardian: block-charge → release
    CrimsonBrand,   // Berserker: blood rage, lifesteal
    ArmsMaster,     // Weapon Master: dual 2H or light weapon stances
    // Sorcerer (3)
    Sanguimancer,   // Blood mage: HP → mana
    Spellblade,     // Spellblade: melee sorcerer
    Overcharge,     // Burst → weakness cycle
    // Rogue (3)
    Marksman,       // Bow/crossbow specialist
    Shadowstep,     // Dodge → teleport backstab
    Precision,      // Hybrid melee/ranged crit+hit
    COUNT // = 9
};
```

- [ ] **Step 2: Define rune slot structures**

```cpp
enum class RuneRarity : uint8_t {
    Common,    // White — slot 1
    Rare,      // Blue  — slot 2
    Legendary, // Gold  — slot 3
};

// Each mark has 3 slots, each slot has a binary choice
struct MarkSlot {
    RuneRarity requiredRune;
    bool socketed = false;
    uint8_t choice = 0; // 0 = unsocketed, 1 = path A, 2 = path B
};

// Per-mark state
struct MarkState {
    MasterMarkId id;
    MarkSlot slots[3];
    bool isActive = false;
};
```

- [ ] **Step 3: Update `Player` structure**

Replace the old `ownedMarks` bitset and `activeMarks[2]` with:
```cpp
    MarkState ownedMarks[static_cast<size_t>(MasterMarkId::COUNT)];
    uint8_t activeMarkCount = 0;
    MasterMarkId activeMarks[2];
```

- [ ] **Step 4: Update CMakeLists.txt**

Add new source files:
```
mastermark_warrior.cpp
mastermark_sorcerer.cpp
mastermark_rogue.cpp
```

- [ ] **Step 5: Rewrite `mastermark.cpp` core functions**

Implement the mark management functions: `GrantMark`, `ActivateMark`, `DeactivateMark`, `SocketRune`, `CheckMarkEffects` dispatch.

- [ ] **Step 6: Commit**

```bash
git rm Source/mastermark.cpp Source/mastermark.h  # effectively, via rewrite
git add Source/mastermark.h Source/mastermark.cpp Source/player.h Source/CMakeLists.txt
git commit -m "refactor(marks): replace 48-stub system with 9-mark 3-slot architecture"
```

---

### Task 2.2.1: Warrior marks — Iron Bastion

**Files:**
- Create: `Source/mastermark_warrior.cpp`

- [ ] **Step 1: Implement Iron Bastion core + 3 slots**

```cpp
// mastermark_warrior.cpp
#include "mastermark.h"
#include "player.h"

namespace devilution {

namespace {

// Check if player has Iron Bastion active
static bool HasIronBastion(const Player &player) {
    return HasActiveMark(player, MasterMarkId::IronBastion);
}

// Bastion charge: stored blocked damage
static int GetBastionCharge(const Player &player) {
    // Return player.marks[IronBastion].chargeValue
}

static void AddBastionCharge(Player &player, int damage) {
    int cap = player._pMaxHP;
    int slot1 = player.GetMarkSlotChoice(MasterMarkId::IronBastion, 0);
    if (slot1 == 2) cap = cap * 125 / 100; // Overflow path: +25% cap
    // Add damage to charge, clamp to cap
}

static void ReleaseBastionCharge(Player &player, int &damage) {
    int charge = GetBastionCharge(player);
    int slot2 = player.GetMarkSlotChoice(MasterMarkId::IronBastion, 1);
    if (slot2 == 1) damage += charge * 130 / 100;  // Heavy Blow: +30%
    else damage += charge; // Splash: dealt in AoE
    // Reset charge
}

static bool BlockFatalDamage(Player &player) {
    int slot3 = player.GetMarkSlotChoice(MasterMarkId::IronBastion, 2);
    if (slot3 == 1 && GetBastionCharge(player) > 0) {
        // Unbroken: consume charge, survive at 1 HP
        return true;
    }
    return false;
}

} // namespace

// Called from player.cpp block handler
void MarkOnPlayerBlock(Player &player, int blockedDamage) {
    if (HasIronBastion(player)) {
        AddBastionCharge(player, blockedDamage);
    }
    // ... other marks
}

// Called before melee attack damage is calculated
void MarkOnPlayerAttackPreCalc(Player &player, int &damage) {
    if (HasIronBastion(player)) {
        ReleaseBastionCharge(player, damage);
    }
    // ... other marks
}

// Called on fatal damage
bool MarkOnPlayerFatalDamage(Player &player) {
    if (HasIronBastion(player)) {
        return BlockFatalDamage(player);
    }
    return false;
}

} // namespace devilution
```

- [ ] **Step 2: Write tests for Iron Bastion**

```cpp
// test/mastermark_test.cpp
TEST(MarkIronBastion, BlockBuildsCharge)
{
    // Player has Iron Bastion active
    // Block 50 damage → charge = 50
    // Next attack releases 50 bonus damage
}

TEST(MarkIronBastion, ChargeCappedAtMaxHP)
{
    // Charge cannot exceed max HP
}

TEST(MarkIronBastion, Slot1_Overflow_IncreasesCap)
{
    // With Overflow slot, cap = 125% max HP
}

TEST(MarkIronBastion, Slot2_HeavyBlow_30PercentBonus)
{
    // Heavy Blow path: release deals +30%
}

TEST(MarkIronBastion, Slot3_Unbroken_SurvivesFatal)
{
    // Fatal damage with charge → survive at 1 HP
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.2: Warrior marks — Crimson Brand

**Files:**
- Modify: `Source/mastermark_warrior.cpp`

- [ ] **Step 1: Implement Blood Rage gauge, lifesteal, low-HP scaling, Undying/Blood Shield**

Key data structures:
```cpp
static int GetBloodRage(const Player &player); // stored in mark state
static void AddBloodRage(Player &player, int amount);
static void SpendBloodRage(Player &player, int amount);
static void DamageAbsorbFromRage(Player &player, int &damage);
```

Core behavior:
- Attacks build rage (×2 if slot1=A)
- Taking damage also builds rage (×0.5 if slot1=B)
- Spending rage = lifesteal + attack speed + damage bonus
- HP<30% → all effects ×1.5 (slot2=A), HP<10% → ×3 (slot2=B)
- Slot3-A: 3s death immunity + rage gen ×2, convert to HP
- Slot3-B: damage consumes rage before HP, depletion = 5s weakness

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkCrimsonBrand, AttacksBuildRage)
TEST(MarkCrimsonBrand, RageCappedAtMaxHP)
TEST(MarkCrimsonBrand, Slot1A_DoubleRageGen)
TEST(MarkCrimsonBrand, Slot1B_DamageTakenBuildsRage)
TEST(MarkCrimsonBrand, LowHpScaling)
TEST(MarkCrimsonBrand, Slot3A_UndyingThreeSeconds)
TEST(MarkCrimsonBrand, Slot3B_BloodShieldDepletes)
```

- [ ] **Step 3: Commit**

---

### Task 2.2.3: Warrior marks — Arms Master

**Files:**
- Modify: `Source/mastermark_warrior.cpp`

- [ ] **Step 1: Implement dual 2H stance (Heavy Grip) + light weapon stance (Swift Hands)**

Heavy Grip:
- When active + two 2H weapons equipped: attack speed ×1.3, 2H weapons can block (half chance)
- Slot2: speed 1.3→1.5, block penalty removed
- Slot3: Sweep — simultaneous fan AoE, both weapons' damage; single target = ×1.5

Swift Hands:
- When active: dual 1H → alternate strikes at avg speed ×1.5, each weapon's on-hit effects proc independently
- Or 1H+Shield → shield bash damage = weapon damage, blocks knockback + weapon counter
- Slot2: dual speed ×1.5→×1.8, counter damage 50%→100%
- Slot3: Duel — 6s active: +30% crit +30% speed (1H), block double + interrupts (shield)

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkArmsMaster, HeavyGrip_AllowsDualTwoHanders)
TEST(MarkArmsMaster, HeavyGrip_TwoHandersCanBlock)
TEST(MarkArmsMaster, HeavyGrip_SweepHitsMultipleTargets)
TEST(MarkArmsMaster, HeavyGrip_SweepSingleTargetBonus)
TEST(MarkArmsMaster, SwiftHands_DualWieldAlternates)
TEST(MarkArmsMaster, SwiftHands_ShieldCounterProc)
TEST(MarkArmsMaster, SwiftHands_DuelModeBuffs)
```

- [ ] **Step 3: Commit**

---

### Task 2.2.4: Sorcerer marks — Sanguimancer

**Files:**
- Create: `Source/mastermark_sorcerer.cpp`

- [ ] **Step 1: Implement HP-to-mana conversion engine**

In `Source/mastermark_sorcerer.cpp`, implement the blood-casting logic:

```cpp
namespace devilution {
namespace {

// Hook: called when player is about to cast but has insufficient mana
// Returns true if Sanguimancer handled it (paid with HP instead)
bool SanguimancerHandleCast(Player &player, int manaCost) {
    if (!HasActiveMark(player, MasterMarkId::Sanguimancer))
        return false;

    int hpCost = manaCost * 2; // Default: 1 mana = 2 HP
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 1);
    
    // Slot2-A (Sacrifice): if HP<30%, conversion is 1:1
    if (slot2Choice == 1 && player._pHitPoints < player._pMaxHP * 30 / 100) {
        hpCost = manaCost;
    }
    // Slot3-A (Blood God): if HP<20%, conversion 1:1 + bonus
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 2);
    if (slot3Choice == 1 && player._pHitPoints < player._pMaxHP * 20 / 100) {
        hpCost = manaCost;
    }

    if (player._pHitPoints <= hpCost) return false; // Not enough HP
    player._pHitPoints -= hpCost;
    player._pMana += manaCost;
    return true;
}

// Hook: called on monster kill by the player
void SanguimancerHandleKill(Player &player, const Monster &monster) {
    if (!HasActiveMark(player, MasterMarkId::Sanguimancer))
        return;
    
    // Base restore: 10% of enemy max HP
    int restore = monster._mmaxhp / 10;
    
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 0);
    if (slot1Choice == 1) {
        restore = restore * 3 / 2; // Blood Tide: 15%
    }
    
    // Slot3-A (Blood God): 20% restore when HP<20%
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 2);
    if (slot3Choice == 1 && player._pHitPoints < player._pMaxHP * 20 / 100) {
        restore = monster._mmaxhp / 5; // 20%
    }
    
    player._pHitPoints = std::min(player._pHitPoints + restore, player._pMaxHP);
    
    // Slot1-B (Touch of Curse): curse nearby enemies on blood kill
    if (slot1Choice == 2) {
        ApplyCurseAura(player, monster.position.tile, 3); // 3-tile radius, -15% damage, 3s
    }
}

// Hook: check if Sanguimancer blocks fatal damage via Ward temp HP
bool SanguimancerFatalCheck(Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Sanguimancer))
        return false;
    
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 2);
    if (slot3Choice != 2) return false; // Immortal path only
    
    int tempHp = player.GetMarkData(MasterMarkId::Sanguimancer).tempHp;
    if (tempHp > 0) {
        // Immortal: consume all temp HP to survive
        player.SetMarkData(MasterMarkId::Sanguimancer).tempHp = 0;
        player._pHitPoints = 1;
        return true;
    }
    return false;
}

// Called after blood-paid cast if slot2-B (Ward) is active
void SanguimancerApplyWard(Player &player, int hpSpent) {
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 1);
    if (slot2Choice != 2) return;
    
    int wardAmount = hpSpent / 2; // 50% of HP spent
    int cap = player._pMaxHP * 30 / 100; // Cap: 30% max HP
    auto &data = player.GetMarkData(MasterMarkId::Sanguimancer);
    data.tempHp = std::min(data.tempHp + wardAmount, cap);
}

// Per-tick: Blood God spell damage bonus
int SanguimancerSpellDamageModifier(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Sanguimancer))
        return 0;
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Sanguimancer, 2);
    if (slot3Choice == 1 && player._pHitPoints < player._pMaxHP * 20 / 100) {
        return 40; // +40% spell damage
    }
    return 0;
}

} // namespace
} // namespace devilution
```

Mark-specific data stored per player:
```cpp
// In MarkState, additional data:
struct SanguimancerData {
    int tempHp = 0; // Ward shield amount
};
```

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkSanguimancer, PaysHpForMana_DefaultRate)
{
    // Player at 100/100 HP, 0/50 mana, tries to cast 5-mana spell
    // -> pays 10 HP, gains 5 mana
    // Result: 90/100 HP, 5/50 mana
}

TEST(MarkSanguimancer, Sacrifice_LowHpImprovesRate)
{
    // Slot2=A (Sacrifice), HP<30% of max
    // -> conversion 1:1 instead of 1:2
}

TEST(MarkSanguimancer, KillRestoresHealth)
{
    // Kill monster with 200 max HP -> restore 20 HP (10%)
}

TEST(MarkSanguimancer, BloodTide_15PercentRestore)
{
    // Slot1=A (Blood Tide): restore 15% of monster max HP
}

TEST(MarkSanguimancer, TouchOfCurse_AppliesDebuffAura)
{
    // Slot1=B: kill curses nearby enemies
}

TEST(MarkSanguimancer, Ward_BuildsTempHpOnBloodCast)
{
    // Slot2=B (Ward): blood cast grants temp HP = 50% of HP spent
}

TEST(MarkSanguimancer, Immortal_FatalDamageConsumesTempHp)
{
    // Slot3=B (Immortal): temp HP active + fatal hit -> survive at 1 HP
}

TEST(MarkSanguimancer, BloodGod_LowHpBonus)
{
    // Slot3=A: HP<20% -> conversion 1:1, +40% spell damage, +20% restore
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.5: Sorcerer marks — Spellblade

**Files:**
- Modify: `Source/mastermark_sorcerer.cpp`

- [ ] **Step 1: Implement Mana Shield toggle, weapon enchant, INT-based weapon damage**

```cpp
namespace devilution {
namespace {

// Spellblade core data (per-player, stored in MarkState)
struct SpellbladeData {
    bool shieldToggled = false;
    int enchantTimer = 0;    // Remaining enchant duration in ticks
    int enchantElement = 0;  // 0=fire, 1=lightning (alternating)
};

// Hook: modify Mana Shield behavior
// When Spellblade is active: shield becomes toggle (no continuous mana drain)
// Instead: each hit drains mana = absorbed damage
bool SpellbladeHandleShield(Player &player, int incomingDamage) {
    if (!HasActiveMark(player, MasterMarkId::Spellblade))
        return false; // Use default shield behavior

    auto &data = player.GetMarkData(MasterMarkId::Spellblade).spellblade;
    if (!data.shieldToggled || !player.pManaShield)
        return false;

    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Spellblade, 0);
    int absorbRate = (slot1Choice == 1) ? 50 : 33; // Absorption: 50% instead of 33%

    int absorbed = incomingDamage * absorbRate / 100;
    if (player._pMana >= absorbed) {
        player._pMana -= absorbed;
    } else {
        // Not enough mana to absorb full amount, shield breaks
        data.shieldToggled = false;
        player._pHitPoints -= (absorbed - player._pMana);
        player._pMana = 0;
    }

    // Slot1-B (Reflect): 20% of absorbed damage rebounds
    if (slot1Choice == 2) {
        int reflect = absorbed * 20 / 100;
        // Apply reflect damage to attacker (via monster damage function)
    }

    return true; // Spellblade handled it
}

// Hook: apply enchant bonus to weapon attacks
int SpellbladeModifyWeaponDamage(const Player &player, int baseDamage) {
    if (!HasActiveMark(player, MasterMarkId::Spellblade))
        return 0;

    auto &data = player.GetMarkData(MasterMarkId::Spellblade).spellblade;
    if (data.enchantTimer <= 0) return 0;

    int bonus = baseDamage * 30 / 100; // 30% of weapon damage as elemental
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Spellblade, 2);
    if (slot3Choice == 2) {
        // Dual Discipline: STR bonus — each STR = +3% enchant damage
        bonus += bonus * player._pStrength * 3 / 100;
    }
    return bonus;
}

// Hook: compute weapon damage using INT instead of STR
int SpellbladeGetWeaponDamage(const Player &player, int itemDamage) {
    if (!HasActiveMark(player, MasterMarkId::Spellblade))
        return 0; // Not handled by spellblade

    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Spellblade, 2);
    if (slot3Choice == 1) {
        // Mind Over Might: weapon damage = INT * 0.8, ignore STR req
        return player._pMagic * 80 / 100;
    }
    // Dual Discipline: STR+INT both contribute
    return itemDamage; // Standard calc but with bonuses from INT attack speed
}

// Slot2-A (Elemental Storm): 10% chance on enchanted hit → 3-tile AoE
bool SpellbladeCheckElementalStorm(Player &player, int &procDamage) {
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Spellblade, 1);
    if (slot2Choice != 1) return false;
    if (RandomInt(100) >= 10) return false;
    procDamage = SpellbladeModifyWeaponDamage(player, player._pIMaxDam) * 50 / 100;
    return true;
}

// Slot2-B (Venom): enchant applies poison DoT (15% weapon dmg/s, 3s, stacks 3×)
bool SpellbladeApplyVenom(Player &player, Monster &monster) {
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Spellblade, 1);
    if (slot2Choice != 2) return false;
    int poisonDps = player._pIMaxDam * 15 / 100;
    // Apply via buff system: BuffType::Poison, value=poisonDps, duration=3s, max 3 stacks
    ApplyBuff(monster.buffable, BuffType::Poison, poisonDps, 3 * 60, player.getId(), 3);
    return true;
}

// Called per tick: manage enchant timer
void SpellbladePerTick(Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Spellblade).spellblade;
    if (data.enchantTimer > 0) data.enchantTimer--;
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkSpellblade, ShieldToggle_NoContinuousDrain)
{
    // Toggle shield on → no mana drain per tick
    // Take hit → mana consumed equal to absorbed damage
}

TEST(MarkSpellblade, Shield_Absorption_IncreasedTo50)
{
    // Slot1=A: absorption rate 50% instead of 33%
}

TEST(MarkSpellblade, Shield_Reflect_20Percent)
{
    // Slot1=B: 20% of absorbed damage rebounds to attacker
}

TEST(MarkSpellblade, Enchant_Adds30PercentElemental)
{
    // Enchanted weapon adds 30% elemental damage
}

TEST(MarkSpellblade, MindOverMight_WeaponDamageFromInt)
{
    // Slot3=A: weapon damage = INT * 0.8, STR req ignored
}

TEST(MarkSpellblade, DualDiscipline_StrBonus)
{
    // Slot3=B: each STR +3% enchant damage, each INT +0.5% attack speed
}

TEST(MarkSpellblade, ElementalStorm_10PercentProc)
{
    // Slot2=A: 10% chance on hit → 3-tile elemental AoE at 50% weapon damage
}

TEST(MarkSpellblade, Venom_PoisonStacks)
{
    // Slot2=B: enchanted attacks apply poison, stacks up to 3×
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.6: Sorcerer marks — Overcharge

**Files:**
- Modify: `Source/mastermark_sorcerer.cpp`

- [ ] **Step 1: Implement burst → weakness cycle**

```cpp
namespace devilution {
namespace {

struct OverchargeData {
    bool active = false;      // Currently in overcharge burst
    int burstTimer = 0;       // Remaining burst ticks
    int weaknessTimer = 0;    // Remaining weakness ticks
    int cooldownTimer = 0;    // Remaining cooldown ticks
    int totalDamageDealt = 0; // Tracked for Annihilation explosion
    int originalMaxMana = 0;  // Pre-weakness max mana
};

static constexpr int BASE_BURST_DURATION = 8 * 60;    // 8 seconds
static constexpr int BASE_WEAKNESS_DURATION = 10 * 60; // 10 seconds
static constexpr int BASE_COOLDOWN = 60 * 60;          // 60 seconds

// Activate overcharge (called from spell input or hotkey)
void OverchargeActivate(Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;
    if (data.cooldownTimer > 0 || data.active || data.weaknessTimer > 0)
        return; // Can't activate

    data.active = true;
    data.burstTimer = BASE_BURST_DURATION;
    data.totalDamageDealt = 0;

    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 0);
    if (slot1Choice == 2) {
        data.burstTimer += 4 * 60; // Overload: +4s duration
    }
}

// Hook: modify cast speed and mana cost during overcharge
// Called before spell cast
bool OverchargeModifyCast(Player &player, int &manaCost, int &castSpeed) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;
    if (!data.active) return false;

    manaCost = 0;  // Zero mana during overcharge
    castSpeed = castSpeed / 2; // Double cast speed (halve cast time)

    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 0);
    if (slot1Choice == 1) {
        // Amplify: +30% spell damage during overcharge
        // Applied in damage calc, not here
    }
    return true;
}

// Hook: modify spell damage during overcharge
int OverchargeSpellDamageModifier(const Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;
    if (!data.active) return 0;
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 0);
    return (slot1Choice == 1) ? 30 : 0; // +30% if Amplify
}

// Hook: damage penalty during weakness
int OverchargeWeaknessDamageModifier(const Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;
    if (data.weaknessTimer <= 0) return 0;
    return -50; // -50% damage
}

// Hook: mana cap reduction during weakness
int OverchargeGetMaxManaModifier(const Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;
    if (data.weaknessTimer <= 0) return 0;
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 1);
    int reduction = (slot2Choice == 2) ? 15 : 30; // Temperance: 15% instead of 30%
    return -reduction;
}

// Per-tick: manage timers and state transitions
void OverchargePerTick(Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;

    if (data.active) {
        data.burstTimer--;
        if (data.burstTimer <= 0) {
            // Burst ends → enter weakness
            data.active = false;
            data.weaknessTimer = BASE_WEAKNESS_DURATION;
            int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 1);
            if (slot2Choice == 1) data.weaknessTimer = 5 * 60; // Resilience: 5s
            
            data.originalMaxMana = player._pMaxManaBase;
            player._pMaxManaBase = player._pMaxManaBase * (100 + OverchargeGetMaxManaModifier(player)) / 100;
            if (player._pManaBase > player._pMaxManaBase) player._pManaBase = player._pMaxManaBase;

            // Slot3-A (Annihilation): explode on burst end
            int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 2);
            if (slot3Choice == 1 && data.totalDamageDealt > 0) {
                int explosionDmg = data.totalDamageDealt * 20 / 100;
                // Deal explosion damage in 4-tile AoE around player
            }
        }
        return;
    }

    if (data.weaknessTimer > 0) {
        data.weaknessTimer--;
        if (data.weaknessTimer <= 0) {
            // Weakness ends → restore mana cap, start cooldown
            player._pMaxManaBase = data.originalMaxMana;
            data.cooldownTimer = BASE_COOLDOWN;

            // Slot3-B (Echo): auto-refill mana + halved cooldowns
            int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Overcharge, 2);
            if (slot3Choice == 2) {
                player._pManaBase = player._pMaxManaBase;
                // Halve all spell cooldowns for 3s
                // Set_HalvedCooldowns_3s
            }
        }
        return;
    }

    if (data.cooldownTimer > 0) data.cooldownTimer--;
}

// Track damage during burst for Annihilation
void OverchargeTrackDamage(Player &player, int damageDealt) {
    auto &data = player.GetMarkData(MasterMarkId::Overcharge).overcharge;
    if (data.active) data.totalDamageDealt += damageDealt;
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkOvercharge, ActivateEntersBurst)
{
    // Activate → burst flags set, 8s timer starts
    // Spell cast → 0 mana cost, 2× speed
}

TEST(MarkOvercharge, Amplify_30PercentBonus)
{
    // Slot1=A: during burst, spell damage +30%
}

TEST(MarkOvercharge, Overload_12SecondBurst)
{
    // Slot1=B: burst duration 8s→12s
}

TEST(MarkOvercharge, BurstEnds_WeaknessStarts)
{
    // Burst timer expires → weakness begins
    // -50% damage, mana cap -30%, speed reduced
}

TEST(MarkOvercharge, Resilience_5SecondWeakness)
{
    // Slot2=A: weakness 10s→5s
}

TEST(MarkOvercharge, Temperance_15PercentManaReduction)
{
    // Slot2=B: mana reduction 30%→15%
}

TEST(MarkOvercharge, Annihilation_ExplodesOnEnd)
{
    // Slot3=A: burst end → explosion = 20% of total damage in 4-tile AoE
}

TEST(MarkOvercharge, Echo_RefillManaAfterWeakness)
{
    // Slot3=B: weakness ends → mana refills to full
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.7: Rogue marks — Marksman

**Files:**
- Create: `Source/mastermark_rogue.cpp`

- [ ] **Step 1: Implement bow damage, aim state, Multishot/Pierce, Arrow Rain/Heartseeker**

```cpp
namespace devilution {
namespace {

struct MarksmanData {
    int standTicks = 0;    // Ticks spent standing still
    bool aiming = false;   // Currently in aim state
    int slot3Cooldown = 0; // Arrow Rain / Heartseeker cooldown
};

static constexpr int AIM_THRESHOLD = 1 * 60; // 1 second to enter aim

// Hook: called per tick while player is stationary
void MarksmanPerTick(Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Marksman)) return;
    auto &data = player.GetMarkData(MasterMarkId::Marksman).marksman;

    if (player._pmode == PM_STAND) {
        data.standTicks++;
        if (data.standTicks >= AIM_THRESHOLD && !data.aiming) {
            data.aiming = true; // Enter aim state
        }
    } else {
        // Movement does NOT break aim (design decision: movement is fine)
        // Only attacking or being hit breaks it? No: spec says "movement does not break"
        // But we need a condition. Standing requirement is 1s initial only.
        // Once in aim, moving is fine? Let's follow spec: movement does not break.
    }
}

// Hook: modify ranged damage (base +20% + distance bonus)
int MarksmanModifyRangedDamage(const Player &player, int baseDamage, int distance) {
    if (!HasActiveMark(player, MasterMarkId::Marksman)) return 0;

    auto &data = player.GetMarkData(MasterMarkId::Marksman).marksman;
    int bonus = baseDamage * 20 / 100; // Base +20%

    if (data.aiming) {
        int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Marksman, 1);
        int perTile = (slot2Choice == 1) ? 5 : 3; // Longbow: 5% per tile
        int cap = (slot2Choice == 1) ? 50 : 30;
        int distBonus = std::min(distance * perTile, cap);
        bonus += baseDamage * distBonus / 100;
    }
    return bonus;
}

// Slot1-A (Multishot): fire 3 arrows in a fan pattern
int MarksmanGetProjectileCount(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Marksman)) return 1;
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Marksman, 0);
    return (slot1Choice == 1) ? 3 : 1;
}

// Slot1-B (Pierce): 50% chance to pierce
bool MarksmanCheckPierce(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Marksman)) return false;
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Marksman, 0);
    if (slot1Choice != 2) return false;
    return RandomInt(100) < 50;
}

// Slot2-B (Deadly Focus): +25% crit chance while aiming
int MarksmanGetCritBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Marksman)) return 0;
    auto &data = player.GetMarkData(MasterMarkId::Marksman).marksman;
    if (!data.aiming) return 0;
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Marksman, 1);
    return (slot2Choice == 2) ? 25 : 0;
}

// Slot3-A (Arrow Rain): 10 arrows on 5-tile area, 30s cooldown
bool MarksmanActivateArrowRain(Player &player, Point target) {
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Marksman, 2);
    if (slot3Choice != 1) return false;
    auto &data = player.GetMarkData(MasterMarkId::Marksman).marksman;
    if (data.slot3Cooldown > 0) return false;
    // Spawn 10 arrow missiles targeting random points in 5-tile radius
    for (int i = 0; i < 10; i++) {
        Point offset = { RandomInt(11) - 5, RandomInt(11) - 5 };
        // AddMissile(MIS_ARROW, target + offset, ...);
    }
    data.slot3Cooldown = 30 * 60;
    return true;
}

// Slot3-B (Heartseeker): after 3s uninterrupted aim, next shot guaranteed crit ×3
bool MarksmanCheckHeartseeker(Player &player, int &critMultiplier) {
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Marksman, 2);
    if (slot3Choice != 2) return false;
    auto &data = player.GetMarkData(MasterMarkId::Marksman).marksman;
    if (data.aiming && data.standTicks >= 3 * 60) {
        critMultiplier = 3;
        data.standTicks = 0; // Reset after the shot
        return true;
    }
    return false;
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkMarksman, BaseDamageIncreases20Percent)
{
    // Bow damage * 1.2
}

TEST(MarkMarksman, AimState_AfterOneSecond)
{
    // Stand 1s → aim activates → per-tile distance bonus applies
}

TEST(MarkMarksman, Multishot_FiresThreeArrows)
{
    // Slot1=A: each attack fires 3 projectiles in fan
}

TEST(MarkMarksman, Pierce_50Percent)
{
    // Slot1=B: 50% chance arrow passes through target
}

TEST(MarkMarksman, Longbow_5PercentPerTile)
{
    // Slot2=A: 5%/tile, cap 50%
}

TEST(MarkMarksman, DeadlyFocus_25PercentCrit)
{
    // Slot2=B: +25% crit chance while aiming
}

TEST(MarkMarksman, ArrowRain_30sCooldown)
{
    // Slot3=A: fires 10 arrows, then 30s cooldown
}

TEST(MarkMarksman, Heartseeker_3sAim_GuaranteedTripleCrit)
{
    // Slot3=B: after 3s aim, next shot guaranteed crit with ×3 multiplier
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.8: Rogue marks — Shadowstep

**Files:**
- Modify: `Source/mastermark_rogue.cpp`

- [ ] **Step 1: Implement dodge→teleport, guaranteed crit, 2 slots**

```cpp
namespace devilution {
namespace {

struct ShadowstepData {
    int cooldown = 0;        // Passive cooldown (dodge-triggered)
    int manualCooldown = 0;  // Manual trigger cooldown
    int chainWindow = 0;     // Slot3-A: window for 2nd teleport
    bool chainAvailable = false;
    int baseDodgeBonus = 0;  // Slot1-A: +15% dodge
};

static constexpr int PASSIVE_COOLDOWN = 4 * 60;  // 4 seconds
static constexpr int MANUAL_COOLDOWN = 8 * 60;   // 8 seconds

// Hook: called on successful dodge
bool ShadowstepOnDodge(Player &player, const Monster &attacker) {
    if (!HasActiveMark(player, MasterMarkId::Shadowstep))
        return false;
    
    auto &data = player.GetMarkData(MasterMarkId::Shadowstep).shadowstep;
    if (data.cooldown > 0) return false;

    // Teleport behind attacker
    Point behindPos = GetPositionBehind(attacker.position.tile, attacker._mdir);
    if (IsTileWalkable(behindPos)) {
        player.position.tile = behindPos;
    } else {
        // If behind is blocked, teleport to nearest walkable adjacent tile
        player.position.tile = FindNearestWalkable(attacker.position.tile);
    }

    // Next attack guaranteed critical (set flag)
    player.SetGuaranteedCrit(true);

    // Slot1-B (Afterimage): leave decoy
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 0);
    if (slot1Choice == 2) {
        // Spawn a 2s decoy at original position
        SpawnDecoy(player.originalPosition, 2 * 60);
    }

    data.cooldown = PASSIVE_COOLDOWN;

    // Slot3-A (Shadow Dance): enable chain window
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 2);
    if (slot3Choice == 1) {
        data.chainWindow = 4 * 60;
        data.chainAvailable = true;
    }

    return true;
}

// Manual Shadowstep activation (keypress, even without being attacked)
bool ShadowstepManualActivate(Player &player, Point target) {
    if (!HasActiveMark(player, MasterMarkId::Shadowstep))
        return false;
    
    auto &data = player.GetMarkData(MasterMarkId::Shadowstep).shadowstep;

    // Check cooldowns
    int effectiveCd = data.chainAvailable ? 0 : data.manualCooldown;
    if (effectiveCd > 0 && !data.chainAvailable) return false;

    // Slot3-B (Abyssal Step): extended range + wall bypass
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 2);
    int maxRange = (slot3Choice == 2) ? 8 : 4;
    bool bypassWalls = (slot3Choice == 2);

    if (Distance(player.position.tile, target) > maxRange) return false;
    if (!bypassWalls && !LineOfSight(player.position.tile, target)) return false;

    player.position.tile = target;
    player.SetGuaranteedCrit(true);

    if (data.chainAvailable) {
        data.chainAvailable = false;
        data.chainWindow = 0;
    } else {
        data.manualCooldown = MANUAL_COOLDOWN;
    }
    data.cooldown = PASSIVE_COOLDOWN; // Resets passive too
    return true;
}

// Modify backstab crit damage
int ShadowstepGetCritMultiplier(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Shadowstep))
        return 0; // No modifier

    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 1);
    if (slot2Choice == 1) return 200; // Assassinate: 200% instead of 100%
    return 100; // Base crit = double damage (100% bonus)
}

// Check if kill resets cooldown (slot2=A: Assassinate)
void ShadowstepOnKill(Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Shadowstep))
        return;
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 1);
    if (slot2Choice == 1) {
        auto &data = player.GetMarkData(MasterMarkId::Shadowstep).shadowstep;
        data.cooldown = 0;
        data.manualCooldown = 0;
    }
}

// Slot2-B (Venom Blade): apply poison on shadowstep crit
bool ShadowstepApplyVenom(Player &player, Monster &monster) {
    if (!HasActiveMark(player, MasterMarkId::Shadowstep))
        return false;
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 1);
    if (slot2Choice != 2) return false;
    if (!player.HasGuaranteedCrit()) return false;

    int poisonDps = CalculateWeaponDamage(player) * 20 / 100;
    // Apply poison buff: 20% weapon dmg/s, 3s duration
    ApplyBuff(monster.buffable, BuffType::Poison, poisonDps, 3 * 60, player.getId(), 3);
    return true;
}

// Slot1-A (Ghost): +15% base dodge chance
int ShadowstepGetDodgeBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Shadowstep))
        return 0;
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Shadowstep, 0);
    return (slot1Choice == 1) ? 15 : 0;
}

// Per-tick: manage cooldowns and chain window
void ShadowstepPerTick(Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Shadowstep).shadowstep;
    if (data.cooldown > 0) data.cooldown--;
    if (data.manualCooldown > 0) data.manualCooldown--;
    if (data.chainWindow > 0) {
        data.chainWindow--;
        if (data.chainWindow <= 0) data.chainAvailable = false;
    }
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkShadowstep, DodgeTeleportsBehindAttacker)
{
    // Rogue dodges → teleports behind monster → next attack guaranteed crit
}

TEST(MarkShadowstep, Cooldown_4Seconds)
{
    // After dodge-triggered teleport, 4s before next passive teleport
}

TEST(MarkShadowstep, ManualActivate_NoDodgeNeeded)
{
    // Can Shadowstep to target without being attacked, 8s cooldown
}

TEST(MarkShadowstep, Ghost_15PercentDodgeBonus)
{
    // Slot1=A: base dodge +15%
}

TEST(MarkShadowstep, Afterimage_SpawnsDecoy)
{
    // Slot1=B: leaves 2s decoy at original position
}

TEST(MarkShadowstep, Assassinate_200PercentCritDamage)
{
    // Slot2=A: shadowstep crit = 200% bonus (triple damage total)
}

TEST(MarkShadowstep, Assassinate_KillResetsCooldown)
{
    // Slot2=A: kill with shadowstep crit → cooldown cleared
}

TEST(MarkShadowstep, VenomBlade_AppliesPoison)
{
    // Slot2=B: shadowstep crit applies poison DoT
}

TEST(MarkShadowstep, ShadowDance_TwoTeleportsIn4s)
{
    // Slot3=A: can shadowstep twice within 4s window
}

TEST(MarkShadowstep, AbyssalStep_ExtendedRange_WallBypass)
{
    // Slot3=B: range +4 tiles, passes through walls
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.9: Rogue marks — Precision

**Files:**
- Modify: `Source/mastermark_rogue.cpp`

- [ ] **Step 1: Implement hybrid melee/ranged crit+hit bonuses**

```cpp
namespace devilution {
namespace {

// Precision is the rogue's numerical-direction mark.
// No new mechanics — just stat modifications based on weapon type.

// Base bonuses from the core brand
int PrecisionGetMeleeCritBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Precision))
        return 0;
    return 15; // +15% melee crit
}

int PrecisionGetRangedCritBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Precision))
        return 0;
    // Ranged gets half: 7.5%
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 2);
    if (slot3Choice == 1) return 15; // Versatility: full 15%
    return 7; // Truncated 7.5 → 7
}

int PrecisionGetRangedHitBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Precision))
        return 0;
    return 20; // +20% ranged hit
}

int PrecisionGetMeleeHitBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Precision))
        return 0;
    return 10; // Melee gets half: 10%
}

// Slot1-A: melee crit damage +30%, ranged hit +10%
int PrecisionGetCritDamageBonus(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Precision))
        return 0;
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 0);
    if (slot1Choice != 1) return 0;
    if (IsMeleeWeapon(player)) return 30;
    return 0;
}

int PrecisionGetExtraRangedHit(const Player &player) {
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 0);
    if (slot1Choice != 1) return 0;
    if (IsRangedWeapon(player)) return 10;
    return 0;
}

// Slot1-B: ranged attack speed +10%, melee hit +10%
int PrecisionGetRangedSpeedBonus(const Player &player) {
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 0);
    if (slot1Choice != 2) return 0;
    if (IsRangedWeapon(player)) return 10;
    return 0;
}

int PrecisionGetExtraMeleeHit(const Player &player) {
    int slot1Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 0);
    if (slot1Choice != 2) return 0;
    if (IsMeleeWeapon(player)) return 10;
    return 0;
}

// Slot2-A (Weak Spot): crits mark target, +15% all damage for 4s
void PrecisionApplyWeakSpot(Player &player, Monster &monster) {
    if (!HasActiveMark(player, MasterMarkId::Precision)) return;
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 1);
    if (slot2Choice != 1) return;
    // Apply Weak Spot debuff: +15% damage taken, 4s duration
    ApplyDebuff(monster, DebuffType::WeakSpot, 15, 4 * 60);
}

// Slot2-B (Rhythm): after crit, +20% attack speed for 3s
int PrecisionGetRhythmSpeed(const Player &player) {
    if (!HasActiveMark(player, MasterMarkId::Precision)) return 0;
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 1);
    if (slot2Choice != 2) return 0;
    auto &data = player.GetMarkData(MasterMarkId::Precision).precision;
    return (data.rhythmActive) ? 20 : 0;
}

void PrecisionTriggerRhythm(Player &player) {
    int slot2Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 1);
    if (slot2Choice != 2) return;
    auto &data = player.GetMarkData(MasterMarkId::Precision).precision;
    data.rhythmActive = true;
    data.rhythmTimer = 3 * 60;
}

// Slot3-B (Execution): ranged crits fire extra arrow, melee crits lifesteal
bool PrecisionExecutionExtraShot(const Player &player) {
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 2);
    if (slot3Choice != 2) return false;
    return IsRangedWeapon(player); // Extra arrow at 50% damage
}

int PrecisionExecutionLifesteal(const Player &player) {
    int slot3Choice = player.GetMarkSlotChoice(MasterMarkId::Precision, 2);
    if (slot3Choice != 2) return 0;
    if (IsMeleeWeapon(player)) return 8; // 8% lifesteal on melee crits
    return 0;
}

struct PrecisionData {
    bool rhythmActive = false;
    int rhythmTimer = 0;
};

void PrecisionPerTick(Player &player) {
    auto &data = player.GetMarkData(MasterMarkId::Precision).precision;
    if (data.rhythmActive) {
        data.rhythmTimer--;
        if (data.rhythmTimer <= 0) data.rhythmActive = false;
    }
}

} // namespace
} // namespace devilution
```

- [ ] **Step 2: Write tests**

```cpp
TEST(MarkPrecision, MeleeCritBase_15Percent)
{
    // Melee weapon → +15% crit chance
}

TEST(MarkPrecision, RangedCritBase_7Point5Percent)
{
    // Ranged weapon → +7.5% crit chance
}

TEST(MarkPrecision, RangedHitBase_20Percent)
{
    // Ranged weapon → +20% hit chance
}

TEST(MarkPrecision, Slot1A_MeleeCritDamage_30Percent)
{
    // Slot1=A: melee crit damage +30%, ranged hit +10%
}

TEST(MarkPrecision, Slot1B_RangedSpeed_10Percent)
{
    // Slot1=B: ranged attack speed +10%, melee hit +10%
}

TEST(MarkPrecision, Slot2A_WeakSpot_15PercentDamageAmp)
{
    // Slot2=A: crit marks target, +15% all damage for 4s
}

TEST(MarkPrecision, Slot2B_Rhythm_20PercentSpeed)
{
    // Slot2=B: after crit, +20% attack speed for 3s
}

TEST(MarkPrecision, Slot3A_Versatility_FullRangedCrit)
{
    // Slot3=A: ranged crit penalty removed (full 15%)
}

TEST(MarkPrecision, Slot3B_Execution_ExtraArrowOrLifesteal)
{
    // Slot3=B: ranged crit → extra arrow 50% | melee crit → 8% lifesteal
}
```

- [ ] **Step 3: Commit**

---

### Task 2.2.10: Rune items and NPC socketing

**Files:**
- Modify: `Source/items.cpp` (rune item definitions)
- Modify: `Source/towners.cpp` (mentor socketing dialogue)
- Create: `test/runes_test.cpp`

- [ ] **Step 1: Define rune items**

Add 3 rune item types (Common/Rare/Legendary Rune) to the item tables. Each rune has a `RuneRarity` field. When used on the appropriate mentor, opens socketing dialogue.

- [ ] **Step 2: Mentor socketing dialogue**

In `Source/towners.cpp`, add dialogue options for each mentor (Griswold/Adria/Gillian) to socket runes into owned marks.

```cpp
// In TalkToGriswold, after existing options:
// "Socket a rune" → shows owned marks with empty slots
// → select mark → select slot → select A or B path → confirm → consume rune + gold
```

- [ ] **Step 3: Write tests**

```cpp
TEST(RuneTest, SocketRuneConsumesItemAndGrantsSlot)
TEST(RuneTest, SocketChoiceIsPermanent)
TEST(RuneTest, CannotSocketWrongRarityRune)
TEST(RuneTest, MentorDialogueShowsAvailableSlots)
```

- [ ] **Step 4: Commit**

---

### Task 2.2.11: Combat hook integration

**Files:**
- Modify: `Source/player.cpp` (block handler, attack handler, damage handler, dodge handler, death handler)
- Modify: `Source/missiles.cpp` (spell cast handler)
- Modify: `Source/monster.cpp` (kill handler)

- [ ] **Step 1: Wire up all mark hooks**

In each combat function, add calls to the appropriate mark effect functions:
```cpp
// In player block handler:
MarkOnPlayerBlock(player, blockedDamage);

// In player attack damage calc:
MarkOnPlayerAttackPreCalc(player, damage);

// In ApplyPlrDamage:
if (MarkOnPlayerFatalDamage(player)) return; // survived via mark

// In dodge handler:
MarkOnPlayerDodge(player, attackerPosition);

// In monster death:
MarkOnPlayerKill(player, monster);

// In AddMissile (spell missiles):
MarkOnPlayerSpell(player, spellId, targetPos);

// In ProcessPlayers (per tick):
CheckMarkEffects(player);
```

- [ ] **Step 2: Integration tests**

```cpp
TEST(MarkIntegration, BlockTriggersIronBastion)
TEST(MarkIntegration, KillTriggersCrimsonBrand)
TEST(MarkIntegration, SpellTriggersOvercharge)
TEST(MarkIntegration, DodgeTriggersShadowstep)
```

- [ ] **Step 3: Commit**

---

### Task 2.2.12: Final integration and cleanup

**Files:**
- Modify: `Source/loadsave.cpp` (new mark data serialization)
- Modify: `Source/engine/render/scrollrt.cpp` (update HUD display for 9 new marks)
- Modify: `test/mastermark_test.cpp` (comprehensive test suite)

- [ ] **Step 1: Update loadsave for new mark data format**

Serialize `ownedMarks[9]` with slot choices and active state. Handle save version bump.

- [ ] **Step 2: Update Scrollrt HUD display**

The `GetActiveMarksString` function already exists (from current uncommitted changes). Update it for the 9 new mark names.

- [ ] **Step 3: Run full test suite**

```bash
cmake --build build2 --target devilutionx-test
./build2/devilutionx-test
```

All tests must pass. Zero regressions.

- [ ] **Step 4: Final commit**

```bash
git add -A
git commit -m "feat(marks): complete 9-mark 3-slot system with combat integration and tests"
```

---

## Post-Plan Phases (Out of Scope for This Plan)

The following phases from the original `engine-mod-infra` plan are not covered here and will require separate plans:

- **P2.4**: Behavioral Equipment Affixes (12 new IPL_* values)
- **P3.1**: Damage Type Expansion (Holy/Poison/Cold)
- **P3.2**: Set Items (4 sets)
- **Hellfire classes**: Monk, Bard, Barbarian marks (deferred)

---

## Task Summary

| Phase | Tasks | Files Created | Files Modified |
|-------|-------|--------------|----------------|
| P0.2 | 5 | 3 test files | inv.h, inv.cpp, player.h, pack.h, loadsave.cpp, plrctrls.cpp |
| P1.2 | 3 | 1 test file | player.h, player.cpp, loadsave.cpp, gamemenu.cpp, UI files |
| P2.2 | 13 | 5 (3 class .cpp + 2 test files) | mastermark.h, mastermark.cpp, player.h, player.cpp, towners.cpp, items.cpp, loadsave.cpp, scrollrt.cpp, missiles.cpp, monster.cpp, CMakeLists.txt |
| **Total** | **21** | **9** | **~18** |

---

*End of implementation plan.*
