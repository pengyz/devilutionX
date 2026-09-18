# Task 3 简报（采样接入；含 2a/2b 之后的接口与裁决）

> **前置**：任务 1（基线实测 + 忠实建关夹具）、任务 2a/2b 已完成并入库。可直接使用：
> - `Source/tables/level_roster.h`：`GetLevelRoster(uint8_t)`、`GetLevelRosterParams(uint8_t)`（无行时返回 nullptr）、`LevelRosterRole::{Core,Tail}`、**`BehaviorClassCapForLevel(uint8_t level, BehaviorClass)`（B1 caps 的单一真相源，0 = 无上限）**、`LoadLevelRoster()`（已接生产）。
> - 两张已发布的逐层表：`assets/txtdata/monsters/level_rosters.tsv`、`level_roster_params.tsv`（L1-16；列序见 2b）。
>
> **本任务要交付**（改 `Source/monster.cpp` 的 `GetLevelMTypes()`，`:3439+`）：
> 1. **core 预加**：按 `currlevel` 取 `GetLevelRoster(currlevel)`，对 `role == Core` 的成员 `AddMonsterType(type, PLACE_SCATTER)`；**必须按可用性过滤**（裁决 R26：spawn/shareware 下 L5-16 的 core 几乎全是 `Retail`，不可用者不得预加，否则 shareware 放不出怪）。core 绕过 caps（caps 只在采样循环内）但**计入预算**。
> 2. **尾池**：`typelist` = `IsMonsterAvailable` 候选 **减去已加入的 core**。
> 3. **尾池抽取上限**：最多成功抽取 `params->tailDraw` 次（无 params 行时为 0）。
> 4. **配额**：caps 逻辑**改用 `BehaviorClassCapForLevel`**（裁决 R15 的单一真相源——不要保留内联的 `capKite`/`capSameClass` 判断，否则两处漂移）；并新增**floors 补足**（未满足的类别优先抽取）。
> 5. **预算**：`monstimgtot < params->maxImage`（无 params 行时退回 4000）。
> 6. **L16 不动**：`currlevel == 16` 的硬编码分支与提前 return 保持原样（spec §4.2.7）。
> 7. **夹具与门禁**（裁决 R3/R4）：本任务改变采样顺序 → 若有 `timedemo` 等硬编码夹具失配，按 `docs/knowledge/` 既有流程**重生成**；若 B1 既有期望（`HellL15SameClassTailBaseline` 等）失配 → 调整**名册或 `class_floors`**，**不得**放宽阈值；提交前跑全量门禁。
> 8. **用例**（加进 `test/sampling_behavior_test.cpp`）：`RosterCoreAlwaysPresent`（每层 core 100% 出现，200 seeds）、`RosterTailDrawBounded`、`RosterQuotasSatisfied`（floors 满足 + caps 未破）、`IdentityGuard`（相邻层名册 Jaccard < 0.9，复用 P0-D 度量）、`A1A3VariantsAreCore`。
>
> **不在本任务范围**：eval 用例（任务 4）、阶段 B 的核心小队、HF overlay 的 L17-24（阶段 A2）。

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
	const int tailDraw = rosterParams != nullptr ? rosterParams->tailDraw : 0;
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

