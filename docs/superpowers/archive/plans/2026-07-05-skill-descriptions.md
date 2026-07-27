> **已执行并归档。**

# Skill Description System — Implementation Plan

> **Goal:** Add spell/skill descriptions and upgrade preview to floating tooltips.

**Architecture:** Data layer — `SpellData.sDescription` loaded from `spelldat.tsv#description`. Display layer — `spell_list.cpp` hover and `spell_book.cpp` hover append description + upgrade preview via `FloatingInfoString`.

**Tech Stack:** C++20, CMake/Ninja, GTest, vcpkg

**Status:** Data + display layers implemented in `f2ed740e9`. Remaining: TDD tests for TSV loading.

---

### Task 1: spelldat TSV loading test

**Files:**
- Create: `test/spelldat/spelldat_test.cpp`
- Modify: `CMake/Tests.cmake`

- [ ] **Step 1: Write failing test — description field is loaded from TSV**

```cpp
#include <gtest/gtest.h>
#include "tables/spelldat.h"

namespace devilution {

TEST(SpelldatTest, DescriptionLoaded)
{
    // Firebolt is entry 1, description should be non-empty
    const SpellData &firebolt = GetSpellData(SpellID::Firebolt);
    EXPECT_FALSE(firebolt.sDescription.empty());
    EXPECT_NE(firebolt.sDescription.find("bolt of fire"), std::string::npos);
}

TEST(SpelldatTest, SpellCount)
{
    // Diablo has 37 spells + Null
    EXPECT_GE(SpellsData.size(), 37u);
}

} // namespace devilution
```

- [ ] **Step 2: Run test to verify it fails (or passes if implementation is already correct)**

Run: `ctest --test-dir build_vs18 -R spelldat -V`
Or via cmake: `cmake --build build_vs18 --target run-spelldat_test`

- [ ] **Step 3: If TSV file is not loaded by test infra, add LoadSpellData() call**

```cpp
class SpelldatTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        LoadSpellData();
    }
};
```

- [ ] **Step 4: Register in CMake/Tests.cmake**

Add `spelldat_test` to the test list.

- [ ] **Step 5: Run and verify**

Run: `ctest --test-dir build_vs18 -R spelldat --output-on-failure`
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add test/spelldat/spelldat_test.cpp CMake/Tests.cmake
git commit -m "test: add spelldat TSV loading tests"
```

---

### Task 2: Verify full build

- [ ] **Step 1: Full build + run all tests**

```bash
cmake --preset=vs18
cmake --build build_vs18 -j 12
ctest --test-dir build_vs18 --output-on-failure
```

Expected: build succeeds, all tests pass.
