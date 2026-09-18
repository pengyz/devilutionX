## Task 9: 门禁 3-4（测试目标 + 全量测试）

**文件：**
- 修改：无（除非测试注册漂移，按判据处理）

**接口：**
- 依赖输入：任务 8 的编译通过
- 对外产出：`ctest.failed == 0`

- [ ] **步骤 1：全部测试目标构建**

```bash
python3 tools/run_tests.py --json /tmp/ci-sync.json 2>&1 | tail -30
```

预期：`build.ok == true`；随后 ctest 全量执行。

- [ ] **步骤 2：判定并记录**

```bash
python3 -c "
import json; d=json.load(open('/tmp/ci-sync.json'))
print('ctest', d['steps']['ctest']); print('drift_ok', d['steps']['drift']['drift_ok'])"
```

预期：`failed == 0`、`passed_pct == 100`。测试数量若不再是 698：**必须记录变化原因**（上游新增/删除测试），判据仍是零失败。

- [ ] **步骤 2b：按名字确认 timedemo 仍为 Skipped（不能只看聚合计数）**

`run_tests.py` 的 `summary` 只记录 skipped **计数**，不记录被跳过用例的名字，因此聚合数字不变也可能掩盖状态翻转：

```bash
grep -n "WarriorLevel1to2" build/Testing/Temporary/LastTest.log
```

预期：出现 `Timedemo.WarriorLevel1to2 (Skipped)`（规格 §6 验收项 9）。

- [ ] **步骤 2c：确认本地测试目标列表未与 `CMake/Tests.cmake` 脱钩**

`tools/run_tests.py` 的 `TEST_TARGETS` 是手工列表，`check_drift.py` 的 A 项只校验 `Tests.cmake` ↔ `test/*.cpp`，**不校验这个列表**：

```bash
git diff b3e52b1ea origin/master -- CMake/Tests.cmake | grep -E '^[+-]  [a-z_0-9]+_test'
```

预期：无输出（本轮上游对 `Tests.cmake` 的改动只涉及 `dun_render_benchmark` 的链接依赖与 `test_main`，未增删改任何 `tests`/`standalone_tests` 成员）。**若有输出：必须同步更新 `tools/run_tests.py` 的 `TEST_TARGETS`**，否则该工具会静默漏跑新测试。

- [ ] **步骤 3：失败处理（若发生）**

```bash
cd build && ctest --output-on-failure -R "<失败测试名>"
```

单测失败 → 在 `build/Testing/Temporary/LastTest.log` 读栈与断言 → 判断是「上游行为改变」还是「fork 适配缺失」，**不要用改测试断言的方式让门禁变绿**；若确认是上游的既有行为变化，按 `docs/knowledge/` 规则记录并在此计划末尾追加「执行中暴露的问题」小节。

---

