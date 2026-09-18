# Task 4 报告：小队形成率的量化守卫与按层回退

BASE：`1a90a16ae docs(spec): bind the fallback adjacency bound to its three parameters (RB18)`
分支：`feature/qol-upgrades`

## 待办
- [x] 步骤 1：500 seed/层 实测形成率（出厂表）
- [x] 步骤 2：据实测导出阈值 + 决定按层 `squad_leashed` 回退（结论：无需回退）
- [x] 步骤 3：落地 `SquadFormationRate` 守卫（具名阈值数组 + 无条件打印）
- [x] 步骤 4：反证 ×2（改坏→红→恢复→绿）
- [x] 步骤 5：验收 8 / 9c 本轮证据
- [x] 步骤 6：门禁（run_tests + eval）+ eval 计数同步 + 增量提交 + CI 终态

## 约束确认（控制者裁决）
- RB17：L14 冻结——`squad_chance/squad_size/squad_leashed/class_floors/tail_draw` 一律不动。
- RB14：`class_floors` 不是自由杠杆（验收 8 ↔ 9c 互换）。本轮不动 `class_floors`。
- R4：不动 `kRangedShareCeiling` 或任何既有阈值。
- §4.3.4 回退路径已有可失败守卫（`UnleashedFallbackPlacesNeighboursWithoutLeashing`，Task 3 修复轮 F2），不重复实现、不删除。

## 进展

（边做边追加）

### 步骤 1 实测：逐层形成率（出厂表，500 seed/层，seed base 51000）

指标定义（brief 原文）：**尝试成队的 roll 中真正放下 ≥1 只随从的比例**。

**为什么必须新加一个计数器**：既有 `SquadRollCounters::realised` 的判定是
`rosterParams->squadLeashed && leader.packSize > 0`，而 `PlaceGroup` **只在 leashed 时**写
`packSize`。若用 `realised` 做形成率分子，任何走 §4.3.4 回退（`squad_leashed=0`）的层会
**恒读 0%** —— 守卫会因为"它自己要求的补救措施被采用"而变红。因此加
`SquadRollCounters::formed`（**与 leash 无关**：本 roll 放下 ≥1 只随从即计数，按 roll 而非按只计）。
分母沿用循环自己的 `rolls`（非由 `squad_chance` 反推），故 L14（chance=10）按其**实际** roll 数评判。

| 层 | rolls | formed | **形成率** | floor |
|---|---|---|---|---|
| 1 | 6694 | 6691 | **99.955%** | 0.95 |
| 2 | 5142 | 5140 | **99.961%** | 0.95 |
| 3 | 3999 | 3998 | **99.975%** | 0.95 |
| 4 | 3703 | 3702 | **99.973%** | 0.95 |
| 5 | 3199 | 3199 | **100.000%** | 0.95 |
| 6 | 3084 | 3084 | **100.000%** | 0.95 |
| 7 | 3277 | 3276 | **99.970%** | 0.95 |
| 8 | 3447 | 3446 | **99.971%** | 0.95 |
| 9 | 2660 | 2658 | **99.925%** | 0.95 |
| 10 | 2718 | 2715 | **99.890%**（最差层） | 0.95 |
| 11 | 2698 | 2697 | **99.963%** | 0.95 |
| 12 | 2587 | 2587 | **100.000%** | 0.95 |
| 13 | 3201 | 3200 | **99.969%** | 0.95 |
| 14 | 1364 | 1364 | **100.000%** | 0.95 |
| 15 | 4854 | 4854 | **100.000%** | 0.95 |

与 Task 3 的 50-seed 实测一致（99.6-100%）。最差层 L10 = 2718 roll 里丢 3 次。

### 步骤 2 处置：**各层保持 `squad_leashed=1`，无需回退**

计划步骤 2 的下限建议是 50%，实测最低 99.890%（L10），**高出下限 ~50pp**，
三条打折路径（`totalmonsters` 钳制 / `PlaceGroup` 10 次放弃 / 4 格 leash）合计吃掉 <0.11% 的 roll。
根因同 Task 3：`squad_size=2 < na(3-5)`，成队比它替代的普通组**更省槽位**，钳制更难触发。

→ **`assets/txtdata/monsters/level_roster_params.tsv` 的 `squad_leashed` 列一列未改**（15 层全 1）。
明确记录：**本轮无任何层需要 §4.3.4 回退**。回退路径本身仍由 Task 3 修复轮 F2 的
`UnleashedFallbackPlacesNeighboursWithoutLeashing` 守卫（未改动、未删除）。
RB17（L14 冻结）与 RB14（`class_floors` 非自由杠杆）因此均未被触碰：本轮**零数据改动**。

### 阈值 0.95 的导出与"非紧界"声明

阈值不是实测值本身，而是取在实测证明为空的间隙里：最差实测 0.99890 → floor 0.95
（约为最差损耗的 45 倍余量）。理由两条，都写进了用例注释：
1. **仍可失败**：出厂配置不会产生 0.95-0.998 之间的值；放置路径一坏就直接掉到 0.0（见下反证）。
2. **不把数据改动变成红灯**：`squad_size`/`tail_draw`/core 集是 Task 5 及以后的旋钮，
   某层从 0.9989 合法漂到 0.99 不该判红——验收 7 要求的是"小队能成形"，不是"恰好这么频繁"。
   计划自己建议的下限是 50%，0.95 远严于它，同时仍留出数据自由度。

**这是实测分隔线，不是紧界，也不是规格常量。** 逐层实测值**无条件打印** `[ SQUADFORM ]`，
使漂移在变红之前先可见。

### 步骤 4 反证：两次「改坏 → 必红 → 恢复 → 绿」实跑

#### 反证 1：放置路径改坏（`squad_size` → 0，仅 L10）
`Source/monster.cpp`：`PlaceGroup(partnerIndex, currlevel == 10 ? 0 : rosterParams->squadSize, ...)`
```
[ SQUADFORM ] level 9 leashed 1 rolls 2660 formed 2658 rate 0.999248 floor 0.95
[ SQUADFORM ] level 10 leashed 1 rolls 3071 formed 0 rate 0 floor 0.95
level 10 formed only 0 squads out of 3071 rolls (0), under its measured floor 0.94999999999999996;
  per spec 4.3.4 this level's squad_leashed goes to 0 and the measurement is recorded,
  rather than the floor being lowered
[ SQUADFORM ] level 11 leashed 1 rolls 2698 formed 2697 rate 0.999629 floor 0.95
[  FAILED  ] SquadPlacementTest.SquadFormationRate
```
→ **只有被改坏的那层变红**（0.0），相邻层数值分毫未动。这也证实了阈值选取的理由：
放置一坏就是**掉到 0**，不存在 0.95-0.998 之间的中间态。

#### 反证 2：跳过分支被滥用（表说 chance=30，但分支不再被触达）
`Source/monster.cpp`：`const bool squadEligible = currlevel != 11 && rosterParams != nullptr ...`
```
[ SQUADFORM ] level 10 leashed 1 rolls 2718 formed 2715 rate 0.998896 floor 0.95
level 11 has squad_chance 30 but made no squad roll in 500 seeds,
  so the squad branch is no longer reached on it
[ SQUADFORM ] level 11 squads disabled by the table (squad_chance 0)
[  FAILED  ] SquadPlacementTest.SquadFormationRate
```
→ 「rolls==0 就跳过该层」这条豁免**不是逃生口**：只有 `squad_chance==0`（表自己关掉）才允许跳过；
分支静默失联同样判红。

#### 恢复后（`git diff --stat Source/monster.cpp` = 仅 `formed` 计数器的 5 行）
```
[ SQUADFORM ] level 1 ... rate 0.999552 ... level 10 ... rate 0.998896 ... level 15 ... rate 1
[       OK ] SquadPlacementTest.SquadFormationRate (289006 ms)
```
逐层数值与步骤 1 实测**逐位一致** → 恢复完全、无残留。

### 验收 9c 本轮证据（上一轮 finding6）

`SamplingBaselineTest.HellUniqueBasesRemainReachable`，**本轮实跑**（非历史日志），
HEAD = BASE + 本任务改动，二进制当轮重新链接：
```
[ MEASURED ] level 13 unique bases reachable 2/5
[ MEASURED ] level 14 unique bases reachable 6/6
[ MEASURED ] level 15 unique bases reachable 2/2
[       OK ] SamplingBaselineTest.HellUniqueBasesRemainReachable (35 ms)
```
→ **L14 仍 6/6**。L13 的 2/5 是用例里 `kCapBlocked` 记录的既有 L13 Melee cap 后果
（3 个 base 被显式 `EXPECT_FALSE` 断言为不可达并写明理由），非本轮回归。

### 验收 8 本轮证据

`LevelRosterBaselineTest.PlacedClassMixWithinBaseline`，**本轮实跑**，L1-15 逐层：
```
[ MEASURED ] level 1  ranged share 0        (0/18585),     baseline 0,        ceiling 0.05
[ MEASURED ] level 2  ranged share 0.104775 (2554/24376),  baseline 0.0916301, ceiling 0.14163
[ MEASURED ] level 3  ranged share 0.128262 (3254/25370),  baseline 0.109937, ceiling 0.159937
[ MEASURED ] level 4  ranged share 0.181854 (4604/25317),  baseline 0.163892, ceiling 0.213892
[ MEASURED ] level 5  ranged share 0.182218 (4060/22281),  baseline 0.148663, ceiling 0.198663
[ MEASURED ] level 6  ranged share 0.26699  (4628/17334),  baseline 0.257395, ceiling 0.307395
[ MEASURED ] level 7  ranged share 0.290046 (5213/17973),  baseline 0.291148, ceiling 0.341148
[ MEASURED ] level 8  ranged share 0.257684 (4670/18123),  baseline 0.243231, ceiling 0.293231
[ MEASURED ] level 9  ranged share 0.388464 (6755/17389),  baseline 0.528292, ceiling 0.578292
[ MEASURED ] level 10 ranged share 0.264815 (4634/17499),  baseline 0.497895, ceiling 0.547895
[ MEASURED ] level 11 ranged share 0.269276 (4788/17781),  baseline 0.466284, ceiling 0.516284
[ MEASURED ] level 12 ranged share 0.338016 (6496/19218),  baseline 0.453762, ceiling 0.503762
[ MEASURED ] level 13 ranged share 0.230199 (5403/23471),  baseline 0.227515, ceiling 0.277515
[ MEASURED ] level 14 ranged share 0.60512  (14276/23592), baseline 0.55876,  ceiling 0.60876
[ MEASURED ] level 15 ranged share 0.495638 (11646/23497), baseline 0.583495, ceiling 0.633495
[       OK ] LevelRosterBaselineTest.PlacedClassMixWithinBaseline (119257 ms)
```
→ **L1-15 全部在 ceiling 内**。L14 = 0.60512 对 0.60876，余量 **0.36pp**（最紧的一层，
与 Task 3 定的 `squad_chance=10` 一致）。本轮零数据改动，故该余量未被消耗。

## 改动清单

| 文件 | 改动 |
|---|---|
| `Source/monster.h` | `SquadRollCounters` 新增 `formed`（与 leash 无关的成队计数），注释写明为何不能复用 `realised` |
| `Source/monster.cpp` | 小队分支：本 roll 放下 ≥1 随从时 `formed++`（5 行） |
| `test/level_roster_baseline_test.cpp` | 新增 `SquadPlacementTest.SquadFormationRate` + 具名阈值数组 `kSquadFormationFloor`（含出处注释） |
| `eval/cases/rng/level-rosters.yaml` | `passed_min` 9→10、`output_contains` 加 `[ SQUADFORM ]` 与 `PASSED ] 10 tests.`、补记本轮结论 |
| `assets/txtdata/monsters/level_roster_params.tsv` | **零改动**（实测无需回退） |

**未动**：`kRangedShareCeiling` 及任何阈值（R4）；L14 任何列（RB17）；`class_floors`（RB14）；
`UnleashedFallbackPlacesNeighboursWithoutLeashing`（Task 3 修复轮 F2 的回退守卫，保留不动）。

**无 `DISABLED_`/跳过式占位**（禁令 6）：新用例唯一的跳过是两条既有的素材探测
（`missingMpqAssets_` / `missingRetailTrn_`，后者从被测对象真实依赖 `monsters\monsters\genrl.trn` 探测），
以及"表自己把 squad_chance 设为 0"这一条**带断言的**豁免（见反证 2）。
eval case 已声明 `setup.retail_or_hf_required: true`（既有）。

**TSV 未改**，故无需 `ninja devilutionx_mpq` 重量；构建仍走 `ninja -C build level_roster_baseline_test`，
其 `Trimming assets` 步骤会同步 `build/assets`（实测输出与出厂表一致，L14 rollRate 反映 chance=10）。

## 门禁

```
python3 tools/run_tests.py --json /tmp/ci.json
  ctest.passed 764  failed 0  skipped 3  passed_pct 100
  drift.drift_ok true  (PASS A/B/C/C2/E)
  EXIT 0
python3 -m tools.eval.backend --smoke      -> 36/36 全 PASS, EXIT 0
python3 -m tools.eval.backend --run level-rosters -> [PASS] (50/50), EXIT 0
```

用例数：`level_roster_baseline_test` 9 → **10**（`LevelRosterBaselineTest` 3 + `SquadPlacementTest` 7）。

## 提交

| SHA | 内容 |
|---|---|
| `a7a56dcd1` | `feat(monster): count squad rolls that placed a minion regardless of leash` |
| `bb3a17326` | `test(monster): pin the squad formation rate per level from measurement` |

## 顾虑

1. **运行时**：新用例 500 seed × 15 层 ≈ **290s**，使 `level_roster_baseline_test` 从 184s 涨到 ~475s。
   eval case 的 `timeout: 900` 目前够用（实跑通过），但余量已不宽裕；若 Task 5 再往这个二进制加
   长用例，需一并上调该 timeout。500 seed 是简报的硬要求（"≥500 seeds"），故未下调。
2. **阈值是统一值而非逐层值**：实测各层都在 99.89% 以上、彼此差异 <0.11pp，逐层给不同 floor 只会
   把噪声固化成规格。数组仍按层索引（保留逐层分化能力），当前 15 层同值 0.95，出处与余量理由写在注释里。
3. **`formed` 与 `realised` 并存**：本轮在 leashed 层断言二者相等、在 unleashed 层断言 `realised==0`，
   所以两个计数器互为守卫；但若将来真有某层回退到 `squad_leashed=0`，Task 3 的
   `ShippedSquadChanceRealisesSquadsOnEveryLevel` 里的 `EXPECT_GT(realised, 0)` 会对该层变红——
   那条断言用的是 leash 相关的 `realised`。届时需把它改为 `formed`。当前无层回退，故未预改（禁令：不写未实现内容）。
4. 规格「实施记录」/附录 E 的更新按简报属**控制者**职责，本轮未改规格文件。

## CI 终态（R43）

推送 `1a90a16ae..bb3a17326` → `myrepo/feature/qol-upgrades`。
`Better D1 CI` run **35151879181**：
```
✓ feature/qol-upgrades Better D1 CI · 35151879181
✓ build-and-test in 5m11s   (Build / Run full test suite / Drift check 全 ✓)
- eval-nightly / eval-smoke in 0s  (workflow_dispatch 专属，按设计跳过)
```
唯一 annotation 是既有的 `actions/upload-artifact@v4` Node 20 弃用提示，与本轮改动无关。
