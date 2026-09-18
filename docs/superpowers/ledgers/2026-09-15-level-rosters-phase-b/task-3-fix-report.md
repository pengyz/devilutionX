# Task 3 修复轮次 1 报告（阶段 B / 小队接入）

轮次：1/5。目标：为两条无守卫的规格要求补上**可失败**的测试守卫，并对四条断言各做一次"改坏→红→恢复→绿"反证。

## 待办
- [x] F1 RB15：拆出具名用例 `SquadMinionsKeepOwnAi`
- [x] F2 回退路径相邻性成为可失败守卫
- [x] F3 partner ≠ leader 类型可失败
- [x] F4 `squad_leashed` 解析器拒绝拼写错误用例
- [x] 四条反证实跑
- [x] 门禁 + eval 计数同步
- [x] 分块提交

## 进展

### 设计决定

**F2 的关键障碍**：unleashed 随从的 `leaderRelation` 是 `None`、`leader == Monster::NoLeader`，
放置完成后**无法**把它和 leader 重新配对——观察器从已放置怪物侧永远看不到这条关系。
因此邻近性必须在**放置那一刻**测量：给 `SquadRollCounters` 加两个计数
（`partnersPlaced` 样本量 + `maxPartnerLeaderDistance` 实测最大 Chebyshev 距离），
在 `Source/monster.cpp` 小队分支里对本轮新放置的随从逐个记录。这属于评审明确允许的
"扩展观察器记录实际位置/距离"。

**F2 的界怎么推**（不照抄 4）：
- 锚点 = `leader->position.tile + Direction(GenerateRnd(8))` → 与 leader 的 Chebyshev 距离恒为 1
  （`Displacement::fromDirection` 的 |dx|,|dy| ≤ 1）；
- 每次候选被拒后走一步 `Displacement(Direction(...))`，x/y 各变化 ≤ 1
  （`yp += ...deltaX` 这个既有 BUGFIX 注释里的 bug 也不会放大：deltaX ∈ [-1,1]）；
- 内层 `try2 < 100` → 最多 100 步。
→ **无 leash 时可证上界 = 1 + 100 = 101**。这是诚实的最坏界而非紧界，但仍是真守卫：
不传 leader 的话锚点来自 `GenerateRnd(80) + 16`（全图任意位置），会直接冲破 101。

**为什么最终界是 32 而不是 101**：101 虽可证，但**不可失败**——散布区是 `[16, 96)`，
全图任意两格的 Chebyshev 距离 ≤ 80，所以连"完全不传 leader"也过得了 101。
守卫必须落在"真实放置"与"无 leader 锚点"之间：
- 出厂路径实测（L1-15 × 40 seed，25204 个随从）：**max 17**
- `&leader` → `nullptr` 实测（L9-12 × 10 seed，与用例同口径）：**66**
→ 取 **32**（≈1.9× 实测最大，且不到 66 的一半）。这不是规格常量，只是这条分隔线；
注释里把两次实测与推导都写清，并无条件打印 `[ SQUADFALLBACK ]` 实测最大距离，
让漂移在变红之前就可见。

**F4 的死亡测试形态**：解析失败走 `RecordReader::failOnError` →
`DataFile::reportFatalFieldError` → `app_fatal`，**退出码 1**（不是 SIGABRT），
故沿用既有 `ParseClassFloors` 用例的 `ExitedWithCode(1)`。匹配文本用解析器自己的
`"Invalid squad_leashed value"`，而非通用的 `"Invalid value ... for squad_leashed"`，
这样钉住的是这个解析器而不只是"加载失败了"。

## 四条反证实跑（改坏 → 红 → 恢复 → 绿）

### F1 `SquadMinionsKeepOwnAi`
改坏：`MinionOptions{.inheritAi = false}` → `true`（即让 setLeader 的 leader AI 留在随从上）。
```
--- F1 BROKEN RUN ---
level 9 seed 0: G2 requires a squad minion to keep its own type's AI, but setLeader's leader AI survived on 30 minion(s)
level 9 seed 1: ... survived on 30 minion(s)
level 9 seed 2: ... survived on 31 minion(s)
```
恢复后：
```
--- F1 RESTORED ---
[==========] 1 test from 1 test suite ran. (1329 ms total)
[  PASSED  ] 1 test.
```

### F2 回退路径邻近性
改坏：`PlaceGroup(partnerIndex, size, &leader, ...)` → `nullptr`（评审实测此前全绿的那次改动）。
```
--- F2 BROKEN RUN ---
level 9 seed 0: spec 4.3.4 keeps passing the leader to PlaceGroup, so an unleashed minion must still be seeded from the leader's neighbourhood
level 9 seed 1..7: 同上（每个 seed 均红）
```
恢复后：
```
--- F2 RESTORED ---
[ SQUADFALLBACK ] unleashed partners 1251 max leader distance 12 bound 32
[  PASSED  ] 1 test.
```

### F3 partner 必须与 leader 异类型
改坏：删掉 `PickCorePartnerTypeIndex` 的 `entry.type == leaderType` 跳过
（`if (entry.role != Core || entry.type == leaderType)` → `if (entry.role != Core)`）。
```
--- F3 BROKEN RUN ---
level 9 seed 0: spec 4.3.3 requires a squad to mix two DIFFERENT core types
level 9 seed 1..7: 同上（每个 seed 均红）
```
恢复后（`SquadPlacementTest.*` 全套）：
```
--- F3 RESTORED ---
[ SHIPPEDSQUAD ] level 14 eligible 1375 rolls 121 rollRate 0.088 realised 121 realisation 1 minions 238 lost(leader/partner) 0/0
[==========] 6 tests from 1 test suite ran. (63217 ms total)
[  PASSED  ] 6 tests.
```

### F4 `squad_leashed` 拒绝拼写错误
改坏：把 `return std::unexpected(StrCat("Invalid squad_leashed value ...))` 换成 `return false;`（静默默认）。
```
--- F4 BROKEN RUN ---
[  PASSED  ] 0 tests.
[  FAILED  ] LevelRosterFixtureLoadTest.LoadingRejectsANonBooleanSquadLeashedSpelling
[  FAILED  ] LevelRosterFixtureLoadTest.LoadingRejectsAnOutOfRangeNumericSquadLeashed
```
恢复后（`level_roster_test` 全套）：
```
--- F4 RESTORED ---
[==========] 37 tests from 4 test suites ran. (322 ms total)
[  PASSED  ] 37 tests.
```

## 门禁

- `cmake --build build --target level_roster_baseline_test sampling_behavior_test level_roster_test -j8` → 全绿
- `python3 tools/run_tests.py --json /tmp/ci.json` → `passed 763 / failed 0 / passed_pct 100`，
  `drift_ok true`（A/B/C/C2/E 全 PASS），exit 0
- `python3 -m tools.eval.backend --smoke` → exit 0（全类别 1.0）
- `python3 -m tools.eval.backend --run level-rosters` → PASS (45/45)

## eval 计数同步

`eval/cases/rng/level-rosters.yaml`：filter 覆盖的用例 8 → **9**（新增 `SquadMinionsKeepOwnAi`），
同步 `passed_min: 9` 与 `output_contains` 的 `"[  PASSED  ] 9 tests."`，
并新增 `"[ SQUADFALLBACK ]"`（回退邻近性的实测行，防止该测量被悄悄删掉）。
F4 的两条解析器用例在 `level_roster_test` 二进制里（37 个用例全绿），不属于本 case 的 filter，
无需改其它 YAML；`sampling-anti-monopoly.yaml` 的 30 未受影响（未动 sampling_behavior_test）。

## 未做（按禁令）

- 未动 `squad_chance`/`squad_size`/`squad_leashed` 任何数值（RB17：L14 冻结）
- 未动 `kRangedShareCeiling` 或任何阈值（R4）
- 未写 `docs/`（RB16 已由控制者完成）
- 无 `DISABLED_`/占位用例（漂移检查 B PASS）

## 顾虑

1. **F2 的界是实测分隔线，不是紧界**。`PlaceGroup` 的游走是 100 步无界随机步，代码本身只能
   证到 101，而 101 因地图尺寸不可失败。32 取自"实测 17 / 无 leader 66"之间；若将来
   `try2` 上限、散布区范围或 `squad_size` 变大，实测最大值可能上移。`[ SQUADFALLBACK ]`
   行会在变红前先把漂移打出来，但这条界确实需要随那些参数一起复核。
2. **`maxPartnerLeaderDistance` 是产品代码里的测量计数**。它与既有 `SquadRollCounters` 同源
   （同样被 verbose 诊断与测量读取），不改变任何放置行为；但它确实是为了让规格保证可观测而加的，
   若评审认为产品侧不应承载该测量，替代方案只有"让 unleashed 随从也留下 leader 反向指针"，
   那会改变 §4.3.4 的语义，代价更大。
3. F1 拆出的具名用例与 `SquadFormsAroundACoreLeader` 的内联 AI 断言并存（重复一次断言），
   保留是为了后者失败时也能报出 AI 违规；如认为冗余可删内联那条。
