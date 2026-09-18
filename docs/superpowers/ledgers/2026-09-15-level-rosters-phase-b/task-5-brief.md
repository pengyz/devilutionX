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
