# Task 5 报告：阶段 B 收尾

状态：**完成**

## 1. 冻结树全量验证（自己跑，非引用他人结果）

```
python3 tools/run_tests.py --json /tmp/ci.json
```
结果（`/tmp/ci.json`）：
- `build.ok = true`
- `ctest.passed = 764, failed = 0, skipped = 3, total = 764, passed_pct = 100, returncode = 0`
- `drift.drift_ok = true, passes = 5`（A/B/C/C2/E 全过；merge-base `e00b7260f`，check E allowlist 10 个上游符号）

`python3 -m tools.eval.backend --smoke`：**PASS**（36/36，pass_rate 1.0；`level-rosters` 不在 smoke 集合内，`_smoke.yaml` 只收录 MPQ 无关的快速用例，符合其设计）。

`python3 -m tools.eval.backend --run level-rosters`（本 case 需 `retail_or_hf_required`，故单独跑；`--smoke` 集合本身按设计排除它）：**PASS**（50/50，实测耗时约 450-460s，落在 `timeout: 900` 内、余量约 47%——与计划 §7b 记录的"~475s"一致，已在 yaml 里补充 RB22 记账注释）。

## 2. 附录 D 两条可证伪预测

### 预测 3："能看到一只核心怪 + 1-2 只贴身同行的小队"

**覆盖**。覆盖用例与真实输出（均在 `test/level_roster_baseline_test.cpp`，`level_roster_baseline_test` 二进制，本轮实跑）：

- `SquadPlacementTest.SquadFormsAroundACoreLeader`、`SquadPlacementTest.SquadMinionsKeepOwnAi`：PASS（真实建关放置链路，`squad_chance=100`，断言"至少 1 个 leader 带 leashed 随从"、随从保留自身 AI、随从与 leader 不同类、随从在 4 格 leash 内、leader 必为该层 core 成员）。
- `SquadPlacementTest.SquadRateIsMeasuredPerLevel`（`squad_chance=100`，L1-15 各若干千次 roll）实测输出：
  ```
  [ SQUADRATE ] level 1 rolls 1330 realised 1329 rate 0.999248 minions 2641 per-squad 1.98721 lost(leader/partner) 0/0
  [ SQUADRATE ] level 8 rolls 1061 realised 1061 rate 1 minions 2111 per-squad 1.98963 lost(leader/partner) 0/0
  [ SQUADRATE ] level 15 rolls 1491 realised 1491 rate 1 minions 2973 per-squad 1.99396 lost(leader/partner) 0/0
  ```
  （per-squad 恒 ≈1.99，即"1 leader + squad_size(=2) 随从"中几乎总能放下 2 只——正是"1-2 只贴身同行"的实测证据。）
- `SquadPlacementTest.ShippedSquadChanceRealisesSquadsOnEveryLevel`（**出厂表** `squad_chance` 值，含 L14=10）实测输出：
  ```
  [ SHIPPEDSQUAD ] level 1 eligible 2105 rolls 640 rollRate 0.304038 realised 640 realisation 1 formed 640 minions 1276 lost(leader/partner) 0/0
  [ SHIPPEDSQUAD ] level 14 eligible 1375 rolls 121 rollRate 0.088 realised 121 realisation 1 formed 121 minions 238 lost(leader/partner) 0/0
  [ SHIPPEDSQUAD ] level 15 eligible 1641 rolls 490 rollRate 0.298598 realised 490 realisation 1 formed 490 minions 973 lost(leader/partner) 0/0
  ```
  出厂配置下 15 层全部 `realisation` 在 0.996-1.0（除 L14 rollRate 本身低是 chance=10 的设计选择，一旦 roll 到就几乎必然成队）。
- `SquadPlacementTest.SquadFormationRate`（出厂表、500 seed/层，逐层 floor 0.95）实测输出（本轮实跑）：
  ```
  [ SQUADFORM ] level 1 leashed 1 rolls 6694 formed 6691 rate 0.999552 ... floor 0.95
  [ SQUADFORM ] level 10 leashed 1 rolls 2718 formed 2715 rate 0.998896 ... floor 0.95   <- 全层最差
  [ SQUADFORM ] level 15 leashed 1 rolls 4854 formed 4854 rate 1 ... floor 0.95
  ```
  全 15 层 rate 均 ≥0.9989，远高于 floor 0.95。
- `SquadPlacementTest.UnleashedFallbackPlacesNeighboursWithoutLeashing` 实测输出：
  ```
  [ SQUADFALLBACK ] unleashed partners 1251 max leader distance 12 bound 32
  ```
  即便按 §4.3.4 回退路径（`leashed=false`）放置，随从仍在 leader 12 格范围内初置（远小于 32 的邻近性上界），验证"贴身同行"在放置那一刻成立、不依赖后续 leash 逻辑。

结论：预测 3 由**放置时刻**（真实 CreateDungeon→GetLevelMTypes→InitMonsters 链路）的多条用例覆盖，逐层出厂配置下实测形成率 99.89%-100%，且随从与 leader 的放置距离受控（≤12，远小于回退边界 32）。玩家在游戏中看到"1 核心怪 + 1-2 只贴身同行"的概率与本用例实测的形成率一致。

### 预测 5："把 leader 引开 >4 格，leashed 随从会脱队、靠近后重新贴回"

**未覆盖，需要人工试玩验证。**

判断依据（逐条给出不可覆盖的具体理由）：

1. **归队/脱队逻辑本体（`GroupUnity`）是内部链接、不可从测试直接调用**。用
   `nm -C build/Source/CMakeFiles/libdevilutionx_monster.dir/monster.cpp.o` 实测确认：
   ```
   0000000000035b81 t devilution::(anonymous namespace)::GroupUnity(devilution::Monster&)
   000000000003524f t devilution::(anonymous namespace)::IsLineNotSolid(...)
   00000000000353ee t devilution::(anonymous namespace)::FollowTheLeader(devilution::Monster&)
   ```
   小写 `t` = 该符号仅在 `monster.cpp` 翻译单元内可见（匿名 namespace，`Source/monster.h` 未导出任何声明）。测试二进制与 `monster.cpp` 分属不同编译单元链接进同一可执行文件，但测试代码物理上无法 `#include` 或直接调用匿名 namespace 里的符号——这不是"没写测试"，是链接边界本身不允许。
2. **`GroupUnity` 的唯一调用点在 `ProcessMonsters()` 内部的一个 while 循环里**（`monster.cpp:4629`），且该循环还依赖：
   - `IsTileVisible`（渲染可见性判定，依赖相机/视口状态）；
   - `UpdateEnemy`（敌人锁定，依赖 `Players`/`MyPlayer` 完整状态与寻路）；
   - `AiPlanPath`/`AiProc[...]`（每种怪物的完整 AI 状态机，包含移动、攻击判定）；
   - `UpdateModeStance` 驱动的 `MonsterWalk`/`MonsterAttack` 等，这些函数改变 `monster.position.future` ——而 `GroupUnity` 恰恰读取 `leader.position.future` 来判定"是否被拉开 >4 格"。
   要让 `leader.position.future` 真的移动 >4 格并让 `GroupUnity` 观察到分离/归队，必须让 `ProcessMonsters()` 完整跑若干 tick，这意味着要跑通整层怪物的 AI/寻路/动画状态机，而不是像 Task 1 的 `LeaderDeathReleasesMinions` 那样"只推进死亡动画到最后一帧再手动调用一次公开的 `M_UpdateRelations`"——`M_UpdateRelations` 是被导出的公开函数（`Source/monster.h:642`），`GroupUnity` 不是，且它的调用点在整个游戏循环 tick 的内部，语义上不可单独抽出而不牵连整条 AI 管线。
3. `test/sampling_behavior_test.cpp` 里已有的先例（`RunEngineDeath`）也印证了这个边界：作者显式选择"只推进死亡动画到最后一帧再手调 `M_UpdateRelations`"而不是跑整个 `ProcessMonsters` tick，理由写在注释里——"Running a whole ProcessMonsters tick here would also run every AI on the level"。归队/脱队没有 `M_UpdateRelations` 这种可单独调用的导出钩子，`GroupUnity` 是唯一入口且不导出，所以同样的手法在这里不适用。
4. 全仓库搜索确认：无任何现有测试驱动 `ProcessMonsters()`/`GameLogic()` 或完整 AI tick 循环（`grep -rln "ProcessMonsters\|GameLogic(" test/*.cpp` 只匹配到 `sampling_behavior_test.cpp` 里的**注释**说明为何不这么做，无实际调用）。引入这类测试意味着要为怪物寻路、动画推进、玩家位置模拟搭建完整的无头游戏循环夹具，这是本任务范围之外的新增基础设施，且违反"不引入新功能"的约束。

**结论：预测 5 依赖真实的游戏 tick 循环（`ProcessMonsters` 内部状态机 + 玩家移动 + 怪物 AI 寻路），其判定逻辑函数无公开导出，无头测试无法在不新增基础设施的前提下覆盖。按任务要求如实标注：未覆盖，需要人工试玩验证——具体验证方法：进入含小队的层（如 L9-L12，`squad_chance` 表值下真实成队率 ≈27%-31%），找到一只贴身同行小队的核心怪，把随从引开 >4 格观察其 `leaderRelation` 是否转 Separated（脱队，随从停止跟随 leader 移动）、随后靠近至 <4 格观察是否转回 Leashed（重新贴回）。**

## 3. eval 用例数与 passed_min 同步

`level_roster_baseline_test --gtest_filter="LevelRosterBaselineTest.*:SquadPlacementTest.*" --gtest_list_tests` 实测列出 **10** 个用例（`LevelRosterBaselineTest` 3 个 + `SquadPlacementTest` 7 个），与 `eval/cases/rng/level-rosters.yaml` 现有 `passed_min: 10` 及 `output_contains` 里的 `"[  PASSED  ] 10 tests."` 一致，**未改动**。

## 4. RB22 记账（timeout 余量）

单跑 `level_roster_baseline_test` 全部 10 个用例耗时约 450-460s（本轮 eval 实测），`SquadFormationRate` 单个用例即 289.7s（本轮直接 gtest 实跑测得）。相对 `timeout: 900`，余量约 47%，与计划 §7b 记录的"约 475s"一致。已在 `eval/cases/rng/level-rosters.yaml` 的 `setup` 段追加 RB22 记账注释："若再加长用例须同步上调 timeout"。

## 5. 存档格式台账行

在 `docs/knowledge/decision_save_format_policy.md` 的台账表追加一行（见"2026-09-16（**已落地**，Phase B）"）：核心小队接入后，**普通怪**也会带非默认 `leaderRelation`/`packSize`（此前只有 unique 才会）——存档**内容模式**变化，`Source/loadsave.cpp` 的序列化布局/文件格式**未变**；按宪章决策 35 不做兼容，注明"未来若做兼容层需把本行纳入内容语义变化清单"。

## 6. 计划「实施记录」表

已在 `docs/superpowers/plans/2026-09-15-level-rosters-phase-b.md` 的「实施记录（执行者填写）」表填全：G1/G2 验收结果（真实用例名+实测耗时）、逐层小队形成率（15 层实测值，最差 L10 0.998896）、`squad_leashed` 回退层（**无**，`level_roster_params.tsv` 零改动）、夹具重生成（**未重生成**，`timedemo` 早已隔离）、CI run（占位指向本报告，push 后补 run id + conclusion）。

## 7. 提交与 CI

提交（分块）：
- `3a404677b` `docs(plans): record the phase B implementation results`
- `a5475fdb1` `docs(knowledge): note the squad-related save-content change`
- `dfbef1f1b` `test(eval): record the RB22 timeout margin for level-rosters`

推送：`git push myrepo feature/qol-upgrades` → `bb24de4d2..dfbef1f1b`

CI（`gh run list --repo pengyz/devilutionX --branch feature/qol-upgrades`）：run id **35166422076**（`Better D1 CI`，触发于本次 push），`gh run watch --exit-status` 跟踪到终态：
- `build-and-test`：✓ success（5m28s，含 Build / 全量测试套件 / Drift check / Upload test report）
- `eval-nightly` / `eval-smoke`：非本次 push 的必经门（workflow_dispatch 可选项），不阻塞本次 push 的判定
- **最终 `conclusion: success`**（`gh run view` JSON 确认：`{"conclusion":"success","status":"completed",...}`）

## 待完成清单
- [x] 冻结树全量验证（run_tests.py：764/0/100%，drift 5/5）
- [x] eval smoke（36/36 PASS）+ level-rosters 单跑（PASS，耗时 ~450-460s，余量 47%）
- [x] 附录 D 预测 3/5 覆盖判定（预测 3 覆盖；预测 5 未覆盖，需人工试玩）
- [x] 存档格式台账行
- [x] 计划「实施记录」表填全
- [x] eval 用例数与 passed_min 同步（无需改动，10=10）+ RB22 timeout 记账
- [x] 提交 + push + CI 跟踪到终态（run 35166422076，conclusion success）

## 顾虑

无阻塞性顾虑。唯一需要人工跟进的是附录 D 预测 5（leash 脱队/归队）：本任务已如实标注为"未覆盖，需要人工试玩验证"，具体验证方法见上方第 2 节。若未来要补这条无头覆盖，需要新增一套驱动完整 `ProcessMonsters` tick（含 AI/寻路/动画）的测试基础设施，这超出本任务（不引入新功能/新基础设施）的范围，留给后续任务视需要决定是否投入。

**状态：完成。**
