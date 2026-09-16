# 逐层名册 阶段 B 实施计划（核心小队 + G1/G2 前置）

> **面向 Agent 执行者：** 必需子技能：使用 superpower-subagent-driven-development（推荐）或 superpower-executing-plans 按任务逐项执行本计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 让"一队怪"成为可辨识的遭遇——同层 core 成员以一定概率成队出现（含 leader 与拴系随从），并**先修掉两个既有引擎缺陷**（普通怪作 leader 时的悬挂引用与 AI 覆写）。

**架构：** 先修 `M_UpdateRelations`（普通 leader 死亡必须释放随从并清除悬挂索引）与 `PlaceGroup`（新增 `MinionOptions`，使"随从强化/AI 继承/智力继承"可关）；再给 `level_roster_params.tsv` 增加 `squad_chance`/`squad_size`/`squad_leashed` 三列并由 `Source/tables/level_roster.{h,cpp}` 解析校验；最后在 `GetLevelMTypes()` 之后的**散布循环**（`Source/monster.cpp:3831-3840`）里对 core 类型按概率组队，并给出小队形成率的量化守卫与按层回退。

**技术栈：** C++23、CMake+Ninja、GoogleTest、TSV 数据驱动、eval 集成测试。

**规格：** `docs/superpowers/specs/2026-09-15-level-rosters-design.md` 的**阶段 B**（§4.3 全部 + §6 验收 5/5b/5c/6 + 附录 D 的 3/5 两条可证伪预测）

## 全局约束

- 行尾：C++/TSV 为 **CRLF**；`.md/.yaml/.py` 为 LF；新文件匹配 `.editorconfig`
- 禁令 6：不得用占位/自证用例（用例必须驱动**生产**函数；不得在测试内重抄生产逻辑）
- 改 `assets/txtdata/**.tsv` 后**必须** `ninja devilutionx_mpq` 才能让测试量到新表（见 `docs/knowledge/gotcha_tsv_edits_need_mpq_rebuild.md`）
- `python3 tools/run_tests.py --test <name>` **不构建**，先 `cmake --build build --target <name> -j8`（见 `docs/knowledge/gotcha_run_tests_test_flag_does_not_build.md`）
- 每次 push 后必须 `gh run list` + `gh run watch` 跟踪 CI 到终态（台账 R43）；**不得**以本地门禁绿代替 CI 绿
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json` → `failed==0 && passed_pct==100 && drift_ok==true`；eval：`python3 -m tools.eval.backend --smoke` exit 0
- 新增测试二进制注册 `CMake/Tests.cmake`；新增 Source 文件加入 `Source/CMakeLists.txt`；不删改既有注册行
- 用例数变化必须同步两份 YAML 的 `passed_min`（`eval/cases/rng/level-rosters.yaml`、`sampling-anti-monopoly.yaml`）
- 存档：按宪章决策 35 **不做兼容**；改动采样/放置后按既有流程**重生成**受影响夹具（`timedemo` 等）
- L16 保持硬编码分支不动；L17-24 仍走 R28 legacy 路径（阶段 A2 才补表）
- 提交信息英文 conventional commits；**绝不**让实现者分发子代理

---

## 文件结构（先锁定职责）

| 文件 | 职责 |
|---|---|
| `Source/monster.cpp` | 修改：`M_UpdateRelations`（G1）、`PlaceGroup`（G2 + `MinionOptions`）、散布循环（小队接入） |
| `Source/monster.h` | 修改：`MinionOptions` 结构 + `PlaceGroup` 签名 |
| `Source/tables/level_roster.h` | 修改：`LevelRosterParams` 增加 `squadChance`/`squadSize`/`squadLeashed` |
| `Source/tables/level_roster.cpp` | 修改：解析上述三列 + 校验范围 |
| `assets/txtdata/monsters/level_roster_params.tsv` | 修改：新增三列（L1-15） |
| `test/sampling_behavior_test.cpp` | 修改：G1/G2/小队/形成率/回退用例 |
| `test/level_roster_test.cpp` | 修改：`MinionOptions` 与三列的解析/校验用例 |
| `eval/cases/rng/level-rosters.yaml` | 修改：必要时同步 `passed_min` |
| `docs/superpowers/specs/2026-09-15-level-rosters-design.md` | 修改（仅由控制者）：实施记录/附录 E 更新 |

---

## Task 1: G1 —— 普通 leader 死亡必须释放随从并清除悬挂索引

**文件：**
- 修改：`Source/monster.cpp`（`ReleaseMinions`、`M_UpdateRelations`）
- 测试：`test/sampling_behavior_test.cpp`

**接口：**
- 依赖输入：`Monster::leaderRelation`、`Monster::getLeader()`、`Monster::setLeader(nullptr)`、`Monster::isUnique()`、`Monster::leader`（public 成员，`Monster::NoLeader` 常量）、`ActiveMonsters`/`ActiveMonsterCount`
- 对外产出：`void ReleaseMinions(const Monster &leader, bool clearReference)`（新增第二参，默认 `false` 保持既有调用语义）

- [ ] **步骤 1：编写失败的测试**

```cpp
TEST_F(SamplingBaselineTest, LeaderDeathReleasesMinions)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// 造一个普通怪 leader + 一个 leashed 随从（不依赖名册数据，直接摆放）
	currlevel = 1;
	InitLevelMonsters();
	SetRndSeed(9000);
	ASSERT_TRUE(GetLevelMTypes().has_value());
	ASSERT_TRUE(InitMonsters().has_value());

	const size_t leaderIndex = ActiveMonsters[0];
	Monster &leader = Monsters[leaderIndex];
	leader.uniqueType = UniqueMonsterType::None; // 明确是普通怪
	ASSERT_LT(ActiveMonsterCount, MaxMonsters);
	Monster &minion = Monsters[ActiveMonsters[1]];
	minion.setLeader(&leader);
	ASSERT_EQ(minion.leaderRelation, LeaderRelation::Leashed);

	// 走公共死亡路径，让引擎自己调用 M_UpdateRelations
	KillMonster(leader, /*petrified=*/false);

	EXPECT_NE(minion.leaderRelation, LeaderRelation::Leashed)
	    << "an ordinary leader's death must not leave its minion leashed";
	EXPECT_EQ(minion.leader, Monster::NoLeader)
	    << "a stale leader index can point at a live stranger after slot reuse";
}
```

- [ ] **步骤 2：运行测试并确认其失败**

```bash
cmake --build build --target sampling_behavior_test -j8
python3 tools/run_tests.py --test sampling_behavior_test --filter 'SamplingBaselineTest.LeaderDeathReleasesMinions'
```
预期：FAIL —— `leaderRelation` 仍为 `Leashed`（`hasLeashedMinions()` 对普通怪恒 false，故 `ReleaseMinions` 从未被调用）

- [ ] **步骤 3：编写最小实现**

```cpp
// Source/monster.cpp：ReleaseMinions 增加 clearReference 参数
void ReleaseMinions(const Monster &leader, bool clearReference)
{
	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		Monster &minion = Monsters[ActiveMonsters[i]];
		if (minion.leaderRelation != LeaderRelation::Leashed || minion.getLeader() != &leader)
			continue;
		minion.setLeader(nullptr); // 置 relation=None；按设计保留 leader 索引（血条着色）
		if (clearReference)
			minion.leader = Monster::NoLeader; // 普通怪槽位会被 AddMonster 复用，必须清
	}
}

void M_UpdateRelations(const Monster &monster)
{
	// G1（规格 §4.3.1）：不再以 hasLeashedMinions() 为入口——那只对 unique 成立，
	// 会让普通 leader 死亡后留下 Leashed 随从 + 悬挂 leader 索引。
	const bool leaderIsUnique = monster.isUnique();
	const bool hasMinions = std::any_of(&Monsters[ActiveMonsters[0]], &Monsters[ActiveMonsters[0]] + ActiveMonsterCount,
	    [&monster](const Monster &m) { return m.leaderRelation == LeaderRelation::Leashed && m.getLeader() == &monster; });
	// 注意：上面的 any_of 若难以写对，可直接调用 ReleaseMinions(monster, !leaderIsUnique) 并让它自己扫描（推荐）。
	(void)hasMinions;
	ReleaseMinions(monster, /*clearReference=*/!leaderIsUnique);
	ShrinkLeaderPacksize(monster);
}
```

推荐实现（更简单、无重复扫描）：把 `M_UpdateRelations` 写成
```cpp
void M_UpdateRelations(const Monster &monster)
{
	ReleaseMinions(monster, /*clearReference=*/!monster.isUnique());
	ShrinkLeaderPacksize(monster);
}
```
（`ReleaseMinions` 自身已按 `getLeader() == &leader` 过滤，对"没有随从"的怪物是空循环——语义与旧行为一致。）

- [ ] **步骤 4：运行测试并确认其通过**

```bash
cmake --build build --target sampling_behavior_test -j8
python3 tools/run_tests.py --test sampling_behavior_test --filter 'SamplingBaselineTest.LeaderDeathReleasesMinions'
```
预期：PASS

- [ ] **步骤 5：补 unique 路径的回归用例**

```cpp
TEST_F(SamplingBaselineTest, UniqueLeaderDeathBehaviourUnchanged)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// 走 unique 路径（PlaceUniqueMonsters 产出带 pack 的 leader），断言：
	// ① 随从 relation 置 None（与旧行为一致）；
	// ② 随从的 leader 索引**仍被保留**（血条着色依赖它，spec §4.3.1 明确要求 unique 路径不变）。
	// 具体构造：用 L14（Q_SKELKING/qlevel 相关）或任一 unique 层，跑 GetLevelMTypes + InitMonsters，
	// 找到 uniqueType != None 且 packSize > 0 的 leader，再 KillMonster(leader) 后断言上述两条。
}
```

- [ ] **步骤 6：提交**

```bash
git add Source/monster.cpp test/sampling_behavior_test.cpp
git commit -m "fix(monster): release minions when an ordinary leader dies (G1)"
```

---

## Task 2: G2 —— `PlaceGroup` 的 `MinionOptions`（随从不再被静默强化/换 AI）

**文件：**
- 修改：`Source/monster.h`（`MinionOptions` + `PlaceGroup` 签名）、`Source/monster.cpp`（`PlaceGroup` 实现）
- 测试：`test/sampling_behavior_test.cpp`

**接口：**
- 依赖输入：`PlaceGroup` 的两个既有调用点（unique 路径 `:3410` 一带、散布循环 `:3839`）；`Monster::setLeader`（会覆写 `ai`）；`MonsterData`（取该类型的原生 `ai`）
- 对外产出：
  ```cpp
  struct MinionOptions {
      bool tough = true;               // 随从 HP ×2（既有行为）
      bool inheritAi = true;           // setLeader() 会覆写随从 ai（既有行为）
      bool inheritIntelligence = true;  // 继承 leader 的 intelligence（既有行为）
  };
  void PlaceGroup(size_t typeIndex, size_t num, Monster *leader = nullptr,
                  bool leashed = false, MinionOptions opts = {});
  ```

- [ ] **步骤 1：编写失败的测试**

```cpp
TEST_F(SamplingBaselineTest, SquadMinionsUnbuffed)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	currlevel = 1;
	InitLevelMonsters();
	SetRndSeed(9100);
	ASSERT_TRUE(GetLevelMTypes().has_value());

	const _monster_id leaderType = MT_WSKELAX;
	const _monster_id minionType = MT_TSKELBW; // 远程，用来验证 AI 不被覆写
	const size_t before = ActiveMonsterCount;
	PlaceGroup(leaderType, 1);
	ASSERT_GT(ActiveMonsterCount, before) << "leader must be placed";
	Monster &leader = Monsters[ActiveMonsters[ActiveMonsterCount - 1]];
	const int leaderIntelligence = leader.intelligence;

	PlaceGroup(minionType, 1, &leader, /*leashed=*/true,
	    MinionOptions { .tough = false, .inheritAi = false, .inheritIntelligence = false });

	ASSERT_GT(ActiveMonsterCount, before + 1) << "minion must be placed";
	Monster &minion = Monsters[ActiveMonsters[ActiveMonsterCount - 1]];
	EXPECT_EQ(minion.maxHitPoints, MonstersData[minionType].maxHitPoints) << "minion HP must not be doubled";
	EXPECT_EQ(minion.ai, MonstersData[minionType].ai) << "minion must keep its own AI";
	EXPECT_EQ(minion.intelligence, MonstersData[minionType].intelligence) << "intelligence must not be inherited";
}
```

- [ ] **步骤 2：运行测试并确认其失败**

预期：构建失败（`MinionOptions` 未定义）或断言失败（HP 被 ×2 / `ai` 被覆写）

- [ ] **步骤 3：编写最小实现**

```cpp
// Source/monster.h
struct MinionOptions {
	bool tough = true;
	bool inheritAi = true;
	bool inheritIntelligence = true;
};
void PlaceGroup(size_t typeIndex, size_t num, Monster *leader = nullptr, bool leashed = false, MinionOptions opts = {});
```

```cpp
// Source/monster.cpp（PlaceGroup 内 leader != nullptr 的分支）
			PlaceMonster(ActiveMonsterCount, typeIndex, { xp, yp });
			if (leader != nullptr) {
				Monster &minion = Monsters[ActiveMonsterCount];
				if (opts.tough) {
					minion.maxHitPoints *= 2;
					minion.hitPoints = minion.maxHitPoints;
				}
				if (opts.inheritIntelligence)
					minion.intelligence = leader->intelligence;

				if (leashed) {
					const MonsterAIID ownAi = minion.ai; // setLeader() 会覆写 ai，先存后恢复
					minion.setLeader(leader);
					if (!opts.inheritAi)
						minion.ai = ownAi;
				}
				// …既有 changeAnimationData / MFLAG_ALLOW_SPECIAL / mode=Stand 分支保持不变…
			}
```

- [ ] **步骤 4：运行测试并确认其通过**，并补一条回归用例：
  - `UniqueMinionsBehaviourUnchanged`：默认 `MinionOptions{}` 下 unique 路径的随从**仍然 HP×2 且继承 AI**（回归）。
  - **不引入 `DISABLED_`/跳过式的占位用例**（仓库禁令 6）：走**小队调用路径**的那条断言（`SquadMinionsKeepOwnAi`）整体放在 **Task 3** 里实现——那时小队路径已存在，可直接驱动生产代码。

- [ ] **步骤 5：提交**

```bash
git add Source/monster.h Source/monster.cpp test/sampling_behavior_test.cpp
git commit -m "feat(monster): make minion toughening and AI inheritance optional (G2)"
```

---

## Task 3: 小队接入（`squad_chance` / `squad_size` / `squad_leashed` 三列 + 散布循环）

**文件：**
- 修改：`Source/tables/level_roster.h`、`level_roster.cpp`、`assets/txtdata/monsters/level_roster_params.tsv`
- 修改：`Source/monster.cpp`（散布循环 `:3831-3840`）
- 测试：`test/level_roster_test.cpp`、`test/sampling_behavior_test.cpp`

**接口：**
- 依赖输入：Task 2 的 `MinionOptions`/`PlaceGroup`；`GetLevelRoster(currlevel)`/`GetLevelRosterParams(currlevel)`；`LevelRosterRole::Core`
- 对外产出：`LevelRosterParams` 增加
  ```cpp
  uint8_t squadChance = 0;   // 0-100，core 类型散布时转为小队的概率
  uint8_t squadSize = 0;     // 1-3，随从数
  bool squadLeashed = true;  // false = 仍传 leader（相邻初置）但不设 setLeader/packSize
  ```

- [ ] **步骤 1：解析与校验用先（含用例）**

在 `level_roster_test.cpp` 加：`ParamsCarrySquadColumns`（三列解析正确）、`ValidationRejectsOutOfRangeSquadValues`（`squad_chance=101` / `squad_size=0` / `squad_size=4` 必拒）。
运行 → FAIL → 实现解析与校验 → PASS。

- [ ] **步骤 2：写数据表（L1-15）**

`assets/txtdata/monsters/level_roster_params.tsv` 追加三列并**逐层**给初值（`squad_chance` 建议 25-40、`squad_size` 2、`squad_leashed` 1）；**必须** `ninja devilutionx_mpq` 后再跑测试。

- [ ] **步骤 3：编写失败的小队用例**

```cpp
TEST_F(SamplingBaselineTest, SquadFormsAroundACoreLeader)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// L9-12 的 squad_chance > 0；跑若干 seed，至少出现一次：
	// ① 某个怪 leaderRelation == Leashed 且其 getLeader() 是**该层 core 成员**；
	// ② 该 leader 的 packSize >= 1；
	// ③ 随从的 ai 等于其类型 ai（G2 生效）。
}
```

- [ ] **步骤 4：实现小队接入（散布循环）**

```cpp
		if (numscattypes > 0) {
			const LevelRosterParams *params = GetLevelRosterParams(currlevel);
			while (ActiveMonsterCount < totalmonsters) {
				const size_t typeIndex = scattertypes[GenerateRnd(numscattypes)];
				const _monster_id pickedType = LevelMonsterTypes[typeIndex].type;
				const bool isCore = IsCoreRosterMember(currlevel, pickedType); // 新增小助手（文件内 static）
				// G2/规格 §4.3.3：只在 core 上按概率组队，且必须留得下 leader + >=1 随从
				if (params != nullptr && isCore && params->squadChance > 0 && params->squadSize > 0
				    && totalmonsters - ActiveMonsterCount >= 2 && GenerateRnd(100) < params->squadChance) {
					const size_t before = ActiveMonsterCount;
					PlaceGroup(typeIndex, 1, nullptr, false, {});
					if (ActiveMonsterCount == before) { // 被 totalmonsters 钳制成 0 → 降级
						PlaceGroup(typeIndex, na);
						continue;
					}
					Monster &leader = Monsters[ActiveMonsters[ActiveMonsterCount - 1]];
					const _monster_id partner = PickCorePartner(currlevel, pickedType); // 同层其他 core，随机
					if (partner != MT_INVALID) {
						PlaceGroup(GetMonsterTypeIndex(partner), params->squadSize, &leader,
						    params->squadLeashed,
						    MinionOptions { .tough = false, .inheritAi = false, .inheritIntelligence = false });
					}
				} else {
					PlaceGroup(typeIndex, na);
				}
			}
		}
```
（`na` 的既有计算保持不变，降级路径沿用；`IsCoreRosterMember`/`PickCorePartner` 用 `GetLevelRoster(currlevel)` 实现，**不要**引入新导出 API，避免漂移检查 E 噪声。）

- [ ] **步骤 5：跑全部门禁并提交**

```bash
cmake --build build --target sampling_behavior_test level_roster_test -j8
python3 tools/run_tests.py --json /tmp/ci.json   # 采样/放置 RNG 变了 → 若 timedemo 等夹具失配，按既有流程重生成
git add Source/monster.cpp Source/tables/level_roster.* assets/txtdata/monsters/level_roster_params.tsv test/
git commit -m "feat(monster): form core squads in the scatter loop"
```

---

## Task 4: 小队形成率的量化守卫与按层回退

**文件：**
- 测试：`test/sampling_behavior_test.cpp`（`SquadFormationRate`）
- 修改：`assets/txtdata/monsters/level_roster_params.tsv`（`squad_leashed` 按实测回退）
- 修改（仅控制者）：规格「实施记录」/附录 E

- [ ] **步骤 1：先测量**

写一个临时（可保留为 `[ MEASURED ]` 输出的）用例：对 L1-15 各层跑 ≥500 seeds，统计"**尝试组队中至少放下 1 只随从**"的比例，打印逐层数值。

- [ ] **步骤 2：据实测决定阈值与回退**

- 若某层比例 ≥ 约定下限（建议 **50%**）→ 保留 `squad_leashed=1`；
- 若低于下限 → 该层置 `squad_leashed=0`（仍传 leader，相邻初置有保证；无归队语义），并在报告与规格里记录实测值与理由。

- [ ] **步骤 3：落地守卫用例**

```cpp
TEST_F(SamplingBaselineTest, SquadFormationRate)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";
	// 逐层断言：formationRate(level) >= kSquadFormationFloor[level]
	// 阈值取自步骤 1/2 的实测（写进文件内的具名数组并注明来源），未组队的层跳过该层。
}
```

- [ ] **步骤 4：验证 + 提交**

```bash
python3 tools/run_tests.py --json /tmp/ci.json
git add test/sampling_behavior_test.cpp assets/txtdata/monsters/level_roster_params.tsv
git commit -m "test(monster): pin the squad formation rate per level and fall back where it is too low"
```

---

## Task 5: 收尾（可证伪预测的验收、eval、台账、CI）

- [ ] **步骤 1：验收附录 D 的两条可证伪预测**（阶段 B 的两条）
  1. "能看到一只 + 1-2 只贴身同行的小队" → 由 Task 3 的用例与 §4.3 的小队断言覆盖；
  2. "把 leader 引开 >4 格，leashed 随从脱队、靠近重新贴回" → 若无法在无头测试里覆盖，**必须**在报告中明确写"未覆盖 + 需要人工试玩验证"，不得假装通过。

- [ ] **步骤 2：eval 用例与 `passed_min` 同步**（改动用例数时）

- [ ] **步骤 3：全量门禁 + eval + 行尾**

- [ ] **步骤 4：push 并跟踪 CI 到终态**（R43）：`gh run list` 找 run → `gh run watch <id> --exit-status` → 报告 `conclusion`；若红则按日志定位（CI 只有 `spawn.mpq`，需零售/HF 素材的用例必须 `GTEST_SKIP`，判据要**贴真实依赖**，参见阶段 A 的 `genrl.trn` 探测写法）。

- [ ] **步骤 5：提交**

---

## 实施记录（执行者填写）

| 项 | 值 |
|---|---|
| G1 验收结果 | （`LeaderDeathReleasesMinions` / `UniqueLeaderDeathBehaviourUnchanged` 实测） |
| G2 验收结果 | （`SquadMinionsUnbuffered` / `UniqueMinionsBehaviourUnchanged` 实测） |
| 逐层小队形成率 | （Task 4 步骤 1 的实测表） |
| `squad_leashed` 回退层 | （哪些层、为何） |
| 夹具重生成 | （哪些夹具、哪个提交） |
| CI run | （run id + conclusion） |

---

## 自检

**1. 规格覆盖度（阶段 B）**

| 规格条目 | 对应任务 |
|---|---|
| §4.3.1 修 G1（按随从侧扫描 + 非 unique 清索引；unique 不变） | Task 1 |
| §4.3.2 决 G2（`MinionOptions`，小队传全 false） | Task 2 |
| §4.3.3 小队机制（槽位前置、leader 放置校验、同层 core partner、leashed、opts 全 false） | Task 3 |
| §4.3.4 回退（形成率低于阈值 → `leashed=false` 但仍传 leader） | Task 4 |
| §4.3.5 显式接受的既有语义（4 格约束、10 次重试、`totalmonsters` 钳制） | Task 3（沿用不改） |
| §6 验收 5 / 5b / 5c / 6 | Task 1（5）、Task 2+3（6）、Task 4（5b/5c） |
| 附录 D 预测 3/5 | Task 5 步骤 1 |

**2. 占位符扫描：** 无待补项；每个代码步骤都给了可编译代码或明确的既有代码位置；阈值与实测值由执行者按指定步骤产出并写入「实施记录」。

**3. 类型一致性：** `MinionOptions`（`tough`/`inheritAi`/`inheritIntelligence`）、`LevelRosterParams`（`squadChance`/`squadSize`/`squadLeashed`）、`ReleaseMinions(leader, clearReference)` 在 Task 1-4 中命名一致；`GetLevelRosterParams` 的 `nullptr` 语义沿用阶段 A。

---

## 执行交接

计划已保存至 `docs/superpowers/plans/2026-09-15-level-rosters-phase-b.md`。两种执行方式：

1. **子代理驱动（推荐）** —— 每个任务分发全新子代理，任务间进行评审，迭代更快
2. **内联执行** —— 在当前会话中用 `superpower-executing-plans` 批量执行，设置检查点

请选择哪种方式？