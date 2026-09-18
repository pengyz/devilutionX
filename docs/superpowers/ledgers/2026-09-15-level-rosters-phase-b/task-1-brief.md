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

