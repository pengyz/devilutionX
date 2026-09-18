## Task 2: 名册数据与加载/校验单元

**文件：**
- 新建：`assets/txtdata/monsters/level_rosters.tsv`、`assets/txtdata/monsters/level_roster_params.tsv`
- 新建：`Source/tables/level_roster.h`、`Source/tables/level_roster.cpp`
- 新建：`test/level_roster_test.cpp` + 修改 `CMake/Tests.cmake`
- 修改：`Source/CMakeLists.txt`（在 `tables/monstdat.cpp` 之后追加 `tables/level_roster.cpp`）

**接口：**
- 依赖输入：`MonstersData`（`Source/tables/monstdat.h`）、`UniqueMonstersData`、`IsMonsterAvailable` 的**语义**（逐层 `minDunLvl/maxDunLvl`；校验函数自行实现同名判定，因为生产函数是文件内静态）
- 对外产出：
  - `enum class LevelRosterRole : uint8_t { Core, Tail };`
  - `struct LevelRosterEntry { uint8_t level; _monster_id type; LevelRosterRole role; bool allowUniqueBoost; };`
  - `struct LevelRosterParams { uint8_t level; int maxImage; int tailDraw; std::vector<std::pair<BehaviorClass, uint8_t>> classFloors; };`
  - `void LoadLevelRoster();`
  - `std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level);`
  - `const LevelRosterParams *GetLevelRosterParams(uint8_t level);`
  - `std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params);`（返回错误说明；空 optional 表示通过）

- [ ] **步骤 1：编写失败的测试**

```cpp
// test/level_roster_test.cpp
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "engine/assets.hpp"
#include "tables/level_roster.h"
#include "tables/monstdat.h"

using namespace devilution;

TEST(LevelRosterTest, ValidationAcceptsAWellFormedRoster)
{
	const std::vector<LevelRosterEntry> entries {
		{ 1, MT_WSKELAX, LevelRosterRole::Core, false },
		{ 1, MT_NZOMBIE, LevelRosterRole::Tail, false },
	};
	const std::vector<LevelRosterParams> params {
		{ 1, 6000, 2, { { BehaviorClass::Melee, 2 } } },
	};
	EXPECT_FALSE(ValidateLevelRoster(entries, params).has_value());
}

TEST(LevelRosterTest, ValidationRejectsAMonsterUnavailableAtThatLevel)
{
	// MT_DIABLO 只在 L16 可用
	const std::vector<LevelRosterEntry> entries { { 1, MT_DIABLO, LevelRosterRole::Core, false } };
	const std::vector<LevelRosterParams> params { { 1, 6000, 2, {} } };
	const auto error = ValidateLevelRoster(entries, params);
	ASSERT_TRUE(error.has_value());
	EXPECT_NE(error->find("not available"), std::string::npos);
}
```

- [ ] **步骤 2：运行测试并确认其失败**

运行：`python3 tools/run_tests.py --test level_roster_test --filter 'LevelRosterTest.*'`
预期：构建失败（`tables/level_roster.h` 不存在）

- [ ] **步骤 3：编写最小实现**

`Source/tables/level_roster.h`：

```cpp
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "tables/monstdat.h"

namespace devilution {

enum class LevelRosterRole : uint8_t { Core, Tail };

struct LevelRosterEntry {
	uint8_t level;
	_monster_id type;
	LevelRosterRole role;
	bool allowUniqueBoost;
};

struct LevelRosterParams {
	uint8_t level;
	int maxImage;
	int tailDraw;
	std::vector<std::pair<BehaviorClass, uint8_t>> classFloors;
};

void LoadLevelRoster();
std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level);
const LevelRosterParams *GetLevelRosterParams(uint8_t level);
std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params);

} // namespace devilution
```

`Source/tables/level_roster.cpp`（校验与加载的骨架，字段逐个读取）：

```cpp
#include "tables/level_roster.h"

#include <algorithm>

#include "engine/assets.hpp"
#include "tables/monstdat.h"
#include "utils/endian_read.hpp"

namespace devilution {
namespace {

std::vector<LevelRosterEntry> Entries;
std::vector<LevelRosterParams> Params;
std::vector<LevelRosterEntry> EmptySpanSource;

// 与 monster.cpp 的 IsMonsterAvailable 同语义（该函数是文件内静态，无法复用）
bool IsAvailableAt(uint8_t level, _monster_id type)
{
	const MonsterData &data = MonstersData[static_cast<size_t>(type)];
	if (data.availability == MonsterAvailability::Never)
		return false;
	if (gbIsSpawn && data.availability == MonsterAvailability::Retail)
		return false;
	return level >= data.minDunLvl && level <= data.maxDunLvl;
}

} // namespace

std::optional<std::string> ValidateLevelRoster(std::span<const LevelRosterEntry> entries, std::span<const LevelRosterParams> params)
{
	for (const LevelRosterEntry &entry : entries) {
		if (static_cast<size_t>(entry.type) >= MonstersData.size())
			return StrCat("roster row names unknown monster id ", static_cast<int>(entry.type));
		if (!IsAvailableAt(entry.level, entry.type))
			return StrCat("monster ", static_cast<int>(entry.type), " is not available at level ", entry.level);
		if (!entry.allowUniqueBoost && IsUniqueBaseForLevel(entry.level, entry.type))
			return StrCat("monster ", static_cast<int>(entry.type), " is a unique's base at level ", entry.level, " and needs allow_unique_boost");
	}
	for (const LevelRosterParams &param : params) {
		// floors 可满足性：该层候选（含 core）中该类别数量 >= floor
		for (const auto &[cls, floor] : param.classFloors) {
			size_t available = 0;
			for (size_t i = 0; i < MonstersData.size(); i++) {
				const _monster_id type = static_cast<_monster_id>(i);
				if (IsAvailableAt(param.level, type) && GetBehaviorClass(MonstersData[i].ai) == cls)
					available++;
			}
			if (available < floor)
				return StrCat("level ", param.level, " class floor ", static_cast<int>(cls), " needs ", floor, " but only ", available, " candidates exist");
		}
	}
	return std::nullopt;
}

std::span<const LevelRosterEntry> GetLevelRoster(uint8_t level)
{
	const auto begin = std::find_if(Entries.begin(), Entries.end(), [level](const LevelRosterEntry &e) { return e.level == level; });
	if (begin == Entries.end())
		return {};
	const auto end = std::find_if(begin, Entries.end(), [level](const LevelRosterEntry &e) { return e.level != level; });
	return { &*begin, static_cast<size_t>(std::distance(begin, end)) };
}

const LevelRosterParams *GetLevelRosterParams(uint8_t level)
{
	const auto it = std::find_if(Params.begin(), Params.end(), [level](const LevelRosterParams &p) { return p.level == level; });
	return it == Params.end() ? nullptr : &*it;
}

} // namespace devilution
```

同时补两个辅助实现（`level_roster.cpp` 内）：`bool IsUniqueBaseForLevel(uint8_t level, _monster_id type)`（遍历 `UniqueMonstersData`，比较 `mtype` 与 `mlevel`）与 `void LoadLevelRoster()`（`DataFile::loadOrDie("txtdata\\monsters\\level_rosters.tsv")` 与 `...level_roster_params.tsv`，逐行填 `Entries`/`Params`，`class_floors` 用 `magic_enum::enum_cast<BehaviorClass>` 解析，最后调用 `ValidateLevelRoster` 并在失败时 `app_fatal`）。

- [ ] **步骤 4：运行测试并确认通过**

运行：`python3 tools/run_tests.py --test level_roster_test --filter 'LevelRosterTest.*'`
预期：PASS（两条用例）

- [ ] **步骤 5：补齐拒绝路径的用例**

用例（每条一个 `TEST`，全部用合成表驱动，不依赖真实 TSV）：
1. `ValidationRejectsAnInvalidRangeRow`（`MT_WSKELAX` 的可用层是 1-2，写进 `level=3` 应拒绝）
2. `ValidationRejectsAnUnsatisfiableClassFloor`（某层 `BehaviorClass::Boss, 99` 应拒绝）
3. `ValidationRejectsAUniqueBaseWithoutTheWhitelist`（取 `unique_monstdat.tsv` 中 `level=N` 的 `type`，以 `allowUniqueBoost=false` 放入 `level=N` 应拒绝；`true` 时通过）

- [ ] **步骤 6：编写真实 TSV 并验证加载**

按规格附录 C 写入 L1-16 的 core 行（每层 4-5 条），并写入 `level_roster_params.tsv` 的 16 行。**必须满足**：每层含该层 A1/A3 承载（`MT_RSKELAX`@L2-4、`MT_XSKELAX`@L3-5、`MT_BMAGMA`@L9-11、`MT_STORML`@L11-13；承载类型的可用层以 `monstdat.tsv` 为准）；不含 unique 的 base（除非白名单）。

用例：`TEST(LevelRosterTest, LoadsTheShippedRosterAndValidatesIt)` —— 调用 `LoadCoreArchives()` + `LoadMonsterData()` + `LoadLevelRoster()`，断言 `GetLevelRoster(9)` 非空且 `GetLevelRosterParams(9) != nullptr`。

运行：`python3 tools/run_tests.py --test level_roster_test --filter 'LevelRosterTest.*'`
预期：全部 PASS

- [ ] **步骤 7：提交**

```bash
git add Source/tables/level_roster.h Source/tables/level_roster.cpp Source/CMakeLists.txt assets/txtdata/monsters/level_rosters.tsv assets/txtdata/monsters/level_roster_params.tsv test/level_roster_test.cpp CMake/Tests.cmake
git commit -m "feat(roster): add per-level roster tables with load-time validation"
```

---

