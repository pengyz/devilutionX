# Task 3 报告（采样接入：core 预加 + 尾池上界 + 配额补足 + 逐层预算）

> 增量维护中。每完成一个可提交单元即在此追加。

## 状态

**完成**（见文末「接手完成（第二位实现者）」）。前一位实现者在跑全量门禁时被中断，由第二位接手补 R28 裁决、核对 R3/R4 处置、跑全量门禁并分块提交。

## 前置勘查结论（写代码前的事实核对）

- `GetLevelMTypes()` 位于 `Source/monster.cpp:3439`；`currlevel == 16` 分支在 `:3441-3446` 提前 return（本任务不动）。
- 既有预加顺序：`MT_GOLEM` → L18/19/20/24 的 HF 专属 → `!setlevel` 下的任务 unique → SKING 层骷髅 → `typelist` 构建（`:3491`）→ `classCounts` 初始化（`:3509-3512`）→ 采样循环（`:3516`）。
  因此 **core 预加插入点 = SKING 骷髅之后、`typelist` 构建之前**：这样 core 既进入 `LevelMonsterTypes`（被 `classCounts` 初始化统计到，caps 不会低估），又能在 `typelist` 构建时被 `GetMonsterTypeIndex` 判定为已加入而从尾池排除。
- `IsMonsterAvailable(const MonsterData &)` 在 `:3161`，与 `GetLevelMTypes()` 同 TU（`Source/monster.cpp` 匿名段之前的普通函数），可直接调用；它同时校验 `availability` 与 `minDunLvl/maxDunLvl` 区间。
- `AddMonsterType()`（`:3296`）对已存在的类型只做 `placeFlags |= placeflag`，不会重复计入 `monstimgtot`，也不会增长 `LevelMonsterTypeCount` —— 所以「新增才计数」必须靠 `countBefore != LevelMonsterTypeCount` 判定（既有循环已是这个写法）。
- L17-24 在基础 `monstdat.tsv` 下候选池为 0（那批怪 `availability=Never`），HF overlay 才有 5-6 个候选；两张逐层表当前只覆盖 L1-16，故 L17-24 走 `params == nullptr` 分支：预算退回 4000、`tailDraw = 0`。**关键推论**：若 `tailDraw` 退回 0 且 L17-24 无 core 行，Nest/Crypt 层将只剩预加类型（HF 下 `MT_HORKSPWN` 等），散布池可能为空。见下方「顾虑」。

## 实现（`Source/monster.cpp` `GetLevelMTypes()`）

按简报 8 条与 R15/R26 裁决落地，插入点严格遵守「既有预加之后、`typelist` 构建之前」：

1. **core 预加**：遍历 `GetLevelRoster(currlevel)`，`role == Core` 且 `IsMonsterAvailable(MonstersData[entry.type])` 为真才 `AddMonsterType(entry.type, PLACE_SCATTER)`（R26）。core 不受 caps 约束（caps 只在循环内），但**计入 `monstimgtot`**（走 `AddMonsterType`）并被随后的 `classCounts` 初始化统计 → caps 不会被低估（简报陷阱 1）。
2. **尾池排除已加入类型**：`typelist` 构建时新增 `GetMonsterTypeIndex(i) != LevelMonsterTypeCount` 过滤（规格 §4.2.3），使有界尾池不会把抽取次数浪费在重复类型上。
3. **caps 改用单一真相源**：删除内联 `capKite`/`capSameClass`，改为 `BehaviorClassCapForLevel(currlevel, cls)`，`cap == 0` 表示无上限，仅在 `classCounts[cls] >= cap` 时淘汰（保持既有「超过才淘汰」语义，R15）。
4. **floors 补足**：抽取前扫描 `rosterParams->classFloors`，找出未达下限的类别并在 `typelist` 中定位其成员作为 `preferred`；`preferred < 0` 时才 `GenerateRnd(nt)`。floors 只作用于「未满足」的类别，且仍在 caps 淘汰之后执行 → 不可能借此提高远程占比（规格 §4.5）。
5. **逐层预算**：`monstimgtot < maxImage`，且池内 image 过滤同步改为 `maxImage - monstimgtot`；`params == nullptr` 时 `maxImage = 4000`、`tailDraw = 0`。
6. **尾池计数**：`tailAdded` 仅在 `LevelMonsterTypeCount != countBefore`（真正新增）时自增，镜像既有 `countBefore` 写法（简报陷阱 4）。
7. **L16 不动**：`currlevel == 16` 分支与提前 return 原样保留，core 预加位于其后，故 L16 的名册行只是登记（规格 §4.2.7）。

## 用例（`test/sampling_behavior_test.cpp`，+5）

- `RosterCoreAlwaysPresent`：L1-16 × 200 seeds，每个 core 100% 出现；并断言每个 core 都是该层候选（可用性=区间检查，即 AC1 的「均为该层候选」）。L16 因硬编码提前 return，只断言候选性不断言出场。
- `RosterTailDrawBounded`：尾池数 = `LevelMonsterTypeCount - (1 Golem + 可用 core 数)`，断言 `<= tailDraw`。
- `RosterQuotasSatisfied`：L1-15 × 200 seeds，floors 全部满足；且每个类别不超过 `max(BehaviorClassCapForLevel(...), 豁免数)`——豁免数 = Golem + 该层可用 core（它们按设计绕过 caps）。
- `IdentityGuard`：相邻层 200 seeds 并集 Jaccard < 0.9（复用 P0-D 度量）。
- `A1A3VariantsAreCore`：`MT_RSKELAX`@3、`MT_XSKELAX`@3、`MT_BMAGMA`@9、`MT_STORML`@11 在 core 中，**并额外断言引擎真的实现了它们**（避免只查表的自证）。

先跑（改生产代码前）：`RosterCoreAlwaysPresent`/`RosterTailDrawBounded`/`RosterQuotasSatisfied`/`A1A3VariantsAreCore` 4 条 FAIL，`IdentityGuard` PASS（它是守卫，改动前本就成立，非本次改动的目标）。改完后 5 条全绿。

## 既有期望的失配处理（裁决 R3/R4：调名册/floors，不放宽阈值）

采样顺序与类型数变化后，同二进制内 3 条既有用例失配。逐条定性：

| 用例 | 现象 | 定性 | 处置 |
|---|---|---|---|
| `CavesKiteTailBaseline`（L11 kite tail 0% → 100%） | L11 名册的 4 个 core 里 `MT_STORML`/`MT_BMAGMA`/`MT_BACID` **3 个都是 RangedKite** | **名册数据缺陷**：core 绕过 caps，3 个 kite core 等于把 B1「洞穴不得全 kite」契约在数据层直接推翻 | 改名册：`11 MT_BACID` → `11 MT_OBLORD`（Rhino/Melee，L10-12 可用，非同层 unique base）。L11 kite core 降到 2，等于 cap 上限 |
| `QuestPreAddRePickDoesNotDoubleCount`（L14 same-class 3 > 2） | L14 core 含 `MT_VTEXLRD`+`MT_BALROG` 两个 Melee，叠加 Q_VEIL 预加的 `MT_RBLACK`（Melee）= 3 | **名册数据缺陷**：该层 cap 为 2/类，而 core+任务预加已到 3，运行期必然破 cap | 改名册：`14 MT_BALROG` → `14 MT_SNOWWICH`（Succubus/RangedTurret，L13-15 可用，L14 非 unique base）。L14 Melee core 降到 1，留出任务预加的空间 |
| `CavesAnyClassTailBaseline`（L9 1.7%→59.93%，L10-12 →100%） | 「任意类别 ≥3 种」的占比 | **非 cap 契约，且新值是算术必然**：每层现在实现 1 Golem + 4 core + ≤3 尾池 = 8 种；而在「每类 ≤2」前提下，L9/10/11/12 的候选池按类别最多只能容纳 8/7/6/7 种 → L10-12 **不可能**让所有类别都 ≤2，100% 是构造使然，L9 恰在边界 | 按实测重新钉值（59.93 / 100 / 100 / 100）并在注释里写明算术推导。**未放宽任何 B1 保证**：kite ≤2 由 `CavesKiteTailBaseline` 断言，caps 由 `RosterQuotasSatisfied` 断言 |

两处名册改动都重新过了任务 2 的加载期校验（`level_roster_test` 27 用例全绿，含 floors 可满足性与 unique base 白名单检查）。

`13 MT_GUARD` 曾一并试改为 `MT_COUNSLR`，但没有任何失败用例驱动它 → 已还原，保持数据改动最小。

## 夹具：`level_roster_baseline_test` 缺名册加载（真实缺陷，非阈值问题）

改动后 `LevelRosterBaselineTest` 2 条用例 placed 数**全零**。根因不是阈值：该夹具只调 `LoadMonsterData()`，没有跟上生产侧紧随其后的 `LoadLevelRoster()`；于是 `GetLevelRoster()` 全空、`GetLevelRosterParams()` 全 nullptr → `tailDraw` 退回 0 且无 core → **该层一个 `PLACE_SCATTER` 类型都没有** → `InitMonsters()` 的散布循环（`numscattypes > 0` 才进入）整段跳过。修复 = 在夹具里补 `LoadLevelRoster()`，复刻 `diablo.cpp:2812-2813` 的真实顺序。`sampling_behavior_test` 同样补齐。

**这条连带暴露一个生产风险，见「顾虑」。**

---

## 接手完成（第二位实现者）

前一位的工作区成果全部保留，未回退任何一项。本节记录接手后的增量。

### 状态

**完成。** 全量门禁绿，3 个提交已落地，工作区干净。

### 提交列表

| SHA | 提交 |
|---|---|
| `5f74250d1` | `test(roster): load the roster in the placement baseline fixture` |
| `c1e5d357c` | `feat(roster): sample each level from its roster (core + bounded tail + class floors)` |
| `c62a07e0f` | `test(roster): prove levels without roster params keep legacy sampling (R28)` |

切分理由：夹具缺陷修复（`LoadLevelRoster()`）是前置且自身成立，单独一个提交；采样改造 + 两处名册数据修正 + `CavesAnyClassTailBaseline` 重钉值是同一个行为单元，不可拆（拆开任一半都会留下红用例）；R28 的证据用例独立成第三个提交。三个提交各自都通过了 `sampling_behavior_test` / `level_roster_baseline_test` / `level_roster_test` 的构建与运行（逐个验证，非仅末态）。

**R28 的代码在提交 2 里就是正确值**，而不是先提交 `tailDraw = 0` 再由提交 3 修掉——否则提交 2 自身会让 L17-24 散布怪物消失，违反「每个提交自身可编译、测试可过」。提交 3 只补证据。

### R28 的实现与证据

实现（`Source/monster.cpp:3522`）：

```cpp
const int tailDraw = rosterParams != nullptr ? rosterParams->tailDraw : std::numeric_limits<int>::max();
```

`<limits>` 已在既有 include 列表中（`:19`），无需新增。`maxImage` 的 4000 fallback 未动。注释已改写为说明「退回旧行为」而非原先的「no bounded tail」。

证据：新增 `HellfireNoParamsSamplingTest` 套件（2 条用例）。**该路径在测试环境里可达**——基础 `monstdat.tsv` 下 L17-24 全部 `availability=Never`（候选池为 0，此时「采样不出东西」是正确行为而非回归），因此用例按简报建议走 `TestInitGame(fullQuests=true, originalCathedral=true, hellfire=true)` 挂载 `hf` overlay，L17-24 各有 5-7 个候选。套件独立于 `SamplingBaselineTest`，并在 `TearDownTestSuite()` 里卸载 overlay、重载 Diablo 数据，避免污染既有基线用例（那些基线是按 Diablo 数据测得的）。

- `LevelsWithoutParamsStillSampleTypes`：L17-24 × 20 seeds，断言每层至少 1 个 `PLACE_SCATTER` 类型（这正是 `InitMonsters()` 统计的 `numscattypes`）。
- `NoParamsTailExceedsTheParameterisedCap`：断言某个无参数行的层实现的散布类型数**超过 L1-16 表里最大的 `tail_draw`**——只有「预算受限的旧行为」能做到，任何小的 `tail_draw` 值都不行。这条把「不设上限」量化，避免只断言「> 0」这种弱形式。

**两条都实测验证过能捕获缺陷**：把 fallback 临时改回 `0` 重新编译后，
- 第一条失败：`level 23 seed 19 has no PLACE_SCATTER type: InitMonsters() would place no scattered monsters`；`LevelMonsterTypeCount` 为 1（仅 MT_GOLEM）；
- 第二条失败：`actual: 1 vs 3`。

改回 `std::numeric_limits<int>::max()` 后两条转绿。即：非同义反复、非自证。

每条用例都先守卫自己的前提再断言（overlay 已挂载 / 该层确实无 params 行 / 该层确实无 core 行 / 候选池非空），所以阶段 A2 给 L17-24 补表之后这些用例会**显式失败并提示迁移**，而不会静默变成空断言。

### 核对前一位的 R3/R4 处置

逐项复核了「既有期望的失配处理」一节，改动确实在工作区且成立：

- **L11 `MT_BACID` → `MT_OBLORD`**：核对 `monstdat.tsv`——`MT_BACID` = Acid = RangedKite；`MT_OBLORD` = Rhino = Melee，`minDunLvl-maxDunLvl` = 10-12（覆盖 L11）。改前 L11 四个 core 里 STORML(Storm)/BMAGMA(Magma)/BACID(Acid) 三个都是 RangedKite，core 绕过 caps ⇒ 数据层直接推翻 B1「洞穴不得全 kite」。定性为名册数据缺陷、改数据，成立。
- **L14 `MT_BALROG` → `MT_SNOWWICH`**：`MT_VTEXLRD` = Mega = Melee、`MT_BALROG` = Mega = Melee、`MT_RBLACK`（Q_VEIL 预加）= SkeletonMelee = Melee ⇒ 3 个 Melee > cap 2，运行期必破。`MT_SNOWWICH` = Succubus = RangedTurret，13-15 可用。成立。
- **`CavesAnyClassTailBaseline` 重新钉值**：独立复算了「算术必然」这个论证（按 `monstdat.tsv` 枚举 L9-12 候选池、按 `GetBehaviorClass` 归类、算每类 ≤2 时能容纳的类型数）：L9=8 / L10=7 / L11=6 / **L12=6**，而每层实现 1 Golem + 4 core + 3 tail = 8 类型。结论成立（L10-12 不可能全类 ≤2 ⇒ 100%；L9 恰在边界）。但前一位注释里写的是 `8/7/6/7`，**L12 的 7 是错的（实为 6）**，已修正注释；结论与钉值不受影响。同时把「with RangedKite capped at 2」改为「with every class held at <= 2」——原措辞与该指标（任意类别）不符。
- **未放宽任何 B1 保证**：确认 `HellL13/14/15SameClassTailBaseline` 仍是 `EXPECT_EQ(tail, 0.0)`、`CavesKiteTailBaseline` 四层仍是 `EXPECT_EQ(..., 0.0)`，阈值一个都没动；`BehaviorClassCapForLevel` 是唯一 caps 真相源，`RosterQuotasSatisfied` 断言运行期不破 caps。

### 全量门禁真实输出

```
python3 tools/run_tests.py --json /tmp/ci.json
```

```json
{"passed": 738, "failed": 0, "skipped": 3, "not_run": 0, "total": 738,
 "failures": [], "passed_pct": 100, "returncode": 0}
{"drift_ok": true, "passes": 5}
PASS A  Tests.cmake entries have source files
PASS B  no placeholder assertions in tests
PASS C  modified files keep line endings
PASS C2 added files match .editorconfig
PASS E  no test-only production functions
```

`failed == 0`、`passed_pct == 100`、`drift_ok == true`，达标。

**没有重新生成任何夹具。** 采样顺序确实变了，但没有产生夹具失配需要重生成：`timedemo_test` 的 `Timedemo.WarriorLevel1to2` 是 3 条 skip 之一，且其 skip 原因是既有的隔离决策（`quarantined: upstream-recorded demo desyncs under the Dark Expedition drop filter`，见 `docs/knowledge/gotcha_timedemo_isOnActiveLevel_failure.md`），与本任务无关——不是本次改动导致的失配，故未触发重生成流程。

`sampling_behavior_test` 24 条全绿（既有 17 + 前一位新增 5 + 本次 R28 新增 2）；`level_roster_test` 27 条全绿（含名册改动后的加载期校验）；`level_roster_baseline_test` 2 条全绿。

### 剩余顾虑

1. **L17-24 目前只有旧行为，没有名册身份。** R28 只保证「不回归」，Nest/Crypt 仍是纯随机采样，拿不到 core 必然出场与配额。这是阶段 A2 的既定范围（HF overlay 表），不是缺陷，但在 A2 落地前这 8 层与规格 §4.2 的承诺不一致。
2. **`HellfireNoParamsSamplingTest` 依赖套件执行顺序的健壮性。** 它在 `TearDownTestSuite()` 里恢复 Diablo 数据。同二进制内 gtest 默认按声明顺序跑、该套件在最后，当前无碍；但如果将来加 `--gtest_shuffle` 到 CI，需要确认恢复逻辑足够（已实测该套件单独跑与整体跑都绿，未实测 shuffle）。
3. **两处名册数据修正是被测试驱动的最小改动，但没有回到设计意图层面复核。** `MT_OBLORD`/`MT_SNOWWICH` 满足可用性与 caps，也过了任务 2 的加载期校验，但「这两个怪是否是该层想要的身份代表」属于名册设计判断，建议在阶段 B 核心小队排布时一并回看。
4. **本任务未产 eval 用例**（按简报，任务 4 负责）。采样行为已变更，`docs/knowledge/` 的「行为变更必产 eval」在任务 4 完成前处于未闭合状态。

---

## 修复轮次 1/5：评审意见处理

评审判定「需修复」，1 严重 + 2 重要。全部处理完毕，无遗留。

### S1【严重｜R29】caps 断言自证 —— 已修复

问题成立：`RosterQuotasSatisfied` 原先用 `CapExemptClassCount(level, cls)` 抬高预算，而该豁免数是**从被测 roster 自身推导**的，所以一个列了 3 个同类 core 的表会自己抬高自己的上限，断言永远无法失败。

三项要求全部落地：

1. **加载期核查**（`Source/tables/level_roster.cpp`）：在 core 非空循环之后、`gbIsSpawn` 提前返回**之前**新增按 level × `BehaviorClass` 的核查。当 `BehaviorClassCapForLevel(level, cls) != 0` 时要求 core 计数 ≤ cap，否则返回错误，文案含 level / class / core 计数 / cap 四项：

   ```
   level 10 core roster has 3 monsters of behaviour class RangedKite but the B1
   sampling cap for that level and class is 2; core bypasses the cap, so this
   breaks the guarantee in the data
   ```

   放在 spawn 分支之前，因为 B1 保证是**出厂表**的性质，与当前生效的可用性规则无关。

2. **验收断言改为引擎推导**（`test/sampling_behavior_test.cpp`）：删除 `CapExemptClassCount`，改为 `EnginePreAddClassCount(level, cls)` —— 只统计 Golem 与 `UniqueMonstersData` 中 `mlevel == level` 的同层任务 unique，经 `GetBehaviorClass(mtype)` 归类。断言变为 `EXPECT_LE(counts[i], cap + enginePreAdds)`，豁免数与被测 roster 完全无关。

3. **注释**已写明豁免只来自引擎无条件 pre-add，并引用 R29 说明为何不能来自 roster 自身。

**负向测试证据**（`test/level_roster_test.cpp`，新增 2 例）：

- `ValidationRejectsThreeSameClassCoreMembersUnderTheB1CapAtL10`：L10 列 3 个 RangedKite core（`MT_BMAGMA`/`MT_WMAGMA`/`MT_RSTORM`）必须被拒；并断言降到恰好 2 个时**必须通过**，证明拒绝来自「计数超 cap」而非这几个类型本身不合法。
- `ValidationRejectsThreeSameClassCoreMembersInSpawnModeToo`：L14 三个 Melee core 在 spawn 模式下同样被拒。

**非自证证明**：把校验条件临时改成 `if (false)` 重编译后，两例双双失败（`Value of: error.has_value() / Actual: false`）；恢复后 `level_roster_test` 29/29 通过。即断言确实由新校验驱动。

### I1【重要｜R30】Quests 全局态跨 suite 泄漏 —— 已修复

问题成立：`HellfireNoParamsSamplingTest` 经 `TestInitGame(hellfire=true)` 调用 `InitQuests()`，把 `Quests[*]._qactive` 从零初始化的 `QUEST_NOTAVAIL` 推到 `QUEST_INIT`，teardown 只还原了 mod 数据。

修复采用**保存-还原**（不臆造初值）：`SetUpTestSuite` 快照 `Quests`（`std::vector<Quest>`）、`Players.size()`、`Players[0].pOriginalCathedral`、`sgGameInitInfo`、`gbIsMultiplayer`、`gbIsHellfire`、`gbIsSpawn`，`TearDownTestSuite` 逐项还原。`Players` 无法按值快照（`Player::operator=(const Player&)` 已删除），故只快照规模与被改写的字段。

**泄漏确实存在的证据**（先删掉 `Quests` 还原行、保留其余，构造 Hellfire suite 先跑的顺序）：

```
Repeating all tests (iteration 2) . . .
[ RUN      ] SamplingBaselineTest.HellL13SameClassTailBaseline
test/sampling_behavior_test.cpp:170: Failure
Expected equality of these values:
  tail
    Which is: 100
  0.0
    Which is: 0
Hell L13 same-class tail must be 0 after cap (was 0%)
[  FAILED  ] SamplingBaselineTest.HellL13SameClassTailBaseline (228 ms)
```

正是评审预判的路径：Q_WARLORD@L13 的 `MT_BTBLACK` 是 Melee，叠加 L13 既有 2 个 Melee → 同类尾巴 100%。恢复还原行后同一顺序 3 次迭代全绿。

**实跑验证**（最终状态）：

```
Repeating all tests (iteration 1) . . .
Note: Randomizing tests' orders with a seed of 54396 .
[==========] 24 tests from 2 test suites ran. (4295 ms total)
[  PASSED  ] 24 tests.
Repeating all tests (iteration 2) . . .
Note: Randomizing tests' orders with a seed of 54397 .
[==========] 24 tests from 2 test suites ran. (4326 ms total)
[  PASSED  ] 24 tests.
Repeating all tests (iteration 3) . . .
Note: Randomizing tests' orders with a seed of 54398 .
[==========] 24 tests from 2 test suites ran. (4309 ms total)
[  PASSED  ] 24 tests.
```

### I2【重要｜R31】L16 语义未标注 —— 已修复

1. **1-15 vs 1-16 循环范围注释**：三处都加了 `Range note (R31)`。`RosterCoreAlwaysPresent` 跑 1-16，但说明 L16 只能验证 AC1 的静态一半（行合法、是该层候选、已注册），realised-core 断言对 L16 跳过，因为 `GetLevelMTypes()` 走 L16 硬编码分支并在 roster pre-add 之前返回。`RosterTailDrawBounded` 与 `RosterQuotasSatisfied` 跑 1-15，因为尾巴/floors/caps 都是 roster 采样循环的性质，而 L16 从不进入该循环；L16 的 cap 保证是那份硬编码清单的性质，由 `Level16HardcodedTypes` 覆盖。

2. **删除不可达的 L16 params 行**：`level_roster_params.tsv` 中 `16\t18000\t3\tMelee=2,RangedTurret=2` 已删除，该表现在只覆盖 L1-15。L16 **roster** 行保留并在 `Source/tables/level_roster.h` 的 `GetLevelRoster`/`GetLevelRosterParams` 注释块中标注为 registration-only（spec §4.2.7, R31），同时说明为何标注写在头文件而非 TSV（`Source/data/` 的 TSV 解析器无注释语法，已核实）。

3. 加载与全部测试仍绿，见下。

### 修复轮次门禁

- 定向：`cmake --build build --target sampling_behavior_test -j 20` → 成功；`sampling_behavior_test` 24/24、`level_roster_test` 29/29 通过。
- 全量：`python3 tools/run_tests.py --json /tmp/ci.json` → `passed: 740`、`failed: 0`、`passed_pct: 100`、`skipped: 3`、`drift_ok: true`（A/B/C/C2/E 五项 PASS），退出码 0。
- 乱序：`./build/sampling_behavior_test --gtest_shuffle --gtest_repeat=3` → 三轮各 24/24 通过（种子 54396/54397/54398），输出见上。

### 未处理项（按评审要求）

5 条 minor 全部按指示不动，包括 `EXPECT_NEAR(l9, 59.93, 0.6)`。评估后判断放宽容差**并非零风险**：该数值是 L9 在「每类 ≤2」下容量恰好处于边界（8 槽 / 8 容量）的测量结果，容差直接决定这条边界断言的判别力，放宽会削弱它。故不在本轮扩散 diff。
