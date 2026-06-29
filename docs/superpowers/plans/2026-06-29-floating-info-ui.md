# Floating Info UI + Skill Bar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the fixed info panel with floating tooltips, add a spell quickbar and level info bar.

**Architecture:** Extract floating tooltip logic into `qol/floating_info`, migrate all 18 `InfoString` display paths to `FloatingInfoString` with proper positioning, remove `InfoBoxRect` from the panel, fill the freed space with spell quickbar + level info.

**Tech Stack:** C++17, GTest, CMake. All changes in `Source/` under `devilution` namespace.

**Build & Test Commands:**
- Configure: `cmake -S. -Bbuild -DBUILD_TESTING=ON`
- Build all: `cmake --build build`
- Build specific target: `cmake --build build --target devilutionx`
- Build test: `cmake --build build --target <test_name>`
- Run test: `./build/<test_name>`
- Run all tests: `ctest --test-dir build`
- Run filtered tests: `ctest --test-dir build -R FloatingInfo`

---

### Task 1: Floating Tooltip Engine — Position Modes & Clamping

**Files:**
- Create: `Source/qol/floating_info.h`
- Create: `Source/qol/floating_info.cpp`
- Create: `test/floating_info_test.cpp`
- Modify: `Source/CMakeLists.txt` (add `qol/floating_info.cpp`)
- Modify: `CMake/Tests.cmake` (add test target)

- [ ] **Step 1: Write the failing test for FloatingInfoEngine**

```cpp
// test/floating_info_test.cpp
#include <gtest/gtest.h>
#include "qol/floating_info.h"

using namespace devilution;

TEST(FloatingInfoEngine, Init_CreatesDefaultState)
{
    FloatingInfoEngine engine;
    EXPECT_TRUE(engine.GetText().empty());
    EXPECT_EQ(engine.GetColor(), UiFlags::ColorWhite);
    EXPECT_EQ(engine.GetPosition(), FloatingPosition::Cursor);
}

TEST(FloatingInfoEngine, SetText_StoresCorrectly)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Test Item", UiFlags::ColorWhitegold,
                   FloatingPosition::WorldEntity, {100, 200});
    EXPECT_EQ(engine.GetText(), "Test Item");
    EXPECT_EQ(engine.GetColor(), UiFlags::ColorWhitegold);
    EXPECT_EQ(engine.GetPosition(), FloatingPosition::WorldEntity);
}

TEST(FloatingInfoEngine, GetRect_CursorMode_ClampsToScreen)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Tooltip text", UiFlags::ColorWhite,
                   FloatingPosition::Cursor, {630, 0});
    auto rect = engine.GetRect();
    // Must not exceed screen bounds
    EXPECT_LE(rect.position.x + rect.size.width, 640);
    EXPECT_GE(rect.position.x, 0);
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target floating_info_test
./build/floating_info_test
```
Expected: FAIL — `FloatingInfoEngine` not defined.

- [ ] **Step 3: Write the header**

```cpp
// Source/qol/floating_info.h
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "DiabloUI/ui_flags.hpp"
#include "engine/rectangle.hpp"
#include "utils/ui_fwd.h"

namespace devilution {

enum class FloatingPosition : uint8_t {
    Cursor,
    WorldEntity,
    PanelElement,
};

class FloatingInfoEngine {
public:
    void SetInfo(std::string_view text, UiFlags color,
                 FloatingPosition pos, Point anchor);

    void SetInfo(std::string &&text, UiFlags color,
                 FloatingPosition pos, Point anchor);

    void Clear();

    [[nodiscard]] std::string_view GetText() const;
    [[nodiscard]] UiFlags GetColor() const;
    [[nodiscard]] FloatingPosition GetPosition() const;
    [[nodiscard]] Rectangle GetRect() const;

    void SetScreenBounds(int width, int height);

private:
    std::string text_;
    UiFlags color_ = UiFlags::ColorWhite;
    FloatingPosition position_ = FloatingPosition::Cursor;
    Point anchor_ { 0, 0 };
    int screenWidth_ = 640;
    int screenHeight_ = 480;
};

} // namespace devilution
```

- [ ] **Step 4: Write the implementation**

```cpp
// Source/qol/floating_info.cpp
#include "qol/floating_info.h"

#include <algorithm>
#include <cstdint>

#include "utils/str_split.hpp"

namespace devilution {

void FloatingInfoEngine::SetInfo(std::string_view text, UiFlags color,
                                 FloatingPosition pos, Point anchor)
{
    text_ = std::string(text);
    color_ = color;
    position_ = pos;
    anchor_ = anchor;
}

void FloatingInfoEngine::SetInfo(std::string &&text, UiFlags color,
                                 FloatingPosition pos, Point anchor)
{
    text_ = std::move(text);
    color_ = color;
    position_ = pos;
    anchor_ = anchor;
}

void FloatingInfoEngine::Clear()
{
    text_.clear();
    color_ = UiFlags::ColorWhite;
    position_ = FloatingPosition::Cursor;
}

std::string_view FloatingInfoEngine::GetText() const { return text_; }
UiFlags FloatingInfoEngine::GetColor() const { return color_; }
FloatingPosition FloatingInfoEngine::GetPosition() const { return position_; }

Rectangle FloatingInfoEngine::GetRect() const
{
    if (text_.empty()) return { { 0, 0 }, { 0, 0 } };

    constexpr int lineHeight = 14;
    constexpr int textSpacing = 2;
    constexpr int hPadding = 5;
    constexpr int vPadding = 4;

    auto lines = SplitByChar(text_, '\n');
    int maxW = 0;
    for (const auto &line : lines) {
        int w = GetLineWidth(line, GameFont12, textSpacing, nullptr);
        maxW = std::max(maxW, w);
    }
    size_t newlineCount = std::count(text_.begin(), text_.end(), '\n');
    int lineCount = 1 + static_cast<int>(newlineCount);
    int totalH = lineCount * lineHeight;

    int x = anchor_.x;
    int y = anchor_.y;

    switch (position_) {
    case FloatingPosition::Cursor:
        x = anchor_.x + 16;
        y = anchor_.y + 16;
        break;
    case FloatingPosition::WorldEntity:
        y = anchor_.y - totalH - vPadding * 2;
        break;
    case FloatingPosition::PanelElement:
        x = anchor_.x - maxW / 2;
        y = anchor_.y - totalH - vPadding * 2;
        break;
    }

    x = std::clamp(x, hPadding, screenWidth_ - maxW - hPadding * 2);
    if (y < 0) y = anchor_.y + 32;

    return { { x, y }, { maxW + hPadding * 2, totalH + vPadding * 2 } };
}

void FloatingInfoEngine::SetScreenBounds(int width, int height)
{
    screenWidth_ = width;
    screenHeight_ = height;
}

} // namespace devilution
```

- [ ] **Step 5: Register source in CMakeLists.txt**

```cmake
# In Source/CMakeLists.txt, add after existing qol entries:
# qol/
  qol/floating_info.cpp
```

- [ ] **Step 6: Register test target in Tests.cmake**

In `CMake/Tests.cmake`, add `floating_info_test` to the `standalone_tests` list:

```cmake
set(standalone_tests
  ...
  floating_info_test
)
```

- [ ] **Step 7: Run tests and verify they pass**

```bash
cmake --build build --target floating_info_test
./build/floating_info_test
```
Expected: All 3 tests PASS.

- [ ] **Step 8: Commit**

```bash
git add Source/qol/floating_info.h Source/qol/floating_info.cpp Source/CMakeLists.txt test/floating_info_test.cpp CMake/Tests.cmake
git commit -m "feat(qol): add floating info engine with position modes and clamping"
```

---

### Task 2: FloatingInfoEngine — Full Clamping Tests

**Files:**
- Modify: `test/floating_info_test.cpp`

- [ ] **Step 1: Add edge case tests**

Add to `test/floating_info_test.cpp`:

```cpp
TEST(FloatingInfoEngine, GetRect_EmptyString_ReturnsZeroRect)
{
    FloatingInfoEngine engine;
    auto rect = engine.GetRect();
    EXPECT_EQ(rect.size.width, 0);
    EXPECT_EQ(rect.size.height, 0);
}

TEST(FloatingInfoEngine, GetRect_TopEdge_ClampsBelow)
{
    FloatingInfoEngine engine;
    // Anchor near top edge with small world entity
    engine.SetInfo("Test", UiFlags::ColorWhite,
                   FloatingPosition::WorldEntity, {320, 2});
    auto rect = engine.GetRect();
    EXPECT_GT(rect.position.y, 0);
}

TEST(FloatingInfoEngine, GetRect_RightEdge_ClampsLeft)
{
    FloatingInfoEngine engine;
    engine.SetInfo("A very long piece of text that might overflow",
                   UiFlags::ColorWhite,
                   FloatingPosition::Cursor, {630, 200});
    auto rect = engine.GetRect();
    EXPECT_LE(rect.position.x + rect.size.width, 640);
}

TEST(FloatingInfoEngine, GetRect_Multiline_CalculatesCorrectHeight)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Line 1\nLine 2\nLine 3", UiFlags::ColorWhite,
                   FloatingPosition::Cursor, {100, 100});
    auto rect = engine.GetRect();
    EXPECT_GT(rect.size.height, 30);
}

TEST(FloatingInfoEngine, Clear_ResetsAllState)
{
    FloatingInfoEngine engine;
    engine.SetInfo("Some text", UiFlags::ColorRed,
                   FloatingPosition::PanelElement, {50, 50});
    engine.Clear();
    EXPECT_TRUE(engine.GetText().empty());
    EXPECT_EQ(engine.GetColor(), UiFlags::ColorWhite);
}
```

- [ ] **Step 2: Run and verify all tests pass**

```bash
cmake --build build --target floating_info_test && ./build/floating_info_test
```
Expected: 8 tests PASS.

- [ ] **Step 3: Commit**

```bash
git add test/floating_info_test.cpp
git commit -m "test(qol): add edge case tests for floating info engine"
```

---

### Task 3: Spell Quickbar — Data Model & Tests

**Files:**
- Create: `Source/qol/skill_bar.h`
- Create: `Source/qol/skill_bar.cpp`
- Create: `test/skill_bar_test.cpp`
- Modify: `Source/CMakeLists.txt`
- Modify: `CMake/Tests.cmake`

- [ ] **Step 1: Write the failing test**

```cpp
// test/skill_bar_test.cpp
#include <gtest/gtest.h>
#include "qol/skill_bar.h"

using namespace devilution;

TEST(SkillSlot, Default_CreatesEmptySlot)
{
    SkillSlot slot;
    EXPECT_EQ(slot.spellId, SpellID::Null);
    EXPECT_EQ(slot.spellType, SpellType::Skill);
    EXPECT_FALSE(slot.isActive);
    EXPECT_EQ(slot.charges, 0);
}

TEST(SkillSlot, IsEmpty_ReturnsTrueForNullSpell)
{
    SkillSlot slot;
    EXPECT_TRUE(slot.IsEmpty());
}

TEST(SkillSlot, IsEmpty_ReturnsFalseForBoundSpell)
{
    SkillSlot slot;
    slot.spellId = SpellID::Firebolt;
    EXPECT_FALSE(slot.IsEmpty());
}

TEST(SkillSlot, HasConsumable_ScrollWithCharges)
{
    SkillSlot slot;
    slot.spellType = SpellType::Scroll;
    slot.charges = 3;
    EXPECT_TRUE(slot.HasConsumable());
    EXPECT_EQ(slot.GetChargeText(), "3");
}

TEST(SkillSlot, HasConsumable_StaffWithCharges)
{
    SkillSlot slot;
    slot.spellType = SpellType::Charges;
    slot.charges = 12;
    EXPECT_TRUE(slot.HasConsumable());
    EXPECT_EQ(slot.GetChargeText(), "12");
}

TEST(SkillSlot, HasConsumable_LearnedSpell_ReturnsFalse)
{
    SkillSlot slot;
    slot.spellType = SpellType::Spell;
    slot.charges = 0;
    EXPECT_FALSE(slot.HasConsumable());
}

TEST(SkillSlot, HasConsumable_ZeroChargeScroll_ReturnsTrue)
{
    SkillSlot slot;
    slot.spellType = SpellType::Scroll;
    slot.charges = 0;
    EXPECT_TRUE(slot.HasConsumable());
    EXPECT_EQ(slot.GetChargeText(), "0");
}

TEST(SkillBar, LoadFromPlayer_MapsHotkeysCorrectly)
{
    // Test that SkillBar::LoadFromPlayer reads _pSplHotKey correctly
    // This requires a mock or a minimal player setup
    GTEST_SKIP() << "Requires player mock setup — implemented in integration phase";
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target skill_bar_test
./build/skill_bar_test
```
Expected: FAIL — `SkillSlot` not defined.

- [ ] **Step 3: Write the header**

```cpp
// Source/qol/skill_bar.h
#pragma once

#include <cstdint>
#include <string>

#include "spells.h"
#include "tables/spelldat.h"

namespace devilution {

struct SkillSlot {
    SpellID spellId = SpellID::Null;
    SpellType spellType = SpellType::Skill;
    bool isActive = false;
    int charges = 0;

    [[nodiscard]] bool IsEmpty() const
    {
        return spellId == SpellID::Null;
    }

    [[nodiscard]] bool HasConsumable() const
    {
        return spellType == SpellType::Scroll
            || spellType == SpellType::Charges;
    }

    [[nodiscard]] std::string GetChargeText() const
    {
        return std::to_string(charges);
    }
};

struct Player; // forward

class SkillBar {
public:
    static constexpr int SlotCount = 4;

    void LoadFromPlayer(const Player &player);
    void Draw(const Surface &out, Point position);
    void FreeGraphics();

    [[nodiscard]] const SkillSlot &GetSlot(int index) const;

private:
    SkillSlot slots_[SlotCount];
};

} // namespace devilution
```

- [ ] **Step 4: Write the implementation**

```cpp
// Source/qol/skill_bar.cpp
#include "qol/skill_bar.h"

#include "engine/clx_sprite.hpp"
#include "engine/load_clx.hpp"
#include "engine/render/clx_render.hpp"
#include "player.h"

namespace devilution {

void SkillBar::LoadFromPlayer(const Player &player)
{
    for (int i = 0; i < SlotCount; i++) {
        if (player._pSplHotKey[i] != SpellID::Null) {
            slots_[i].spellId = player._pSplHotKey[i];
            slots_[i].spellType = player._pSplTHotKey[i];
            slots_[i].isActive = (player._pRSpell == player._pSplHotKey[i])
                              && (player._pRSplType == player._pSplTHotKey[i]);
            // Charges: fetch from inventory for scrolls / staves
            if (slots_[i].spellType == SpellType::Scroll
                || slots_[i].spellType == SpellType::Charges) {
                slots_[i].charges = player.GetSpellCharges(slots_[i].spellId);
            }
        } else {
            slots_[i] = SkillSlot {};
            slots_[i].isActive = false;
        }
    }
}

void SkillBar::Draw(const Surface &out, Point /*position*/)
{
    // Rendering will be implemented in Task 8 after panel layout is done.
    // For now, Draw is a stub — data model is the priority.
}

void SkillBar::FreeGraphics() { }

const SkillSlot &SkillBar::GetSlot(int index) const
{
    return slots_[index];
}

} // namespace devilution
```

- [ ] **Step 5: Register in CMakeLists.txt and Tests.cmake**

In `Source/CMakeLists.txt`, add `qol/skill_bar.cpp` to the qol section.

In `CMake/Tests.cmake`, add `skill_bar_test` to `standalone_tests`.

- [ ] **Step 6: Add object target linkage in Tests.cmake**

```cmake
# In CMake/Tests.cmake, after existing standalone linkage blocks:
target_link_dependencies(skill_bar_test PRIVATE libdevilutionx_debug_spelldat)
target_include_directories(skill_bar_test PRIVATE "${PROJECT_SOURCE_DIR}/Source")
```

- [ ] **Step 7: Build and run tests**

```bash
cmake --build build --target skill_bar_test && ./build/skill_bar_test
```
Expected: 7 tests PASS, 1 SKIPPED.

- [ ] **Step 8: Commit**

```bash
git add Source/qol/skill_bar.h Source/qol/skill_bar.cpp Source/CMakeLists.txt test/skill_bar_test.cpp CMake/Tests.cmake
git commit -m "feat(qol): add skill bar data model with unit tests"
```

---

### Task 4: Migrate GetItemStr → FloatingInfoString

**Files:**
- Modify: `Source/items.cpp` (lines 3822-3831)
- Modify: `test/items_test.cpp` (add test)

- [ ] **Step 1: Read current GetItemStr and existing test**

The current implementation in `items.cpp:3822-3831`:
```cpp
void GetItemStr(Item &item)
{
    if (item._itype != ItemType::Gold) {
        InfoString = item.getName();
        InfoColor = item.getTextColor();
    } else {
        const int nGold = item._ivalue;
        InfoString = fmt::format(fmt::runtime(ngettext("{:s} gold piece", "{:s} gold pieces", nGold)), FormatInteger(nGold));
    }
}
```

Note: `FloatingInfoString` is already being set in `control_infobox.cpp:407` for ground items, but `GetItemStr` writes to `InfoString`. The migration is to change `GetItemStr` to use `FloatingInfoString` and set the position accordingly. But wait — `GetItemStr` doesn't know the screen position. The best approach: have `GetItemStr` write to a callback or return value, then the caller sets `FloatingInfoString` with position.

Actually, simpler: have `GetItemStr` write to `FloatingInfoString` (since both `InfoString` and `FloatingInfoString` were being set in parallel in `DrawInfoBox` already). The position is determined by the caller in `DrawInfoBox` which has access to `cursPosition` for world entities.

- [ ] **Step 2: Read the existing items test**

```bash
grep -n "GetItemStr\|getItemStr" test/items_test.cpp | head -10
```

- [ ] **Step 3: Modify GetItemStr to write FloatingInfoString**

In `Source/items.cpp`, change `GetItemStr`:
```cpp
void GetItemStr(Item &item)
{
    if (item._itype != ItemType::Gold) {
        FloatingInfoString = item.getName();
        InfoColor = item.getTextColor();
    } else {
        const int nGold = item._ivalue;
        FloatingInfoString = fmt::format(fmt::runtime(ngettext("{:s} gold piece", "{:s} gold pieces", nGold)), FormatInteger(nGold));
    }
}
```

- [ ] **Step 4: Copy InfoString → FloatingInfoString in DrawInfoBox for ground items**

In `Source/control/control_infobox.cpp`, in `DrawInfoBox()`, change:
```cpp
// Old:
if (pcursitem != -1)
    GetItemStr(Items[pcursitem]);

// New:
if (pcursitem != -1)
    GetItemStr(Items[pcursitem]);
// InfoString is NOT set here anymore - GetItemStr sets FloatingInfoString
// The old PrintInfo() path drops this case naturally
```

- [ ] **Step 5: Commit**

```bash
git add Source/items.cpp Source/control/control_infobox.cpp
git commit -m "refactor(info): migrate GetItemStr to use FloatingInfoString"
```

---

### Task 5: Migrate GetObjectStr → FloatingInfoString

**Files:**
- Modify: `Source/objects.cpp` (lines 4903-4917)

- [ ] **Step 1: Modify GetObjectStr**

In `Source/objects.cpp`:
```cpp
void GetObjectStr(const Object &object)
{
    FloatingInfoString = object.name();
    const ClassAttributes &classAttributes = GetClassAttributes(MyPlayer->_pClass);
    if (HasAnyOf(classAttributes.classFlags, PlayerClassFlag::TrapSense)) {
        if (object._oTrapFlag) {
            FloatingInfoString = fmt::format(fmt::runtime(_("Trapped {:s}")), FloatingInfoString.str());
            InfoColor = UiFlags::ColorRed;
        }
    }
    if (object.IsDisabled()) {
        FloatingInfoString = fmt::format(fmt::runtime(_("{:s} (disabled)")), FloatingInfoString.str());
        InfoColor = UiFlags::ColorRed;
    }
}
```

- [ ] **Step 2: Update the caller in control_infobox.cpp**

```cpp
// Old:
if (ObjectUnderCursor != nullptr)
    GetObjectStr(*ObjectUnderCursor);

// New: GetObjectStr now sets FloatingInfoString directly
// ObjectUnderCursor is already checked, GetObjectStr fills FloatingInfoString
```

- [ ] **Step 3: Commit**

```bash
git add Source/objects.cpp Source/control/control_infobox.cpp
git commit -m "refactor(info): migrate GetObjectStr to use FloatingInfoString"
```

---

### Task 6: Migrate Monster/NPC/Player info → FloatingInfoString

**Files:**
- Modify: `Source/monster.cpp` (lines 4498-4582)
- Modify: `Source/control/control_infobox.cpp` (lines 410-437)

- [ ] **Step 1: Change AddInfoBoxString default to floating**

In `Source/control/control_infobox.cpp`, change the default parameter of `AddInfoBoxString`:

```cpp
// Old (line 286):
void AddInfoBoxString(std::string_view str, bool floatingBox /*= false*/)
// New:
void AddInfoBoxString(std::string_view str, bool floatingBox /*= true*/)
```

Same change for the `std::string&&` overload (line 296).

This single change makes ALL downstream calls to `AddInfoBoxString` (from `PrintMonstHistory`, `PrintUniqueHistory`, `DrawInfoBox` monster/player sections) automatically write to `FloatingInfoString`.

- [ ] **Step 2: Rewrite monster info section in DrawInfoBox**

In `Source/control/control_infobox.cpp`, in `DrawInfoBox()` (around line 410):

```cpp
// Old:
if (pcursmonst != -1) {
    if (leveltype != DTYPE_TOWN) {
        const Monster &monster = Monsters[pcursmonst];
        InfoColor = UiFlags::ColorWhite;
        InfoString = monster.name();
        if (monster.isUnique()) {
            InfoColor = UiFlags::ColorWhitegold;
            PrintUniqueHistory();
        } else {
            PrintMonstHistory(monster.type().type);
        }
    } else if (pcursitem == -1) {
        InfoString = std::string_view(Towners[pcursmonst].name);
    }
}

// New:
if (pcursmonst != -1) {
    if (leveltype != DTYPE_TOWN) {
        const Monster &monster = Monsters[pcursmonst];
        InfoColor = UiFlags::ColorWhite;
        FloatingInfoString = monster.name();
        if (monster.isUnique()) {
            InfoColor = UiFlags::ColorWhitegold;
            PrintUniqueHistory(); // Writes to FloatingInfoString now (default parameter changed)
        } else {
            PrintMonstHistory(monster.type().type); // Writes to FloatingInfoString
        }
    } else if (pcursitem == -1) {
        FloatingInfoString = std::string_view(Towners[pcursmonst].name);
    }
}
```

- [ ] **Step 3: Rewrite player info section**

```cpp
// Old:
if (PlayerUnderCursor != nullptr) {
    InfoColor = UiFlags::ColorWhitegold;
    const auto &target = *PlayerUnderCursor;
    InfoString = std::string_view(target._pName);
    AddInfoBoxString(fmt::format(fmt::runtime(_("{:s}, Level: {:d}")), target.getClassName(), target.getCharacterLevel()));
    AddInfoBoxString(fmt::format(fmt::runtime(_("Hit Points {:d} of {:d}")), target._pHitPoints >> 6, target._pMaxHP >> 6));
}

// New: InfoString → FloatingInfoString
if (PlayerUnderCursor != nullptr) {
    InfoColor = UiFlags::ColorWhitegold;
    const auto &target = *PlayerUnderCursor;
    FloatingInfoString = std::string_view(target._pName);
    AddInfoBoxString(fmt::format(fmt::runtime(_("{:s}, Level: {:d}")), target.getClassName(), target.getCharacterLevel()));
    AddInfoBoxString(fmt::format(fmt::runtime(_("Hit Points {:d} of {:d}")), target._pHitPoints >> 6, target._pMaxHP >> 6));
}
```

- [ ] **Step 4: Rewrite portrait hover and held-item sections**

```cpp
// Portrait hover (line 432):
if (PortraitIdUnderCursor != -1) {
    InfoColor = UiFlags::ColorWhitegold;
    auto &target = Players[PortraitIdUnderCursor];
    FloatingInfoString = std::string_view(target._pName);
    AddInfoBoxString(_("Right click to inspect"));
}

// Held gold/item / Requirements not met (lines 395-404):
if (!myPlayer.HoldItem.isEmpty()) {
    if (myPlayer.HoldItem._itype == ItemType::Gold) {
        const int nGold = myPlayer.HoldItem._ivalue;
        FloatingInfoString = fmt::format(fmt::runtime(ngettext("{:s} gold piece", "{:s} gold pieces", nGold)), FormatInteger(nGold));
    } else if (!myPlayer.CanUseItem(myPlayer.HoldItem)) {
        FloatingInfoString = _("Requirements not met");
    } else {
        FloatingInfoString = myPlayer.HoldItem.getName();
        InfoColor = myPlayer.HoldItem.getTextColor();
    }
}
```

- [ ] **Step 5: Commit**

```bash
git add Source/monster.cpp Source/control/control_infobox.cpp
git commit -m "refactor(info): migrate monster/player/NPC info to FloatingInfoString"
```

---

### Task 7: Migrate Stair/Portal/Quest Text → FloatingInfoString

**Files:**
- Modify: `Source/levels/trigs.cpp` (3 locations: town trigger, level trigger, up-stair)
- Modify: `Source/cursor.cpp` (2 locations: CheckTown, CheckRportal)
- Modify: `Source/quests.cpp` (1 location: quest portal)

- [ ] **Step 1: Migrate trigs.cpp**

In `Source/levels/trigs.cpp`, change all `InfoString =` to `FloatingInfoString =`:

Lines 355, 364, 374, 384, 394, 404, 419 — replace `InfoString` with `FloatingInfoString`.

- [ ] **Step 2: Migrate cursor.cpp**

In `Source/cursor.cpp`, `CheckTown()` (line 630) and `CheckRportal()` (line 644):
Replace `InfoString` with `FloatingInfoString`.

- [ ] **Step 3: Migrate quests.cpp**

In `Source/quests.cpp` (line 372):
Replace `InfoString` with `FloatingInfoString`.

- [ ] **Step 4: Migrate CheckPanelInfo() button descriptions**

In `Source/control/control_infobox.cpp`, `CheckPanelInfo()`, change all:
```cpp
InfoString = _(PanBtnStr[i]);
// to:
FloatingInfoString = _(PanBtnStr[i]);
```

And all subsequent `AddInfoBoxString` calls within `CheckPanelInfo()` already use the default (now `floatingBox=true`).

Also change the `InfoString = _("Select current spell button")` at line 341 and spell info section at lines 348-371.

Also change the XP bar info check at line 381 reference:
```cpp
if (CheckXPBarInfo())
    MainPanelFlag = true;
// CheckXPBarInfo internally sets FloatingInfoString now
```

- [ ] **Step 5: Commit**

```bash
git add Source/levels/trigs.cpp Source/cursor.cpp Source/quests.cpp Source/control/control_infobox.cpp
git commit -m "refactor(info): migrate stair/portal/quest text to FloatingInfoString"
```

---

### Task 8: Remove Legacy Info Panel

**Files:**
- Modify: `Source/control/control.hpp` (remove `InfoBoxRect`, `InfoString`)
- Modify: `Source/control/control_infobox.cpp` (remove `PrintInfo`, reshape `DrawInfoBox`)
- Modify: `Source/engine/render/scrollrt.cpp` (remove old `DrawInfoBox` call)
- Modify: `Source/control/control_panel.cpp` (remove old `DrawInfoBox` call)

- [ ] **Step 1: Remove declarations from control.hpp**

In `Source/control/control.hpp`:
- Remove: `constexpr Rectangle InfoBoxRect = { { 177, 46 }, { 288, 64 } };` (line 41)
- Remove: `extern StringOrView InfoString;` (line 50) — or keep it for compilation but mark deprecated

For safety, keep `InfoString` declared but add `[[deprecated]]`. Actually, let's just remove it and fix compilation errors.

```cpp
// REMOVE line 41:
// constexpr Rectangle InfoBoxRect = { { 177, 46 }, { 288, 64 } };

// REMOVE line 50:
// extern StringOrView InfoString;
```

Also remove `MainPanelFlag` if no longer needed. Check usage:
```bash
grep -rn "MainPanelFlag" Source/ --include="*.cpp" --include="*.h"
```

If `MainPanelFlag` is only used in the info panel path, remove it too.

- [ ] **Step 2: Remove PrintInfo() from control_infobox.cpp**

Delete the `PrintInfo()` function (lines 26-53 of control_infobox.cpp).

- [ ] **Step 3: Reshape DrawInfoBox() in control_infobox.cpp**

Remove everything that sets `InfoString`. Keep only the FloatingInfoString logic. The function becomes:

```cpp
void DrawInfoBox(const Surface &out)
{
    // No longer draws the fixed info panel.
    // All info is now rendered via DrawFloatingInfoBox().
    // This function is kept as a minimal stub for call-site compatibility
    // until scrollrt.cpp is refactored in Step 4.
}
```

- [ ] **Step 4: Remove DrawInfoBox calls from rendering pipeline**

In `Source/engine/render/scrollrt.cpp` line 1449:
```cpp
// REMOVE:
DrawInfoBox(out);
// REPLACE WITH:
// Info panel removed — all info now rendered via DrawFloatingInfoBox
```

In `Source/control/control_panel.cpp` line 434:
```cpp
// REMOVE:
DrawInfoBox(out);
```

- [ ] **Step 5: Ensure DrawFloatingInfoBox is always called (not gated by option)**

In `Source/engine/render/scrollrt.cpp` line 1896, remove the option gate:

```cpp
// Old:
if (*GetOptions().Gameplay.floatingInfoBox)
    DrawFloatingInfoBox(out);

// New:
DrawFloatingInfoBox(out);
```

- [ ] **Step 6: Build and fix compilation errors**

```bash
cmake --build build --target devilutionx 2>&1 | head -50
```

Fix any remaining references to `InfoString` or `InfoBoxRect`.

- [ ] **Step 7: Commit**

```bash
git add Source/control/control.hpp Source/control/control_infobox.cpp Source/control/control_panel.cpp Source/engine/render/scrollrt.cpp
git commit -m "refactor(info): remove legacy info panel, all info now floating"
```

---

### Task 9: Spell Quickbar Rendering

**Files:**
- Modify: `Source/qol/skill_bar.cpp` (fill in Draw implementation)
- Modify: `Source/qol/skill_bar.h` (add rendering constants)
- Modify: `Source/engine/render/scrollrt.cpp` (call `DrawSkillBar`)

- [ ] **Step 1: Define layout constants in header**

Add to `Source/qol/skill_bar.h`:
```cpp
// Quickbar positioned in the old InfoBoxRect area relative to main panel
inline constexpr Point SkillBarPosition { 237, 46 };  // Offset within main panel
inline constexpr Size SkillBarSlotSize { 28, 28 };
inline constexpr int SkillBarSlotSpacing = 6;
```

- [ ] **Step 2: Implement Draw method**

In `Source/qol/skill_bar.cpp`, fill in `Draw()`:

```cpp
void SkillBar::Draw(const Surface &out, Point basePosition)
{
    static OptionalOwnedClxSpriteList spellIcons;

    if (!spellIcons) {
        spellIcons = LoadOptionalClx("data\\spelicon.clx");
    }

    for (int i = 0; i < SlotCount; i++) {
        const int x = basePosition.x + SkillBarPosition.x
                      + i * (SkillBarSlotSize.width + SkillBarSlotSpacing);
        const int y = basePosition.y + SkillBarPosition.y;

        // Draw empty slot background
        DrawHalfTransparentRectTo(out, x, y,
            SkillBarSlotSize.width, SkillBarSlotSize.height);

        if (slots_[i].IsEmpty())
            continue;

        // Draw spell icon
        const size_t iconIdx = static_cast<size_t>(GetSpellData(slots_[i].spellId).sIconIndex);
        if (spellIcons && iconIdx < spellIcons->size()) {
            RenderClxSprite(out, (*spellIcons)[iconIdx], { x + 2, y + 2 });
        }

        // Draw active highlight border
        if (slots_[i].isActive) {
            DrawHalfTransparentRectTo(out, x, y,
                SkillBarSlotSize.width, SkillBarSlotSize.height);
        }

        // Draw charge count for consumable spells
        if (slots_[i].HasConsumable()) {
            std::string chargeText = slots_[i].GetChargeText();
            DrawString(out, chargeText,
                { { x + SkillBarSlotSize.width - 16, y + SkillBarSlotSize.height - 14 },
                  { 16, 12 } },
                { .flags = UiFlags::AlignRight | UiFlags::ColorWhite });
        }
    }
}
```

- [ ] **Step 3: Add render hook in scrollrt.cpp**

In `Source/engine/render/scrollrt.cpp`, after the `DrawXPBar` call and before `DrawFloatingInfoBox`:

```cpp
// In the render function, after DrawXPBar:
static SkillBar gSkillBar;
gSkillBar.LoadFromPlayer(*MyPlayer);
gSkillBar.Draw(out, GetMainPanel().position);
```

- [ ] **Step 4: Build and verify**

```bash
cmake --build build --target devilutionx
```

- [ ] **Step 5: Commit**

```bash
git add Source/qol/skill_bar.cpp Source/qol/skill_bar.h Source/engine/render/scrollrt.cpp
git commit -m "feat(qol): add spell quickbar rendering in main panel"
```

---

### Task 10: Level Info Bar

**Files:**
- Create: `Source/panels/level_info.h`
- Create: `Source/panels/level_info.cpp`
- Modify: `Source/engine/render/scrollrt.cpp`
- Modify: `Source/CMakeLists.txt`

- [ ] **Step 1: Write header**

```cpp
// Source/panels/level_info.h
#pragma once

#include <string>

#include "utils/ui_fwd.h"

namespace devilution {

class LevelInfoBar {
public:
    void Draw(const Surface &out, Point basePosition);

private:
    [[nodiscard]] std::string GetLevelName() const;
    [[nodiscard]] std::string GetDifficultyText() const;
};

} // namespace devilution
```

- [ ] **Step 2: Write implementation**

```cpp
// Source/panels/level_info.cpp
#include "panels/level_info.h"

#include <fmt/format.h>

#include "engine/render/text_render.hpp"
#include "game_mode.hpp"
#include "levels/gendung.h"
#include "player.h"
#include "utils/language.h"

namespace devilution {

namespace {

const char *LevelTypeNames[] = {
    /* DTYPE_TOWN       */ "Tristram",
    /* DTYPE_CATHEDRAL  */ "Catacombs",
    /* DTYPE_CATACOMBS  */ "Catacombs",
    /* DTYPE_CAVES      */ "Caves",
    /* DTYPE_HELL       */ "Hell",
    /* DTYPE_NEST       */ "Hive",
    /* DTYPE_CRYPT      */ "Crypt",
};

const char *DifficultyNames[] = { "Normal", "Nightmare", "Hell" };

} // namespace

std::string LevelInfoBar::GetLevelName() const
{
    if (currlevel == 0) {
        return _("Tristram");
    }
    const char *typeName = _(LevelTypeNames[static_cast<size_t>(leveltype)]);
    return fmt::format(fmt::runtime(_("{:s} Level {:d}")), typeName, currlevel);
}

std::string LevelInfoBar::GetDifficultyText() const
{
    if (gbIsMultiplayer || currlevel == 0) {
        return "";
    }
    return _(DifficultyNames[static_cast<size_t>(sgGameInitInfo.nDifficulty)]);
}

void LevelInfoBar::Draw(const Surface &out, Point basePosition)
{
    // Positioned at the left side of the old InfoBoxRect area
    constexpr int xOffset = 177;
    constexpr int yOffset = 50;
    constexpr int width = 60;

    std::string levelName = GetLevelName();
    std::string difficulty = GetDifficultyText();

    auto line1Rect = Rectangle {
        { basePosition.x + xOffset, basePosition.y + yOffset },
        { width, 14 }
    };
    DrawString(out, levelName, line1Rect,
        { .flags = UiFlags::AlignLeft | UiFlags::ColorButtonface });

    if (!difficulty.empty()) {
        auto line2Rect = Rectangle {
            { basePosition.x + xOffset, basePosition.y + yOffset + 16 },
            { width, 14 }
        };
        DrawString(out, difficulty, line2Rect,
            { .flags = UiFlags::AlignLeft | UiFlags::ColorWhitegold });
    }
}

} // namespace devilution
```

- [ ] **Step 3: Add render hook in scrollrt.cpp**

```cpp
// In the render function, after the skill bar:
static LevelInfoBar gLevelInfoBar;
gLevelInfoBar.Draw(out, GetMainPanel().position);
```

- [ ] **Step 4: Register in CMakeLists.txt**

Add `panels/level_info.cpp` to `Source/CMakeLists.txt` in the panels section.

- [ ] **Step 5: Build**

```bash
cmake --build build --target devilutionx
```

- [ ] **Step 6: Commit**

```bash
git add Source/panels/level_info.h Source/panels/level_info.cpp Source/engine/render/scrollrt.cpp Source/CMakeLists.txt
git commit -m "feat(ui): add level info bar showing dungeon name and difficulty"
```

---

### Task 11: Integration Testing & Final Cleanup

**Files:**
- Modify: `test/floating_info_test.cpp` (add integration-style tests)
- Potentially: `Source/control/control_infobox.cpp` (remove remaining `InfoString` references)

- [ ] **Step 1: Remove MainPanelFlag and any remaining InfoString usage**

```bash
grep -rn "InfoString\|MainPanelFlag" Source/ --include="*.cpp" --include="*.h" --include="*.hpp"
```

If any remain in files other than the already-migrated ones (`inv.cpp`, `stash.cpp`, `visual_store.cpp` which still set BOTH strings), those need fixing. The goal is: `FloatingInfoString` is the ONLY info display string.

For `inv.cpp`, `stash.cpp`, `visual_store.cpp` — they already set `FloatingInfoString`. Remove their `InfoString` lines too.

For `MainPanelFlag`:
- If only used in `CheckPanelInfo()`, remove the variable entirely.
- If used elsewhere for panel interaction handling, keep it but remove the info-specific assignment.

- [ ] **Step 2: Remove DrawInfoBox entirely from control.hpp**

Once all callers are removed, delete the declaration from `control.hpp`:
```cpp
// REMOVE:
void DrawInfoBox(const Surface &out);
```

And delete the stub function from `control_infobox.cpp`.

- [ ] **Step 3: Rename DrawFloatingInfoBox → DrawTooltips**

In `control_infobox.cpp` and `control.hpp` and `scrollrt.cpp`, rename:
```cpp
void DrawFloatingInfoBox(const Surface &out);  // old
void DrawTooltips(const Surface &out);          // new
```

This clarifies it's no longer "floating info box in addition to panel" — it's THE tooltip system.

- [ ] **Step 4: Full build and test**

```bash
cmake --build build --target devilutionx
cmake --build build --target floating_info_test
cmake --build build --target skill_bar_test
./build/floating_info_test
./build/skill_bar_test
ctest --test-dir build -R "FloatingInfo|SkillBar"
```

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "refactor(ui): final cleanup — remove all legacy info panel traces"
```

---

### Task 12: Run Full Test Suite & Manual Verification

**Files:** None — verification only.

- [ ] **Step 1: Run full test suite**

```bash
ctest --test-dir build --output-on-failure
```

Confirm zero regressions. Any pre-existing test failures must be noted and verified as pre-existing (not caused by these changes).

- [ ] **Step 2: Manual verification checklist**

Build and run the game. Verify:

- [ ] Hover monster in dungeon → floating box appears at monster head with name/history
- [ ] Hover ground item → floating box appears at item with name/color
- [ ] Hover object (chest/door) → floating box appears with name/trap info
- [ ] Hover NPC in town → floating box appears with name
- [ ] Hover other player → floating box appears with name/class/level/HP
- [ ] Hold item over cursor → floating box follows cursor
- [ ] Hover stairs/portal → floating box appears with destination text
- [ ] Hover main panel buttons → floating box at button
- [ ] Spell quickbar icons visible on main panel
- [ ] Click quickbar slot → spell changes
- [ ] Quickbar highlights active spell
- [ ] Level info bar shows correct level name and difficulty
- [ ] Old info panel area is gone (no black 288x64 box in main panel center)

- [ ] **Step 3: Push**

```bash
git push myrepo feature/qol-upgrades
```
