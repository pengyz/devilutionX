# 全分支最终评审：逐层名册 阶段 A

- 区间：`90a7c386f..44e8bcb2e`（26 提交，24 文件，+2252/-46）
- 评审包：`review-90a7c386f..44e8bcb2e.diff`（146KB），按「数据表 → 加载单元 → 采样接入 → 三个测试文件 → eval/文档」分 6 轮阅读
- 只读评审：未改动工作区/暂存区/HEAD/分支。（评审中曾临时加过一个探针测试并已删除；`git status` 干净，`CMake/Tests.cmake` 已 `git checkout` 还原，临时 worktree 已 `worktree remove`。）
- 复核手段：除读 diff 外，独立用 python 重放校验逻辑与旧采样循环（base + `mods/hf` 两套 monstdat），并实跑三个测试二进制 + `check_drift.py`

## 结论摘要

三个测试二进制 **58/58 全绿**（`level_roster_test` 29、`sampling_behavior_test` 26、`level_roster_baseline_test` 3），`check_drift.py` A/B/C/C2/E 全 PASS。代码质量、注释密度、测试判别力都明显高于一般水平，本会话此前抓过的"测试内重抄生产逻辑"这类占位用例**本轮未再发现**（见"优点"第 4 条的正面证据）。

但发现 **1 个严重问题**：L13/L14/L15 的实现结果是**每个 seed 完全相同的固定组合**，连带使这三层的**绝大多数 unique 怪物永远无法出现**。这不是阈值/风格问题，是玩家可见的内容回归，且现有守卫（含那道占比阈值）**结构上无法发现它**。建议修复后再合并。

---

## 优点

1. **单一真相源做得彻底、可验证**。`BehaviorClassCapForLevel`（`Source/tables/level_roster.cpp:138-148`）被校验器（`:248`、`:284`）与采样循环（`Source/monster.cpp:3569`）**同一函数**调用，旧的 `capKite`/`capSameClass` 布尔对已彻底删除。我逐位对照旧实现，语义等价（L13-16 任意类 ≤2、L9-12 kite ≤2、其余无限制），争议点 1 的"层段边界一致性"**成立**。
2. **L16 不可达处理正确且到处交代清楚**。`GetLevelMTypes()` 在 `Source/monster.cpp:3443-3448` 提前 return，故名册 L16 行仅登记；`level_roster.h:98-103` 明确写了"REGISTRATION ONLY"、params 故意不给 L16 行；测试 `RosterCoreAlwaysPresent` 只对 L16 断言静态半边并跳过实现半边（`test/sampling_behavior_test.cpp:841`）。三处说法一致，没有"注释说一套代码做一套"。
3. **加载期校验的顺序与模式划分是想清楚了的**。类型存在性检查放在 `gbIsSpawn` 分支**之前**（`level_roster.cpp:171-174`），因为 `MT_INVALID` 能被 `enum_cast` 解析出来 → 否则宽松模式下会漏出越界索引；这一点还有专门用例（`ValidationRejectsAnOutOfRangeMonsterIdInSpawnMode`）。core-vs-cap 检查（R29，`:239-257`）识别到了"core 绕过 cap 所以数据能破坏代码已无法防御的保证"这层，把它挡在加载期，判断准确。
4. **测试判别力有主动自证，不是自我认证**。`RosterQuotaAllowanceIsBinding`（`test/sampling_behavior_test.cpp:963-1018`）不硬编码层号，而是**从出厂数据搜索**"core 正好填满某受限 class"的层，再断言 `maxRealised == allowance`——即证明这个界是**紧的**、能发现"多一个类型"的回归。`EnginePreAddExemptionTracksQuestAvailability` 进一步证明豁免会**跟随**任务可用性（挂到别的层不给额度）。这两条正面回答了争议点 3：豁免口径**保有判别力**。
5. **R28 legacy fallback 的两个用例先证前提再证结论**。`LevelsWithoutParamsStillSampleTypes` 依次断言 overlay 已挂载、该层确无 params 行、确无 core 行、候选池非空，**然后**才断言 `scatterTypes > 0`；`NoParamsTailExceedsTheParameterisedCap` 用"实测 scatter 数 > L1-16 最大 tail_draw"证明 fallback 真的是 `INT_MAX` 而非某个小值。争议点 2 的"是否真的保护了 L17-24"**成立**。
6. **测试全局态隔离是本轮的亮点**。`HellfireNoParamsSamplingTest`（`:1151-1213`）对 `Quests`/`Players`/`sgGameInitInfo`/`gbIs*` 做**快照-还原**而不是猜初值，注释里点明了动机（R30：`InitQuests()` 会把 `_qactive` 抬离 `QUEST_NOTAVAIL`，Q_WARLORD 残留会给 L13 加第 3 个 Melee → 只在 `--gtest_shuffle` 下暴雷）。`LevelRosterFixtureLoadTestBase` 对 `AssetsPath` 做 per-test 存取，消掉了顺序依赖。
7. **文档诚实**。规格附录 E 主动记录了"L14 `tail_draw=2` 是可行替代解、只是余量 1.2pp 故不采纳"，以及 O2 缺口；台账 `decision_save_format_policy.md` 加了 `levelType` 语义变化行。这种"把没做的和为什么没做写清楚"的写法值得保持。
8. **数据表在两套 monstdat 下都自洽**。我用 python 独立重放校验逻辑：62 条 core 行在 base 与 `mods/hf` 下**全部可用**，无 core>cap，无 floor 不可满足 → `LuaReloadActiveMods()` 的 Hellfire 重载路径不会因名册 `app_fatal`。这一点原报告没覆盖，实测通过。

---

## 问题

### 严重（必须修复）

#### S1. L13/L14/L15 变成"每个 seed 完全相同"，且该层多数 unique 永久无法出现

**位置**：`assets/txtdata/monsters/level_roster_params.tsv:14-15`（L13/L14 `tail_draw=1`）与 `assets/txtdata/monsters/level_rosters.tsv:50-58`（L13 仅 2 core、L15 4 core），机制在 `Source/monster.cpp:3579-3599`（floors 优先于 RNG）。

**实测证据**（我加临时探针跑 500 seeds/层，随后已删除该探针）：

| 层 | 500 seeds 下不同组合数 | 固定组合 |
|---|---|---|
| L1 | **1** | Zombie / Fallen One / Skeleton / Fallen One / Scavenger / Skeleton Captain / Golem |
| L2-L12 | 20-175（正常） | — |
| L13 | **1** | Lava Maw / Guardian / Black Knight / Golem |
| L14 | **1** | Lava Maw / Vortex Lord / Balrog / Snow Witch / Golem |
| L15 | **1** | Balrog / Azure Drake / Snow Witch / Magistrate / Golem |
| L16 | 1（硬编码，本来如此，无回归） | — |

`MeasurementRosterIdentity` 的 P0-D 表同样显示 L13/L14/L15 的 **roster size 只有 4/5/5**（L12 是 13，L9 是 17）——这张表**就在本次测试里、每次都在跑**，但没有任何断言盯它。

**根因**（三者叠加，缺一不可）：
1. L15 的 4 个 core 恰好把 Melee 和 RangedTurret **双双顶到 cap 2**，加上 Golem(Boss)，候选池里**再无任何类别有余量** → 尾池被 cap 过滤器清空 → `tail_draw=3` 一次也用不上。
2. L13/L14 `tail_draw=1`，且各自留了**恰好一条未满足的 floor**（L13 `RangedKite=1`、L14 `Melee=2`）。floors 逻辑（`monster.cpp:3583-3599`）在 RNG **之前**选类型，且在候选里取**第一个**匹配项（`typelist` 顺序由 `MonstersData` 枚举序决定，与 seed 无关）→ 唯一那次抽取被完全确定。
3. `tail_draw` 减到 1、core 减到 2/3 正是任务 4 为压占比阈值所做的 R4 数据调整——**守卫压住了占比，代价转移到了多样性，而没有任何守卫盯多样性**。

**为何重要（玩家可见，且比多样性下降更严重）**：`PlaceUniqueMonsters()`（`Source/monster.cpp:507-513`）要求 unique 的 base 类型**已在 `LevelMonsterTypes` 里**，否则 `continue` 跳过该 unique。组合固定 ⇒ unique 可达性从"按 seed 概率"退化成**二值常量**：

| 层 | 改动前可达 unique（3000 次重放旧循环实测） | 改动后可达 |
|---|---|---|
| L13 | 5/9（Blackskull、Rustweaver、Doomcloud、Witchmoon、Gorefeast） | **0/9 靠采样**；仅 Warlord of Blood 靠 quest 预加 |
| L14 | **6/6** | **0/6 靠采样**；仅 Lachdanan 靠 quest 预加 |
| L15 | 2/3（The Vizier、Bloodlust） | **0/3** |

即 **Blackskull / Rustweaver / Doomcloud / Witchmoon / Gorefeast / Fangskin / Graywar the Slayer / Dreadjudge / Stareye the Witch / Steelskull the Hunter / The Vizier / Bloodlust 这 12 只 unique 在地狱段永久消失**（各自原本的 `mlevel` 层已不再产出其 base 类型）。这直接撞宪章：红线"开关关=原版"与"不改变玩家可记概念"——unique 怪是 D1 老玩家最在意的可见内容之一，而阶段 A 是 **Base/常开**、无开关可退。

**为什么现有守卫全都发现不了**：
- `PlacedClassMixWithinBaseline` 只看远程**占比**。固定组合的占比是个确定值，照样落在带内（实测 L13 23.6%）——**占比守卫对"多样性塌缩"完全不敏感**。
- `IdentityGuard` 用 Jaccard `< 0.9` 比**相邻层**，比的是"两层是否互相可替换"，不是"同层跨 seed 是否有变化"；L13 vs L14 的 Jaccard 是 0.3，轻松通过。
- `RosterTailDrawBounded` 断言 `tail <= tail_draw`，是**上界**；`tail == 0` 恒满足。
- `RosterQuotasSatisfied` 断言 floors 被满足、caps 未破；固定组合两者都满足。
- 规格附录 E 第 1 条**记录了物种数偏低（≈4-5 低于设计目标 7-9）**，但把它当作"多样性归阶段 B"的取舍留下了；**没有人注意到它已经退化到 1 种组合、并波及 unique 可达性**。这是本次评审认为规格/裁决层面判断失准的地方：R32/R34 权衡的是"物种数少一点"，实际发生的是"零随机性 + 12 只 unique 消失"，两者不是同一量级的取舍。

**如何修复**（按侵入性从小到大，任选其一即可解除阻塞；建议 ①）：
1. **只改数据**：给 L13/L14/L15 补 `tail` 行或抬 `tail_draw`，并让 core 不要把 cap 占满。附录 E 已记录 L14 `tail_draw=2` 实测 0.5964 < ceiling 0.60876 可行；L15 需把 4 core 减到 3（腾出 1 个类别余量）；L13 需在保住 22.8%+5pp 的前提下加 1 条 Melee tail（Melee 不计入远程分子，理论上不冲阈值）。改完必须重跑占比门禁。
2. **补一道多样性守卫**（无论选哪种修法都建议加）：断言"每层 200 seeds 的实现组合数 ≥ 2"或"roster union size ≥ core 数 + 1"，并对 L1-15 生效（L1 现在也是 1，见 M1）。这道守卫本该与占比守卫成对存在——占比防"变成远程画廊"，多样性防"塌成一个固定组合"。
3. **补一道 unique 可达性守卫**：断言每层 `mlevel` 命中的 unique 中，base 可被采样到的比例不低于改动前基线。这条最贴近玩家感知，也最能防住后续名册编辑再次踩坑。

若控制者认为"L13-15 固定组合"是**有意接受**的阶段 A 代价，那也必须：把它写进规格（现在附录 E 的表述不足以涵盖"零随机 + unique 消失"）、在台账里按玩家可见行为变更登记、并明确 unique 消失是否可接受。但以宪章红线口径看，我认为不该在阶段 A 常开路径上接受。

### 重要（建议修复）

#### I1. L1 也已塌成固定组合（同类问题，但成因不同）

**位置**：`level_roster_params.tsv:2`（L1 `tail_draw=2`、`Melee=2`）+ `level_rosters.tsv:2-5`（4 core）。

实测 L1 在 500 seeds 下同样只有 **1 种组合**，P0-D 的 roster size = 7。L1 候选池本就小（shareware 起始层），旧行为也不算丰富，但改动前 P0-D 记录的 L1 roster size 同样是 7 —— 所以**L1 大概率不是回归**，属于既有状态被名册固化。与 S1 分开列在"重要"而非"严重"，因为没有 unique 可达性损失（L1 无 unique 命中）。仍建议在 S1 的多样性守卫里一并覆盖 L1，避免"守卫写了但把唯一已知的边界层排除掉"。

#### I2. `RosterTailDrawBounded` 的 pre-add 计数会低估，掩盖尾抽超额

**位置**：`test/sampling_behavior_test.cpp:878-882`。

```cpp
size_t preAdded = 1; // MT_GOLEM
preAdded += AvailableCoreCount(level);
const size_t tail = LevelMonsterTypeCount > preAdded ? LevelMonsterTypeCount - preAdded : 0;
```
`preAdded` 只算 Golem + core，**没算 quest 预加**（`monster.cpp:3464-3475` 那 6 处）。本套件里恰好没有任务处于 available（`SetUpTestSuite` 不调 `LoadQuestData()`），所以现在数值对得上；但一旦以后有人在这个套件里激活任务（`QuestPreAddRePickDoesNotDoubleCount`、`EnginePreAddExemptionTracksQuestAvailability` 已经在同套件里临时激活 `Q_VEIL` 了），`preAdded` 就会偏小、`tail` 偏大，`EXPECT_LE(tail, tailDraw)` 会**误红**；反过来若配合 `LevelMonsterTypeCount > preAdded ? ... : 0` 这个夹逼，也可能把真实超额吞成 0。建议直接复用同文件已有的 `EnginePreAddClassCount` 思路，把 quest 预加计入，或显式断言"本套件无任务可用"这一前提。

#### I3. 名册未登记 tail 行，`LevelRosterRole::Tail` 在生产中是死代码

**位置**：`assets/txtdata/monsters/level_rosters.tsv` 全表（62 行**全部** `core`）；消费侧 `monster.cpp:3503-3510` 只处理 `Core`，`Tail` 分支只在测试与解析器里出现。

规格 §4.1 的示例表也只给 core，故不算偏离计划；但结果是 `role` 这一列目前**没有任何生产语义**——尾池实际是"候选池减去已加入类型"（`monster.cpp:3534`），与名册里标不标 `tail` 无关。这会让后续维护者误以为"写一行 `tail` 能把某怪放进尾池"，实际上写了也不起作用（甚至因为 `role != Core` 而被 core 预加跳过、又因已在候选池里而照常参与随机抽取）。建议二选一：要么在解析期拒绝 `tail` 行并注明"阶段 A 不支持"，要么在 `.h` 里明确写清 `Tail` 当前无生产效果、留待哪一阶段启用。修 S1 若采用"补 tail 行"的方案，则需先给 `Tail` 补上真实语义。

#### I4. `max_image` 从 4000 抬到 6000/9000/16000/18000，是玩家可见的种类总量上升，但只有占比守卫

**位置**：`level_roster_params.tsv` 全表；消费侧 `monster.cpp:3521`。

规格 §4.1 给了依据（"L13 最重 6 种 core 合计 13406""内存不是约束"），属计划内。但这是**每层怪物种类上限**的普遍放宽：L2-L12 实测组合数与 placed 构成都明显变化（如 L5 Sneak 从 1904 → 3378、L11 Melee 从 6447 → 11046，见我实跑的 post-change 表 vs 计划 A-baseline 表）。红线 14 的守卫只覆盖 **L13-15 的远程占比**，L1-12 的构成变化**完全没有量化守卫**。考虑到 core 预加在 L11 已占 7656 图像预算、旧 4000 装不下 4 个 core，抬预算是必需的；但建议把占比守卫扩到 L1-12（阈值表已经预留了 `kRangedShareUnconstrained` 哨兵位，填基线即可），否则"改动前后玩家感受差异"在 3/4 的层上无据可查。

### 轻微（可选优化）

#### N1. `MeasurementRealisedPerLevel` 报告标题仍写死 "sprite budget 4000"
`test/sampling_behavior_test.cpp:539`。预算已逐层化（6000-18000），标题会让读表人误判口径。改成"per-level budget"即可。

#### N2. `CavesAnyClassTailBaseline` 的 L9 容差偏脆（已知延期项，确认为轻微）
`test/sampling_behavior_test.cpp:227`：`EXPECT_NEAR(l9, 59.93, 0.6)`。L9 恰在"每类 ≤2"的容量边界上，是唯一非 0/100 的层，任何无关采样微调都可能推翻它。注释已说明成因，10000 iterations 也压住了统计噪声，故只是脆而非错。建议放宽到 ±2.0 或改断"落在 (0,100) 开区间且 L10-12 为 100"。

#### N3. `ParseClassFloors` 用 `app_fatal` 而非 `RecordReader` 错误通道（已知延期项）
`Source/tables/level_roster.cpp:68/73/77`。丢掉文件名/列名/行号上下文，出错时排查成本高于同仓其它加载器。属工具层一致性问题，不影响正确性。

#### N4. `ParseClassFloors` 接受 `255`（R27，已知）
`level_roster.cpp:74-78` 用 `uint8_t` 且不设上限，`255` 能解析通过，随后被可满足性检查挡住。防御深度够，但错误信息会指向"floor 不可满足"而非"floor 写错了"。可选：加一条 `floor > MaxLvlMTypes` 的早期拒绝。

#### N5. `LogLoadedRosterSummary` 遍历 `Params`，漏诊"只在 `Entries` 出现的层"（已知）
`level_roster.cpp:305`。注释解释了选 `Params` 是为了 A2 自动覆盖，但 L16 这种"有 entries 无 params"的层因此不会出现在 verbose 诊断里。可选：改成遍历 `entries ∪ params`（校验器 `:215-221` 已有该并集，可复用写法）。

#### N6. 基类注释 "once per binary" 与实际两套件不符（已知）
`test/level_roster_test.cpp:362-370` 说 `SetUpTestSuite` 每个二进制只跑一次，实际有两个 fixture 各跑一次。注释自己也承认"repeated calls just log a benign warning"，属措辞不精确。

#### N7. `R35` 硬编码用例计数
`eval/cases/rng/level-rosters.yaml` 与 `sampling-anti-monopoly.yaml` 的 `passed_min` / `"[  PASSED  ] N tests."` 是手工计数。我核对过：`level-rosters` 3 个用例对得上；`sampling-anti-monopoly` 的 24 也对（filter `SamplingBaselineTest.*` 排除了 2 个 Hellfire 用例，文件里共 26 个）。两处都有醒目维护提醒，属工具层问题（eval backend 缺"用例数=声明数"的强校验），不阻塞。

---

## 延期项分拣

### 必须合并前处理
- **S1（新发现，非原延期项）**：L13-15 固定组合 + 12 只 unique 永久消失。这是唯一的合并阻塞项。
- **I2**：`RosterTailDrawBounded` 的 pre-add 计数漏 quest。改动小（几行），且同套件已经在激活任务，风险是现实的而非理论的。
- **规格附录 E 第 1 条的表述**：需要按 S1 的实测重写。现在写的是"物种数 ≈4-5 低于目标 7-9"，实际是"1 种组合 + unique 可达性归零"。无论 S1 怎么修，这条记录都必须与事实对齐——否则阶段 B 会基于错误的严重度评估做决策。

### 可留后续（认可原裁决）
- **R11**（测量夹具省略 `HoldThemeRooms`/`InitThemes`/`InitGolems`/`InitObjects`）：阈值是"相对本夹具基线"，同口径相抵，且 `CreateDungeonForMeasurement` 已补齐 SOLData/til/trigs 三项关键状态（这三项才是"所有层放置数恒等"的病根，已沉淀到 `docs/knowledge/gotcha_level_state_when_bypassing_loadgamelevel.md`）。可留。
- **任务 1 轻微（L16 恒定 placed 值无回归保护）**：L16 走硬编码分支，`Level16HardcodedTypes` 已锁死 4 个类型；placed 数无守卫属可选。可留。
- **R27 / N3 / N4 / N5 / N6 / N1**：均为诊断质量与措辞，不影响正确性。可留。
- **N2（L9 容差）**：脆但当前稳定通过，且 10000 iterations 已压噪声。可留，但若 S1 按方案 ① 改了 L13-15 数据，**需重跑确认 L9 未被带偏**。
- **R35（`passed_min` 硬编码）**：eval 工具层，两处均有维护提醒。可留。
- **clang-format 14 vs CI 18**：工具层环境问题，实现者本轮已自解 18.1.8 验证。可留。
- **O2（校验未把 quest 无条件预加计入 cap 合计）**：属阶段 B。注意 O2 与 S1 无关（O2 是"cap 可能被 quest 顶破"，S1 是"根本没东西可抽"），两者方向相反。可留。
- **L17-24 走 legacy（阶段 A2 补表）**：R28 fallback 已被两个非自证用例证明有效，且我实测 hf 数据下 L17-24 候选池非空、旧 4000 预算可容 4-5 种。可留。

---

## 改进建议

1. **守卫要成对**。这次的教训很清楚：只加了"占比不许涨"的守卫，压占比的数据调整就把代价转移到了"多样性"这个无人看守的维度。凡是用数据参数压某个指标，就要问一句"被压下去的量会跑到哪个指标上"，并给那个指标也留一道守卫。S1 的修复建议 ②③ 就是这道对偶守卫。
2. **已有的测量表就是现成的守卫底座**。`MeasurementRosterIdentity` 的 P0-D 表**每次都在跑**，L13/L14/L15 的 roster size 4/5/5 一直印在输出里，只是没有断言。把已有测量升级成断言，成本几乎为零，比新写一套 harness 划算得多。
3. **对"实现结果的随机性"直接建模**。这类采样特性天然该有"同层跨 seed 组合数 ≥ N"的断言。它比占比、比 Jaccard 都更贴近"这层是否还有意外感"这个设计意图。
4. **`Tail` 角色要么落地要么封禁**（I3）。留一个解析得了、校验得过、但生产无效果的枚举值，是后续踩坑的高发点。

---

## 总体评估

**是否可合并？** 修复后可合并

**理由：** 工程质量确实过硬——单一真相源真正被两侧共用、校验顺序与宽松模式划分经过推敲、测试有主动自证判别力（`RosterQuotaAllowanceIsBinding` 尤其好）、全局态隔离到位、58 个测试与漂移校验全绿，我独立重放校验逻辑与 Hellfire 数据也未发现新的加载期缺口。但 L13/L14/L15 已退化为**每 seed 完全相同的固定组合**，连带使这三层 **12 只 unique 怪永久无法出现**，而这是常开的 Base 类改动、无开关可退，且现有五道守卫（占比/相邻层 Jaccard/尾抽上界/配额/L16 硬编码）在结构上都盯不到这个维度——修数据或补多样性守卫即可解除，不需要返工架构。
