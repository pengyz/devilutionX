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

