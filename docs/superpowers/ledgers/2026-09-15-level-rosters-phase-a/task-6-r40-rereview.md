# Task 6 — R40 范围化复审（`9bbeaa59a..7364e0fa7`）

范围：仅核验裁决 R40（把 L2/L3/L4/L8 的远程占比修回 baseline+5pp 内）是否真的落地、R41 注释是否补齐、以及该修复是否引入新问题。非全面重审。

**核实手段**：只读一次 diff 文件；独立实跑三个聚焦测试二进制（先用 `ninja -n` 确认 MPQ 与三个测试目标**均已是最新**——dry-run 只回 "Re-running CMake"，无任何编译/打包动作，故实测量到的是当前 TSV，不是旧表）；用 Python 从计划 A-baseline 原始分子/分母独立重算 16 个 ceiling；工作区/暂存区/HEAD 全程未改（`git status --porcelain` 空）。

---

## 逐条裁决（1-8）

### 1. 四处占比是否真的回到带内 — ✅ 成立（独立实测）

我独立跑了 `./level_roster_baseline_test --gtest_filter='*PlacedClassMixWithinBaseline*'`（128s，PASS）。`[ MEASURED ]` 输出与报告声称**逐位一致**：

| 层 | ceiling（我独立重算） | 实测 share | 分子/分母 | 余量 |
|---|---|---|---|---|
| L2 | 0.141630 | **0.101657** | 2478/24376 | 4.00pp |
| L3 | 0.159937 | **0.128301** | 3255/25370 | 3.16pp |
| L4 | 0.213892 | **0.189398** | 4795/25317 | 2.45pp |
| L8 | 0.293231 | **0.258842** | 4691/18123 | 3.44pp |

其余 11 层同样实测在带内，且与报告数字一致（L1 0 / L5 0.182936 / L6 0.264913 / L7 0.283982 / L9 0.382713 / L10 0.269387 / L11 0.274450 / L12 0.324956 / L13 0.236164 / L14 0.604908 / L15 0.500787）。**L1-15 十五层全部 ≤ 各自 ceiling**，无一例外。

分母口径正确：`ActiveMonsterCount`（真实放置怪物数），驱动 `CreateDungeonForMeasurement -> GetLevelMTypes -> InitMonsters` 生产链路，不是规则重模拟（`test/level_roster_baseline_test.cpp:354-364`）。

### 2. ceiling 是否未被改动 — ✅ 成立（逐位重算比对）

比对方式：从计划 `docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md:665-682` 的 A-baseline 表**原始整数**（placed / RangedTurret / RangedKite）用 Python 重算 `(turret+kite)/placed + 0.05`，再与测试源码 `kRangedShareCeiling` 及 `[ MEASURED ]` 打印的 ceiling 三方对照。**16 个值全部 OK，零 MISMATCH**。

- `kRangedShareTolerance = 0.05` 未动（`level_roster_baseline_test.cpp:159`）。
- 本轮 diff 对 `level_roster_baseline_test.cpp` 的改动**全在 `PlacedClassMixWithinBaseline` 函数体内**（diff 行 135-238），`kRangedShareBaseline`/`kRangedShareCeiling` 两个数组（源码 :162-202）一字未碰——diff 里没有它们的任何 hunk。
- 阈值仍以 `numerator/denominator` 分数形式落地（如 `(1561.0 + 2652.0) / 17321.0` for L8），出处可审计，不是四舍五入字面量。
- `ASSERT_LT(baseline, kRangedShareUnconstrained)` 前提断言保留（:350），拒绝"进了断言循环却挂着哨兵"。L16 仍是哨兵并注明原因。

结论：**为通过而调阈值的嫌疑不成立**。

### 3. 异常表是否真的删除 — ✅ 成立（无残留豁免）

全树 grep `kKnownR4Breaches|KnownBreach|breachesSeen` → **零匹配**。`struct KnownBreach`、`kKnownR4Breaches`、`EXPECT_GT`(仍超标前提)、`EXPECT_LE(lock)`、`continue` 跳过分支、`breachesSeen` 收尾断言全套删除。

当前 `PlacedClassMixWithinBaseline`（:316-377）正是要求的形态：L1-15 单循环 → 每层 `ASSERT_LT` 哨兵前提 → 200 seeds 累计 → `ASSERT_GT(total,0)` 夹具健全性 → **无条件** `[ MEASURED ]` 输出 → 每层一条 `EXPECT_LE(share, ceiling)`。无 `if`、无 `continue`、无第二阈值、无 skip。注释显式写明 "There is deliberately NO exception list here"（:331），并把四层的历史（超了多少、怎么改数据修的）留作 history 注释而非可执行豁免。

**没有任何"永久合法"的豁免残留。** 自失效机制按设计完成使命后被删除，而不是留在树里。

### 4. 组合数未回塌 — ✅ 成立且来源可核

`RosterPerSeedVariety` 确实是 L1-15 循环、500 seeds、`EXPECT_GE(combinations.size(), 2u)`（L2-15）+ `EXPECT_EQ(...,1u)`（L1 显式豁免），并**无条件打印**每层实测组合数（`sampling_behavior_test.cpp:1202-1240`）。组合键 `RealisedTypeKey()` 是排序后的完整 realized 类型向量，去重靠 `std::set`——口径正确。

我实跑 `./sampling_behavior_test` 的 `[ MEASURED ]` 行与报告**逐位一致**：

```
L1 1  L2 360  L3 439  L4 448  L5 117  L6 66  L7 55  L8 116
L9 136  L10 35  L11 20  L12 56  L13 6  L14 39  L15 4
```

四个改动层全部**显著上升**（L2 117→360、L3 184→439、L4 147→448、L8 66→116），未改动层与第一轮完全一致。**不存在"压占比换固定组合"**——S1 的教训被真正兑现，而不是口头声称。L13-15 保持 6/39/4，本轮 diff 的 TSV 改动只触及 L2/L3/L4/L8 行（diff 行 19-39、44-87 可逐行核对），L13-15 三行 `params` 与其 roster 行均未出现在 hunk 变更侧。

### 5. unique 可达性未变 — ✅ 成立；R41 注释已按要求补齐

实跑 `HellUniqueBasesRemainReachable` → PASS，`[ MEASURED ]` 为 **L13 2/5、L14 6/6、L15 2/2**，与第一轮一致。

`kCapBlocked` 三条（L13 `MT_BALROG`/`MT_RTBLACK`/`MT_VTEXLRD`）的注释（`sampling_behavior_test.cpp:1252-1273`）确实补上了 R41 要求的**两段**：
- **为何本阶段接受**：完整因果链（ceiling 0.2775 → 只容 2 Melee + 1 ranged → 两 Melee 槽必须是 core → Melee 顶 B1 cap 2 → 尾循环整类剪掉 → Melee base 永不可抽），并点명三条解除路径各自为何越权（R4 直接禁止抬 ceiling / 抬 L13 cap / `allow_unique_boost` 按 spec §4.4.3 正是门控出现率提升的闸门）。
- **什么变更会让它失败**：具体点名 `BehaviorClassCapForLevel` 抬 L13 cap 过 2、L13 core 减到一个 Melee、或该 base 被 `allow_unique_boost` 收为 core；并说明因为断言是 `EXPECT_FALSE`，任一发生用例即红、必须删条目；`blockedSeen` premise 另保证条目不能指向本循环不检查的 base。

三条共因同源写成一段共享说明，我认为**满足 R41 的"逐条理由"实质**（逐条重复三遍同一因果链只会稀释信息）。

### 6. 新引入的耦合与身份影响 — 两条自报均**成立且有据**；判定：可接受，但第 2 条应留痕

**(a) L8 不再有保障的 RangedTurret core** — 自报准确，**不违反规格**，但有一处需要纠正报告的措辞。

我核实的 L8 事实（从 `monstdat.tsv` 提取 AI → `GetBehaviorClass` 映射）：

| L8 core | AI | 行为类别 | image |
|---|---|---|---|
| MT_TOAD | Fat | Melee | 1130 |
| MT_HORNED | Rhino | Melee | 1630 |
| MT_YMAGMA | Magma | **RangedKite** | 1680 |
| MT_UNSEEN | Sneak | Sneak | 992 |
| MT_ILLWEAV | Sneak | Sneak | 992 |
| MT_FAMILIAR | Bat | Charge | 364 |

规格附录 C 的约束原文是「**每层 4-5 只**；必须含该层 A1/A3 承载；不得含 unique 的 base（除非白名单）；`Σimage` 远低于逐层上限」——它**没有**"覆盖 2-3 个行为类别"这一条，简报里的那半句在规格中无对应文字（附录 C 是"初稿，待作者调整"）。按规格实际文字判定：
- **行为类别覆盖**：L8 core 覆盖 **4 个类别**（Melee/RangedKite/Sneak/Charge），比"2-3 个"更宽，**远程存在感仍由 `MT_YMAGMA` core 保底**。所谓"失去远程 core"不准确——失去的是 **RangedTurret 这一子类**的 core，远程大类仍有保障的 core。报告第 3 条顾虑的表述比事实更悲观，我按事实修正。
- **4-5 只约束**：L8 现有 **6 个 core**，L2 有 **7 个**，L3 有 **6 个**——三层超出"每层 4-5 只"。但这不是本轮引入的新违规类别：附录 C 明确是待调整初稿，且 core 数没有任何测试或校验器约束（校验器只管 cap 可满足性 / 存在性 / unique 白名单）。同时预算约束仍满足：L2 5003<9000、L3 3586<6000、L4 3727<9000、L8 7174<12000，`MaxLvlMTypes=24` 也远未触及（最大 core+golem = 7 类型）。
- RangedTurret 仍可由尾池抽到（L8 候选池含 2 个 RangedTurret，`RangedKite=1` floor 不排斥它），且实测 L8 组合数 66→116 上升，说明尾池活跃度提高而非降低。

判定：**可接受，非阻塞**。理由是"保留两个远程 core 无解"有实测扫描支撑（`tail_draw` 2-10 全扫，占比恒在 30.3%-33.0% > 29.3%），而 R4 明令只能改数据。建议（非阻塞）：把"L8 RangedTurret 退为尾池可选"这一身份取舍记进规格附录 E，与 L13/L14 的同类让步并列。

**(b) `tail_draw` 被 `NoParamsTailExceedsTheParameterisedCap` 间接钉在 ≤4** — 耦合**真实存在**，但**代码/数据/规格里没有任何注释或文档记录**。

我核实了机制：该守卫（`sampling_behavior_test.cpp:1569-1605`）取 L1-16 表内 `max(tail_draw)` 作比较基准，断言 L17-24 legacy 的实测最佳 scatter 数**严格大于**它。当前表内最大 `tail_draw = 4`（L2/L3/L4），守卫 PASS（我单跑该用例确认）。若将来把某层 `tail_draw` 提到 5 且 legacy 最佳恰为 5，严格 `>` 会失效——这是一条**跨任务、跨文件、无显式声明**的隐式上界。

判定：**Minor（非阻塞），但应留痕**。让数据让步而不是放宽守卫是**正确取舍**（守卫本身合法，放宽它会削弱 R28 保护）。问题在于这个约束只存在于任务报告里：`level_roster_params.tsv` 无注释（TSV 加载器不支持 `#` 注释，我确认 `level_roster.cpp` 无注释跳过逻辑，所以写在 TSV 里不可行），`NoParamsTailExceedsTheParameterisedCap` 处也没有反向提示"抬高任一层 tail_draw 前先看这里"。下一个改 `tail_draw` 的人会撞上一次莫名失败。建议在该守卫上方加一行反向注释，或记入规格附录 E——**不构成合并阻塞**，因为一旦撞上是**红灯**（守卫会失败）而非静默错误。

### 7. 一致性 — ✅ 全部既有保证仍满足；`passed_min` 与真实用例数一致

我实跑三个二进制（均为当前 MPQ + 当前二进制，`ninja -n` 已确认无待重建）：

| 二进制 | 结果 |
|---|---|
| `level_roster_test` | **29 passed**（加载期 core ≤ cap / floors 可满足 / unique 白名单 / 出厂表校验 `LoadsTheShippedRosterAndValidatesIt` PASS）|
| `sampling_behavior_test` | **28 passed**（SamplingBaselineTest 26 + HellfireNoParams 2）|
| `level_roster_baseline_test` | 聚焦跑 `PlacedClassMixWithinBaseline` PASS；`--gtest_list_tests` 确认共 **3** 个用例 |

逐项点名核对：
- **B1 契约 `EXPECT_EQ(tail, 0.0)`**：`CavesKiteTailBaseline`、`HellL13/14/15SameClassTailBaseline` 全在 26 个 PASS 内。
- **`RosterQuotasSatisfied`** PASS；`RosterQuotaAllowanceIsBinding` PASS（配额断言非空转）。
- **R28 legacy 两用例** `LevelsWithoutParamsStillSampleTypes` + `NoParamsTailExceedsTheParameterisedCap` PASS。
- **`RosterTailDrawBounded` 的 quest 预加口径**：`PreAddedTypes(level)` 去重集合口径保留（:788），末尾 L14 激活 `Q_VEIL` 的可失败性断言保留（:933-956），本轮未触碰。
- **`IdentityGuard`** PASS（相邻层 Jaccard < 0.9）——L2/L3/L4/L8 core 大改后相邻层仍未变得可互换，这一点值得单独指出，因为补 core 最容易把相邻层拉近。
- **`A1A3VariantsAreCore`** PASS：L3 的 `MT_RSKELAX`/`MT_XSKELAX` 两个 A1 承载 core 未被本轮改动（TSV 可见两行仍在）。
- **`passed_min` 一致性**：`sampling-anti-monopoly.yaml` `passed_min: 26` + `output_contains "[  PASSED  ] 26 tests."`，filter 为 `SamplingBaselineTest.*` → 实跑该套件正是 **26**，一致。`level-rosters.yaml` `passed_min: 3` + `"[  PASSED  ] 3 tests."`，binary `level_roster_baseline_test` → `--gtest_list_tests` 正是 **3**，一致。本轮未增删用例，两处 `passed_min` 无需变动，报告说法正确。
- **行尾**：两个 TSV 与两个 `.cpp` 实测 CRLF，`level-rosters.yaml` 实测 LF，符合 `.editorconfig`（禁令 7）。
- **工作区洁净**：`git status --porcelain` 空，HEAD = `7364e0fa7`，本轮复审未改动任何受版本控制文件。

### 8. 新破坏 — 未发现 Critical / Important

本轮 diff 只触及 2 个 TSV + 1 个 yaml 描述 + 2 个测试文件，**零生产代码改动**（`Source/` 无 hunk）。数据改动的全部效应都被既有六道守卫 + 两道新守卫覆盖，且我实测全绿。

一个需要说明的观察：`PlacedClassMixWithinBaseline` 运行中打印一条 UBSan 诊断 `Source/levels/drlg_l2.cpp:2072: index 40 out of bounds for type 'unsigned char [40]'`。我核实这是**既存越界、与本轮无关**：`FixTilesPatterns()` 的 `dungeon[i][j+1]` 在 `j == DMAXY-1` 时越界，该函数上次改动是 `53b91fd7a "Untangle more dependencies"`（远早于本特性），且属 catacombs（L5-8）地形生成、不在名册路径上。它在 L5 与 L6 之间被触发，因为本轮 L8 的 `max_image` 抬高使更多 seed 走到该 catacombs 代码路径打印了一次——**是曝光而非引入**。全量门禁日志（`build/Testing/Temporary/LastTest.log`）里也只有这 1 条。归为范围外观察（非阻塞），建议单独开单。

---

## 修复 Diff 中的新增破坏

**无 Critical / Important。**

Minor（非阻塞，均可留到后续波）：
1. `tail_draw ≤ 4` 的隐式上界只记录在任务报告里，代码/规格无留痕（见裁决 6b）。
2. L2（7）/L3（6）/L8（6）的 core 数超出规格附录 C 的"每层 4-5 只"初稿约束；附录 C 自称待调整、且无测试约束，但规格散文与出厂数据现已不一致，建议同步附录 C 或在附录 E 记账。
3. 报告第 3 条顾虑"L8 不再有保障的远程 core"表述过悲观：实际保留了 RangedKite core `MT_YMAGMA`，失去的只是 RangedTurret 子类（见裁决 6a）。事实无误，措辞需修正。

---

## 范围外观察

- **既存 UBSan 越界** `drlg_l2.cpp:2072`（`FixTilesPatterns` 在 `j+1 == DMAXY` 越界），本轮曝光而非引入，建议单独开单。
- **L14 余量仍只有 0.39pp**（0.604908 vs 0.608760）：本轮未动 L14，是评审已认可的既有支付，但它现在是全表最脆的一层。
- **`level-rosters` eval case 132s**：报告论证"不用 `SAMPLING_REPORT` 降 seed 数"的理由（5 seeds 抖动足以让真实超标漏网、等于把门禁降级成抽查）我认为**成立**，阈值型用例不该按报告型用例的约定降级。
- 已被裁决延期项（I3、I1、R11、R27、R35、clang-format、O2、L17-24 legacy、7 条轻微）本轮未触碰，我复核后也**不认为其中任何一条实为严重/重要**，不重开。

---

## 裁决

**是否可合并？ 是。**

R40 已按裁决实质落地：四层占比全部回到带内（独立实测 L2 0.1017/L3 0.1283/L4 0.1894/L8 0.2588，余量 2.4-4.0pp），16 个 ceiling 逐位未动（从 A-baseline 原始整数重算三方比对零偏差），`kKnownR4Breaches` 整表删除且无任何豁免残留，组合数四层显著上升（117→360/184→439/147→448/66→116）而非压塌，L13-15 的 6/39/4 与 unique 可达性 2/5、6/6、2/2 未受影响，R41 的两段说明已按要求补齐。三个测试二进制在当前 MPQ 上实跑 29/28/3 全绿，`passed_min` 与真实用例数一致，行尾合规。

上列 3 条 Minor 建议在后续波处理，**不构成合并阻塞**——其中 `tail_draw` 上界一旦被撞是红灯而非静默错误，另两条是文档一致性。
