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

