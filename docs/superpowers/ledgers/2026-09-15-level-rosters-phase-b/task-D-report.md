# 任务 D 报告：L14 core 名单重塑（尝试让 L14 用 squad_chance=30）

状态：DONE — **采纳配置 B**（L14 去掉 RangedKite core，squad_chance 10 → 30）

## 目标
- 验收 8：L14 placed 远程占比 ≤ 0.60876（ceiling 不动）
- 验收 9c：L14 unique base 可达性 6/6（不得下降）
- 只动 L14 的 core 名单 + 它的 squad_chance

## 尝试记录表
| # | L14 core 配置 | squad_chance | placed 远程占比(200 seeds) | 9c | 结论 |
|---|---|---|---|---|---|
| A（现状基线） | VTEXLRD(Melee) + SNOWWICH(Turret) + XACID(Kite) | 10 | 0.605120 (14276/23592) | 6/6 | 通过但余量仅 0.36pp，rollRate 0.088 |
| Task 3（先例，已回退） | 同 A，但 class_floors Melee 1→2 | 30 | 过 ceiling | 2/6 | 回退 |
| B | VTEXLRD(Melee) + SNOWWICH(Turret)（去掉 XACID kite core） | 30 | **0.543672 (12785/23516)** | **6/6** | **两条都过，余量 6.5pp，rollRate 0.313** |

## 结构判据（为什么只有一个杠杆）

1. **小队的 partner 从「其他 core」抽取**（`PickCorePartnerTypeIndex`，`Source/monster.cpp`）：
   原 3 个 core 里 2 个是远程 → 每支小队的随从有 2/3 概率是远程 → `squad_chance` 一抬，
   placed 远程占比就被推过 ceiling。降低远程占比的正解是**降低 core 中远程的比例**。
2. **不能靠加 Melee core**：L14 的 6 个 unique base 是 **4 Melee + 2 RangedTurret**，
   B1 cap 为 **2/类**（`BehaviorClassCapForLevel`，L13-16）。core 是在尾抽之前预加、且计入 classCounts，
   所以某一类的 core 数达到 2 时，尾抽循环会把该类整类从 typelist 里剔除 → 该类的 unique base 永久不可达。
   这正是 Task 3 抬 `class_floors` Melee 1→2 时 9c 从 6/6 掉到 2/6 的机制（4 个 Melee base 全灭 → 只剩 2 个 Turret base）。
   同理**也不能加第二个 RangedTurret core**。
3. **RangedKite 在 L14 没有任何 unique base**（L14 uniques: RBLACK/BSNAKE/RTBLACK/BTBLACK = Melee，MAGISTR/HLSPWN = Turret）。
   所以去掉 kite core `MT_XACID` 是**唯一**既降远程占比、又不关闭任何 unique base 尾抽的动作。
   附带收益：kite core 让位后，尾抽面对的是 Melee 偏重的候选池（8 Melee / 5 Turret / 2 Kite），占比进一步下降。

## 采纳的改动

- `assets/txtdata/monsters/level_rosters.tsv`：删除 `14  MT_XACID  core  -` 一行
  → L14 core = `MT_VTEXLRD`(Melee) + `MT_SNOWWICH`(RangedTurret)
- `assets/txtdata/monsters/level_roster_params.tsv`：L14 `squad_chance` **10 → 30**
  → 全 15 层统一 30，RB17 的 L14 冻结让步解除
- **未触碰**：`kRangedShareCeiling`、`kSquadFormationFloor`、`kRangedShareBaseline`、
  `class_floors`（仍 Melee=1）、`squad_size`/`squad_leashed`、`BehaviorClassCapForLevel`、其它任何层

## 前后对比（实测）

| 指标 | A（改前，chance=10） | B（改后，chance=30） | 判据 |
|---|---|---|---|
| L14 placed 远程占比（200 seeds） | 0.605120 (14276/23592) | **0.543672 (12785/23516)** | ≤ 0.60876（余量 0.36pp → **6.5pp**）|
| L14 unique base 可达性 | 6/6 | **6/6** | 不得下降 ✅ |
| `[ SHIPPEDSQUAD ] level 14` rollRate | 0.088 | **0.313406** | 名义 30% ✅ |
| L14 每 seed 组合数（500 seeds） | — | **51** | ≥2 ✅ |
| L1-13、L15 占比 | 见基线 | **逐位不变** | 未波及其它层 ✅ |
| 逐层形成率 | 99.890-100% | 全部 ≥0.95 floor（`SquadFormationRate` 绿） | ✅ |

对照证据（证明守卫是**紧的**、B 不是巧合）：把 core 恢复成原 3 个、chance 保持 30，
实测 **0.615081 > 0.60876 判红**，与分发稿引用的 Task 3 读数一致；换成 B 后同一用例转绿。

## 门禁

- `level_roster_test` 37/37、`sampling_behavior_test` 35/35（含 `HellUniqueBasesRemainReachable`
  6/6、`RosterPerSeedVariety`、`RosterQuotasSatisfied`、`RosterQuotaAllowanceIsBinding`、
  `HellL13/14/15SameClassTailBaseline` 与 `CavesKiteTailBaseline` 的 `EXPECT_EQ(tail,0.0)`）
- `level_roster_baseline_test`（`LevelRosterBaselineTest.*:SquadPlacementTest.*`）全绿
- 加载期校验：core≤cap、unique base 白名单、class floor 可满足性均通过（`LevelRosterShippedLoadTest`）
- 全量：`ctest 767/767, failed 0, passed_pct 100`，`drift_ok true`（A-F，含检查 F）
- eval：`--smoke` exit 0

## 说明

- 改 TSV 后每次测量前都跑了 `ninja devilutionx_mpq`。
- 对照用的临时改动（A@30）已恢复，最终工作区只含采纳配置。
- 规格文件未改（控制者维护）；本轮结论已写入 `eval/cases/rng/level-rosters.yaml` 的 description
  与两处测试注释（`PlacedClassMixWithinBaseline` 的 R4 history、`HellUniqueBasesRemainReachable` 的 scope note 3）。
