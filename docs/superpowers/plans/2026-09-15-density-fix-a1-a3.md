# 密度修复框架 A1/A3 实施计划（harness → A1 教堂骷髅区分 → A3 洞穴风筝组合）

> **面向 Agent 执行者：** 必需子技能：使用 superpower-subagent-driven-development（推荐）或 superpower-executing-plans 按任务逐项执行本计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 先落地一个最小怪物 AI tick harness（Infra，纯测试），再按已定稿规格实现 A1（教堂 8 行骷髅拆 3 变体：冲锋/狂乱/普通）与 A3（2 只洞穴风筝怪转条件性冲锋），每项都有可执行的验收与 eval case。

**架构：** harness 分两层——**决策层**直接调 `AiProc[ai](monster)`（快、无环境依赖，用于判定分支）、**行为层**调真实 `ProcessMonsters()`（含可见性/动画/弹道，用于验证冲锋弹道与 `AddMissile` 真正发生）。A1/A3 各加两个/一个新 AI 枚举值 + `AiProc` 表条目 + 新 AI 函数（派生自 `SkeletonAi`/`AiRangedAvoidance`，**逐字复用引擎既有的冲锋与追击调用点**，不发明 API），数据改动只落在 `monstdat.tsv` 的 `ai` 列与 `minDamageSpecial/maxDamageSpecial` 列。

**技术栈：** C++23 / CMake+Ninja / GoogleTest + ctest / Python 工具链（`tools/run_tests.py`、`tools/check_drift.py`、`tools/eval/backend.py`、`tools/eval/sync_case_sets.py`）

**规格：** `docs/superpowers/specs/2026-08-10-cathedral-skeleton-differentiation-design.md`（A1，v4）、`docs/superpowers/specs/2026-08-10-cave-kite-combination-design.md`（A3，v4）、`docs/superpowers/specs/2026-08-10-density-fix-framework-overview.md`（框架总览）、`docs/superpowers/specs/2026-08-10-monster-behavior-density-design.md`（C2，harness 前置来源）。**计划与规格冲突时以规格为准**；本计划对规格的两处偏离已显式裁决（见「探测阶段已确认的事实」§D）。

## 全局约束

- 分类已定：A1/A3 属**扩充层（Expansion）**，按各自规格第 2 节；harness 属 **Infra**（纯测试、玩家零感知）。
- **零新美术**：只复用既有 16 帧 special / Attack 动画与 tint，不新增贴图。
- **不改存档格式**：`Monster` 会被 `SaveMonster`/`LoadMonster`（`Source/loadsave.cpp`）与 MP 的 `TSyncMonster` 序列化，**禁止新增 `Monster` 字段**；冷却复用已持久化的 `goalVar3`（见事实 §D1）。
- **数据改动只允许**改 `assets/txtdata/monsters/monstdat.tsv` 与 `mods/hf/txtdata/monsters/monstdat.tsv` 的既有行；**不得新增怪物行**；两处必须同步（它们在这些行上逐字相同，实测见事实 §C）。
- 行尾：C++ 与 `.tsv` 均为 **CRLF**；`.md/.yml/.yaml/.py` 为 **LF**。改动后逐文件自查。
- 测试注册**两处都要**：`CMake/Tests.cmake`（`tests` 列表）**与** `tools/run_tests.py` 的 `TEST_TARGETS`。本轮上游同步刚修过这两处脱钩导致的覆盖缺口（`sampling_behavior_test`），不要再造同类问题。
- 门禁判据：`python3 tools/run_tests.py --json /tmp/ci.json` → `ctest.failed == 0`、`passed_pct == 100`、`drift.drift_ok == true`（5 项 PASS）；`python3 -m tools.eval.backend --smoke` exit 0。
- 每个任务一个提交（conventional commits）；只有 Task 1（harness）是纯测试、不碰游戏代码。

---

## 探测阶段已确认的事实（2026-09-15 实测，实施时直接引用）

### A. harness 配方（已用一次性 spike 跑通，spike 已删除）

| 环节 | 做法 | 依据 |
|---|---|---|
| 数据加载 | `LoadCoreArchives()`+`LoadGameArchives()`（需 MPQ）+ `LoadSpellData()`/`LoadPlayerDataFiles()`/`LoadMissileData()`/`LoadMonsterData()`/`LoadItemData()` | spike 实测 |
| 玩家 | `Players.resize(1)`、`MyPlayer=&Players[0]`、给 `position.tile/future` 与 HP | spike |
| 网格（可走性） | **可走性读 `SOLData[dPiece[x][y]]`，与 `dungeon` 无关**（`Source/levels/dun_tile_data.hpp:206`）。harness 设 `memset(dPiece,0,…)` + `SOLData[0]=TileProperties::None`（地板）、`SOLData[1]=TileProperties::Solid`（墙，用于墙角/LOS 构造）；另 `memset(dungeon/dMonster/dPlayer/dTransVal,0,…)` | `test/tile_properties_test.cpp` 先例 |
| 关卡 | `currlevel` / `leveltype`（`dungeon_type`）/ `setlevel` | `Source/levels/gendung.h:134-137` |
| **怪物池初始化** | **必须用 `InitLevelMonsters()`**（`monster.h:500`），它会清 `LevelMonsterTypeCount`/`monstimgtot` 并重建 `ActiveMonsters`。手写 memset 会让池只剩 1 个——采样循环有 `monstimgtot < 4000` 预算 | spike 实测（踩过） |
| 池组成 | `SetRndSeed(x)` → `GetLevelMTypes()`；L1 教堂池 7 类、L9 洞穴池 4 类，可复现 | spike 实测 |
| **种子时序** | **`SetRndSeed()` 必须在 `AddMonster()` 之前**——`InitMonster` 的 HP 掷骰与动画帧掷骰消费 LCG，设在之后则 HP 不可复现 | spike 实测（踩过） |
| 怪物对象 | `AddMonster(pos, dir, levelTypeIndex, true)`；**不要 `memset(Monsters,…)`**（`Monster` 首字段是 `unique_ptr`，memset 是 UB）→ 用 `monster = Monster{}` | spike 实测 |
| 可见性 | `IsTileVisible` 就是 `dFlags[x][y] & DungeonFlag::Visible`；只调 `AiProc` 可手动设 `activeForTicks=UINT8_MAX` 绕过；跑 `ProcessMonsters()` 必须先置可见位 | `Source/levels/gendung.h` |
| 行为层 | `ProcessMonsters()`（`monster.h:550`）+ `InitMissiles()`：spike 中 L9 的 Acid 怪 200 tick 内**真的发出了导弹**（`Missiles` 非空，`_mitype=57`） | spike 实测 |
| 确定性 | 裸 `AiProc` + `animInfo.processAnimation` 连跑 30 tick，同种子两次快照完全一致 | spike 实测 |
| 音效 | `PlayEffect` 在 `gbSndInited`/`gbSoundOn` 守卫**之前**就 `GenerateRnd(2)`（`Source/monster.cpp`），headless 下不播声音但**照样消耗 1 次 LCG**；冲锋路径会走它。故 harness 固定 `gbSoundOn=false; gbSndInited=false;`（声明在 `Source/engine/sound.h`）——MP 确定性用例必须两侧同样设定。先例 `test/timedemo_test.cpp` 显式设两者 | 复核实测 |

### B. 引擎调用点（**实现时逐字复用，不要照本计划的伪代码发明签名**）

| 用途 | 权威来源 |
|---|---|
| 即时冲锋（同 tick `AddMissile(Rhino)` + `mode=Charge`） | `Source/monster.cpp` 的 `RhinoAi` 与 `SnakeAi` 实现（约 `monster.cpp:2284-2292` / `2675-2683`，行号以现行代码为准） |
| 冲锋的视线/距离门控 | 同上（`RhinoAi` 用的 LOS 助手与距离常量） |
| 狂乱者的追击分支（`goal=Attack`：近距 `StartAttack` / 远距 `RandomWalk`） | `FallenAi` 的 pursuit 分支（约 `monster.cpp:2364-2368`）与 `goalVar1` 倒计时（约 `2316-2320`） |
| 风筝基线行为 | `AiRangedAvoidance`（A3 的 `KiteChargerAi` 在其之前做条件判定，不满足时直接调用它） |

### C. 数据表事实（`assets/txtdata/monsters/monstdat.tsv` 与 `mods/hf/…` 在这些行上逐字相同）

| `_monster_id` | name | 现行 `ai` | 现行 special | `trnFile` | 层段 |
|---|---|---|---|---|---|
| `MT_WSKELAX` | Skeleton | SkeletonMelee | 0-0 | `skelaxe\white` | 1-2 |
| `MT_TSKELAX` | Corpse Axe | SkeletonMelee | 0-0 | `skelaxe\skelt` | 2-3 |
| `MT_RSKELAX` | Burning Dead | SkeletonMelee | 0-0 | *(空=基础棕)* | 2-4 |
| `MT_XSKELAX` | Horror | SkeletonMelee | 0-0 | `skelaxe\black` | 3-5 |
| `MT_WSKELSD` | Skeleton Captain | SkeletonMelee | 0-0 | `skelsd\white` | 1-3 |
| `MT_TSKELSD` | Corpse Captain | SkeletonMelee | 0-0 | `skelsd\skelt` | 2-4 |
| `MT_RSKELSD` | Burning Dead Captain | SkeletonMelee | 0-0 | *(空)* | 3-5 |
| `MT_XSKELSD` | Horror Captain | SkeletonMelee | 0-0 | `skelsd\black` | 4-6 |
| `MT_BMAGMA` | Hell Stone | Magma | **0-0** | `magma\blue` | 9-11 |
| `MT_STORML` | Storm Lord | Storm | **4-16** | `thin\thinv2` | 11-13 |
| `MT_XACID` | Lava Maw | Acid | 0-0 | `acid\acidr` | 12-14 |

### D. 计划对规格的两处偏离（已裁决）

- **D1 — `chargeCooldown` 复用 `goalVar3`，不新增 `Monster` 字段。** 依据：`Source/loadsave.cpp` 的 `SaveMonster`/`LoadMonster` 会序列化 `goalVar1/2/3`（`goalVar1` 写成 int32 读回 int16、`goalVar2/3` 读回 int8）与 `var1/2/3`，`Monster` **不是**临时结构；新增字段要么改存档格式（红线：存档兼容）、要么不持久化（读档后冷却丢失）。而 `goalVar1` 被 A1 的狂乱倒计时占用、`goalVar2` 被 `AiRangedAvoidance` 的 `RoundWalk` 占用，**`goalVar3` 在这两类 AI 上无使用者**（`monster.h` 注明它只被 `ScavengerAi`/`MegaAi`/`GolemAi` 使用），且 A3 规格原文即写「复用 goalVar 模式」。故本计划统一用 `goalVar3` 作为冲锋冷却（int8 语义，范围 0-127 tick 足够）。**若错的代价**：低——冷却语义完全等价，且不碰存档格式；若日后需要独立字段，再单独立项改存档版本。
- **D2 — A1 规格 §4.3 的伪代码只作意图说明，签名与门控一律以引擎现行调用点为准**（见事实 §B）。依据：规格 v4 已把「前摇」「攻速/移速加成」等虚构机制删除，但伪代码里的 `LineClear(...)`/`AddMissile(...)` 参数列表仍是示意；直接照抄可能编译不过或行为不等价。**若错的代价**：低——编译门禁立即暴露。
- **D3 — Storm Lord 的冲锋伤害是「有意的口径例外」**：TSV 实测 `MT_STORML` 普通伤害 12-24、special 4-16，与「special = 普通伤害值」的 A1/A3 口径不符；规格 A3 §4.2 明文要求 Storm Lord 不改列，故保持 4-16 并在 Task 8 步骤 1 显式声明该例外（否则会被误判为漏改）。若规格作者本意是 12-24，**先改 A3 规格 §4.2 再改数据**。**若错的代价**：低——只是一个承载的冲锋伤害偏低，且与规格字面一致。

---

## 文件结构

**新建**

| 文件 | 职责 |
|---|---|
| `test/monster_ai_harness.hpp` | harness 共享设施：数据加载、最小世界构造、`AddMonster` 包装、决策层/行为层 tick 驱动、快照比较。**只放 harness，不放任何具体 AI 的断言** |
| `test/monster_behavior_test.cpp` | 所有 A1/A3 的 gtest 用例（按测试套件名分组：`SkeletonDifferentiation*`、`KiteCombination*`），注册为新的测试目标 |
| `eval/cases/combat/monster-skeleton-differentiation.yaml` | A1 的 eval case（gtest 二进制 + filter + 断言计数） |
| `eval/cases/combat/monster-kite-combination.yaml` | A3 的 eval case |

**修改**

| 文件 | 改动 |
|---|---|
| `Source/tables/monstdat.h` | `MonsterAIID` 追加 3 个枚举值（`SkeletonCharge`、`SkeletonBerserk`、`KiteCharger`），加在 `BoneDemon` 之后 |
| `Source/monster.cpp` | `AiProc` 表追加 3 个条目；新增 `SkeletonChargeAi` / `SkeletonBerserkAi` / `KiteChargerAi` 与文件内常量 |
| `assets/txtdata/monsters/monstdat.tsv`、`mods/hf/txtdata/monsters/monstdat.tsv` | A1：3 行 `ai` 列 + 2 行 special 伤害列；A3：2 行 `ai` 列 + 1 行 special 伤害列 |
| `CMake/Tests.cmake` | `tests` 列表加入 `monster_behavior_test` |
| `tools/run_tests.py` | `TEST_TARGETS` 加入 `monster_behavior_test` |
| `eval/cases/_nightly.yaml` | 用 `python3 -m tools.eval.sync_case_sets --write` 重新生成（该文件头注明「不要手改」） |
| `docs/superpowers/specs/2026-08-10-*.md` | 实施完成后按各规格第 8 节把状态改为「已实施」 |

---

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
#include "engine/sound.h"
#include "game_mode.hpp"
#include "levels/dun_tile_data.hpp"
#include "levels/gendung.h"
#include "levels/tile_properties.hpp"
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

		// PlayEffect rolls GenerateRnd(2) *before* its gbSndInited/gbSoundOn
		// guard (Source/monster.cpp), so the charge path consumes an LCG draw
		// even with sound off. Pin both flags so draw counts are reproducible.
		gbSoundOn = false;
		gbSndInited = false;
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

		// Walkability reads SOLData[dPiece[x][y]], NOT `dungeon`
		// (Source/levels/dun_tile_data.hpp:206). Precedent for this pairing:
		// test/tile_properties_test.cpp sets dPiece and SOLData together.
		memset(dPiece, 0, sizeof(dPiece));
		SOLData[0] = TileProperties::None; // tile 0: walkable floor
		SOLData[1] = TileProperties::Solid; // tile 1: wall (cornered/LOS setups)

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

	/** Turns a tile into a wall: dPiece -> a solid SOLData entry. */
	static void SetWall(Point position)
	{
		dPiece[position.x][position.y] = 1;
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
registered = block('tests') | block('standalone_tests')
# Conditional registrations such as text_render_integration_test are added via
# list(APPEND ...), so parse those too instead of hardcoding one name.
registered |= set(re.findall(r'list\(APPEND\s+(?:tests|standalone_tests)\s+([a-z_0-9]+)\)', cmake))
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

## Task 2: A1 —— 三个新 AI 枚举值与 `AiProc` 表条目

**文件：**
- 修改：`Source/tables/monstdat.h`（`MonsterAIID`）、`Source/monster.cpp`（`AiProc`）

**接口：**
- 依赖输入：Task 1 的 harness（仅用于后续测试）
- 对外产出：`MonsterAIID::SkeletonCharge`、`MonsterAIID::SkeletonBerserk`（A1）与 `MonsterAIID::KiteCharger`（A3）三个枚举值；`AiProc` 对应槽位指向三个新函数（函数体在 **Task 3/7** 实现）；同时完成 `GetBehaviorClass` 登记与枚举遍历上界扩展（否则 B1 会被静默破坏、新值无人检查）

- [ ] **步骤 1：追加枚举值**

在 `Source/tables/monstdat.h` 的 `enum class MonsterAIID : int8_t` 中，`BoneDemon` 之后追加（保持注释风格一致）：

```cpp
	SkeletonCharge, // A1: Horror-family skeleton that charges with Rhino
	SkeletonBerserk, // A1: Burning Dead that pursues once below half HP
	KiteCharger, // A3: cave kite monster that charges when cornered or dying
```

- [ ] **步骤 2：在 `AiProc` 表中登记槽位并补前置声明**

在 `Source/monster.cpp` 的 `AiProc` 初始化列表末尾（`BoneDemon` 条目之后）追加：

```cpp
	/*MonsterAIID::SkeletonCharge  */ &SkeletonChargeAi,
	/*MonsterAIID::SkeletonBerserk */ &SkeletonBerserkAi,
	/*MonsterAIID::KiteCharger     */ &KiteChargerAi,
```

**位置初始化列表必须与枚举顺序严格对应**（错位会让整张表偏移）。在 `AiProc` 定义之前补三条前置声明（与文件中既有 `void XxxAi(Monster &monster);` 风格一致，**注意分号**）：

```cpp
void SkeletonChargeAi(Monster &monster);
void SkeletonBerserkAi(Monster &monster);
void KiteChargerAi(Monster &monster);
```

- [ ] **步骤 3：把 3 个新 AI 登记进行为分类（否则静默破坏 B1）**

**这不是可选项**：`Source/tables/monstdat.cpp` 的 `GetBehaviorClass` 以 `default: return BehaviorClass::Boss;` 收尾（无 `-Wswitch` 报错，编译静默通过），而 `Source/monster.cpp` 的 B1 采样 cap **正是**按 `GetBehaviorClass(...)` 计数（`capKite = currlevel >= 9 && currlevel <= 12`，`overCap` 判 `cls == BehaviorClass::RangedKite`）。若不登记：

- `MT_BMAGMA`/`MT_STORML` 改派 `KiteCharger` 后会落入 `Boss`，**洞穴 kite cap 直接失去两个成员**，L13-15 的同类 cap 也会把它们计成 Boss——这与 A3 规格「洞穴 38% 都是风筝怪」的立论冲突；
- `test/sampling_behavior_test.cpp` 的 `CavesKiteTailBaseline` / `CavesAnyClassTailBaseline` 会因分布改变而可能翻红。

改 `Source/tables/monstdat.cpp` 的 `GetBehaviorClass`：`KiteCharger` 归入返回 `BehaviorClass::RangedKite` 的那一组（与 `Magma`/`Storm`/`Acid`/`BoneDemon` 同组），`SkeletonCharge`/`SkeletonBerserk` 归入返回 `BehaviorClass::Melee` 的一组（与 `SkeletonMelee` 同组）；同步更新 `Source/tables/monstdat.h` 里 `GetBehaviorClass` 上方的注释（枚举清单）。

- [ ] **步骤 4：扩展枚举遍历上界（否则新值无人检查）**

`test/ai_registry_test.cpp` 与 `test/lua_integration_test.cpp` 都用 `for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++)` 遍历，新增值排在 `BoneDemon` 之后**不会被检查**（`AiProc` 漏条目/错位都不报错）。把两处上界改为 `MonsterAIID::KiteCharger`，并把 `Source/monster.cpp` 中 `static_assert(static_cast<int>(MonsterAIID::BoneDemon) < AiProc.size(), ...)` 的上界同步改为 `MonsterAIID::KiteCharger`。

- [ ] **步骤 5：跑派发表与 Lua 自检**

```bash
python3 tools/run_tests.py --test ai_registry_test
python3 tools/run_tests.py --test lua_integration_test
```

预期：两者通过（`lua_integration_test` 里 `AiProc[FireMan] == nullptr` 的断言不受影响）。

- [ ] **步骤 6：提交**

```bash
git add Source/tables/monstdat.h Source/tables/monstdat.cpp Source/monster.cpp \
        test/ai_registry_test.cpp test/lua_integration_test.cpp
git commit -F - <<'MSG'
feat(monster): register SkeletonCharge/SkeletonBerserk/KiteCharger AI ids

Adds three MonsterAIID values after BoneDemon, their AiProc slots and forward
declarations, and - crucially - their behaviour classes. GetBehaviorClass ends
in `default: return BehaviorClass::Boss`, so an unregistered AI silently becomes
a Boss: the B1 sampling cap counts RangedKite members per level band, and A3
re-assigns two cave kite monsters to KiteCharger, so leaving them unregistered
would drop them out of the kite cap that A3's premise depends on.

Also extends the enum-iteration bounds in ai_registry_test, lua_integration_test
and the AiProc static_assert from BoneDemon to KiteCharger - otherwise the new
values are never checked.
MSG
```

---

## Task 3: A1 —— `SkeletonChargeAi` / `SkeletonBerserkAi` 实现

**文件：**
- 修改：`Source/monster.cpp`

**接口：**
- 依赖输入：Task 2 的枚举值与 `AiProc` 条目
- 对外产出：`SkeletonChargeAi(Monster &)`、`SkeletonBerserkAi(Monster &)`；文件内常量 `A1ChargeMinDistance`、`A1ChargeCooldownTicks`、`A1ChargeMaxLevel`、`A1BerserkHpPercent`、`A1BerserkTicks`（供 Task 7 复用冷却常量）

- [ ] **步骤 1：新增常量块（匿名命名空间，不新增头文件符号）**

在 `Source/monster.cpp` 的 `SkeletonAi` 定义之后新增：

```cpp
namespace {
/** A1: charge only from at least this far away (RhinoAi uses 5). */
constexpr unsigned A1ChargeMinDistance = 5;
/** A1 spec §4.3: ~1.5s cooldown, prevents the low-HP chain kill-wall. */
constexpr int8_t A1ChargeCooldownTicks = 30;
/** A1 spec §5 red line 12 / AC8: charge only at currlevel <= 4, so Horror
 *  Captain (data band 4-6) cannot leak the charge into the catacombs. */
constexpr uint8_t A1ChargeMaxLevel = 4;
/** A1 spec §4.1: berserk latches below 50% HP. */
constexpr int A1BerserkHpPercent = 50;
/** A1: how long the pursuit goal stays latched (FallenAi uses
 *  30 * intelligence + 105; we use the same order of magnitude). */
constexpr int16_t A1BerserkTicks = 105;
} // namespace
```

- [ ] **步骤 2：实现两个 AI 函数**

```cpp
void SkeletonChargeAi(Monster &monster)
{
	// Layer guard (spec line 233 + AC8): charge only at currlevel <= 4, so
	// Horror Captain (band 4-6) cannot leak the charge into 5-6.
	if (currlevel > A1ChargeMaxLevel) {
		SkeletonAi(monster);
		return;
	}
	if (monster.mode != MonsterMode::Stand || monster.activeForTicks == 0) {
		SkeletonAi(monster);
		return;
	}

	if (monster.goalVar3 > 0)
		monster.goalVar3--; // charge cooldown, persisted in goalVar3

	const Direction md = GetDirection(monster.position.tile, monster.position.last);
	const unsigned distanceToEnemy = monster.distanceToEnemy();
	const int chargeRoll = GenerateRnd(100); // same draw order as RhinoAi
	if (monster.goalVar3 == 0 && distanceToEnemy >= A1ChargeMinDistance
	    && chargeRoll < 2 * monster.intelligence + 43
	    && LineClear([&monster](Point position) { return IsTileAvailable(monster, position); }, monster.position.tile, monster.enemyPosition)) {
		if (AddMissile(monster.position.tile, monster.enemyPosition, md, MissileID::Rhino, TARGET_PLAYERS, monster, 0, 0) != nullptr) {
			if (monster.data().hasSpecialSound)
				PlayEffect(monster, MonsterSound::Special);
			monster.occupyTile(monster.position.tile, true);
			monster.mode = MonsterMode::Charge;
			monster.goalVar3 = A1ChargeCooldownTicks;
			return;
		}
	}
	SkeletonAi(monster);
}

void SkeletonBerserkAi(Monster &monster)
{
	// Mirrors FallenAi's order (Source/monster.cpp:2314-2322): latch first,
	// then run the countdown, then the Stand early-return.
	if (monster.goal == MonsterGoal::Normal
	    && monster.hitPoints * 100 < monster.maxHitPoints * A1BerserkHpPercent) {
		monster.goal = MonsterGoal::Attack;
		monster.goalVar1 = A1BerserkTicks;
	}
	if (monster.goal == MonsterGoal::Attack) {
		if (monster.goalVar1 != 0)
			monster.goalVar1--;
		else
			monster.goal = MonsterGoal::Normal;
	}
	if (monster.mode != MonsterMode::Stand || monster.activeForTicks == 0) {
		return;
	}

	if (monster.goal == MonsterGoal::Attack) {
		// FallenAi's pursuit branch (Source/monster.cpp:2364-2368), verbatim.
		// SkeletonAi never reads goal, so latching the goal without this
		// branch would be a no-op.
		if (monster.distanceToEnemy() < 2)
			StartAttack(monster);
		else
			RandomWalk(monster, GetMonsterDirection(monster));
	} else {
		SkeletonAi(monster);
	}
	monster.checkStandAnimationIsLoaded(monster.direction);
}
```

**与 RhinoAi 的关系**（实施时按此判断，不要自行发明）：

- **LOS 门控、概率门、`AddMissile` 实参表逐字取自 `RhinoAi`**（`Source/monster.cpp`）：`LineClear([&monster](Point position) { return IsTileAvailable(monster, position); }, …)`、最小距离 5、`chargeRoll < 2 * monster.intelligence + 43`、`AddMissile(…, MissileID::Rhino, TARGET_PLAYERS, monster, 0, 0) != nullptr`；成功后 `PlayEffect(…, MonsterSound::Special)`（当 `data().hasSpecialSound`）、`occupyTile(tile, true)`、`mode = Charge`。**若与现行代码不符以现行代码为准**并在提交信息写明文件:行。
- **保留概率门**（**不做**「确定性命中」的简化）：RhinoAi 的门是三重（距离 + 概率 + LOS），冷却在规格 §4.3/§5 红线 14 里的职责是**走廊公平性余量**，不是用来抵掉概率门带来的额外频率。若去掉概率门，教堂 1-4 的 Horror 会变成「距离够 + LOS 通就每 30 tick 必冲」，频率高于 Rhino 系，与规格的公平性论证冲突。
  因此 **AC1 的「同 tick 冲锋」改用固定种子断言**：实现时用一条探针找出「首 tick 的 `GenerateRnd(100)` < `2*intelligence+43`」的种子并写死到测试里（见 Task 5 AC1 行），而不是删掉门控。

- [ ] **步骤 3：编译并跑 harness 自检**

```bash
cmake --build build --target monster_behavior_test -j 20
python3 tools/run_tests.py --test monster_behavior_test
```

预期：编译通过、harness 自检仍通过。

- [ ] **步骤 4：提交**

```bash
git add Source/monster.cpp
git commit -F - <<'MSG'
feat(monster): implement A1 skeleton charge and berserk AI

SkeletonChargeAi: same-tick charge (AddMissile(Rhino) + mode=Charge) copied
from RhinoAi/SnakeAi, gated by distance, line of sight and a cooldown, and
restricted to currlevel <= 4 (A1 spec §5 red line 12) so Horror Captain
(band 4-6) cannot leak the charge into 5-6.

SkeletonBerserkAi: latches goal=Attack below 50% HP and copies FallenAi's
pursuit branch verbatim - SkeletonAi never reads goal, so setting the goal
alone would be a no-op (this exact mistake was caught in spec review v4).

The charge cooldown reuses goalVar3 rather than adding a Monster field:
SaveMonster/LoadMonster serialise goalVar1/2/3 and var1/2/3, so a new field
either changes the save format (red line) or is lost on load. goalVar3 is
unused by SkeletonAi/AiRangedAvoidance (monster.h: ScavengerAi/MegaAi/
GolemAi only) and A3's spec already expects the goalVar pattern.

Spec: docs/superpowers/specs/2026-08-10-cathedral-skeleton-differentiation-design.md
MSG
```

---

## Task 4: A1 —— 数据改派（两个 TSV）

**文件：**
- 修改：`assets/txtdata/monsters/monstdat.tsv`、`mods/hf/txtdata/monsters/monstdat.tsv`

**接口：**
- 依赖输入：Task 2 的枚举值名（TSV 的 `ai` 列按名字映射）
- 对外产出：`MT_RSKELAX`→`SkeletonBerserk`；`MT_XSKELAX`/`MT_XSKELSD`→`SkeletonCharge` 并设 special 伤害列

- [ ] **步骤 1：改两个 TSV 的 3 行 `ai` 列与 2 行 special 伤害列**

按 A1 规格 §4.1/§4.2 的分配表（**逐行核对，不要多改**）：

| 行 | `ai` 改为 | `minDamageSpecial` | `maxDamageSpecial` |
|---|---|---|---|
| `MT_RSKELAX`（Burning Dead，基础棕 tint） | `SkeletonBerserk` | 不变（0） | 不变（0） |
| `MT_XSKELAX`（Horror，black tint） | `SkeletonCharge` | `4` | `9` |
| `MT_XSKELSD`（Horror Captain，black tint） | `SkeletonCharge` | `5` | `14` |

其余 5 行（`MT_WSKELAX`/`MT_TSKELAX`/`MT_WSKELSD`/`MT_TSKELSD`/`MT_RSKELSD`）**保持不变**。
**两个文件都要改**（`assets/…` 与 `mods/hf/…`；实测这些行在两处逐字相同）。

- [ ] **步骤 2：确认改动只发生在这些列**

```bash
git diff --stat -- assets/txtdata/monsters/monstdat.tsv mods/hf/txtdata/monsters/monstdat.tsv
git diff -- assets/txtdata/monsters/monstdat.tsv | grep -E "^[+-]" | grep -vE "^(\+\+\+|---)"
```

预期：每个文件 3 行被改（每行同时含 `ai` 与 special 列的改动），**不新增/删除任何行**。

- [ ] **步骤 3：行尾自查（TSV 是 CRLF）**

```bash
python3 -c "
for p in ['assets/txtdata/monsters/monstdat.tsv','mods/hf/txtdata/monsters/monstdat.tsv']:
    b=open(p,'rb').read(); print(p,'CRLF',b.count(b'\r\n'),'LF',b.count(b'\n'),'bare',b.count(b'\n')-b.count(b'\r\n'))"
```

预期：`bare` 为 0，且 CRLF == LF。

- [ ] **步骤 4：数据断言（用 harness 验证池映射生效）**

在 `test/monster_behavior_test.cpp` 追加一个 `SkeletonDifferentiationData*` 用例：

```cpp
TEST_F(MonsterBehaviorHarnessTest, SkeletonDifferentiationDataAssignment)
{
	// MonstersData is a std::vector<MonsterData> indexed by _monster_id
	// (Source/tables/monstdat.h; engine precedent Source/monster.cpp:3304).
	const auto dataOf = [](_monster_id id) -> const MonsterData & {
		return MonstersData[static_cast<size_t>(id)];
	};
	// The level pool cannot cover rows that are filtered out by minDunLvl, so
	// assert the TSV mapping directly on the loaded monster data.
	EXPECT_EQ(static_cast<int>(dataOf(MT_XSKELAX).ai), static_cast<int>(MonsterAIID::SkeletonCharge));
	EXPECT_EQ(static_cast<int>(dataOf(MT_XSKELSD).ai), static_cast<int>(MonsterAIID::SkeletonCharge));
	EXPECT_EQ(static_cast<int>(dataOf(MT_RSKELAX).ai), static_cast<int>(MonsterAIID::SkeletonBerserk));
	EXPECT_EQ(static_cast<int>(dataOf(MT_XSKELAX).minDamageSpecial), 4);
	EXPECT_EQ(static_cast<int>(dataOf(MT_XSKELAX).maxDamageSpecial), 9);
	EXPECT_EQ(static_cast<int>(dataOf(MT_XSKELSD).minDamageSpecial), 5);
	EXPECT_EQ(static_cast<int>(dataOf(MT_XSKELSD).maxDamageSpecial), 14);
	// The other five rows stay SkeletonMelee
	for (const _monster_id id : { MT_WSKELAX, MT_TSKELAX, MT_WSKELSD, MT_TSKELSD, MT_RSKELSD })
		EXPECT_EQ(static_cast<int>(dataOf(id).ai), static_cast<int>(MonsterAIID::SkeletonMelee));
}
```

**实现要求**：`MonstersData` 是 `std::vector<MonsterData>`、以 `_monster_id` 作下标（`Source/tables/monstdat.h:353` 的 `extern std::vector<MonsterData> MonstersData;`；引擎自身先例 `Source/monster.cpp:3304` `const MonsterData &monsterData = MonstersData[type];`）。**没有 `MonstersDataById` 这个 API**（全仓 grep 零命中），不要发明它。

- [ ] **步骤 5：运行并提交**

```bash
python3 tools/run_tests.py --test monster_behavior_test
git add test/monster_behavior_test.cpp assets/txtdata/monsters/monstdat.tsv mods/hf/txtdata/monsters/monstdat.tsv
git commit -m "feat(monster): re-assign A1 skeleton rows (charge/berserk/normal)"
```

---

## Task 5: A1 —— 行为断言（AC1-AC9）

**文件：**
- 修改：`test/monster_behavior_test.cpp`

**接口：**
- 依赖输入：Task 1 harness、Task 3 AI、Task 4 数据
- 对外产出：`SkeletonDifferentiation*` 测试套件（供 eval case 用 filter 定位）

- [ ] **步骤 1：按 A1 规格 §6 的 AC 逐条写用例**

必须覆盖（每条对应规格 AC 编号，写在用例名或注释里）：

| AC | 用例要点 | 断言 |
|---|---|---|
| 1 | 距离 ≥ 阈值 + 视线通 + 概率门通过 → 同 tick 冲锋 | `monster.mode == MonsterMode::Charge`（**无前摇**：同一 tick 内）。**概率门保留**（与 `RhinoAi` 一致）：实现时先用一条探针找出「首 tick 的 `GenerateRnd(100)` < `2*intelligence+43`」的种子并写死进用例（`SpawnAt(..., seed)`），从而使「同 tick」可断言——不要为了测试方便删掉门控 |
| 2 | 冲锋弹道确实生成 | 行为层：`TickWorld()` 后 `Missiles` 非空且含 Rhino 类型 |
| 3 | 冲锋冷却 | 冲锋后 `goalVar3 == A1ChargeCooldownTicks`；冷却期内再 tick 不再冲锋 |
| 4 | 近距退回普通 | 距离 < 阈值 → 走 `SkeletonAi`（mode 不进入 Charge） |
| 5 | 狂乱触发 | `hitPoints < maxHitPoints/2` → `goal == MonsterGoal::Attack` |
| 6 | 狂乱追击 | 狂暴期远距 → `RandomWalk` 产生的位移/朝敌行为可观察（用快照比较：狂暴前后 30 tick 的位置变化不同） |
| 7 | 数据分配 | 见 Task 4 步骤 4 的用例（本任务保留） |
| 8 | 冲锋层守卫 | `currlevel = 5` 时 Horror 的 ai 走 `SkeletonAi`（`mode` 不进入 Charge）；`currlevel = 4` 时可冲锋——规格 AC8「层 > 4 不触发」，守卫常量 `A1ChargeMaxLevel = 4` 见 Task 3 步骤 2 |
| 9 | MP 确定性 | `gbIsMultiplayer = true` 下按 `ProcessMonsters` 的 `SetRndSeed(monster.aiSeed)` 模型跑双份模拟，同一 `aiSeed` 下决策一致（比较快照） |

- [ ] **步骤 2：逐条运行并记录 RED/GREEN**

先只加用例、断言故意留空跑一次（RED），再补实现/修正断言（GREEN）。用：

```bash
python3 tools/run_tests.py --test monster_behavior_test --filter "SkeletonDifferentiation*"
```

- [ ] **步骤 3：提交**

```bash
git add test/monster_behavior_test.cpp
git commit -m "test(monster): assert A1 charge/berserk behaviour (AC1-AC9)"
```

---

## Task 6: A1 —— eval case

**文件：**
- 新建：`eval/cases/combat/monster-skeleton-differentiation.yaml`
- 修改：`eval/cases/_nightly.yaml`（重新生成）

**接口：**
- 依赖输入：Task 5 的测试套件名与用例数
- 对外产出：可被 `--smoke`/`--set nightly` 引用的 eval case

- [ ] **步骤 1：写 case（沿用现有 case 的字段结构）**

```yaml
id: "monster-skeleton-differentiation"
name: "A1 教堂骷髅区分：冲锋/狂乱/普通三变体"
category: "combat"
difficulty: "advanced"
tags: ["monster", "ai", "density"]

setup:
  mpq_required: true
  timeout: 120

run:
  backend: "gtest"
  binary: "monster_behavior_test"
  filter: "SkeletonDifferentiation*"

expected:
  exit_code: 0
  failed: 0
  passed_min: 9
  skipped_max: 1
  output_contains: ["[  PASSED  ]"]

description: >
  教堂 8 行骷髅拆 3 变体（冲锋/狂乱/普通）的行为与数据断言：
  即时冲锋（同 tick AddMissile(Rhino)）、冲锋弹道、冲锋冷却、近距退回、
  狂乱触发与追击、special 伤害列改派、冲锋层守卫、MP 确定性。
  依据 A1 规格 §6 的 AC1-AC9。需 MPQ 资源。
```

**实现要求**：`passed_min` 必须等于 Task 5 实际通过的**用例数**（跑一次 `--test monster_behavior_test --filter "SkeletonDifferentiation*"` 数出来再填，**不要照抄 9**）。注意规格 §7 写的「15/15」指的是 **gtest 断言计数**、不是用例数，两者不要混。

- [ ] **步骤 2：重新生成 nightly 集合**

```bash
python3 -m tools.eval.sync_case_sets --write    # 重写 _nightly.yaml（该文件头注明不要手改）
python3 -m tools.eval.sync_case_sets --check    # 一致性校验
git diff --stat eval/cases/_nightly.yaml
```

预期：`_nightly.yaml` 新增一行本 case 的 id；`--check` 输出一致（无缺引用/悬空引用）。

**关于 `_smoke.yaml`**：它是**人工维护**的精选集（`tools/eval/sync_case_sets.py` 注释 `smoke stays curated`，`--write` 不会动它）。本 case 需要 MPQ 且耗时较长，**不加入 `_smoke.yaml`**——因此 Task 6 步骤 3 的 `--smoke` 只是**回归门禁**，并不覆盖本 case；本 case 由步骤 3 的 `--run` 单独验证。

- [ ] **步骤 3：跑该 case 与 smoke 门禁**

```bash
python3 -m tools.eval.backend --run monster-skeleton-differentiation
python3 -m tools.eval.backend --smoke; echo "exit=$?"
```

预期：case PASS；smoke exit 0。

- [ ] **步骤 4：提交**

```bash
git add eval/cases/combat/monster-skeleton-differentiation.yaml eval/cases/_nightly.yaml
git commit -m "test(eval): add monster-skeleton-differentiation case (A1)"
```

---

## Task 7: A3 —— `KiteChargerAi` 实现

**文件：**
- 修改：`Source/monster.cpp`、`Source/monster.h`

**接口：**
- 依赖输入：Task 2 的 `MonsterAIID::KiteCharger` 与 `AiProc` 条目；Task 3 的 `A1ChargeCooldownTicks`（**同一 TU 内定义于 A1 常量块之后，不要重复定义**）
- 对外产出：`KiteChargerAi(Monster &)`；`int CountOpenDirections(Point)`（在 `monster.h` 导出）；常量 `A3ChargeMinDistance`、`A3LastStandHpPercent`、`A3CorneredDirections`

- [ ] **步骤 1：新增常量与函数**

```cpp
namespace {
/** A3: charge only when at least this far away (mirrors RhinoAi's gate). */
constexpr int A3ChargeMinDistance = 3;
/** A3: below this percentage of max HP the kite monster stops fleeing. */
constexpr int A3LastStandHpPercent = 30;
/** A3: <= this many walkable neighbours counts as cornered/corridor. */
constexpr int A3CorneredDirections = 3;
} // namespace

/** Counts how many of the four orthogonal neighbours of `center` are walkable.
 *  The "cornered" test: <= 3 open directions means a corridor or a dead end
 *  (A3 spec §4.3 - the threshold was raised from <2 to <=3 so corridors count).
 *  Declared in Source/monster.h so the AC5 unit test can call it directly;
 *  do NOT make it static and do NOT put it in an anonymous namespace. */
int CountOpenDirections(Point center)
{
	static constexpr Displacement NeighbourOffsets[] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
	int open = 0;
	for (const Displacement &offset : NeighbourOffsets) {
		const Point neighbour = center + offset;
		if (InDungeonBounds(neighbour) && IsTileWalkable(neighbour))
			open++;
	}
	return open;
}

void KiteChargerAi(Monster &monster)
{
	if (monster.mode != MonsterMode::Stand || monster.activeForTicks == 0) {
		AiRangedAvoidance(monster);
		return;
	}

	if (monster.goalVar3 > 0)
		monster.goalVar3--; // shared charge cooldown (defined with A1, Task 3)

	const Direction md = GetDirection(monster.position.tile, monster.position.last);
	const unsigned distanceToEnemy = monster.distanceToEnemy();
	if (monster.goalVar3 == 0 && distanceToEnemy >= A3ChargeMinDistance
	    && LineClear([&monster](Point position) { return IsTileAvailable(monster, position); }, monster.position.tile, monster.enemyPosition)) {
		const bool lastStand = monster.hitPoints * 100 < monster.maxHitPoints * A3LastStandHpPercent;
		const bool cornered = CountOpenDirections(monster.enemyPosition) <= A3CorneredDirections;
		if (cornered || lastStand) {
			if (AddMissile(monster.position.tile, monster.enemyPosition, md, MissileID::Rhino, TARGET_PLAYERS, monster, 0, 0) != nullptr) {
				if (monster.data().hasSpecialSound)
					PlayEffect(monster, MonsterSound::Special);
				monster.occupyTile(monster.position.tile, true);
				monster.mode = MonsterMode::Charge;
				monster.goalVar3 = A1ChargeCooldownTicks; // reuse the A1 cooldown
				return;
			}
		}
	}
	AiRangedAvoidance(monster);
}
```

**为什么 A3 不需要 A1 的那个概率门**：A1 的触发条件只有「距离 + 视线」，概率门用来压频率；A3 的触发条件是**墙角或垂死**——这本身就限制住了频率（开阔地且血量充足时永不冲锋），且 A3 规格 §4.3 与 AC1/AC2 都要求确定性触发。**不要把 `RhinoAi` 的概率门复制到 A3**；若实测冲锋过密，按规格 §4.3「实施时验证走廊场景并记录」先记录，再决定是否加门。
- [ ] **步骤 2：在 `Source/monster.h` 导出助手**

在 `Source/monster.h` 的怪物 AI 声明区（与 `void ProcessMonsters();` 同一区块）新增：

```cpp
/** A3: number of walkable orthogonal neighbours of `center` (cornered test). */
[[nodiscard]] int CountOpenDirections(Point center);
```

`CountOpenDirections` 已在步骤 1 定义在 `KiteChargerAi` **之前**（同文件内顺序即满足编译），并在此导出以供 Task 8 的 AC5 独立单测调用。**不要写成 `static`、不要放进匿名命名空间**。

- [ ] **步骤 3：编译并跑 harness 自检**

```bash
cmake --build build --target monster_behavior_test -j 20
python3 tools/run_tests.py --test monster_behavior_test
```

- [ ] **步骤 4：提交**

```bash
git add Source/monster.cpp Source/monster.h
git commit -F - <<'MSG'
feat(monster): implement A3 KiteChargerAi (cornered / last-stand charge)

Derives from AiRangedAvoidance: the kite monster keeps kiting unless the
player is cornered (<= 3 open directions) or the monster drops below 30% HP,
in which case it charges with the same Rhino missile and cooldown mechanism
as A1. Distance and line-of-sight gating plus the cooldown are the specified
counters (DP2) - they stop wall-piercing charges and低血连锁 kill-walls.

Charge delivery reuses the carrier's special damage columns: Hell Stone gets
2-20 in the data task, Storm Lord already has 4-16 (MissToMonst ->
MonsterAttackPlayer reads the special columns).
MSG
```

---

## Task 8: A3 —— 数据改派与行为断言（AC1-AC7）

**文件：**
- 修改：`assets/txtdata/monsters/monstdat.tsv`、`mods/hf/txtdata/monsters/monstdat.tsv`、`test/monster_behavior_test.cpp`

**接口：**
- 依赖输入：Task 7 的 AI、Task 1 的 harness
- 对外产出：`KiteCombination*` 测试套件

- [ ] **步骤 1：改两个 TSV 的 2 行**

| 行 | `ai` 改为 | `minDamageSpecial` | `maxDamageSpecial` |
|---|---|---|---|
| `MT_BMAGMA`（Hell Stone，Magma 系） | `KiteCharger` | `2` | `20` |
| `MT_STORML`（Storm Lord，Storm 系） | `KiteCharger` | 不变（4） | 不变（16） |

`MT_XACID`（Lava Maw）按规格 §4.2 是**备选**，本计划**不改**。

**口径例外（D3，必须显式声明，否则会被误判为漏改）**：TSV 实测 `MT_STORML` 普通伤害 **12-24**、special **4-16**；而 A1/A3 的冲锋口径是「special 伤害列 = 普通伤害值」（Hell Stone 2-20 即照此设）。规格 A3 §4.2 明文写 Storm Lord「special 4-16 已有真实伤害，无需改列」，故本计划**保持 4-16 不变**并把它记为**有意的口径例外**：Storm Lord 的冲锋伤害（4-16）低于其普通远程攻击（12-24）。若规格作者本意是 12-24，**先改 A3 规格 §4.2 再改数据**。

- [ ] **步骤 2：按 A3 规格 §6 写用例**

| AC | 用例要点 | 断言 |
|---|---|---|
| 1 | 墙角冲锋 | 玩家被墙包围（可走方向 ≤3）→ 冲锋（mode=Charge） |
| 1b | 冲锋伤害投递 | 承载的 special 列被 `MonsterAttackPlayer` 使用：断言 Hell Stone `2/20`、Storm Lord `4/16`（数据层；**Storm Lord 的 4-16 是有意例外，见步骤 1 的 D3**）+ 冲锋命中路径产出的伤害落在该区间（行为层） |
| 2 | 垂死反击 | HP < 30% → 冲锋而非继续风筝 |
| 3 | 正常仍风筝 | 开阔地 + HP 充足 → 走 `AiRangedAvoidance`（mode 不进入 Charge） |
| 4 | 冷却 | 冲锋后 `goalVar3` 置位；冷却期内不冲锋 |
| 4b | 距离/视线门控 | 距离 < 3 或视线阻断 → 不冲锋 |
| 5 | 墙角判定 | `CountOpenDirections`：开阔地 ≥4、走廊/死角 ≤3（**独立单测该助手**） |
| 6 | 承载分配 | `MT_BMAGMA`/`MT_STORML` 的 ai 为 `KiteCharger`，其余风筝行（`MT_XACID` 等）不变 |
| 7 | MP 确定性 | 同 `aiSeed` 双份模拟决策一致 |

- [ ] **步骤 3：运行、记录 RED/GREEN、提交**

```bash
python3 tools/run_tests.py --test monster_behavior_test --filter "KiteCombination*"
git add assets/txtdata/monsters/monstdat.tsv mods/hf/txtdata/monsters/monstdat.tsv test/monster_behavior_test.cpp
git commit -m "test(monster): re-assign A3 kite rows and assert combination behaviour (AC1-AC7)"
```

---

## Task 9: A3 —— eval case

**文件：**
- 新建：`eval/cases/combat/monster-kite-combination.yaml`
- 修改：`eval/cases/_nightly.yaml`（重新生成）

**接口：**
- 依赖输入：Task 8 的测试套件与用例数

- [ ] **步骤 1：写 case（结构同 Task 6，换 id/name/filter/passed_min）**

```yaml
id: "monster-kite-combination"
name: "A3 洞穴风筝组合：墙角冲锋 / 垂死反击"
category: "combat"
difficulty: "advanced"
tags: ["monster", "ai", "density"]

setup:
  mpq_required: true
  timeout: 120

run:
  backend: "gtest"
  binary: "monster_behavior_test"
  filter: "KiteCombination*"

expected:
  exit_code: 0
  failed: 0
  passed_min: 8
  skipped_max: 1
  output_contains: ["[  PASSED  ]"]

description: >
  洞穴 2 只风筝怪（Hell Stone / Storm Lord）转条件性冲锋：
  墙角与垂死触发、正常仍风筝、冲锋冷却、距离/视线门控、墙角判定、
  special 伤害列承载、MP 确定性。依据 A3 规格 §6 的 AC1-AC7。
```

`passed_min` 同样按实测填（不要照抄 8）。

- [ ] **步骤 2：生成 nightly + 跑 case 与 smoke**

```bash
python3 -m tools.eval.sync_case_sets --write
python3 -m tools.eval.sync_case_sets --check    # 一致性校验
python3 -m tools.eval.backend --run monster-kite-combination
python3 -m tools.eval.backend --smoke; echo "exit=$?"
```

- [ ] **步骤 3：提交**

```bash
git add eval/cases/combat/monster-kite-combination.yaml eval/cases/_nightly.yaml
git commit -m "test(eval): add monster-kite-combination case (A3)"
```

---

## Task 10: 门禁与规格状态

**文件：**
- 修改：`docs/superpowers/specs/2026-08-10-cathedral-skeleton-differentiation-design.md`、`docs/superpowers/specs/2026-08-10-cave-kite-combination-design.md`、`docs/superpowers/specs/2026-08-10-density-fix-framework-overview.md`（§7 进度表）

- [ ] **步骤 1：B1 采样用例必须仍全绿（行为分类改动的直接验收）**

Task 2 把 `KiteCharger` 登记为 `RangedKite`、两个 Skeleton 变体登记为 `Melee`，这会改变 B1 cap 的成员计数，进而可能影响 `sampling_behavior_test` 里钉死的分布值（`CavesKiteTailBaseline`、`CavesAnyClassTailBaseline` 等）：

```bash
python3 tools/run_tests.py --test sampling_behavior_test
```

预期：全部 `SamplingBaselineTest.*` 通过。**若数值漂移超出容差**：在同一提交内按**实测**更新该测试的钉死值，并在提交信息里写明测量方法与非确定性（跑多次取范围）——**不要**把断言改成恒真或删掉断言。

- [ ] **步骤 1b：全量门禁**

```bash
python3 tools/run_tests.py --json /tmp/ci-a1a3.json
python3 -m tools.eval.backend --smoke; echo "exit=$?"
```

预期：`ctest.failed == 0`、`passed_pct == 100`、`drift.drift_ok == true`（5 项 PASS）、smoke exit 0。测试总数会比基线增加（新增约 17 个用例）——**记录数量变化**。

- [ ] **步骤 2：timedemo 与存档相关回归**

```bash
cd build && ./timedemo_test --gtest_filter='Timedemo.*'   # 预期仍 Skipped（quarantine 未变）
```

说明：A1/A3 改的是 AI 列与 special 伤害列，不改掉落/商店谓词，**不应**触发已知的种子漂移（`gotcha_vendor_predicate_seed_drift.md`）；若 `pack_test`/`writehero_test` 出现失败，先按该 gotcha 判断是否为本改动引起（本改动不触碰物品生成路径，理论上无关）。

- [ ] **步骤 3：把三份规格的状态改为「已实施」**

各规格第 8 节 / 框架 §7 进度表：把 A1、A3 行从「草案/v4 已修订」改为「已实施（YYYY-MM-DD）」并写明验收结果（AC 通过情况、eval case id、门禁数字）。

- [ ] **步骤 4：提交**

```bash
git add docs/superpowers/specs/2026-08-10-cathedral-skeleton-differentiation-design.md \
        docs/superpowers/specs/2026-08-10-cave-kite-combination-design.md \
        docs/superpowers/specs/2026-08-10-density-fix-framework-overview.md
git commit -m "docs(spec): mark A1/A3 implemented with acceptance results"
```

---

## 自审

**1. 规格覆盖度**

| 规格条目 | 对应任务 |
|---|---|
| A1 §4.1 行为变体（冲锋/狂乱/普通 + tint 分配） | Task 3（AI）、Task 4（数据） |
| A1 §4.2 数值口径（special 伤害列 = 普通伤害值） | Task 4 步骤 1（4-9 / 5-14） |
| A1 §4.3 实现要点（枚举/AiProc/两函数/层守卫/冷却） | Task 2（枚举 + AiProc + **行为分类登记** + **遍历上界**）、Task 3（常量 + 两函数实现） |
| A1 §6 AC1-AC12 | Task 5（AC1-9）、Task 6（AC12 eval）、Task 10（AC10 全量、AC11 漂移） |
| A1 §7 harness 量化 | Task 6 的 case |
| A3 §4.1 组合行为（墙角/垂死 + 反制 5 条） | Task 7（AI）、Task 8（测试） |
| A3 §4.2 承载选择（Hell Stone 需设列、Storm Lord 已有） | Task 8 步骤 1 |
| A3 §4.3 实现要点（门控/冷却/墙角判定） | Task 7（含 `CountOpenDirections` 在 `monster.h` 导出以便 AC5 单测） |
| 新增 `MonsterAIID` 值的连带：`GetBehaviorClass` 分类、枚举遍历上界、`AiProc` 位置对应 | Task 2 步骤 2-4 |
| harness 的网格与音效前提（`SOLData`/`dPiece` 决定可走性；`PlayEffect` 先抽 RNG 再判 sound 开关） | Task 1 步骤 1 + 事实 §A |
| A3 §6 AC1-AC10 | Task 8（AC1-7）、Task 9（AC10 eval）、Task 10（AC8/9） |
| 框架 §7「harness 先行（纯测试不改游戏代码）」 | Task 1 |
| C2 §6「harness 前提：能跑 AiProc tick」 | Task 1（两层 tick；`ProcessPlayers` 到期路径与 `MonsterDeath` 路径**本计划不实现**——它们是 C2 规格的 AC 需求，留待 C2 实施时在其自身任务里扩展 harness，本计划的 harness 已预留 `TickWorld()`） |
| A2（激励者）/ C2（熄灯者）实施 | **不在本计划范围**（框架依赖序：A1 → A3 → B1 → C2 → A2；B1 已完成） |

**2. 占位符扫描**

本计划不含 TBD/TODO/未定内容。Task 3 步骤 2 与 Task 7 步骤 1 的代码**已按引擎现行实现逐字写出**（LOS 门控 lambda、`AddMissile` 实参表与判空、`occupyTile`、`PlayEffect`、`FallenAi` 的倒计时次序与 pursuit 分支），并附「若与现行代码不一致以现行代码为准 + 写明文件:行」的判据。其余每一步都给出确切命令、确切文件/列名与确切预期输出。

**3. 类型与命名一致性**

- `MonsterAIID::SkeletonCharge` / `SkeletonBerserk` / `KiteCharger` 在 Task 2 定义，Task 3/7 实现，Task 4/8 以**同名**写入 TSV 的 `ai` 列。
- `SkeletonChargeAi` / `SkeletonBerserkAi` / `KiteChargerAi` 在 Task 2 前置声明，Task 3/7 定义，`AiProc` 表中引用一致。
- 冷却统一走 `monster.goalVar3`（Task 3 定义常量 `A1ChargeCooldownTicks`，Task 7 复用同一常量——不重复定义）。
- harness 产出的 `MonsterAiHarness`/`SpawnAt`/`SpawnAi`/`TickDecision`/`TickWorld`/`AllTilesVisible`/`UseLevel`/`ResetMonstersOnly`/`FindLevelType` 在 Task 1 定义，后续任务按同名调用。
- eval case 的 `binary` 均为 `monster_behavior_test`，`filter` 与 Task 5/8 的测试套件名前缀一致（`SkeletonDifferentiation*` / `KiteCombination*`）。

**4. 与规格的两处偏离已在事实 §D 显式裁决**（D1 复用 `goalVar3` 替代新增字段；D2 引擎调用点以现行代码为准而非规格伪代码）。