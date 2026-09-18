## Task 1: harness（Infra，纯测试，不改任何游戏代码）

**文件：**
- 新建：`test/monster_ai_harness.hpp`、`test/monster_behavior_test.cpp`
- 修改：`CMake/Tests.cmake`（`tests` 列表）、`tools/run_tests.py`（`TEST_TARGETS`）

**接口：**
- 依赖输入：无
- 对外产出（后续所有任务依赖）：
  - `devilution::test::MonsterAiHarness`（class，`::testing::Test` 派生）：`SetUpTestSuite()` 载入数据；`SetUp()` 构造 L1 教堂最小世界
  - `Monster *SpawnAt(WorldTilePosition, MonsterAIID wanted)`：按 AI 类型从当前关卡池挑一个 `LevelMonsterTypes` 索引并 `AddMonster`
  - `Monster *SpawnAt(WorldTilePosition, size_t levelTypeIndex)`
  - `void TickDecision(Monster &, int ticks = 1)`：裸 `AiProc` + `animInfo.processAnimation`
  - `void TickWorld(int ticks = 1)`：真实 `ProcessMonsters()`（需先 `AllTilesVisible()`）
  - `void AllTilesVisible()`、`void UseLevel(uint8_t currlevel, dungeon_type type)`、`static Snapshot SnapshotOf(const Monster &)`

- [ ] **步骤 1：新建 harness 头文件**

`test/monster_ai_harness.hpp`（**CRLF**）。核心内容（写进文件，供后续任务包含）：

```cpp
#pragma once

#include <cstring>
#include <numeric>

#include <gtest/gtest.h>

#include "engine/assets.hpp"
#include "engine/random.hpp"
#include "game_mode.hpp"
#include "levels/gendung.h"
#include "missiles.h"
#include "monster.h"
#include "player.h"
#include "spells.h"
#include "tables/monstdat.h"
#include "tables/playerdat.hpp"

namespace devilution::test {

/** Two-tier monster AI test harness.
 *
 *  Decision tier: TickDecision() calls AiProc directly - fast, no world state
 *  needed beyond positions. Use it for branch/decision assertions.
 *  Behaviour tier: TickWorld() drives the real ProcessMonsters() loop, which
 *  needs tile visibility (dFlags) and InitMissiles(); use it to verify that an
 *  attack actually produces a missile.
 */
class MonsterAiHarness : public ::testing::Test {
public:
	static void SetUpTestSuite()
	{
		LoadCoreArchives();
		LoadGameArchives();
		if (!HaveMainData()) {
			missingMpq_ = true;
			return;
		}
		LoadSpellData();
		LoadPlayerDataFiles();
		LoadMissileData();
		LoadMonsterData();
		LoadItemData();
	}

	void SetUp() override
	{
		if (missingMpq_)
			GTEST_SKIP() << "MPQ assets not found - skipping";

		Players.resize(1);
		MyPlayer = &Players[0];
		*MyPlayer = {};
		MyPlayer->position.tile = WorldTilePosition { 15, 15 };
		MyPlayer->position.future = WorldTilePosition { 15, 15 };
		MyPlayer->_pMaxHPBase = 64 << 6;
		MyPlayer->_pHPBase = 64 << 6;

		memset(dungeon, 0, sizeof(dungeon));
		memset(dMonster, 0, sizeof(dMonster));
		memset(dPlayer, 0, sizeof(dPlayer));
		memset(dTransVal, 0, sizeof(dTransVal));

		currlevel = 1;
		leveltype = DTYPE_CATHEDRAL;
		setlevel = false;

		// Must use the real entry point: it resets LevelMonsterTypeCount and
		// monstimgtot. Hand-rolling the reset leaves the sampling budget
		// exhausted, and GetLevelMTypes() then yields a single-entry pool.
		InitLevelMonsters();
	}

	/** Switches the simulated level and rebuilds the monster-type pool. */
	void UseLevel(uint8_t level, dungeon_type type, uint32_t seed = 0x5EED)
	{
		currlevel = level;
		leveltype = type;
		InitLevelMonsters();
		SetRndSeed(seed); // the pool composition is sampled from the LCG
		const auto types = GetLevelMTypes();
		ASSERT_TRUE(types.has_value()) << types.error();
	}

	/** Index of the first level monster type whose AI matches, or SIZE_MAX. */
	static size_t FindLevelType(MonsterAIID ai)
	{
		for (size_t i = 0; i < LevelMonsterTypeCount; i++) {
			if (LevelMonsterTypes[i].data().ai == ai)
				return i;
		}
		return SIZE_MAX;
	}

	static void ResetMonstersOnly()
	{
		ActiveMonsterCount = 0;
		std::iota(std::begin(ActiveMonsters), std::end(ActiveMonsters), 0U);
		for (Monster &monster : Monsters)
			monster = Monster {}; // never memset: Monster holds a unique_ptr
		memset(dMonster, 0, sizeof(dMonster));
	}

	/** Spawns a monster of the given level type. Seeding happens *before*
	 *  AddMonster on purpose: InitMonster rolls HP and animation frames from
	 *  the LCG, so seeding after it makes HP unreproducible. */
	static Monster *SpawnAt(WorldTilePosition position, size_t levelTypeIndex, uint32_t seed = 0x1234)
	{
		SetRndSeed(seed);
		Monster *monster = AddMonster(position, Direction::North, levelTypeIndex, true);
		if (monster == nullptr)
			return nullptr;
		monster->enemy = 0;
		monster->enemyPosition = MyPlayer->position.future;
		monster->activeForTicks = UINT8_MAX;
		return monster;
	}

	/** Spawns the first monster of the given AI; returns nullptr when the
	 *  sampled level pool contains no such monster. */
	static Monster *SpawnAi(WorldTilePosition position, MonsterAIID ai, uint32_t seed = 0x1234)
	{
		const size_t index = FindLevelType(ai);
		if (index == SIZE_MAX)
			return nullptr;
		return SpawnAt(position, index, seed);
	}

	static void TickDecision(Monster &monster, int ticks = 1)
	{
		for (int tick = 0; tick < ticks; tick++) {
			AiProc[static_cast<size_t>(monster.ai)](monster);
			monster.animInfo.processAnimation(false);
		}
	}

	void AllTilesVisible()
	{
		for (int x = 0; x < MAXDUNX; x++) {
			for (int y = 0; y < MAXDUNY; y++)
				dFlags[x][y] |= DungeonFlag::Visible;
		}
	}

	void TickWorld(int ticks = 1)
	{
		for (int tick = 0; tick < ticks; tick++)
			ProcessMonsters();
	}

	static bool missingMpq_;
};

inline bool MonsterAiHarness::missingMpq_ = false;

} // namespace devilution::test
```

- [ ] **步骤 2：新建测试文件并注册到两处**

`test/monster_behavior_test.cpp`（**CRLF**）先只放一个 harness 自检用例：

```cpp
#include "monster_ai_harness.hpp"

namespace devilution::test {
namespace {

using MonsterBehaviorHarnessTest = MonsterAiHarness;

TEST_F(MonsterBehaviorHarnessTest, HarnessSpawnsMonsterAndRunsTicks)
{
	UseLevel(1, DTYPE_CATHEDRAL);
	ASSERT_GT(LevelMonsterTypeCount, 0u);
	const size_t index = FindLevelType(MonsterAIID::SkeletonMelee);
	ASSERT_NE(index, SIZE_MAX) << "level 1 cathedral pool has no SkeletonMelee monster";

	ResetMonstersOnly();
	Monster *monster = SpawnAt(WorldTilePosition { 15, 17 }, index);
	ASSERT_NE(monster, nullptr);

	TickDecision(*monster, 30);
	SUCCEED();
}

TEST_F(MonsterBehaviorHarnessTest, HarnessWorldTierFiresRangedMissile)
{
	UseLevel(9, DTYPE_CAVES);
	AllTilesVisible();
	InitMissiles();
	ResetMonstersOnly();
	Monster *monster = SpawnAi(WorldTilePosition { 15, 20 }, MonsterAIID::Acid);
	if (monster == nullptr)
		GTEST_SKIP() << "sampled level 9 pool has no Acid monster";

	for (int tick = 0; tick < 200 && Missiles.empty(); tick++)
		ProcessMonsters();
	EXPECT_FALSE(Missiles.empty()) << "ranged attack never produced a missile";
}

} // namespace
} // namespace devilution::test
```

注册（**两处都要**）：

```bash
# CMake/Tests.cmake：在 tests 列表里加入（按现有归类风格）
#   monster_behavior_test
# tools/run_tests.py：在 TEST_TARGETS 列表里加入
#   "monster_behavior_test",
```

- [ ] **步骤 3：构建并运行**

```bash
python3 tools/run_tests.py --test monster_behavior_test
```

预期：2 个用例通过（第二个若池中无 Acid 则 SKIP，属预期）。

- [ ] **步骤 4：确认注册没脱钩**

```bash
python3 - <<'PY'
import re
cmake = open('CMake/Tests.cmake').read()
def block(name):
    m = re.search(r'set\(' + name + r'\s*(.*?)\n\)', cmake, re.S)
    return {l.strip() for l in m.group(1).splitlines() if l.strip()}
registered = block('tests') | block('standalone_tests') | {'text_render_integration_test'}
run = open('tools/run_tests.py').read()
listed = set(re.findall(r'"([a-z_0-9]+)"', re.search(r'TEST_TARGETS = \[(.*?)\n\]', run, re.S).group(1)))
print('registered-not-listed:', sorted(registered - listed))
print('listed-not-registered:', sorted(listed - registered))
PY
```

预期：两个集合都输出 `[]`。

- [ ] **步骤 5：全量门禁（harness 是纯测试，不应有任何行为回归）**

```bash
python3 tools/run_tests.py --json /tmp/ci-harness.json
```

预期：`ctest.failed == 0`、`passed_pct == 100`、`drift.drift_ok == true`。

- [ ] **步骤 6：提交**

```bash
git add test/monster_ai_harness.hpp test/monster_behavior_test.cpp CMake/Tests.cmake tools/run_tests.py
git commit -F - <<'MSG'
test(monster): add the two-tier monster AI tick harness

Infra prerequisite for the density-fix specs (A1/A3/C2/A2). The existing
dark_expedition_* tests never run AiProc and ai_registry_test only checks
the dispatch table, so nothing could assert on AI decisions or on an attack
actually producing a missile.

Decision tier: TickDecision() calls AiProc directly plus processAnimation.
Behaviour tier: TickWorld() drives the real ProcessMonsters() loop, which
needs InitMissiles() and tile visibility (dFlags).

Three setup details cost real time in a throwaway spike and are encoded
here: InitLevelMonsters() must be used instead of a hand-rolled reset (the
sampling loop stops at monstimgtot >= 4000, so a stale budget leaves a
one-entry pool); SetRndSeed() must run *before* AddMonster() because
InitMonster rolls HP and animation frames from the LCG; and Monsters must
never be memset (Monster holds a unique_ptr).

Registered in both CMake/Tests.cmake and tools/run_tests.py TEST_TARGETS -
the two lists drifted before and cost the gate 13 unverified test cases.
MSG
```

---

