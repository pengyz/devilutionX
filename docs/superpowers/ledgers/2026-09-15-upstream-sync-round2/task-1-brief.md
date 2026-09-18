## Task 1: 预检与基线固化

**文件：**
- 修改：无（只读 + 记录）

**接口：**
- 依赖输入：无
- 对外产出：确认的 `UPSTREAM_TIP`（供任务 2、11 使用的 SHA）、基线门禁 JSON（供任务 10 对比）

- [ ] **步骤 1：确认上游 tip 未变**

```bash
git fetch origin master
git log -1 --format='%H %ad %s' --date=iso origin/master
```

预期：`e00b7260f`，提交信息 `Prevent inlining of blitter operations`。**若 SHA 变了：停止，重新跑探测性 merge 并回报新的冲突面。**

- [ ] **步骤 2：确认工作区干净且分支正确**

```bash
git status --short          # 预期：无输出
git branch --show-current   # 预期：feature/qol-upgrades
git worktree list           # 预期：只有主工作区
```

- [ ] **步骤 3：记录基线门禁**

```bash
python3 tools/run_tests.py --json /tmp/ci-before-sync.json 2>&1 | tail -20
```

预期：`ctest.failed == 0`、`ctest.passed_pct == 100`、`ctest.total == 698`、`drift.drift_ok == true`。**若不满足：先停下来回报——不要在有红灯的基线上做 merge。**

- [ ] **步骤 4：记录基线 drift 输出（供任务 10 对比口径变化）**

```bash
python3 tools/check_drift.py --base origin/master 2>&1 | tee /tmp/drift-before-sync.txt
```

预期：5 项 PASS，`merge-base: b3e52b1ea`。

- [ ] **步骤 5：无需提交（本任务不产生代码变更）**

---

