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

