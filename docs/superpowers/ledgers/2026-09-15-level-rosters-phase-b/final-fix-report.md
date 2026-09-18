# 阶段 B 最终修复轮报告（S1/S2/I1/I2/M1）

开始时间: 2026-09-17T08:59:08+08:00
分支: feature/qol-upgrades
起始 HEAD: 3dc64382019fcd68373efd9fd5b4400b8e2338fa

状态: 进行中（边做边追加）

## 修复清单

### S1【严重】`ReleaseMinions` 漏掉 `LeaderRelation::Separated`（RB26）

`Source/monster.cpp` `ReleaseMinions`：
- 过滤从 `leaderRelation == Leashed` 放宽为 `leaderRelation != None`（同时覆盖 `Leashed` 与 `Separated`）；
- 清索引仍只在 `clearReference == true`（即 `!isUnique()`，普通怪路径）；unique 路径逐字节不变（relation 置 `None`、索引保留）；
- `ShrinkLeaderPacksize` 未改动（`Separated` 在分离那一刻已减过 `packSize`）；
- 注释重写：写明三态各自处理、为何 unique 必须保留索引、以及 `Separated` 这条正常玩法路径如何造成悬挂索引。

新增用例（`test/sampling_behavior_test.cpp`）：
- `LeaderDeathReleasesSeparatedMinions`（普通 leader + `Separated` → relation `None` 且索引清）
- `UniqueLeaderDeathKeepsSeparatedMinionIndex`（unique leader + `Separated` → relation `None`、索引保留）
- 既有 `LeaderDeathReleasesMinions` / `UniqueLeaderDeathBehaviourUnchanged` 覆盖两条 `Leashed` 用例。
- 辅助 `SeparateMinionFromLeader()` 复刻 `GroupUnity` 的分离分支（减 `packSize`、置 `Separated`、保留索引），死亡走既有公共路径 `RunEngineDeath`。

### S2【严重】小队随从被染蓝名（RB27）

- 新增谓词 `IsBuffedMinion(const Monster &)`（声明 `Source/monster.h`，实现 `Source/monster.cpp`）：`leader != nullptr && leader->isUnique()`。
- `Source/qol/monhealthbar.cpp:149` 的 `leader != Monster::NoLeader` 改为 `IsBuffedMinion(monster)`。
- 谓词注释说明它为何不同于 `leader != NoLeader`，并明确「若将来出现非 unique 的强化路径必须重评本判据」。
- 新增用例 `BuffedMinionPredicateTracksToughening`：小队随从（`tough=false`）→ 非 buffed；unique 包随从（`tough=true`）→ buffed；两臂都断言 leader 索引已设置（证明旧判据在此确实分辨不出），并用同种子 A/B 实测 HP 恰好 2 倍来确立「真的被强化」。

### I1【重要】`minionsOutsideLeash` 恒真

- 新增具名常量 `kLeashedAxisBound = 4`（由钳制推导：`|xp-x1| < 4` → ≤3，`x1` 为 leader 的邻格 → 3+1=4）；
- 新增计数 `minionsAtLeashBound`（恰好落在界上的随从数），并在 `SquadFormsAroundACoreLeader` 断言其 **> 0** —— 这是让「不超过 4」成为真守卫的紧界证据；
- 新增 `[ SQUADLEASH ]` 无条件输出行。

### I2【重要】`squad_size` 无守卫

- `SquadFormsAroundACoreLeader` 新增弱但可失败的断言：每支已成形小队的平均随从数 ≥ `kMinMinionsPerSquad = 1.5`；
- 新增 `[ SQUADSIZE ]` 无条件输出行报告实测均值。

### M1【轻微】`squads_off` 夹具注释失准

采用「让夹具真的只差一列」这条更强的修法（而非只改注释）：
- `test/fixtures/txtdata/monsters/level_roster_params_squads_off.tsv` 的 `squad_size` 从 0 改回出厂值 **2**，现在相对出厂表**只差 `squad_chance`**（30 / L14 10 → 0）。这是合法输入：加载器只拒绝反向组合 `squad_chance > 0 && squad_size == 0`（`Source/tables/level_roster.cpp`），`squad_chance == 0 && squad_size == 2` 不触发任何校验。
- A/B 立论因此更强：小队数为 0 只能归因于 `squad_chance`；此前两列同改，读者无法判断是哪一列压掉了小队。
- `SquadsAreAbsentWhenTheTableDisablesThem` 的注释同步改准（明确"只差一列"并说明为何该输入合法）。
- 行尾保持 CRLF（已用 `file` 核实）。

## 夹具真实缺陷（顺带修掉，非评审项）

`PrepareDeathPathPrerequisites` 未加载 `SpellsData`。死亡路径的掉落可以掉书，`GetBookSpell`（`Source/items.cpp:630`）在 `SpellsData` 为空时：`GenerateRnd(0) + 1` 使 `rv == 1`，循环里没有任何法术能把它减下去，`if (s == SpellsData.size()) s = 1` 的回绕也永不命中，于是 `while (rv > 0)` 死循环直到 `s++` 溢出 —— 表现为**挂死**，UBSan 随后在 `items.cpp:648` 报有符号溢出。

首次运行 `UniqueLeaderDeathKeepsSeparatedMinionIndex`（seed 9500）就撞上了：

```
[ RUN      ] SamplingBaselineTest.UniqueLeaderDeathKeepsSeparatedMinionIndex
/home/peng/workspace/DevilutionX/Source/items.cpp:648:4: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
```

命中与否纯看掉落运气（既有 4 条用例的种子恰好没掉书），所以修法是**按 `diablo.cpp` 的顺序补上 `LoadSpellData()`**（在 `LoadItemData()` 之前），而不是换一个"安静"的种子绕开。

## 反证（每条都实跑：改坏 → 红 → 恢复 → 绿）

### 反证 1（S1-a）：过滤改回只 `Leashed`

改动：`ReleaseMinions` 的 `if (minion.leaderRelation == LeaderRelation::None) continue;` → `if (minion.leaderRelation != LeaderRelation::Leashed) continue;`

```
test/sampling_behavior_test.cpp:1762: Failure
Expected equality of these values:
  minion.leader
    Which is: '\0'
  Monster::NoLeader
    Which is: '\xFF' (255)
the SEPARATED minion's index must be cleared as well - this is the RB26 hole: filtering on Leashed alone leaves it pointing at a slot AddMonster can reuse
test/sampling_behavior_test.cpp:1765: Failure
Expected equality of these values:
  minion.getLeader()
    Which is: 0x64e6b60fc7c0
  nullptr
    Which is: (nullptr)
getLeader() must stop resolving to the dead leader's slot
[  FAILED  ] SamplingBaselineTest.LeaderDeathReleasesSeparatedMinions (12 ms)
[ RUN      ] SamplingBaselineTest.UniqueLeaderDeathKeepsSeparatedMinionIndex
test/sampling_behavior_test.cpp:1817: Failure
Expected equality of these values:
  minion.leaderRelation
    Which is: 1-byte object <02>
  LeaderRelation::None
    Which is: 1-byte object <00>
a unique leader's death must clear a separated minion's relation
[  FAILED  ] SamplingBaselineTest.UniqueLeaderDeathKeepsSeparatedMinionIndex (11 ms)
[ RUN      ] SamplingBaselineTest.LeaderDeathReleasesMinions
[       OK ] SamplingBaselineTest.LeaderDeathReleasesMinions (11 ms)
[ RUN      ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged
[       OK ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged (11 ms)
[  PASSED  ] 2 tests.
[  FAILED  ] 2 tests, listed below:
[  FAILED  ] SamplingBaselineTest.LeaderDeathReleasesSeparatedMinions
[  FAILED  ] SamplingBaselineTest.UniqueLeaderDeathKeepsSeparatedMinionIndex
```

判读：两条 `Separated` 用例判红（普通路径的索引残留 + unique 路径的 relation 未清），两条既有 `Leashed` 用例仍绿 —— 正是"漏点只在 `Separated`"。恢复后 4 条全绿。

### 反证 2（S1-b，unique 路径回归）：索引无条件清

改动：`if (clearReference) minion.leader = Monster::NoLeader;` → 去掉 `if`，无条件清。

```
    Which is: '\0'
the unique path must RETAIN the leader index in the Separated case too (IsBuffedMinion / monhealthbar colouring, spec 4.3.1)
[  FAILED  ] SamplingBaselineTest.UniqueLeaderDeathKeepsSeparatedMinionIndex (12 ms)
[ RUN      ] SamplingBaselineTest.LeaderDeathReleasesMinions
[       OK ] SamplingBaselineTest.LeaderDeathReleasesMinions (12 ms)
[ RUN      ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged
test/sampling_behavior_test.cpp:1924: Failure
Expected equality of these values:
  minion.leader
    Which is: '\xFF' (255)
  retainedIndex
    Which is: '\0'
the unique path must RETAIN the leader index (monhealthbar colouring)
[  FAILED  ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged (12 ms)
[  PASSED  ] 2 tests.
[  FAILED  ] 2 tests, listed below:
[  FAILED  ] SamplingBaselineTest.UniqueLeaderDeathKeepsSeparatedMinionIndex
[  FAILED  ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged
```

判读：unique 的 `Leashed` 与 `Separated` 两条回归同时判红，证明"unique 路径逐字节不变"是被守住的、而不是恰好成立。

### 反证 3（S2）：谓词改回 `leader != NoLeader`

改动：`IsBuffedMinion` 的实现 → `return minion.leader != Monster::NoLeader;`

```
Value of: squad.buffed
  Actual: true
Expected: false
a squad minion (MinionOptions::tough = false) is NOT strengthened, so it must not be reported as buffed - the blue name would contradict the squad feature's composition-only promise
[  FAILED  ] SamplingBaselineTest.BuffedMinionPredicateTracksToughening (0 ms)
[  PASSED  ] 0 tests.
[  FAILED  ] 1 test, listed below:
[  FAILED  ] SamplingBaselineTest.BuffedMinionPredicateTracksToughening
```

判读：小队那条（`tough=false` 却被判为 buffed）必红，unique 那条仍绿 —— 与评审要求一致。

### 反证 4a（I1，紧界本身）：把界放宽到数据之上

这是 I1 的**决定性**反证：评审的意见是"`dx > 4` 恒真、指不出什么改动会让它红"，所以真正要证的是**界必须是紧的**。把 `kLeashedAxisBound` 从 4 改成 6（即让界高于钳制真正能产生的距离）：

```
[ SQUADLEASH ] minions 1275 at bound 0 beyond bound 0 (bound 6)
no squad minion reached Chebyshev 6 from its leader over L9-12 x 10 seeds, so kLeashedAxisBound is above what the clamp produces and the minionsOutsideLeash assertions cannot fail on any placement
[ SQUADSIZE ] leaders 641 minions 1275 per-squad 1.98908 floor 1.5
[  FAILED  ] SquadPlacementTest.SquadFormsAroundACoreLeader (1311 ms)
```

判读：界一旦高于数据，`minionsAtLeashBound == 0` 立刻判红并直接说出原因（"这个界之上的断言不可能失败"）。这正是原版 `dx > 4` 缺的东西——原版只有"越界数为 0"，无法区分"界是紧的"和"界高得没意义"。

出厂配置下的紧界实测（修复后、未改动代码）：

```
[ SQUADLEASH ] minions 1275 at bound 24 beyond bound 0 (bound 4)
```

即 1275 只随从中有 24 只**恰好**在界上、0 只越界——界被真实触达，所以"不超过 4"是一条落在钳制边界上的守卫。

### 反证 4b（I1，越界侧）：放宽 PlaceGroup 的拴系钳制

改动：`std::abs(xp - x1) >= 4 || std::abs(yp - y1) >= 4` → `>= 8`。

```
level 11 seed 0: a leashed minion must sit within kLeashedAxisBound (4) tiles of its leader on each axis
level 11 seed 2: ...
level 12 seed 0: ...
[ SQUADLEASH ] minions 1294 at bound 35 beyond bound 0 (bound 4)
[ SQUADSIZE ] leaders 651 minions 1294 per-squad 1.98771 floor 1.5
[  FAILED  ] SquadPlacementTest.SquadFormsAroundACoreLeader (1327 ms)
```

判读：钳制被放宽后随从跑到 5 格以上，L11/L12 多个种子判红。注意这一条**旧判据 `dx > 4` 也能抓到**（两者阈值相同），所以它证明的是越界侧仍然有效，而"恒真"这一评审意见是由 4a 回应的。

### 反证 5（I2）：`PlaceGroup` 第 2 参改常量 1

改动：`PlaceGroup(partnerIndex, rosterParams->squadSize, &leader, ...)` → `PlaceGroup(partnerIndex, 1, &leader, ...)`

```
[ SQUADLEASH ] minions 824 at bound 7 beyond bound 0 (bound 4)
[ SQUADSIZE ] leaders 824 minions 824 per-squad 1 floor 1.5
[  FAILED  ] SquadPlacementTest.SquadFormsAroundACoreLeader (1325 ms)
[  FAILED  ] 1 test, listed below:
[  FAILED  ] SquadPlacementTest.SquadFormsAroundACoreLeader
```

判读：per-squad 从 **1.98908** 掉到**恰好 1**，远低于 floor 1.5，必红。系数 1.5 落在 1.0 与 1.99 之间的空隙里 —— 既能抓住"忽略 squad_size"，又不把合法数据调整变成红灯。

### 反证 6（M1）：`squads_off` 夹具的 `squad_chance` 改回 30

改动：该夹具 `squad_chance` 0 → 30（`squad_size` 保持 2，即只动这一列）。

```
squad_chance 0 must not produce any leashed squad under an ordinary leader on L9-12
[  FAILED  ] SquadPlacementTest.SquadsAreAbsentWhenTheTableDisablesThem (1306 ms)
[  FAILED  ] 1 test, listed below:
[  FAILED  ] SquadPlacementTest.SquadsAreAbsentWhenTheTableDisablesThem
```

判读：夹具现在**只差 `squad_chance` 一列**，且改动这一列就判红 —— A/B 的"只差一个变量"立论成立且可失败。

### 恢复后（全部反证撤销）

```
level_roster_baseline_test --gtest_filter=SquadPlacementTest.*  -> passed: 7, failed: 0
sampling_behavior_test（G1/G2/RB26/RB27 七条） -> passed: 7, failed: 0
```

---

## 门禁结果

### 定向测试

```
level_roster_baseline_test --gtest_filter=LevelRosterBaselineTest.*:SquadPlacementTest.*
  -> returncode 0, passed 10, skipped 0     （用例数不变，仅加断言与输出行）
sampling_behavior_test  --gtest_filter=SamplingBaselineTest.*
  -> passed 33（原 30，新增 RB26 两条 + RB27 一条）
```

### 全量门禁

```
python3 tools/run_tests.py --json /tmp/ci.json
  build.ok        = true
  ctest.passed    = 767
  ctest.failed    = 0
  ctest.skipped   = 3   （Timedemo.WarriorLevel1to2 + VisualStoreTest 两条，既有 Skipped）
  ctest.passed_pct= 100
  drift.drift_ok  = true   （PASS A / B / C / C2 / E 五项）
```

首轮全量门禁曾报 `SquadPlacementTest.SquadFormsAroundACoreLeader` 失败，输出为
`[ SQUADLEASH ] ... (bound 6)`：那是反证 4a 的变异二进制（构建产物 10:31:43 早于源码 mtime 10:32:06），
即撤销源码变异后未重新链接。重建后该用例转绿，再跑全量即 767/767。
教训：反证结束、恢复源码之后必须重新构建一次，再跑门禁，否则门禁读的是变异产物。

### eval

```
python3 -m tools.eval.backend --smoke                          -> exit 0（36 case 全过）
python3 -m tools.eval.backend --run level-rosters              -> PASS (60/60), exit 0
python3 -m tools.eval.backend --run sampling-anti-monopoly-cap -> PASS (15/15), exit 0
```

两条被改过的 YAML 都单独跑过：`sampling-anti-monopoly` 的 `passed_min: 33` /
`output_contains: "[  PASSED  ] 33 tests."` 与 `level-rosters` 新增的
`[ SQUADLEASH ]` / `[ SQUADSIZE ]` 输出断言均已由实跑证实。

### 改动文件清单（8 个）

```
Source/monster.cpp                                              S1 + S2
Source/monster.h                                                S2（IsBuffedMinion 声明与理由）
Source/qol/monhealthbar.cpp                                     S2（改用谓词）
test/sampling_behavior_test.cpp                                 S1/S2 用例 + LoadSpellData 夹具修复
test/level_roster_baseline_test.cpp                             I1 + I2 + M1 注释
test/fixtures/txtdata/monsters/level_roster_params_squads_off.tsv  M1（squad_size 复原为 2）
eval/cases/rng/sampling-anti-monopoly.yaml                      用例数 30 → 33
eval/cases/rng/level-rosters.yaml                               新增输出断言
```

`assets/txtdata/**` 未改动；`kRangedShareCeiling` / `kSquadFormationFloor` 未改动；
`docs/superpowers/specs/**` 未改动。C++ 与 TSV 保持 CRLF，YAML/MD 保持 LF（漂移校验 C/C2 已确认）。

---

## 顺手修掉的门禁缺陷（非评审项，但影响本轮证据可信度）

上面那次"陈旧二进制"不是单纯的操作失误，而是门禁本身的缺口：

- `tools/run_tests.py` 的构建阶段只构建 `TEST_TARGETS`（**手抄清单**），而 `ctest` 跑 `CMake/Tests.cmake` 注册的**全部**用例；
- `level_roster_test` 与 `level_roster_baseline_test` 两个目标从来不在 `TEST_TARGETS` 里；
- 因此全量门禁对这两个二进制**从不重建**，永远执行磁盘上现存的产物 —— 本轮红灯只是让这个缺口显形。

修法：把两个目标补进 `TEST_TARGETS`。校验（清单对齐 + 实跑重建）：

```
Tests.cmake 目标名 - TEST_TARGETS = []        （差集为空）
TEST_TARGETS - Tests.cmake 目标名 = ['text_render_integration_test']   （既有条件目标，注释已说明）

touch test/level_roster_baseline_test.cpp && python3 tools/run_tests.py --json /tmp/ci2.json
  构建日志出现：
    [3/5] Building CXX object CMakeFiles/level_roster_baseline_test.dir/test/level_roster_baseline_test.cpp.o
    [4/5] Linking CXX executable level_roster_test
    [5/5] Linking CXX executable level_roster_baseline_test
  结果：passed 767 / failed 0 / passed_pct 100 / drift_ok true
```

修前 `touch` 该源文件再跑全量门禁不会出现任何 `level_roster*` 的构建行 —— 这就是这条修复的可失败性证据。

## 知识沉淀

```
docs/knowledge/gotcha_run_tests_target_list_misses_new_binaries.md   新增测试二进制必须同步 TEST_TARGETS；产物 mtime 早于源码 = 陈旧产物指纹
docs/knowledge/gotcha_death_path_tests_need_loadspelldata.md         死亡/掉落夹具必须先 LoadSpellData；空表 → GetBookSpell 死循环 → items.cpp:648 溢出
docs/knowledge/MEMORY.md                                            两条索引条目
```

---

## 提交与 CI

分段提交（起点 `3dc64382019fcd68373efd9fd5b4400b8e2338fa`）：

```
a54666156  fix(monster): release separated minions and colour only truly buffed ones   S1 + S2 引擎改动
00ad9173f  test(monster): cover separated-minion release and the buffed-minion predicate   S1/S2 用例 + LoadSpellData 夹具修复 + YAML 用例数
a4d0eebd0  test(rosters): make the squad leash, size and A/B fixture guards falsifiable   I1 + I2 + M1
542afe208  fix(tools): build the level_roster test targets in the full gate   门禁缺陷 + 两条 knowledge
```

提交后工作树干净，再跑一次全量门禁：`passed 767 / failed 0 / skipped 3 / passed_pct 100 / drift_ok true`。

推送：`myrepo feature/qol-upgrades  dfbef1f1b..542afe208`（未直推 master）。

CI（R43，跟到终态）：

```
gh run watch 35178605691 --repo pengyz/devilutionX --exit-status
  ✓ feature/qol-upgrades  Better D1 CI  ·  35178605691
  ✓ build-and-test in 5m14s   （Build / Run full test suite / Drift check 全绿）
  - eval-nightly  skipped     （nightly 仅定时触发）
  - eval-smoke    skipped     （workflow_dispatch 可选项）
  WATCH_EXIT=0
```

`docs/superpowers/ledgers/**` 被 `docs/superpowers/ledgers/.gitignore` 忽略，本报告按既有约定留在本地，不入库。

## 结论

5 项评审意见（S1 / S2 / I1 / I2 / M1）在同一轮内全部修完，每项都有实跑的"破坏→红→恢复→绿"证据（反证 1、2、3、4a、4b、5、6）。
额外修掉一个让本轮证据一度失真的门禁缺陷，并沉淀两条 knowledge。
unique pack 行为逐字节未变；`assets/txtdata/**`、`kRangedShareCeiling`、`kSquadFormationFloor`、`docs/superpowers/specs/**` 均未触碰。
