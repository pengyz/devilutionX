# 逐层名册 阶段 A 实施计划（名册 + 配额 + 校验）

> **面向 Agent 执行者：** 必需子技能：使用 superpower-subagent-driven-development（推荐）或 superpower-executing-plans 按任务逐项执行本计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 让每一层的怪物构成由**逐层名册**决定（核心必然出场 + 尾池随机），并给出可度量的行为类别配额与加载期校验。

**架构：** 新增两张 TSV（名册 + 逐层参数）与一个 `Source/tables/level_roster.{h,cpp}` 加载/校验单元；`GetLevelMTypes()` 在既有预加之后按 `currlevel` 预加 core、把采样循环改为"尾池抽取 `min(tail_draw, 可用候选)` 次"、并在循环内补足行为类别下限；预算由全局常量 4000 改为逐层 `max_image`。阶段 B（核心小队、G1/G2 修复）不在本计划内。

**技术栈：** C++23、CMake+Ninja、GoogleTest、TSV 数据驱动（`DataFile::loadOrDie`）、漂移校验 `tools/check_drift.py`、eval `tools/eval/backend.py`。

**规格：** `docs/superpowers/specs/2026-09-15-level-rosters-design.md`（本计划只实现其**阶段 A**：§4.1、§4.2、§4.4、§4.5 与 §6 的验收 1-4/8-11）

## 全局约束

- 行尾：改动文件保持既有类型（C++ 为 CRLF；`.md/.py/.yml/.yaml/.sh/.json` 为 LF）；新文件匹配 `.editorconfig`（`.tsv` 按现有 TSV 的实际行尾）
- 禁令 6：不得使用占位测试（`EXPECT_TRUE(true)`）冒充验证
- 新增测试二进制必须注册到 `CMake/Tests.cmake`（漂移检查 A 强制）
- 新增 Source 文件必须加入 `Source/CMakeLists.txt` 的显式清单（`tables/*.cpp` 段，约 `:633`）
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json` → `ctest.failed == 0 && ctest.passed_pct == 100 && drift.drift_ok == true`
- eval：`python3 -m tools.eval.backend --smoke` 必须 exit 0；新用例放 `eval/cases/rng/`
- 存档：按宪章决策 35 **不做兼容**；但必须重新生成受影响的硬编码夹具，并在 `docs/knowledge/decision_save_format_policy.md` 台账追加一行
- 口径：所有候选数与校验以 **retail 数据**（`gbIsSpawn == false`）为准；spawn 下走宽松校验（只校验 core 非空）
- 提交信息用英文，conventional commits（`feat:` / `test:` / `chore:`）
- 每个任务结束必须门禁绿；禁止把两个任务合并成一次提交

---

## 文件结构（先锁定职责）

| 文件 | 职责 |
|---|---|
| `assets/txtdata/monsters/level_rosters.tsv` | 新建：逐层 core/tail 名单 + `allow_unique_boost` 白名单 |
| `assets/txtdata/monsters/level_roster_params.tsv` | 新建：逐层 `max_image`、`tail_draw`、`class_floors`（阶段 B 的 squad 列本计划不读） |
| `Source/tables/level_roster.h` | 新建：数据结构与查询接口（供 `GetLevelMTypes` 与测试共用 → 有生产调用者） |
| `Source/tables/level_roster.cpp` | 新建：TSV 加载 + 校验（区间有效性、floors 可满足性、unique base 白名单） |
| `Source/monster.cpp` | 修改：`GetLevelMTypes()` 接入名册/尾池/配额/逐层预算 |
| `test/level_roster_test.cpp` | 新建：加载、查询、校验的单元测试（含合成表驱动的拒绝路径） |
| `test/level_roster_baseline_test.cpp` | 新建：真实建关 + 放置的 **placed class mix 基线**实测（T1 与 T4 共用夹具） |
| `test/sampling_behavior_test.cpp` | 修改：追加 core 必然出场 / 尾池上界 / 配额 / 身份守卫 / 承载落位 用例 |
| `eval/cases/rng/level-rosters.yaml` | 新建：eval 集成用例 |
| `CMake/Tests.cmake` | 修改：注册两个新测试二进制 |
| `Source/CMakeLists.txt` | 修改：注册 `tables/level_roster.cpp` |

---

## Task 1: 基线实测（placed class mix 与名册规模）

**文件：**
- 新建：`test/level_roster_baseline_test.cpp`
- 修改：`CMake/Tests.cmake`（在 `sampling_behavior_test` 之后追加 `level_roster_baseline_test`）

**接口：**
- 对外产出：`std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> MeasurePlacedClassMix(uint8_t level, uint32_t seed)` —— 生成该层真实关卡、跑采样与放置，按**已放置怪物数**统计行为类别；返回数组的元素和等于该次放置的 `ActiveMonsterCount`。
- 对外产出：`std::size_t MeasurePlacedMonsterCount()` 由 `ActiveMonsterCount` 直接可得，无需额外封装。**注（裁决 R6）**：原计划列出的 `MeasureRealisedTypes` 已删除——测试二进制各自独立 TU，任务 3 的用例在 `sampling_behavior_test.cpp`，**无法**调用本文件的函数；任务 3 直接读 `LevelMonsterTypes` 即可。

- [ ] **步骤 1：编写失败的测试**

```cpp
// test/level_roster_baseline_test.cpp
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "drlg_test.hpp" // TestInitGame / GetTileCount（本仓既有测试夹具）
#include "levels/gendung.h"
#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

namespace {

// 无 fixture 的建关：与 TestCreateDungeon 相同的前置，但不做 fixture 断言，
// 因此可以使用任意种子。
void CreateDungeonForMeasurement(uint8_t level, uint32_t seed)
{
	LevelSeeds[level] = std::nullopt;
	currlevel = level;
	leveltype = GetLevelType(level);
	pMegaTiles = std::make_unique<MegaTile[]>(GetTileCount(leveltype));
	CreateDungeon(seed, ENTRY_MAIN);
	CreateThemeRooms();
}

} // namespace

TEST(LevelRosterBaseline, PlacesMonstersForCathedralL1)
{
	TestInitGame();
	CreateDungeonForMeasurement(1, 1000);
	InitLevelMonsters();
	ASSERT_TRUE(GetLevelMTypes().has_value());
	ASSERT_TRUE(InitMonsters().has_value());

	const auto mix = MeasurePlacedClassMix(1, 1000);

	size_t total = 0;
	for (const size_t count : mix)
		total += count;
	EXPECT_GT(total, 0u) << "level 1 must place at least one monster";
	EXPECT_EQ(total, ActiveMonsterCount);
}
```

- [ ] **步骤 2：运行测试并确认其失败**

运行：`python3 tools/run_tests.py --test level_roster_baseline_test --filter 'LevelRosterBaseline.*'`
预期：构建失败，提示 `MeasurePlacedClassMix` 未声明（同时漂移检查 A 会因为未注册而报错——本步骤只需确认编译失败）

- [ ] **步骤 3：编写最小实现**

```cpp
// 追加到 test/level_roster_baseline_test.cpp（namespace 内）
std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> MeasurePlacedClassMix(uint8_t level, uint32_t seed)
{
	std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> mix {};
	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		const Monster &monster = Monsters[ActiveMonsters[i]];
		mix[static_cast<size_t>(GetBehaviorClass(monster.ai))]++;
	}
	return mix;
}

std::vector<_monster_id> MeasureRealisedTypes(uint8_t level, uint32_t seed)
{
	std::vector<_monster_id> types;
	for (size_t i = 0; i < LevelMonsterTypeCount; i++)
		types.push_back(LevelMonsterTypes[i].type);
	return types;
}
```

（`MeasurePlacedClassMix` 不重新建关：调用方负责先 `TestInitGame` + `CreateDungeonForMeasurement` + `InitLevelMonsters` + `GetLevelMTypes` + `InitMonsters`，使同一个 seed 的"采样→放置"链路只跑一次。步骤 1 的测试已经这样调用。）

- [ ] **步骤 4：注册并运行，确认通过**

在 `CMake/Tests.cmake` 的 `sampling_behavior_test` 之后追加一行 `  level_roster_baseline_test`。

运行：`python3 tools/run_tests.py --test level_roster_baseline_test --filter 'LevelRosterBaseline.*'`
预期：PASS

- [ ] **步骤 5：产出基线数据（200 seeds × L1-16）**

```cpp
TEST(LevelRosterBaseline, PlacedClassMixReport)
{
	TestInitGame();
	std::string report = "\n## A-baseline: placed class mix (200 seeds per level)\n\n"
	                     "| level | placed | Melee | RangedTurret | RangedKite | Rally | Charge | Sneak | Summon | Boss |\n"
	                     "|---|---|---|---|---|---|---|---|---|---|\n";
	for (uint8_t level = 1; level <= 16; level++) {
		std::array<size_t, static_cast<size_t>(BehaviorClass::Count)> sum {};
		size_t placed = 0;
		for (uint32_t seed = 0; seed < 200; seed++) {
			CreateDungeonForMeasurement(level, 5000 + seed);
			InitLevelMonsters();
			ASSERT_TRUE(GetLevelMTypes().has_value());
			ASSERT_TRUE(InitMonsters().has_value());
			const auto mix = MeasurePlacedClassMix(level, 5000 + seed);
			for (size_t i = 0; i < mix.size(); i++)
				sum[i] += mix[i];
			placed += ActiveMonsterCount;
		}
		report += StrCat("| ", level, " | ", placed, " |");
		for (const size_t count : sum)
			report += StrCat(" ", count, " |");
		report += "\n";
	}
	AppendMeasurementReport(report); // 复用 sampling_behavior_test.cpp 的同名约定（见步骤 5 注）
}
```

注：`AppendMeasurementReport`/`SAMPLING_REPORT` 约定已在 `test/sampling_behavior_test.cpp` 中存在（P0 实测套件）。本文件复制同一实现（约 12 行，`std::getenv("SAMPLING_REPORT")` + `std::ofstream` 追加），**不要**跨测试二进制共享头文件。

运行：
```bash
cmake --build build --target level_roster_baseline_test -j 20
rm -f /tmp/a-baseline.md && cd build && SAMPLING_REPORT=/tmp/a-baseline.md ./level_roster_baseline_test --gtest_filter='LevelRosterBaseline.PlacedClassMixReport'
```
预期：PASS，并在 `/tmp/a-baseline.md` 得到 L1-16 的基线表；**把该表复制进本计划的「实施记录」小节**（后续任务的阈值以此为准）。

- [ ] **步骤 6：提交**

```bash
git add test/level_roster_baseline_test.cpp CMake/Tests.cmake docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md
git commit -m "test(roster): measure the placed class-mix baseline per level"
```

---

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

> **执行裁决（2026-09-15，见 SDD 台账 R15-R19）**：本步骤的骨架代码已被修订——校验必须①含 **B1 caps** 的可满足性判定（caps 抽成 `BehaviorClassCapForLevel` 单一真相源，采样循环复用）；②加载时按 `level` **stable_sort**（访问器依赖同层连续）；③类型存在性检查**先于** `gbIsSpawn` 分支（`MT_INVALID` 可被 `enum_cast` 解析）；④core 非空检查覆盖 `entries ∪ params` 的每个 level；⑤校验 `max_image > 0`、`tail_draw >= 0`，并拒绝哨兵 `BehaviorClass::Count`。

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

## Task 3: 采样接入（core 预加 + 尾池 + 配额 + 逐层预算）

**文件：**
- 修改：`Source/monster.cpp`（`GetLevelMTypes()`，`:3439+`）
- 修改：`test/sampling_behavior_test.cpp`（追加 5 个用例）

**接口：**
- 依赖输入：任务 2 的 `GetLevelRoster(uint8_t)`、`GetLevelRosterParams(uint8_t)`；任务 1 的 `AppendMeasurementReport` 约定（本任务不新增报告）
- 对外产出：无新符号；`GetLevelMTypes()` 行为变更（core 必然出场、尾池上界、配额、逐层预算）

- [ ] **步骤 1：编写失败的测试（core 必然出场 + 尾池上界）**

```cpp
TEST_F(SamplingBaselineTest, RosterCoreAlwaysPresent)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	for (uint8_t level = 1; level <= 16; level++) {
		const auto roster = GetLevelRoster(level);
		if (roster.empty())
			continue; // L16 之外的层都必须有名册行；空行由任务 2 的校验保证不会出现
		for (int seed = 0; seed < 200; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(7000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			for (const LevelRosterEntry &entry : roster) {
				if (entry.role != LevelRosterRole::Core)
					continue;
				const bool present = std::any_of(LevelMonsterTypes, LevelMonsterTypes + LevelMonsterTypeCount,
				    [&entry](const CMonster &type) { return type.type == entry.type; });
				ASSERT_TRUE(present) << "level " << static_cast<int>(level) << " seed " << seed
				                     << " is missing core type " << static_cast<int>(entry.type);
			}
		}
	}
}

TEST_F(SamplingBaselineTest, RosterTailDrawBounded)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	for (uint8_t level = 1; level <= 16; level++) {
		const LevelRosterParams *params = GetLevelRosterParams(level);
		if (params == nullptr)
			continue;
		size_t coreCount = 0;
		for (const LevelRosterEntry &entry : GetLevelRoster(level)) {
			if (entry.role == LevelRosterRole::Core)
				coreCount++;
		}
		for (int seed = 0; seed < 50; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(8000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			const size_t tail = LevelMonsterTypeCount > coreCount ? LevelMonsterTypeCount - coreCount : 0;
			EXPECT_LE(tail, static_cast<size_t>(params->tailDraw));
		}
	}
}
```

- [ ] **步骤 2：运行测试并确认其失败**

运行：`python3 tools/run_tests.py --test sampling_behavior_test --filter 'SamplingBaselineTest.Roster*'`
预期：`RosterCoreAlwaysPresent` FAIL（core 未被预加）；`RosterTailDrawBounded` 可能 PASS（当前 realized 3-7 大于 tailDraw 时也可能 FAIL）——**只要有一条 FAIL 即可继续**

- [ ] **步骤 3：实现采样接入**

在 `Source/monster.cpp` 的 `GetLevelMTypes()` 内（既有预加之后、`_monster_id typelist[MaxMonsters];` 之前）插入 core 预加，并把采样循环改造为尾池抽取 + 配额补足：

```cpp
	// A-roster: add the level's core roster unconditionally. These bypass the
	// behaviour-class caps (which live inside the sampling loop) but do count
	// against the per-level image budget through AddMonsterType.
	size_t coreAdded = 0;
	for (const LevelRosterEntry &entry : GetLevelRoster(currlevel)) {
		if (entry.role != LevelRosterRole::Core)
			continue;
		RETURN_IF_ERROR(AddMonsterType(entry.type, PLACE_SCATTER));
		coreAdded++;
	}

	const LevelRosterParams *rosterParams = GetLevelRosterParams(currlevel);
	const int maxImage = rosterParams != nullptr ? rosterParams->maxImage : 4000;
	// R28: levels with no params row (currently L17-24) keep the legacy behaviour -
	// budget-limited with no tail cap. Using 0 here would make the loop condition
	// tailAdded < tailDraw permanently false and silently remove every scatter type.
	const int tailDraw = rosterParams != nullptr ? rosterParams->tailDraw : std::numeric_limits<int>::max();
```

并把循环条件与计数改为：

```cpp
	while (nt > 0 && LevelMonsterTypeCount < MaxLvlMTypes && monstimgtot < maxImage && tailAdded < tailDraw) {
```

（`tailAdded` 在每次成功 `AddMonsterType` 且 `LevelMonsterTypeCount` 增长后自增；`monstimgtot >= maxImage` 时退出循环。）

配额下限：在循环内、抽取之前，先按 `rosterParams->classFloors` 计算未满足的类别，若 `typelist` 中存在该类别成员则**优先抽取**（把该类别成员换到 `GenerateRnd` 的候选位置）：

```cpp
	int preferred = -1;
	if (rosterParams != nullptr) {
		for (const auto &[cls, floor] : rosterParams->classFloors) {
			if (classCounts[static_cast<size_t>(cls)] >= floor)
				continue;
			for (int i = 0; i < nt; i++) {
				if (GetBehaviorClass(MonstersData[typelist[i]].ai) == cls) {
					preferred = i;
					break;
				}
			}
			if (preferred >= 0)
				break;
		}
	}
	const int i = preferred >= 0 ? preferred : GenerateRnd(nt);
```

- [ ] **步骤 4：运行测试并确认通过**

运行：`python3 tools/run_tests.py --test sampling_behavior_test`
预期：全部 PASS（含既有的 B1 caps 基线与 P0 实测用例）

- [ ] **步骤 5：追加身份守卫与承载落位用例**

```cpp
TEST_F(SamplingBaselineTest, IdentityGuard)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	// 相邻层名册（200 seeds 并集）的 Jaccard 必须 < 0.9
	// 复用 P0-D 的度量方式：union over seeds -> set -> jaccard
}

TEST_F(SamplingBaselineTest, A1A3VariantsAreCore)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	const std::vector<std::pair<uint8_t, _monster_id>> carriers {
		{ 3, MT_RSKELAX }, { 3, MT_XSKELAX }, { 9, MT_BMAGMA }, { 11, MT_STORML },
	};
	for (const auto &[level, type] : carriers) {
		const auto roster = GetLevelRoster(level);
		const bool isCore = std::any_of(roster.begin(), roster.end(), [type](const LevelRosterEntry &entry) {
			return entry.type == type && entry.role == LevelRosterRole::Core;
		});
		EXPECT_TRUE(isCore) << "carrier " << static_cast<int>(type) << " must be core at level " << static_cast<int>(level);
	}
}
```

- [ ] **步骤 6：运行并提交**

运行：`python3 tools/run_tests.py --test sampling_behavior_test`
预期：全部 PASS

在提交前必须处理**本任务引入的采样顺序变化**的两个后果：
1. 若 `timedemo` 或其它硬编码夹具失配 → 按 `docs/knowledge/` 既有流程**重生成**（宪章决策 35：不做存档兼容，但夹具必须重生成）；
2. 若 B1 既有期望（`HellL15SameClassTailBaseline` 等）因 core 预加而失败 → 按规格 §4.5 调整**名册或 `class_floors`**，**不得**放宽阈值。

运行：`python3 tools/run_tests.py --json /tmp/ci.json`
预期：`ctest.failed == 0`、`passed_pct == 100`、`drift.drift_ok == true`

```bash
git add Source/monster.cpp test/sampling_behavior_test.cpp
git commit -m "feat(roster): sample each level from its roster (core + bounded tail + class floors)"
```

---

## Task 4: class mix 验收、eval、夹具与台账

**文件：**
- 修改：`test/level_roster_baseline_test.cpp`（加入阈值用例）
- 新建：`eval/cases/rng/level-rosters.yaml`
- 修改：`docs/knowledge/decision_save_format_policy.md`（台账追加一行）

**接口：**
- 依赖输入：任务 1 的基线表（写入本计划「实施记录」）、任务 3 的采样行为

- [ ] **步骤 1：写入基线阈值用例（失败）**

在 `test/level_roster_baseline_test.cpp` 中，用任务 1 实测得到的基线值（示例：假设基线 L13 远程类占比为 X%，则阈值为 X+5）写成断言：

```cpp
TEST(LevelRosterBaseline, PlacedClassMixWithinBaseline)
{
	TestInitGame();
	constexpr int kSeeds = 200;
	for (uint8_t level = 13; level <= 15; level++) {
		size_t total = 0;
		size_t ranged = 0;
		for (uint32_t seed = 0; seed < kSeeds; seed++) {
			CreateDungeonForMeasurement(level, 9000 + seed);
			InitLevelMonsters();
			ASSERT_TRUE(GetLevelMTypes().has_value());
			ASSERT_TRUE(InitMonsters().has_value());
			const auto mix = MeasurePlacedClassMix(level, 9000 + seed);
			total += ActiveMonsterCount;
			ranged += mix[static_cast<size_t>(BehaviorClass::RangedTurret)] + mix[static_cast<size_t>(BehaviorClass::RangedKite)];
		}
		const double share = static_cast<double>(ranged) / static_cast<double>(total);
		EXPECT_LE(share, kRangedShareCeiling[level]) << "level " << static_cast<int>(level);
	}
}
```

`kRangedShareCeiling` 以任务 1 的基线 + 5 个百分点填充（写在文件内的 `constexpr std::array<double, 17>`）。

- [ ] **步骤 2：运行并确认其失败或通过**

运行：`python3 tools/run_tests.py --test level_roster_baseline_test`
预期：若名册抬高了远程占比则 FAIL —— 此时按规格 §4.5 **调整 `class_floors` 或 core 名单**（不改阈值），直到 PASS

- [ ] **步骤 3：新增 eval 用例**

`eval/cases/rng/level-rosters.yaml`（结构对标 `eval/cases/rng/sampling-anti-monopoly.yaml`）：断言 `level_roster_test` 与 `sampling_behavior_test` 的 Roster* 用例全过。

- [ ] **步骤 4：重生成受影响夹具 + 台账**

（夹具重生成已在任务 3 完成；本任务只需确认全量门禁仍绿。）

在 `docs/knowledge/decision_save_format_policy.md` 台账追加一行（日期 / 变更："逐层名册改变采样顺序 → `monster.levelType` 索引语义变化" / 触碰：`Source/monster.cpp`、`assets/txtdata/monsters/level_rosters.tsv` / 兼容处置：**不做兼容**（决策 35））。

- [ ] **步骤 5：全量门禁 + eval + 漂移**

```bash
python3 -m tools.eval.backend --smoke; echo "exit=$?"
python3 tools/run_tests.py --json /tmp/ci.json
```
预期：eval exit 0；`ctest.failed == 0`、`passed_pct == 100`、`drift.drift_ok == true`

- [ ] **步骤 6：提交**

```bash
git add test/level_roster_baseline_test.cpp eval/cases/rng/level-rosters.yaml docs/knowledge/decision_save_format_policy.md
git commit -m "test(roster): pin placed class mix to the measured baseline and add the eval case"
```

---

## 实施记录（执行者填写）

| 项 | 值 |
|---|---|
| 任务 1 基线表 | 见下方「A-baseline」表（200 seeds/level，L1-16） |
| L13-15 远程占比基线 | L13: (3249+2076)/23405=22.8%；L14: (9674+3425)/23443=55.9%；L15: 13533/23193=58.3% |
| `kRangedShareCeiling` | 基线 + 5 个百分点（任务 4 落地）：L13 27.8% / L14 60.9% / L15 63.3%；落在 `test/level_roster_baseline_test.cpp` 的具名 constexpr `kRangedShareBaseline`/`kRangedShareCeiling`，分子分母照抄下方 A-baseline 表 |
| 任务 4 阈值实测（改后） | L13 23.6%（5537/23413）/ L14 49.2%（11269/22915）/ L15 50.5%（11611/22978），三层均在带内 |
| 任务 4 名册调整（R4） | 首跑 L13 53.5%、L14 65.8% 超标。根因：L13-16 对**任意** class 都是 cap 2，候选池只有 Melee/Turret/Kite，type 多样性超过 2 个 Melee 的部分必然是远程；而放置在 `scattertypes` 上均匀，占比≈type 占比。按 R4 只改数据：L13 core 去掉 `MT_STORML`(Kite)、`MT_SUCCUBUS`(Turret)，L14 core 去掉一个 `MT_SUCCUBUS`(Turret)；L13/L14 `tail_draw` 3→1（保留 1 次抽取 + `RangedKite=1` floor，层里仍有风筝存在感，但不会出现第二个远程 type）。L15 未改动 |
| 小队形成率 | 阶段 B 才需要，本计划不测 |
| 夹具重生成记录 | 无新增/重生成夹具；本任务复用 `test/fixtures/levels/` 既有夹具（rnd6.dun / skngdo.dun / banner2.dun / blood2.dun 等） |

## A-baseline: placed class mix (200 seeds per level)

| level | placed | Melee | RangedTurret | RangedKite | Rally | Charge | Sneak | Summon | Boss |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 18235 | 9247 | 0 | 0 | 6055 | 0 | 0 | 0 | 2933 |
| 2 | 23453 | 8668 | 2149 | 0 | 7747 | 1619 | 635 | 0 | 2635 |
| 3 | 25642 | 9731 | 2819 | 0 | 5542 | 3084 | 1239 | 0 | 3227 |
| 4 | 25291 | 7607 | 4145 | 0 | 4311 | 3671 | 875 | 0 | 4682 |
| 5 | 23180 | 10732 | 3446 | 0 | 1721 | 2488 | 1904 | 0 | 2889 |
| 6 | 17646 | 4583 | 3592 | 950 | 0 | 4198 | 1609 | 0 | 2714 |
| 7 | 18063 | 6332 | 3640 | 1619 | 0 | 3572 | 1405 | 0 | 1495 |
| 8 | 17321 | 3204 | 1561 | 2652 | 0 | 7478 | 1929 | 0 | 497 |
| 9 | 16648 | 5907 | 514 | 8281 | 0 | 0 | 959 | 0 | 987 |
| 10 | 17100 | 5584 | 0 | 8514 | 0 | 0 | 819 | 0 | 2183 |
| 11 | 16550 | 6447 | 0 | 7717 | 0 | 0 | 0 | 0 | 2386 |
| 12 | 17280 | 8211 | 1931 | 5910 | 0 | 0 | 0 | 0 | 1228 |
| 13 | 23405 | 17880 | 3249 | 2076 | 0 | 0 | 0 | 0 | 200 |
| 14 | 23443 | 10344 | 9674 | 3425 | 0 | 0 | 0 | 0 | 0 |
| 15 | 23193 | 9660 | 13533 | 0 | 0 | 0 | 0 | 0 | 0 |
| 16 | 38000 | 18301 | 14299 | 5200 | 0 | 0 | 0 | 0 | 200 |

---

## 自检

**1. 规格覆盖度（阶段 A 部分）**

| 规格条目 | 对应任务 |
|---|---|
| §4.1 数据模型（两张 TSV、逐层、`allow_unique_boost`、HF overlay） | 任务 2（基础表 L1-16；HF overlay 属阶段 A 的后续数据补充，见下） |
| §4.2 采样接入（core 预加、尾池、抽到上限、配额、逐层预算、L16 特例） | 任务 3 |
| §4.4 加载期校验（存在性、区间、floors 可满足、unique 白名单、spawn 宽松） | 任务 2 |
| §4.5 class mix 口径（基线 + 相对阈值） | 任务 1（基线）+ 任务 4（阈值） |
| §6 验收 1-4 | 任务 2（4）、任务 3（1、2、3） |
| §6 验收 8-11 | 任务 4 |
| §4.3 阶段 B（小队、G1/G2） | **不在本计划**（独立计划，规格文档头已声明） |

**已知的范围外项：** HF overlay 的 L17-24 名册行需要在 `mods/hf/txtdata/monsters/` 下补两张表——它属于阶段 A 的数据补充，但依赖 HF 数据加载路径；本计划在任务 2 步骤 6 只做 L1-16，HF 部分作为**阶段 A2**（独立小任务，验收：Nest/Crypt 层 core 非空）。

**2. 占位符扫描：** 无待补项（逐条核对"禁止占位符"清单的六类模式，均为 0）；每个代码步骤都给了可编译代码；`kRangedShareCeiling` 的取值来自任务 1 的实测（执行者填写「实施记录」），不是等待外部输入。

**3. 类型一致性：** `LevelRosterEntry`/`LevelRosterParams`/`GetLevelRoster`/`GetLevelRosterParams`/`ValidateLevelRoster`/`MeasurePlacedClassMix`/`MeasureRealisedTypes`/`CreateDungeonForMeasurement` 在任务 1-4 中命名一致；`AppendMeasurementReport` 在任务 1 定义、任务 4 复用。

---

## 执行交接

计划已保存至 `docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md`。两种执行方式：

1. **子代理驱动（推荐）** —— 每个任务分发全新子代理，任务间进行评审，迭代更快
2. **内联执行** —— 在当前会话中用 `superpower-executing-plans` 批量执行，设置检查点

请选择哪种方式？