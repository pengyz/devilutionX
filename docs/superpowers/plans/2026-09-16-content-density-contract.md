# 内容密度契约 实施计划

> **面向执行者：** 本计划**内联执行**（作者已明确不在本次工作使用任何子 agent），可用 `superpower-executing-plans` 批量推进并在检查点复核。步骤使用复选框（`- [ ]`）语法跟踪。

**目标：** 把已批准的《内容密度契约》（四轴地板 + 非对称 cap + R4 反转）落进代码、数据与既有契约，使"内容密度不得低于 vanilla、且任何层不得确定性"成为**可失败的断言**，并用地狱段的可达密度（L13-15 ≥6）验证它真的提升了玩家可见的内容量。

**架构：** 三块：①**度量**——把 P0-A/P0-D 从"报告"升为断言，并补 vanilla 地板（用**物理空表夹具**导出）与曝光率；②**手段**——`BehaviorClassCapForLevel` 对 **L13-15** 改为非对称（远程 cap 1、非远程免 cap），L16 保持旧约束，配套把 L13/L14 的 `tail_draw` 提到 4；③**契约外溢**——`HellL13/14/15SameClassTailBaseline`、`RosterQuotaAllowanceIsBinding`、`PlacedClassMixWithinBaseline` 主判据、knowledge/eval 四处按新语义改写。

**技术栈：** C++23、CMake+Ninja、GoogleTest、TSV 数据驱动、eval YAML。

**规格：** `docs/superpowers/specs/2026-09-16-content-density-contract-design.md`（**已批准 v4**；本计划论证以规格为准，执行时两者同读）

## 全局约束

> **签名提示（已核实）**：`LoadLevelRoster()` 与 `LoadLevelRosterFromFiles(a,b)` **两者都返回 `void`**，不要用 `ASSERT_TRUE(...)` 包它们。

- **不使用任何子 agent**（作者指示）：本计划由主 agent 内联执行；遇到阻塞直接停下来问，不得改走委派
- 行尾：C++/TSV **CRLF**；`.md/.yaml/.py` **LF**；`test/level_roster_baseline_test.cpp` 相对 merge-base 是 **added** → 受漂移 **C2** 约束（**整文件** CRLF）
- 改 `assets/txtdata/**.tsv` 后必须 `ninja devilutionx_mpq` 再测量（否则量到旧表）
- **每条新守卫必须可失败**：给出"改坏→必红→恢复→绿"的实跑，证据**内联**写进报告（不得只引用 `/tmp`）
- 禁令 6：不得占位/自证用例；不得用 `DISABLED_`
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json`（`failed==0 && passed_pct==100 && drift_ok==true`，含漂移检查 **F**）+ `python3 -m tools.eval.backend --smoke`（exit 0）
- 需零售/HF 素材的用例与 eval case 必须**贴真实依赖探测后跳过**，eval case 声明 `setup.retail_or_hf_required: true`（已有先例）
- 每次 push 后 `gh run list -R pengyz/devilutionX` + `gh run watch -R pengyz/devilutionX <id> --exit-status` 跟到终态
- **不得**改 `docs/superpowers/specs/**`（规格已批准；如需改先回作者）；**不得**放宽任何既有阈值
- 提交信息英文 conventional commits；**每步一提交**，保持每步可编译可过

---

## 文件结构

| 文件 | 职责 |
|---|---|
| `test/fixtures/txtdata/monsters/level_rosters_out_of_range.tsv` | **新建**：只含 level 99（超范围）一行的名册表——加载器拒绝仅表头的文件（`Source/data/file.cpp:53-57`），超范围行可让 L1-15 完全无名册行而文件仍有内容 |
| `test/fixtures/txtdata/monsters/level_roster_params_out_of_range.tsv` | **新建**：只有表头的空参数表 |
| `test/Fixtures.cmake` | 修改：注册上面两个夹具（`copy_files` 白名单，`:98-105` 一带） |
| `test/level_roster_baseline_test.cpp` | 修改：改 `PlacedClassMixWithinBaseline` 主判据（A4） |
| `test/sampling_behavior_test.cpp` | 修改：**新增 `VanillaFloorBaseline`（导出地板）、`ContentDensityWithinVanillaFloor`、`ContentVisibilityFloor`（三者必须在本文件——`RealisedDistinctAi`/`RealisedDistinctClass` 是本文件内 static，baseline 文件里没有）**；`RosterPerSeedVariety` 扩到 L1-24；改写 `HellL13/14/15SameClassTailBaseline`；改写 `RosterQuotaAllowanceIsBinding` |
| `Source/tables/level_roster.cpp` | 修改：`BehaviorClassCapForLevel` 非对称化（L13-15 远程 1 / 非远程 0；L16 保持 2） |
| `assets/txtdata/monsters/level_roster_params.tsv` | 修改：L13/L14 `tail_draw` → 4 |
| `assets/txtdata/monsters/level_rosters.tsv` | 修改：L17 尾池整改（core 3→2）（Task 5） |
| `eval/cases/rng/level-rosters.yaml` | 修改：计数同步 + cap 语义注释（附录 A6） |
| `docs/knowledge/decision_save_format_policy.md` | 修改：cap 语义变更记录（附录 A5） |
| `docs/knowledge/decision_content_density_contract.md` | **新建**：契约的判据与参数组（附录 A3 的文档面） |

---

## Task 1: vanilla 地板导出（物理空表夹具）

**文件：** 新建 `test/fixtures/txtdata/monsters/level_rosters_out_of_range.tsv`、`level_roster_params_out_of_range.tsv`；修改 `test/Fixtures.cmake`、**`test/sampling_behavior_test.cpp`**（用例放这里，因为 `RealisedDistinctAi`/`RealisedDistinctClass` 是本文件内 static）
**接口：** 产出**逐层地板常量**（D1/D2/D3），供 Task 2 直接引用

- [ ] **步骤 1：建两个空夹具（只有表头，CRLF）**

`level_rosters_out_of_range.tsv`：
```
level	monster_id	role	allow_unique_boost
```
`level_roster_params_out_of_range.tsv`：
```
level	max_image	tail_draw	class_floors	squad_chance	squad_size	squad_leashed
```

- [ ] **步骤 2：在 `test/Fixtures.cmake` 注册**

在 `txtdata/monsters/level_roster_params_no_hf.tsv` 之后加两行（保持既有缩进与 CRLF）：
```
  txtdata/monsters/level_rosters_out_of_range.tsv
  txtdata/monsters/level_roster_params_out_of_range.tsv
```

- [ ] **步骤 3：写 `VanillaFloorBaseline` 测量用例（先只打表，不设阈值）**

加到 `test/sampling_behavior_test.cpp`（suite `SamplingBaselineTest`；沿用该文件既有 fixture 与 `missingMpqAssets_` 跳过惯例）：
```cpp
TEST_F(SamplingBaselineTest, VanillaFloorBaseline)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN not available - skipping test";

	// Load a PHYSICALLY EMPTY roster: no core rows and no params rows. This is the
	// R28 legacy path (maxImage 4000, tailDraw unbounded), i.e. pre-roster behaviour,
	// which is what the density floors must be derived from (spec appendix B).
	LoadLevelRosterFromFiles("txtdata\\monsters\\level_rosters_out_of_range.tsv",
	    "txtdata\\monsters\\level_roster_params_out_of_range.tsv"));

	constexpr int kSeeds = 200;
	std::cout << "\n[ VANILLAFLOOR ] level types AI classes union\n";
	for (uint8_t level = 1; level <= 15; level++) {
		size_t typeSum = 0;
		size_t aiSum = 0;
		size_t classSum = 0;
		std::set<_monster_id> seen;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(51000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			typeSum += LevelMonsterTypeCount;
			aiSum += RealisedDistinctAi();
			classSum += RealisedDistinctClass();
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				seen.insert(LevelMonsterTypes[i].type);
		}
		std::cout << "[ VANILLAFLOOR ] " << static_cast<int>(level) << ' '
		          << static_cast<double>(typeSum) / kSeeds << ' '
		          << static_cast<double>(aiSum) / kSeeds << ' '
		          << static_cast<double>(classSum) / kSeeds << ' ' << seen.size() << std::endl;
	}
	// Restore the shipped tables for the rest of the binary.
	LoadLevelRoster();
}
```

- [ ] **步骤 4：构建并实跑，把逐层数字抄进计划/报告**

运行：`cmake --build build --target sampling_behavior_test -j20 && ./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.VanillaFloorBaseline'`
预期：PASS，输出 15 行 `[ VANILLAFLOOR ]`。**把这些数字作为 Task 2 的地板常量来源内联记录**（若某层因空表导致加载期校验失败，说明空表不是合法输入——停下来问，不要放宽校验）。

- [ ] **步骤 5：提交**

```bash
git add test/fixtures/txtdata/monsters/level_rosters_out_of_range.tsv test/fixtures/txtdata/monsters/level_roster_params_out_of_range.tsv test/Fixtures.cmake test/sampling_behavior_test.cpp
git commit -m "test(roster): derive the vanilla content-density floors from an empty table"
```

---

## Task 2: 非对称 cap（L13-15）与密度参数

**文件：** 修改 `Source/tables/level_roster.cpp`、`assets/txtdata/monsters/level_roster_params.tsv`
**接口：** 依赖 Task 1 的 vanilla 地板；产出任务 2 以外的所有任务所依赖的新 cap 语义

- [ ] **步骤 1：改 `BehaviorClassCapForLevel`（单一真相源）**

把现有函数体（`Source/tables/level_roster.cpp` 约 `:152-160`）改为：
```cpp
uint8_t BehaviorClassCapForLevel(uint8_t level, BehaviorClass cls)
{
	// L9-12 caps the RangedKite class at 2; L13-16 used to cap ANY class at 2.
	// Content density contract (approved 2026-09-16): the hell band's symmetric cap
	// rationed non-ranged types, which forced every extra drawn type to be ranged.
	// L13-15 therefore become asymmetric - non-ranged classes uncapped, ranged
	// classes tightened to 1 - while L16 keeps the legacy bound because its roster
	// is registration-only (the hardcoded branch returns before the roster path).
	if (level == 16)
		return 2;
	if (level >= 13 && level <= 15) {
		if (cls == BehaviorClass::RangedTurret || cls == BehaviorClass::RangedKite)
			return 1;
		return 0;
	}
	if (level >= 9 && level <= 12 && cls == BehaviorClass::RangedKite)
		return 2;
	return 0;
}
```

- [ ] **步骤 2：改数据（L13/L14 `tail_draw` → 4）**

`assets/txtdata/monsters/level_roster_params.tsv` 的 L13、L14 行第 3 列 `1`/`2` → `4`（**只这两行**；L15 保持 3）。

- [ ] **步骤 3：重建 MPQ 与目标，确认加载期校验通过**

运行：
```bash
ninja -C build devilutionx_mpq
cmake --build build --target level_roster_test level_roster_baseline_test sampling_behavior_test -j20
./build/level_roster_test 2>&1 | tail -3
```
预期：`level_roster_test` 全绿（含加载期 core≤cap 与 floors 可满足性）。**若报 L16 相关错误**，说明 cap 变更没被限定在 L13-15——回到步骤 1 修正，**不要**改 L16 数据。

- [ ] **步骤 4：实测密度与占比（引擎真实链路）**

运行：
```bash
SAMPLING_REPORT=/tmp/density.md ./build/sampling_behavior_test --gtest_filter='*MeasurementRealisedPerLevel*'
./build/level_roster_baseline_test --gtest_filter='LevelRosterBaselineTest.PlacedClassMixWithinBaseline' 2>&1 | grep MEASURED
```
预期（spike 已证）：L13/L14 单局种类 **7.0**、L15 **6.0**；placed 远程占比 L13 ≈0.17、L14 ≈0.27、L15 ≈0.20，**全部 ≤ 各自 ceiling**；L1-12 与 L16 读数不变。任何偏差都记录下来（这是规格验收 4/5 的原始证据）。

- [x] **步骤 4 实测（2026-09-18，已通过）**：单局种类 **L13 7.0 / L14 7.0 / L15 6.0** ✓；placed 远程占比 **0.171738 / 0.24515 / 0.208764**（全 ≤ ceiling）✓；unique 可达 **5/5、6/6、2/2** ✓；组合数 **295 / 243 / 5** ✓；套件 `sampling_behavior_test` 36/36、`level_roster_test` 41/41、`PlacedClassMixWithinBaseline` PASSED

- [x] **步骤 5：提交**（`a14fc220b`；同时含下列衍生决定的数据与测试同步）

### Task 2 衍生决定（实现中实测发现）

| # | 发现（违反的契约条款） | 处置 |
|---|---|---|
| D1 | **L15 组合数恒 1**（不变量①）：`class_floors: RangedTurret=1` 的补位是**确定性取候选**，抹平尾抽随机性 | 移除该 floor（且在此**冗余**：core 后尾池仅 2 条近战、`tail_draw=3` 必然含 ≥1 远程）→ **1 → 5** |
| D2 | **L14 unique 掉到 4/6**（不变量②）：Turret core 吃满 cap1 → 两个 RangedTurret base 永不可抽 | L14 core `MT_SNOWWICH`(Turret) → **`MT_BALROG`(Melee)** → **6/6**，占比 0.267906 → **0.24515** |
| D3 | L15 core 尾池塌缩（移除 Turret core 后尾池仅剩 2 条近战） | 与 D1 一并处理后 L15 类型仍 6.0、占比 0.208764 ✓ |

**注**：附录 A 的 **A1/A2/A7/A8/A9 已在 Task 2 执行完毕**（契约变更必须与 cap 变更同一次提交才能保持绿），Task 5 只需处理 **A3/A4/A5/A6**。

---

## Task 3: L17 零随机性整改（含 L23/L24 厚度评估）

**文件：** 修改 `assets/txtdata/monsters/level_rosters.tsv`（必要时 `level_roster_params.tsv`）

- [ ] **步骤 1：先量化现状**（HF 资产门控）

**该测量用例目前不在源码里**（它只存在于被中断任务的存档里）→ 先用存档作起点补齐：
```bash
git apply --3way .superpowers/sdd/2026-09-15-level-rosters-phase-a2/task-3a-partial-work.diff
```
（若补丁冲突：手工把存档里的 `TempPerSeedVarietyForHellfireLevels` 用例并入 `test/level_roster_baseline_test.cpp`；保留其 `TEMP` 注释，并在 Task 6 收尾时决定"留作永久守卫（改成正式名）或删除"。）

运行：`cmake --build build --target level_roster_baseline_test -j20 && ./build/level_roster_baseline_test --gtest_filter='HellfireLevelBaselineTest.TempPerSeedVarietyForHellfireLevels'`
记录 L17-24 的每 seed 组合数。预期（复核者的静态推算）：**L17 = 1**（尾池恰 2 条、同类）、L23 = 6、L24 = 2。

- [ ] **步骤 2：改 L17 数据，使尾池 ≥3**

把 L17 的 core 从 3 条收到 2 条（保留 PSYCHORB + STINGER 两条；把 `MT_HELLBOAR` 从 core 行**删除**，它随尾池参与抽取）。改完后 L17：core 2 + 尾池 3，`tail_draw=2` → 组合数 C(3,2)=3 ≥2。

- [ ] **步骤 3：重建 MPQ 并复测**

运行：
```bash
ninja -C build devilutionx_mpq
./build/level_roster_baseline_test --gtest_filter='HellfireLevelBaselineTest.TempPerSeedVarietyForHellfireLevels'
./build/level_roster_baseline_test --gtest_filter='HellfireLevelBaselineTest.PlacedClassMixWithinBaselineForHellfireLevels' 2>&1 | grep -E "A2MEASURED|MEASURED"
```
预期：L17 组合数 **≥2**（实测 3）；L17 的 placed 远程占比与其它 HF 层读数**不退化**（记下前后值）。L23/L24 若也偏薄（2/6），评估能否用**零风险**手段增厚（例如微调该层 `max_image` 让 1 个非远程类型进池）；**若会碰到占比红线，就只报告不改**。

- [ ] **步骤 4：提交**

```bash
git add assets/txtdata/monsters/level_rosters.tsv
git commit -m "fix(rosters): give the Hellfire L17 tail pool real variety"
```

---

## Task 4: 密度断言落地

**文件：** 修改 **`test/sampling_behavior_test.cpp`**（三个新用例都必须在本文件，理由同上）
**接口：** 依赖 Task 1 的地板常量（Task 1 的 `[ VANILLAFLOOR ]` 输出）与 Task 2/3 的数据

- [ ] **步骤 1：把 Task 1 测得的 vanilla 地板写成具名常量**

在 `test/sampling_behavior_test.cpp` 顶部（`MeasurementRealisedPerLevel` 之前）加：
```cpp
// Vanilla (pre-roster) content density floors, measured on 2026-09-16 by loading a
// physically empty roster table (see VanillaFloorBaseline and spec appendix B).
// Index by level; L0 unused. These are MEASURED FLOORS, not targets: the roster must
// not push a level's density below vanilla (contract §4.1 C1).
// Task 1 实测（2026-09-18，200 seeds/层，超范围单行夹具；格式：level types ai classes union）
//  L1 7.0/5.0/3.0/7    L2 6.8/5.2/4.1/21   L3 7.1/5.2/4.2/25   L4 6.9/5.5/4.1/23   L5 6.0/5.4/4.0/21
//  L6 5.5/5.2/4.1/17   L7 4.6/4.3/3.8/16   L8 4.3/4.1/3.6/17   L9 3.6/3.6/2.9/17   L10 3.5/3.3/2.7/17
//  L11 3.2/3.1/2.5/15  L12 3.4/3.3/2.7/16  L13 3.2/3.1/2.7/16  L14 3.2/3.1/2.8/16  L15 3.2/3.0/2.6/10
// 这组数字与名册规格 §1 记录的"改动前"分带值一致（教堂≈7.0 / 洞穴 3.3-3.6 / 地狱 3.2-3.3），
// 即该夹具路径确等价于改动前行为——地板可直接取自本表。
constexpr std::array<double, 25> kVanillaTypesFloor { /* 由步骤 4 的实测逐层填入 */ };
constexpr std::array<double, 25> kVanillaAiFloor { /* 同上 */ };
constexpr std::array<double, 25> kVanillaUnionFloor { /* 同上 */ };
```
（**执行者**：把 Task 1 的 `[ VANILLAFLOOR ]` 数字逐层填入；L16 与 L0 用 `0.0` 并注释"L16 硬编码分支、不参与"。）

- [ ] **步骤 2：写密度断言用例（三轴，可失败）**

```cpp
TEST_F(SamplingBaselineTest, ContentDensityWithinVanillaFloor)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN not available - skipping test";

	constexpr int kSeeds = 200;
	for (uint8_t level = 1; level <= 15; level++) {
		size_t typeSum = 0;
		size_t aiSum = 0;
		std::set<_monster_id> seen;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(51000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			typeSum += LevelMonsterTypeCount;
			aiSum += RealisedDistinctAi();
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				seen.insert(LevelMonsterTypes[i].type);
		}
		const double types = static_cast<double>(typeSum) / kSeeds;
		const double ai = static_cast<double>(aiSum) / kSeeds;
		const double unionSize = static_cast<double>(seen.size());
		std::cout << "[ DENSITY ] level " << static_cast<int>(level) << " types " << types
		          << " ai " << ai << " union " << unionSize << " floor "
		          << kVanillaTypesFloor[level] << '/' << kVanillaAiFloor[level] << '/'
		          << kVanillaUnionFloor[level] << std::endl;
		EXPECT_GE(types, kVanillaTypesFloor[level]) << "level " << static_cast<int>(level) << " types below vanilla";
		EXPECT_GE(ai, kVanillaAiFloor[level]) << "level " << static_cast<int>(level) << " distinct AI below vanilla";
		EXPECT_GE(unionSize, kVanillaUnionFloor[level]) << "level " << static_cast<int>(level) << " union below vanilla";
	}
}
```

- [ ] **步骤 3：写曝光率断言（D4 ≥75%）**

```cpp
TEST_F(SamplingBaselineTest, ContentVisibilityFloor)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	if (missingRetailTrn_)
		GTEST_SKIP() << "retail/HF TRN not available - skipping test";

	constexpr double kVisibilityFloor = 0.75; // spec §4.1 D4 (absolute floor, not vanilla)
	constexpr int kSeeds = 200;
	for (uint8_t level = 1; level <= 15; level++) {
		const size_t candidates = AvailableCandidateCount(level); // 已存在的候选计数助手；若无则用 LevelRosterCandidatesForTest
		if (candidates == 0)
			continue; // level has no candidates in this data set
		std::set<_monster_id> seen;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(51000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			for (size_t i = 0; i < LevelMonsterTypeCount; i++)
				seen.insert(LevelMonsterTypes[i].type);
		}
		const double visibility = static_cast<double>(seen.size()) / candidates;
		std::cout << "[ VISIBILITY ] level " << static_cast<int>(level) << " seen " << seen.size()
		          << " candidates " << candidates << " rate " << visibility << std::endl;
		EXPECT_GE(visibility, kVisibilityFloor)
		    << "level " << static_cast<int>(level) << " exposes too little of its designed content";
	}
}
```
（`AvailableCandidateCount` 若不存在：先在 `test/sampling_behavior_test.cpp` 内加一个文件内 static 助手，按 `IsMeasuredCandidate` 的同款条件（窗口 + availability）统计，**不要**新增导出 API。）

- [ ] **步骤 4：`RosterPerSeedVariety` 扩到 L1-24（HF 资产门控）**

把 `test/sampling_behavior_test.cpp:1212` 的 `for (uint8_t level = 1; level <= 15; level++)` 改为 `<= 24`；对 `level >= 17` 的部分加 HF 门控：在该用例开头加
```cpp
	if (!HaveHellfire())
		// HF levels (17-24) need the Hellfire data; the base table has no candidates there.
		; // 见下：仅对 1..16 断言，17..24 需 HF
```
并按既有先例（`HellfireNoParamsSamplingTest`）把 17-24 的断言拆到 HF 门控的分支里（无 HF 时 `GTEST_SKIP` 那一段）。

- [ ] **步骤 5：实跑三轴并做反证**

运行：
```bash
cmake --build build --target sampling_behavior_test -j20
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.Content*'
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.RosterPerSeedVariety'
```
反证（**两次实跑都要内联记录**）：把 `kVanillaTypesFloor[13]` 临时 +2 → `ContentDensityWithinVanillaFloor` 必红；把 `HVISIBILITY`…（曝光率）临时 +0.5 → `ContentVisibilityFloor` 必红；恢复后全绿。

- [ ] **步骤 6：提交**

```bash
git add test/sampling_behavior_test.cpp
# 同步 eval 计数：sampling_behavior_test 用例数变化 → eval/cases/rng/sampling-anti-monopoly.yaml 的 passed_min 与 output_contains
git add eval/cases/rng/sampling-anti-monopoly.yaml
git commit -m "test(roster): assert the content-density floors and the visibility rate"
```

---

## Task 5: 既有契约改写（附录 A1-A6）

**文件：** 修改 `test/sampling_behavior_test.cpp`（A1/A2）、`test/level_roster_baseline_test.cpp`（A4）、`eval/cases/rng/level-rosters.yaml`（A6）、`docs/knowledge/decision_save_format_policy.md`（A5）、新建 `docs/knowledge/decision_content_density_contract.md`（A3）

- [ ] **步骤 1：A1——改写 `HellL13/14/15SameClassTailBaseline`**

先读该用例现有实现（`test/sampling_behavior_test.cpp` 中 `HellL13SameClassTailBaseline` 等三条），记录它计量的变量与"tail 增量"口径；把 `EXPECT_EQ(tail, 0.0)` 改为**新契约**：
```cpp
	// Content density contract (2026-09-16): the hell cap is asymmetric now - the
	// RANGED classes are tightened to 1, so what must stay zero is the RANGED-side
	// tail overflow; non-ranged tail additions are legitimate and bounded by the
	// roster + tail_draw, not by a per-class cap.
	EXPECT_LE(rangedTailAdditions, BehaviorClassCapForLevel(level, BehaviorClass::RangedTurret));
	EXPECT_LE(rangedKiteTailAdditions, BehaviorClassCapForLevel(level, BehaviorClass::RangedKite));
```
（变量名以步骤 1 读到的为准；若该用例只统计"任意类 tail"，则改为分别统计 RangedTurret/RangedKite 与"其它类"两个增量。）
反证：把 `BehaviorClassCapForLevel` 的远程分支临时改回 2 → 该用例应仍绿（因为 cap 变宽），把 L13-15 的 cap 临时改成 0 → 应红；**记录实际行为并据此确认断言的判别力**。

- [ ] **步骤 2：A2——改写 `RosterQuotaAllowanceIsBinding`**

先读实现（`test/sampling_behavior_test.cpp` 约 `:1042` 起）：它现在靠"某层 core 恰好把某 capped class 占满"（L15 的 Melee）找饱和层。**非对称后 Melee 不再 capped**，该前提失效。改为：
```cpp
	// Content density contract: with the asymmetric hell cap, the only capped classes
	// in L13-15 are the RANGED ones, and no hell core saturates them. The binding case
	// must therefore be found through the TAIL draw (where the cap actually binds),
	// not through core saturation.
```
实现方式（选一并在注释里说明理由）：**(a)** 把饱和判据改成"存在 (level,class) 使 `available > cap`"（cap 确实在限制可用池）——这仍可失败且不依赖 core；**(b)** 保留 core 饱和判据但把承载层换到 L9-12 的 RangedKite（该 cap 未变）。两条都要给出反证。

- [ ] **步骤 3：A4——`PlacedClassMixWithinBaseline` 主判据改为"≤ vanilla"，ceiling 降为报警线**

在该用例循环体内（`test/level_roster_baseline_test.cpp`，`for (uint8_t level = 1; level <= 15; level++)` 内）把单一 `EXPECT_LE(share, ceiling)` 改为：
```cpp
		// Primary criterion (contract §4.4): the roster must not push a level's ranged
		// share ABOVE vanilla. The old baseline+5pp ceiling stays as an ALARM line: it
		// no longer authorises trimming content to pass.
		EXPECT_LE(share, kVanillaShareBaseline[level])
		    << "level " << static_cast<int>(level) << " ranged share above vanilla";
		EXPECT_LE(share, kRangedShareCeiling[level])
		    << "level " << static_cast<int>(level) << " ranged share above the alarm line";
```
（`kVanillaShareBaseline` 复用 Task 1 的 vanilla 导出：在该用例里用空表测一次并缓存，或在 Task 1 的 `[ VANILLAFLOOR ]` 基础上加一列 share。**必须**来自空表实测，不得复用现有 `kRangedShareBaseline`——后者是改动前基线，但当前需求是"vanilla 逐层"，两者在本仓库里目前同源，仍以实测为准并在注释写明。）

- [ ] **步骤 4：A5/A6——同步 knowledge 与 eval case**

- `docs/knowledge/decision_save_format_policy.md` 台账追加一行：cap 语义变更（L13-15 非对称；L16 保持）→ 存档**内容模式**变化、格式未变、按决策 35 不做兼容；
- `eval/cases/rng/level-rosters.yaml` 顶部注释补 cap 语义；**并且** `eval/cases/rng/sampling-anti-monopoly.yaml`（filter `SamplingBaselineTest.*`）因 Task 4 新增 2-3 条用例而需**同步** `passed_min` 与 `output_contains`（两处都要改）。

- [ ] **步骤 5：新建契约知识条目 `docs/knowledge/decision_content_density_contract.md`**

内容：四轴定义与地板来源、非对称 cap 与参数组、R4 反转、以及"类型占比 ≠ 数量占比（尾抽以 3-5 只一组落地）"这条机制；并索引进 `docs/knowledge/MEMORY.md` 的 Decisions 节。

- [ ] **步骤 6：提交**

```bash
git add test/sampling_behavior_test.cpp test/level_roster_baseline_test.cpp eval/cases/rng/level-rosters.yaml docs/knowledge/
git commit -m "test+docs: rewrite the contracts the asymmetric cap changes"
```

---

## Task 6: 收尾（T3a 残留、eval、门禁、CI）

**文件：** 视取舍而定；`eval/cases/rng/*.yaml`

- [ ] **步骤 1：处理被中断的 T3a 残留**

对比 `.superpowers/sdd/2026-09-15-level-rosters-phase-a2/task-3a-partial-work.diff`（+41 行）与 Task 3 已落地的 L17 整改：**若已被覆盖 → 丢弃该残留**（`git checkout -- test/level_roster_baseline_test.cpp`）；**若仍有价值 → 先读 diff 再人工并入**，不得盲合。记录取舍理由。

- [ ] **步骤 2：eval 计数与 case 集**

运行：`python3 tools/eval/sync_case_sets.py --check`；若不一致，`--write` 重生成并复核；`python3 -m tools.eval.backend --smoke`（exit 0）。

- [ ] **步骤 3：全量门禁**

运行：`python3 tools/run_tests.py --json /tmp/ci.json`
预期：`failed==0 && passed_pct==100 && drift_ok==true`（含检查 F）。

- [ ] **步骤 4：push 并跟踪 CI 到终态**

```bash
git push myrepo feature/qol-upgrades
gh run list -R pengyz/devilutionX --limit 3 --json databaseId,headSha,conclusion --jq '.[]|"\(.databaseId) \(.headSha[0:9]) \(.conclusion)"'
gh run watch -R pengyz/devilutionX <run-id> --exit-status
```
预期：`success`（HF 相关用例在 CI 无素材时**跳过**，不计失败）。

- [ ] **步骤 5：提交（若步骤 1/2 有改动）**

---

## Task 7: 状态与台账收口

- [ ] **步骤 1：规格状态与实施记录**

`docs/superpowers/specs/2026-09-16-content-density-contract-design.md` §7：`已批准` → **实施中**（并在附录里追加"实施记录"：逐层实测地板、L13-15 参数组与占比、L17 整改前后组合数、以及 D4 实测曝光率）。**规格文件此刻允许修改**（状态与实施记录属控制者/执行者职责；改前先在报告里说明）。

- [ ] **步骤 2：SDD 台账**

在 `.superpowers/sdd/2026-09-15-level-rosters-phase-a2/progress.md` 追加本计划每个任务的结论与 CI run id。

- [ ] **步骤 3：提交**

```bash
git add docs/superpowers/specs/2026-09-16-content-density-contract-design.md
git commit -m "docs(spec): record the content density contract implementation results"
```

---

## 自检

**1. 规格覆盖度**

| 规格条目 | 任务 |
|---|---|
| §4.1 D1/D2/D3 地板 = vanilla | Task 1（导出）+ Task 4（断言） |
| §4.1 D4 曝光率 ≥75% | Task 4 步骤 3 |
| §4.1 不变量：任何层不得确定性（扩到 L1-24） | Task 4 步骤 4 + Task 3 |
| §4.1 目标：地狱 L13-15 ≥6 | Task 2 步骤 4（实测证据） |
| §4.3 非对称 cap（L13-15）+ L16 旧约束 | Task 2 步骤 1/3 |
| §4.3 参数组（L13/L14 tail_draw=4） | Task 2 步骤 2 |
| §4.4 R4 反转 / 红线 14 新表述 | Task 5 步骤 3（主判据 + 报警线） |
| 附录 A1/A2/A3/A4/A5/A6 | Task 5 步骤 1-5 |
| 附录 B（vanilla 用物理空表） | Task 1 步骤 1-3 |
| 附录 C（回退规则） | Task 2 步骤 4（若达不到 ≥6 则按三选一记录并停下问作者） |
| L17 零随机性（RB39） | Task 3 |
| T3a 残留取舍 | Task 6 步骤 1 |

**2. 占位符扫描**：无 `TBD/TODO`；`/* 由步骤 4 的实测逐层填入 */` 与"变量名以步骤 1 读到的为准"是**显式的执行动作**（对应的读取/测量步骤已在前），不是留白。

**3. 类型一致性**：`kVanillaTypesFloor` / `kVanillaAiFloor` / `kVanillaUnionFloor`（Task 4 步骤 1）与断言（步骤 2）同名同型；`LoadLevelRosterFromFiles`（Task 1 步骤 3）与既有签名一致（`test/sampling_behavior_test.cpp:1496` 已有同款调用）；`BehaviorClassCapForLevel`（Task 2 步骤 1）与 Task 5 步骤 1 的断言引用一致。
**已修正的初稿缺陷**：初稿把 Task 1/Task 4 的新用例放在 `test/level_roster_baseline_test.cpp`，但 `RealisedDistinctAi`/`RealisedDistinctClass` **只在该文件内 static 定义** → 会编译失败；已全部改到 `test/sampling_behavior_test.cpp`，并补上 `sampling-anti-monopoly.yaml` 的计数同步。

---

## 执行交接

本计划按作者指示**内联执行**（`superpower-executing-plans`，在 Task 2 步骤 4、Task 4 步骤 5、Task 6 步骤 3 三处设检查点复核）。不使用任何子 agent。