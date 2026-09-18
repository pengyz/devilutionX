# Task 5 — 最终评审修复波报告

分支 `feature/qol-upgrades`，三个提交：

| 提交 | 内容 |
|---|---|
| `823a98559` | 数据修复（S1）+ 阈值复测 |
| `40d587f78` | 两个结构守卫（R38，AC9b/9c）+ `passed_min` 同步 |
| `7836a59e4` | I2 尾抽界限收紧、I4 L1-12 远程占比守卫 |

状态：**DONE_WITH_CONCERNS**（并非本波未完成，而是 I4 守卫按设计暴露了 4 个先于本波存在的 R4 超标层，需要独立裁决）。

---

## 逐项处置

### 1【严重 S1】恢复地狱段随机性与 unique 可达性 — 已修复（仅数据）

根因是结构性的，不是 RNG：

* 未满足的 class floor 会让尾抽变成确定性选择——floor 的 "first match" 偏好在 `GenerateRnd()` **之前**取（`Source/monster.cpp:3583-3599`）；
* 被 core 顶到 B1 cap（2）的类别会被整类从尾池剪掉，其类型（含 unique base）永远抽不到。

数据改动（`assets/txtdata/monsters/`，CRLF 保持）：

```
level_roster_params.tsv
-13	18000	1	Melee=2,RangedKite=1      +13	18000	1	Melee=2
-14	18000	1	Melee=2,RangedKite=1      +14	18000	2	Melee=1
-15	18000	3	Melee=2,RangedTurret=2    +15	18000	3	Melee=2,RangedTurret=1

level_rosters.tsv
-15	MT_MAGISTR	core	-              （删除该 core 行）
```

* **L13**：两个 Melee core 与 `tail_draw=1` 都保留，**只删掉 `RangedKite=1` floor**。floor 未满足时尾抽是确定性 first-match；无未满足项后 `GenerateRnd(nt)` 在 6 个远程候选里随机。占比不变而组合数 1→6。
* **L14**：`tail_draw` 1→2 **且** Melee floor 2→1。只调 `tail_draw`（不动 floor）仅得 5 种组合、2/6 unique；降 floor 后两次抽取都自由。
* **L15**：删 `MT_MAGISTR` core 并把 `RangedTurret` floor 2→1，留出 cap 余量使尾池非空。

#### 实测 before → after（500 seeds，真实 `GetLevelMTypes()`）

| level | 组合数 before | after | unique base 可达 before | after |
|---|---|---|---|---|
| L13 | 1 | **6** | 0/5 | **2/5** |
| L14 | 1 | **39** | 0/6 | **6/6** |
| L15 | 1 | **4** | 0/2 | **2/2** |

L1-12 组合数未变：L1 1（豁免）、L2 117、L3 184、L4 147、L5 117、L6 66、L7 55、L8 66、L9 136、L10 35、L11 20、L12 56。

#### 六项同时满足情况

| 条件 | 结果 |
|---|---|
| ① L13-15 放置远程占比 ≤ 各自 ceiling | ✅ L13 0.236164 ≤ 0.277515；L14 0.604908 ≤ 0.60876；L15 0.500787 ≤ 0.633495 |
| ② 每 seed 组合数 ≥ 2（三层） | ✅ 6 / 39 / 4 |
| ③ 地狱段 unique 可达性恢复 | ⚠️ L14 6/6、L15 2/2 完全恢复；**L13 仅 2/5**（见下方硬约束） |
| ④ `level_roster_test` 全绿 | ✅ 29/29 |
| ⑤ B1 契约（`CavesKiteTailBaseline`、`HellL13/14/15SameClassTailBaseline` 的 `EXPECT_EQ(tail,0.0)`）+ `RosterQuotasSatisfied` | ✅ 全绿 |

#### L13 硬约束（必须记录的让步）

L13 候选池 9 Melee / 3 RangedKite / 3 RangedTurret，cap 2/类。放置远程占比 ≈ `R/(M+R)`（已对真实放置链路验证）。ceiling 0.2775 只容许 M=2, R=1（实测 0.2362）；M=1 已实测 0.3036 破线。M=2 意味着两个 Melee 槽都是 core → Melee 顶 cap → 尾抽永远抽不到 Melee → L13 三个近战 unique base（Blackskull/`MT_BALROG`、Rustweaver/`MT_RTBLACK`、Gorefeast/`MT_VTEXLRD`）只能靠把它们设为 core 才可达，而这需要 `allow_unique_boost`，其存在目的正是拦住随之而来的 unique 出现率提升（规格 §4.4.3）。R4 禁止抬 ceiling。**所以 2/5 是 L13 当前可达上限**（自 0/5 提升）。守卫 9c 把这三个 base 写成精确的 `EXPECT_FALSE` 例外表并配 `checkedUniques == 10` premise 界，一旦将来释放出 Melee 槽会直接变红而不是静默。

尝试过并放弃的路线：把 unique base 设为 core（校验器按设计拒绝：`monster 87 is a unique's base at level 13 and needs allow_unique_boost`）；L13 单 Melee core 变体（5/5 unique 但占比 0.3036 破线）；`tail_draw=2` 配 2 core（0.3827）、配 1 core（0.4766）。

### 2【重要 R38】两个结构守卫 — 已落地且可失败

均在 `test/sampling_behavior_test.cpp`，`SamplingBaselineTest` 套件内。

**AC9b `RosterPerSeedVariety`** — L2-15 每层 500 seeds 下实现构成组合数 `EXPECT_GE(..., 2)`。阈值刻意取最弱的非平凡值（"采样是否还依赖 seed"），不是分布质量条。L1 显式豁免为 `EXPECT_EQ(..., 1)`，注释写明原因：候选池仅 6 型（`MT_NZOMBIE`/`MT_RFALLSP`/`MT_NSCAV`/`MT_WSKELAX`/`MT_ZOMBIE`/`MT_FALLSP`），4 个是 core、Golem 占一槽，`tail_draw=2` 与 `max_image` 下构成按算术固定；断 ≥2 会要求扩池，超出本波授权。用 `EXPECT_EQ 1` 而非跳过，是为了让豁免在被打破时报错而不是无声扩大。

**AC9c `HellUniqueBasesRemainReachable`** — L13-15 每个「base 在本层可用」的 unique，其 base 必须在某个 seed 的 realized 集合中出现（依据：`PlaceUniqueMonsters()` 找不到 base 就 `continue`，`Source/monster.cpp:505-528`）。分母排除 5 个 base 在自身 mlevel 就不可用的 unique（既存数据事实，非名册回归）：Warlord of Blood（`MT_BTBLACK` 段 14-16，靠 `Q_WARLORD` 预加照样放置）、Lord of the Pit（`MT_GSNAKE` 段 15-16）、Howlingire（`MT_HOLOWONE` availability=Never）、Bloodmoon Soulfire（`MT_PAINMSTR` Never）、Zamphir（`MT_REALWEAV` Never）。分母以 `EXPECT_GT(candidateBases, 0)` 防空。

**可失败性已实证**：把数据修复 `git stash` 后重跑，两个用例**均 FAILED**；恢复后 PASS。两者都无条件打印 `[ MEASURED ]` 行，数字在 CI 输出里可读。

`eval/cases/rng/sampling-anti-monopoly.yaml`：`passed_min` 24 → **26**，`output_contains` 的 `"[  PASSED  ] 24 tests."` → `"[  PASSED  ] 26 tests."`，顶部 R35 维护提醒同步（已同步）。

### 3【重要 I2】`RosterTailDrawBounded` 的 `preAdded` — 已修

原式为 `1 (MT_GOLEM) + AvailableCoreCount(level)`，漏掉 `Source/monster.cpp:3464-3475` 的六个任务门控类型预加（`Q_BUTCHER` 加普通类型 `MT_CLEAVER`，其余五个加 unique 的 base）。本套件在两个用例里激活 `Q_VEIL`，故该层有任务活跃时会有一个任务类型漏进推导出的 "tail"，界限是错的而非保守。

改为 `PreAddedTypes(level)` 返回**去重的类型集合**（Golem + 任务预加 + 可用 core）——去重是必须的，因为 `AddMonsterType()` 按类型去重（`Source/monster.cpp:3299-3302`：已存在类型只 OR 置位标志、不增 `LevelMonsterTypeCount`），把独立计数相加会高估重叠、把界限收得过紧。另加 `ASSERT_FALSE(UseMultiplayerQuests())`，把 "Q_SKELKING 分支不生效" 从假设变成断言（该分支会再加两个类型）。

**并让修复本身可失败**：用例末尾在 L14 激活 `Q_VEIL`，断言预加集合含 `MT_RBLACK` 且**严格大于** Golem+core 的旧公式值，并在该条件下复验界限。这样再次丢掉任务项会红，而不是因为恰好没有任务活跃而绿。`AvailableCoreCount()` 失去唯一调用者，已删除（否则 `-Wunused-function`）。

### 4【重要 I4】L1-12 远程占比 ceiling — 已填实测基线，并暴露 4 个既存超标层

`test/level_roster_baseline_test.cpp`：`kRangedShareBaseline` 的 L1-12 由 `kRangedShareUnconstrained` 哨兵改为计划 A-baseline 表的实测值（同 200 seed、同 fixture、同分子/分母写法，分数以 `numerator/denominator` 形式保留出处），`kRangedShareCeiling` 相应为基线 +5pp，断言循环从 L13-15 扩到 **L1-15**。L16 保持哨兵并注明原因（硬编码分支在名册路径前 early-return）。**哨兵语义保留并写进注释**：将来 A2 的 L17-24 在没有基线时应当是"按声明不受约束"，而不是被拿去和 0 比较；另加 `ASSERT_LT(kRangedShareBaseline[level], kRangedShareUnconstrained)`，拒绝"进了断言循环却还挂着哨兵"的层。

扩范围后守卫**立刻抓到 4 个先于本波存在的 R4 超标层**（当前出厂名册）：

| level | 实测占比 | R4 ceiling | 超出 |
|---|---|---|---|
| L2 | 0.174287 | 0.141630 | +3.3pp |
| L3 | 0.189022 | 0.159937 | +2.9pp |
| L4 | 0.361194 | 0.213892 | +14.7pp |
| L8 | 0.457950 | 0.293231 | +16.5pp |

处置遵守 R4：**不动 ceiling**。改为把这 4 层锁在今天实测值（+0.005 抖动余量）上——不得更差；并用 `EXPECT_GT(share, ceiling)` 断言它们**仍然超标**，因此一旦其名册修好，本用例会红并强制删除锁条目，豁免不会长期存活；`breachesSeen == 4` 界住列表。整改 4 个教堂/墓穴层的名册是独立的平衡决策，已上报而非埋掉。

完整 L1-15 实测（200 seeds/层，`[ MEASURED ]` 行）：

```
L1  0.000000 (0/18585)        ceiling 0.050000
L2  0.174287 (3972/22790)     ceiling 0.141630   ← 既存超标（锁定）
L3  0.189022 (4563/24140)     ceiling 0.159937   ← 既存超标（锁定）
L4  0.361194 (8807/24383)     ceiling 0.213892   ← 既存超标（锁定）
L5  0.182936 (4076/22281)     ceiling 0.198663
L6  0.264913 (4592/17334)     ceiling 0.307395
L7  0.283982 (5104/17973)     ceiling 0.341148
L8  0.457950 (8119/17729)     ceiling 0.293231   ← 既存超标（锁定）
L9  0.382713 (6655/17389)     ceiling 0.578292
L10 0.269387 (4714/17499)     ceiling 0.547895
L11 0.274450 (4880/17781)     ceiling 0.516284
L12 0.324956 (6245/19218)     ceiling 0.503762
L13 0.236164 (5543/23471)     ceiling 0.277515
L14 0.604908 (14271/23592)    ceiling 0.608760   ← 余量 0.4pp
L15 0.500787 (11767/23497)    ceiling 0.633495
```

`eval/cases/rng/level-rosters.yaml` 的 description 已补记范围扩展与 4 个锁定层；`passed_min` 保持 3（未增删用例）。该 case 运行时长从 ~20s 涨到 ~132s，仍在其 300s timeout 内。

### 5 规格 §6 9b/9c 与附录 E 第 1 条 — 未改动，已核对一致

按要求未触碰规格散文。核对结论：控制器已写入的 9b/9c 行与附录 E 第 1 条（R37 严重度更正）与实现**一致**，无发现不一致。仅一处措辞值得留意：9b 行写"L1 因候选池仅 6 个允许例外，须在用例内注释说明"——实现不是"跳过"而是 `EXPECT_EQ(...,1)` 的显式断言豁免（更强），注释已写明；这是收紧而非偏离。

---

## 门禁实测输出

```
$ cmake --build build --target sampling_behavior_test level_roster_baseline_test level_roster_test -j8
（无 warning / error）

$ ./level_roster_test          → [  PASSED  ] 29 tests.
$ ./sampling_behavior_test     → [  PASSED  ] 28 tests.   （SamplingBaselineTest 26 + HellfireNoParams 2）
$ ./level_roster_baseline_test → [  PASSED  ] 3 tests.

$ python3 -m tools.eval.backend --run level-rosters
  [PASS] [rng] level-rosters (30/30)
$ python3 -m tools.eval.backend --run sampling-anti-monopoly-cap
  [PASS] [rng] sampling-anti-monopoly-cap (15/15)

$ python3 tools/run_tests.py --json /tmp/ci.json
  build.ok = true
  ctest: passed 745, failed 0, skipped 3, passed_pct 100, returncode 0
  drift: drift_ok true（A/B/C/C2/E 五项 PASS）

$ python3 -m tools.eval.backend --smoke
  dark-expedition 4/4, save-load 2/2, mechanics 8/8, data 6/6,
  render 2/2, combat 5/5, utility 9/9 — EXIT=0

$ python3 tools/eval/sync_case_sets.py --check
  OK: 67 cases, 2 suites consistent
```

行尾核对：两个 TSV 与两个 `.cpp` 均 CRLF，两个 `.yaml` 均 LF（漂移校验 C/C2 PASS）。未改动 `CMake/Tests.cmake`（未新增测试文件，两个守卫加在既有 TU 内）。

---

## 顾虑

1. **L13 unique 可达性只能到 2/5。** 剩余三个近战 base 需要 `allow_unique_boost`（改变 unique 出现率）或放宽 cap 或抬 ceiling，三者都是设计裁决。当前以精确例外表 + premise 界记账。
2. **L14 余量仅 0.4pp**（0.604908 vs 0.60876）。这是评审已认可的支付（"值得为恢复随机性支付"），但 L14 后续任何名册/floor 调整都极易破线，且该层没有第二条退路：降 `tail_draw` 会退回 1 种组合。
3. **I4 暴露的 4 个既存 R4 超标层（L2/L3/L4/L8）未修**，其中 L4 +14.7pp、L8 +16.5pp 幅度不小。已锁定不得更差并断言"仍超标"，但真正的名册整改需要单独一波。这是本报告状态为 DONE_WITH_CONCERNS 的唯一原因。
4. **部分塌缩早于 R4 调整。** 复核 `85471bee0^` 的数据发现 L14 与 L15 当时已是 1 种组合，即 S1 并非全部由任务 4 的 R4 调整引入；对照的 legacy 前特性基线（无名册表、有 cap）为 L13 71 组合 5/5、L14 72 组合 6/6、L15 23 组合 2/2。当前修复后的 6/39/4 仍显著低于 legacy，物种多样性的完整恢复仍要靠附录 E 的四条出路（推荐阶段 B 编组层）。
5. **`level-rosters` eval case 运行时长 20s → 132s**（断言范围 L1-15）。仍在 300s timeout 内，但 nightly 总时长会相应增加。
6. 未触碰明确排除项：I3、I1、R11、R27、R35（`passed_min` 机制本身）、clang-format 工具链、O2、L17-24 legacy、7 项 trivial。

---

# 第二轮 — 控制者裁决 R40 / R41 修复

状态：**DONE**。R40 的四个 R4 超标层已按 R4 要求**改数据**修复，例外表整表删除；R41 按要求补齐例外条目的"为何本阶段接受 / 什么改动会让它失败"注释。

## R40 — L2/L3/L4/L8 全部修回带内（阈值一字未动）

第一轮的"只锁定不修"被驳回：这四层的超标由名册特性本身引入（基线是改动前实测值），按 R4 只能改数据。整改后**四层全部落回带内，用例里不再保留任何超标豁免**，L1-15 每层都受真实阈值约束。

### 数据改动（`assets/txtdata/monsters/`，CRLF 保持）

```
level_roster_params.tsv
-2	6000	2	Melee=2            +2	9000	4	Melee=2
-3	6000	2	Melee=2            +3	6000	4	Melee=2
-4	6000	2	RangedTurret=1     +4	9000	4	RangedTurret=1
-8	9000	2	RangedKite=1       +8	12000	3	RangedKite=1

level_rosters.tsv
+2	MT_BZOMBIE	core	-      +2	MT_FIEND	core	-      +2	MT_SNEAK	core	-
+3	MT_FIEND	core	-       +3	MT_BLINK	core	-
-4	MT_TSKELBW	core	-      +4	MT_GZOMBIE	core	-      +4	MT_YZOMBIE	core	-
-8	MT_GGOATBW	core	-      +8	MT_ILLWEAV	core	-
+8	MT_FAMILIAR	core	-     +8	MT_HORNED	core	-
```

选取逻辑（先用 `/tmp` 模拟器筛，再由真实引擎定谳）：

* **L2/L3**：只补廉价非远程 core（Charge/Sneak/Melee，364-992 image），**保留各自唯一的远程 core**（`MT_TSKELBW` / `MT_RSKELBW`）。L3 的两个 A1 承载 core（`MT_RSKELAX`/`MT_XSKELAX`）按 `A1A3VariantsAreCore` 约束保持不动。
* **L4**：两个 RangedTurret core 去掉一个（保留 `MT_XSKELBW`，`RangedTurret=1` floor 仍满足），补两个 Melee core。
* **L8**：这是唯一"保留两个远程 core 就无解"的层——扫遍 `tail_draw` 2-10 的单换方案占比恒在 30.3%-33.0%（阈值 29.3%），故去掉 RangedTurret core（`MT_GGOATBW`），保留 RangedKite core `MT_YMAGMA` 与 `RangedKite=1` floor，补 Sneak/Charge/Melee。
* **`max_image` / `tail_draw` 同步抬高**：core 集变大后原预算装不下（L8 六个 core + Golem = 7174 > 9000 会挤掉尾抽）。二者都是逐层名册参数、**不是 R4 阈值**；spec §4.1 明确 `max_image` 是安全上限且"内存不是约束、故留余量"，L8 抬到 12000 仍低于 L9-12 的 16000。
* **上界纪律**：`tail_draw` 最高只取到 4。取 5 时表内最大 `tail_draw` 会追平 legacy 无参数层的最佳尾抽（5），令 `NoParamsTailExceedsTheParameterisedCap` 的严格 `>` 失效——那是个合法守卫，故让数据让步，而不是放宽断言。

### 实测 before → after（200 seeds/层，真实放置链路）

| 层 | 阈值 | before | after | 余量 |
|---|---|---|---|---|
| L2 | 0.141630 | 0.174287 ❌ | **0.101657** | 4.0pp |
| L3 | 0.159937 | 0.189022 ❌ | **0.128301** | 3.2pp |
| L4 | 0.213892 | 0.361194 ❌ | **0.189398** | 2.4pp |
| L8 | 0.293231 | 0.457950 ❌ | **0.258842** | 3.4pp |

其余各层未改动、实测不变：L1 0 / L5 0.182936 / L6 0.264913 / L7 0.283982 / L9 0.382713 / L10 0.269387 / L11 0.274450 / L12 0.324956 / L13 0.236164 / L14 0.604908 / L15 0.500787 —— **L1-15 全部在带内**。

### 组合数复核（500 seeds）——没有为压占比而压成固定组合

S1 的教训是"占比达标可能靠把层压成单一组合换来"，故对四个改动层复测组合数：

| 层 | before | after |
|---|---|---|
| L2 | 117 | **360** |
| L3 | 184 | **439** |
| L4 | 147 | **448** |
| L8 | 66 | **116** |

四层全部**显著上升**（core 变多 + `tail_draw` 变大同时扩大了组合空间），不存在压缩换指标。未改动层组合数不变：L1 1（6 类候选池，豁免）、L5 117、L6 66、L7 55、L9 136、L10 35、L11 20、L12 56、L13 6、L14 39、L15 4。

### 两个新守卫的交叉验证

* `RosterPerSeedVariety` — PASS（上表即其 `[ MEASURED ]` 输出）。
* `HellUniqueBasesRemainReachable` — PASS，L13 2/5、L14 6/6、L15 2/2，与第一轮一致（本轮只改 L2/L3/L4/L8，不触及 L13-15）。

### 测试侧改动

`test/level_roster_baseline_test.cpp`：删除 `struct KnownBreach` / `kKnownR4Breaches` / `EXPECT_GT` / `EXPECT_LE(lock)` / `breachesSeen` 全套豁免机制，改为注释记录这段历史（哪四层、超了多少、怎么改的数据修的），断言路径回到"每层一律 `EXPECT_LE(share, ceiling)`"。自失效机制按设计完成了它的使命：数据修好后它先让用例变红，逼着删掉条目。

## R41 — 例外表注释补齐

`test/sampling_behavior_test.cpp` 的 `kCapBlocked`（L13 三个近战 unique base）补上逐条要求的两段说明：

* **为何本阶段接受**：L13 阈值 0.2775 只容得下 2 Melee + 1 ranged 散布类型 → 两个 Melee 槽必须是 core → Melee 类到 cap 2 → 尾池整类剪掉 → 近战 base 永远抽不到；解除需要抬阈值（R4 直接禁止）、抬 L13 Melee cap、或给 `allow_unique_boost`（spec §4.4.3 正是为门控出现率提升而设），三者都是本波无权做的设计裁决。
* **什么改动会让它失败**：`BehaviorClassCapForLevel` 把 L13 cap 抬过 2、L13 core 减到一个 Melee、或该 base 被 `allow_unique_boost` 收为 core —— 因为断言是 `EXPECT_FALSE`，任一发生用例即红，必须删条目；`blockedSeen` premise 另行保证条目不能指向本循环不检查的 base。

三条共因同源，故写成一段共享说明而非三份重复注释。

## eval 与门禁

`eval/cases/rng/level-rosters.yaml` 描述重写：原先记载"4 层锁定豁免"的段落改为记载数据整改方案、整改后实测值、以及组合数复核结论。用例数未变（3），`passed_min` 无需再动；`sampling-anti-monopoly.yaml` 未改。

**runtime 优化（可选项）：主动放弃。** 132s 的成本几乎全在 `PlacedClassMixWithinBaseline` 的 200 seeds × 15 层放置循环。仓库既有的 `SAMPLING_REPORT` 约定（默认 5 seeds、设环境变量才跑 200）用在**报告型**用例（`PlacedClassMixReport`）上是安全的，但用在**阈值型**用例上会实质削弱断言：5 seeds 的占比抖动足以让真实超标漏网，等于把门禁降级成抽查。把 500-seed 守卫从阈值用例里拆出也无收益——它们本就在另一个二进制（`sampling_behavior_test`，全程 5s）里。故按简报授权保持 132s 不变。

```
$ ./level_roster_baseline_test
  3 passed（L1-15 全部 [ MEASURED ] 在带内，无豁免条目）

$ python3 tools/run_tests.py --test sampling_behavior_test   → 28 passed
$ python3 tools/run_tests.py --test level_roster_test        → 29 passed
$ python3 tools/run_tests.py --test level_roster_baseline_test → 3 passed

$ python3 tools/run_tests.py --json /tmp/ci.json
  build.ok = true
  ctest: passed 745, failed 0, skipped 3, passed_pct 100, returncode 0
  drift: drift_ok true（A/B/C/C2/E 五项 PASS）

$ python3 -m tools.eval.backend --smoke                     → EXIT=0
$ python3 -m tools.eval.backend --run level-rosters          → PASS (30/30)
$ python3 -m tools.eval.backend --run sampling-anti-monopoly-cap → PASS (15/15)
$ python3 tools/eval/sync_case_sets.py --check               → OK: 67 cases, 2 suites consistent
```

## 排查笔记（值得沉淀）

**测试二进制读的是 `build/devilutionx.mpq`，不是工作区 TSV，也不是 `build/assets/`。** 改完数据后 `run_tests.py --test <name>` 只重建该测试目标，实测值会**一字不变**地复现旧结果——极易误判成"改动无效"或"测试有 bug"。必须 `ninja devilutionx_mpq` 重打包（`ninja assets/txtdata/...` 只更新松散副本，MPQ 优先级更高因而无效）。本轮先后被这一点绊了两次：第一次是松散副本没更新，第二次是 MPQ 更新了但 `level_roster_baseline_test` 二进制还是旧的。

## 顾虑

1. **L14 余量仍只有 0.4pp**（0.604908 vs 0.608760），与第一轮相同，本轮未改 L14。它是全表最紧的一层，后续任何 L14 名册/floor 调整都极易破线。
2. **L13 unique 可达性仍是 2/5**（R41 已裁决接受）。
3. **L8 失去了 RangedTurret core。** 该层现在只有 RangedKite 一种远程 core（floor 也只要求 RangedKite=1）。RangedTurret 类型仍可由尾抽取到，但不再保底。这是把 45.8% 压到 25.9% 的必要代价——保留两个远程 core 的方案全部无解（实测扫描见上）。
4. **`tail_draw` 上界被 legacy 层间接钉在 4。** 若将来某层需要 `tail_draw >= 5`，会与 `NoParamsTailExceedsTheParameterisedCap` 冲突，届时要重新论证该守卫的表达方式（不是放宽它）。
5. **物种多样性相对 legacy 仍偏低**（L13-15 6/39/4 vs legacy 71/72/23），完整恢复仍指向附录 E 的四条出路。
6. 排除项未触碰：I3、I1、R11、R27、R35 机制、clang-format、O2、L17-24 legacy、7 项 trivial。
