# Task 3 评审（采样接入 · 任务级门禁）

Base `33a656a7e` → Head `c62a07e0f`。以 diff 为准；报告中的设计理由只作线索，不降级。
只读评审，未改动工作区/暂存区/HEAD。

## 规格符合度

- ✅ **R15 落地**：内联 `capKite`/`capSameClass` 两个 bool **已删除**（diff 122-123 为 `-` 行），
  过滤统一走 `BehaviorClassCapForLevel(currlevel, cls)` + `cap != 0` 放行
  （`Source/monster.cpp:3567-3576`）。`Source/tables/level_roster.cpp:138-148` 的实现
  在 L13-16 返回 2、L9-12 仅对 `RangedKite` 返回 2、其余返回 0 —— 与被删掉的内联条件
  **逐层等价**，"超过才淘汰"语义（`count >= cap` 才移出候选）保持不变。旧逻辑没有残留。
- ✅ **R26 落地**：core 预加带 `IsMonsterAvailable(MonstersData[entry.type])`
  （`Source/monster.cpp:3506-3507`）。`IsMonsterAvailable`（`:3162-3171`）同时过滤
  `Never`、spawn 下的 `Retail`、并做 `currlevel` 层段检查；因为 core 预加在
  `currlevel` 已被赋值之后，层段检查有效。shareware 不会被预加占预算。
- ✅ **顺序陷阱避开**：core 预加在骷髅预加之后（`:3503`）、`typelist` 声明之前（`:3524`）；
  `classCounts` 初始化在 `:3548-3551`，**位于 core 预加之后**，从
  `LevelMonsterTypeCount` 起算 → core 被计入 caps 起算值，caps 不会被低估。
- ✅ **R28 落地**：无 params 行时 `maxImage = 4000`、`tailDraw = std::numeric_limits<int>::max()`
  （`:3521-3522`）；`<limits>` 已在 `Source/monster.cpp:19` 存在，不依赖传递包含。
  规格/计划同步改到 §4.2.4/§4.2.6（diff 288-291）。
- ✅ **L16 不动**：`:3443-3448` 的硬编码分支与提前 `return` 未被触碰；core 预加位于
  `if (!setlevel)` 内、L16 早退之后。`Level16HardcodedTypes` 用例保留。
- ✅ **尾池扣除**：`GetMonsterTypeIndex(i) != LevelMonsterTypeCount`（`:3534`）。
  `GetMonsterTypeIndex`（`:383-390`）未命中返回 `LevelMonsterTypeCount`，语义正确；
  比简报要求的"减去 core"更严（也减去 Golem/任务 unique），这是合理收紧，不是缺陷。
- ✅ **`tailAdded` 只在真正新增时自增**（`:3605-3608`，`LevelMonsterTypeCount != countBefore`
  守卫内）。配合 `:3534` 的去重，重复抽取不会白耗额度。
- ✅ **CRLF**：`level_rosters.tsv` 与 `level_roster_params.tsv` 实测为 CRLF（`cat -A` 见 `^M`）；
  diff 未见行尾整体重写。
- ✅ **未删改 `CMake/Tests.cmake` 注册行**（diff 文件清单里没有该文件）。
- ✅ **未产 eval**：符合"eval 归任务 4"的划分（见下方"评估"的缺口确认）。
- ⚠️ **无法仅凭 diff 验证**：全量门禁（`ctest.passed_pct == 100` / `drift_ok`）与
  `--gtest_shuffle` 下的套件顺序稳定性。控制器应实跑
  `python3 tools/run_tests.py --json /tmp/ci.json`，并额外跑一次
  `cd build && ./sampling_behavior_test --gtest_shuffle --gtest_repeat=3`
  以证伪下面问题 2 的偶发失败风险。

## 优点

1. **R15 是真替换而非并存**：两处 `const bool cap*` 定义连同 `capSameClass ? ... : ...`
   三元判定整段删除，采样循环里只剩单一真相源调用。验证器
   （`level_roster.cpp:255`）与采样循环现在共用同一函数，漂移面被真正关闭。
2. **顺序陷阱处理正确且写进了注释**（`:3492-3498`），并明确解释了"core 计入 classCounts
   所以 caps 不会被低估"，这正是最容易被后来者改错的地方。
3. **R28 的处理超出"改个常量"**：`HellfireNoParamsSamplingTest` 用真实引擎在 L17-24
   上断言 `PLACE_SCATTER` 类型 > 0，并且 `NoParamsTailExceedsTheParameterisedCap`
   用"最大 tail_draw"做上界对比 —— 后者对 `tailDraw = 0` 与任何"小常量 fallback"
   都会失败，不是同义反复。三条前提断言（params 为 nullptr / roster 为空 / 候选非空）
   把"空断言通过"的路堵住了，是本 diff 里质量最高的测试设计。
4. **测试夹具顺序被显式复刻**：`level_roster_baseline_test.cpp` 与
   `sampling_behavior_test.cpp` 都补了 `LoadLevelRoster()` 并注释了"必须在
   `LoadMonsterData()` 之后"的原因，避免了"名册全空 → 基线全零"的静默失效。
5. **floors 实现符合规格 §4.5 的"只补足"限制**：`preferred` 只在
   `classCounts[cls] < floor` 时置位，且 floors 生效点在 caps 过滤**之后**
   （`:3567-3576` 先剔除超 cap 者，`:3583` 才选 preferred），所以 floors
   在结构上无法把某类别顶到 cap 之上 —— "不得用 floors 提高远程占比"这条得到了
   代码级保证，而不只是注释承诺。

## 问题

### 严重（必须修复）

**S1. B1 caps 的直接契约断言恒真：阈值由被测数据自己提供，core 违反 caps 无人拦截**
`test/sampling_behavior_test.cpp:825-864`（`RosterQuotasSatisfied`）+
`Source/monster.cpp:3503-3510`（core 绕过 caps）。

caps 断言写成 `EXPECT_LE(counts[i], std::max(cap, exempt))`，其中 `exempt` =
Golem + 该层可用 core（`CapExemptClassCount`，:838-853）。L13-15 的 cap 是"任意类别 ≤ 2"，
而 core 恰好每层 4 个：实测 core 类别分布为

| 层 | core 类别分布（+Golem=Boss 1） | cap | max(cap,exempt) |
|---|---|---|---|
| 11 | Kite 2, Melee 2 | Kite 2 | 2（未放宽） |
| 12 | Kite 2, Melee 2 | Kite 2 | 2（未放宽） |
| 13 | Melee 2, Kite 1, Turret 1 | 任意 2 | 2（未放宽） |
| 14 | Turret 2, Melee 1, Kite 1 | 任意 2 | 2（未放宽） |
| 15 | Melee 2, Turret 2 | 任意 2 | 2（未放宽） |

我实测了当前数据下每层 core 的类别分布（含 Golem 计入 Boss）：**`exempt` 最大恰好等于
`cap`（=2），从未超过**。也就是说 `std::max(cap, exempt)` 今天没有掩盖任何真实越界，
当前实现是干净的。**问题在于断言的阈值由被测数据自己提供**：只要以后某层 core 在同一
类别放 3 个，`exempt` 自动升到 3，这条 caps 断言就跟着放宽到 3，**永远不会失败**。
这正是"B1 保证被架空"的形态——一条恒真的契约断言。

更关键的是**你要求核对的那一问**：若某层 core 本身违反 B1 意图（如 L11 曾有 3 个 kite
core：STORML/BMAGMA/BACID），当前测试**发现不了**。证据链：
- `RosterQuotasSatisfied` 会因 `exempt = 3` 而自动放宽，不报错；
- `CavesKiteTailBaseline`（`:171-190`，L9-12 kite ≥3 必须为 0%）**会**报错——
  这是唯一真正的守卫；
- 而本 diff 正是把 `11 MT_BACID` 改成 `11 MT_OBLORD`（diff 219-220，kite→Melee）
  才让它过的。也就是说 **L11 的修法是靠 `CavesKiteTailBaseline` 恰好失败倒推出来的**，
  不是被一个"core 必须自洽于 caps"的规则拦住的。
- L13-15 没有等价的守卫：`HellL1{3,4,5}SameClassTailBaseline` 断言
  `SameClassTailPercent == 0`（任意类别 ≥3 的层占比为 0），它**确实**能抓住
  "core 让某类别到 3"的情况（`RunSampling` 走真实引擎、统计含 core）。所以 L14 的
  `MT_BALROG`→`MT_SNOWWICH` 同样是被这条挡下来倒推的。

**为何重要**：B1 的守卫现在全部来自"逐层基线百分比用例"这类**间接**断言，而**直接**的
caps 断言（`RosterQuotasSatisfied`）被 `exempt` 自我豁免掉了。一旦某层的基线用例
未覆盖（例如未来 A2 给 L17-24 配名册，那里既无 `CavesKiteTail*` 也无
`HellL*SameClassTail*`），core 违反 caps 将**完全无人拦截**。

**如何修复**（择一，建议第 1 项）：
在 `ValidateLevelRoster`（`Source/tables/level_roster.cpp:213+`）加一条**加载期**校验：
对每层统计 `role == Core` 且可用的成员按 `GetBehaviorClass` 分组，若某类别数量 >
`BehaviorClassCapForLevel(level, cls)`（且 cap != 0）则返回错误。这样 core 自洽性由
单一真相源在数据侧强制，不再依赖某条百分比基线"恰好失败"。
随后把 `RosterQuotasSatisfied` 的断言改回 `EXPECT_LE(counts[i], cap)`（去掉 `exempt`），
让它成为真正的契约；并补一条针对该校验的负向用例（构造 3-kite 的 L11 名册 → 期望
`ValidateLevelRoster` 返回错误），以证明守卫存在而不是靠数据巧合。

### 重要（建议修复）

**I1. `HellfireNoParamsSamplingTest` 的全局状态恢复不覆盖 `SetUpTestSuite` 的
早退路径，且对 `SamplingBaselineTest` 的隔离依赖套件执行顺序**
`test/sampling_behavior_test.cpp:955-981`。

- `SetUpTestSuite` 在 `!HaveMainData()` 时 `return`，此时 `gbIsHellfire`/`gbIsSpawn`
  尚未被改，无害；但正常路径设置了 `gbIsHellfire = true` 并挂载 hf overlay，
  恢复只在 `TearDownTestSuite` 做。gtest 保证同一套件的 TearDownTestSuite 会执行，
  所以顺序安全性主要取决于**恢复是否完整**，而不是是否执行。
- 恢复动作是 `UnloadModArchives(); LoadModArchives({}); LoadMonsterData(); LoadLevelRoster();`
  但**没有恢复 `paths::SetPrefPath`**（`TestInitGame` 内部不动 PrefPath，但
  `level_roster_baseline_test` 是另一个二进制，无影响）；也**没有恢复
  `sgGameInitInfo.fullQuests`/`gbIsMultiplayer`**（`TestInitGame` 会把
  `gbIsMultiplayer = !fullQuests` 设为 false，与 `SamplingBaselineTest` 期望一致，
  故当前无害）。真正的残留是 **`TestInitGame` 调用了 `InitQuests()`**，把
  `Quests[*]._qactive` 从 `SamplingBaselineTest::SetUpTestSuite` 的状态改成了
  `QUEST_INIT`（单人路径，`Source/quests.cpp:83-89`）。已确认 `Quests` 是全局
  `Quest Quests[MAXQUESTS]`（`Source/tables/questdat.cpp:17`），静态零初始化 →
  `_qactive == 0 == QUEST_NOTAVAIL`（`questdat.hpp:63`），这正是
  `SamplingBaselineTest` 依赖的默认态（它只在 `QuestPreAddRePickDoesNotDoubleCount`
  里临时改 `Q_VEIL`，并在结尾 `Quests[Q_VEIL] = {}` 复位）。而
  `GetLevelMTypes` 的任务 unique 预加走 `Quests[...].IsAvailable()`
  （`Source/tables/questdat.cpp:64+`：`_qactive != QUEST_NOTAVAIL` 且 `currlevel == _qlevel`）。
  于是**在 `HellfireNoParamsSamplingTest` 之后运行的 `SamplingBaselineTest` 用例，
  会多出 Garbud/Zhar/SnotSpill/Lachdan/WarlordOfBlood 的预加**，直接改变 L9-15 的
  类别构成 → `HellL1{3,4,5}SameClassTailBaseline`（`EXPECT_EQ(tail, 0.0)`）与
  `CavesAnyClassTailBaseline`（`EXPECT_EQ(..., 100.0)` / `EXPECT_NEAR(l9, 59.93, 0.6)`）
  是这类污染的高危对象。默认执行顺序下 `SamplingBaselineTest` 在前（先定义），所以
  现在过；**`--gtest_shuffle` 或将来单独 `--gtest_filter` 跑 Hellfire 套件再跑基线，
  就会偶发失败**。
  具体泄漏面（`questdat.tsv` 的 `qdlvl` × `unique_monstdat.tsv` 的基底类型）：
  Q_GARBUD@L4、Q_ZHAR@L8（基底 MT_COUNSLR / RangedTurret）、Q_LTBANNER@L4
  （MT_BFALLSP）、Q_WARLORD@L13（MT_BTBLACK / **Melee**）、Q_VEIL@L14
  （MT_RBLACK / **Melee**）、Q_BUTCHER@L2（MT_CLEAVER）。其中 **Q_WARLORD@L13
  会给 L13 多加一个 Melee**，而 L13 的 core 已有 Melee 2（NBLACK+GUARD）——
  `HellL13SameClassTailBaseline` 断言 `EXPECT_EQ(tail, 0.0)`，Melee 达到 3 即
  失败。这不是理论风险，是一条可被顺序触发的确定性失败。
- 另外 `TearDownTestSuite` 未恢复 `gbIsSpawn`（两处都设 false，无害）与
  `MyPlayer`/`Players`（`TestInitGame` 会 resize 到 1，`SamplingBaselineTest` 不依赖，无害）。

**为何重要**：这是可预见的偶发失败源，且失败现场（"L15 same-class tail 从 0 变成
非 0"）与真实原因（另一个套件泄漏了 quest 状态）相距很远，排查成本高。

**如何修复**：在 `HellfireNoParamsSamplingTest::TearDownTestSuite` 里显式把
`Quests` 恢复到 `SamplingBaselineTest` 期望的状态（最简单：`for (auto &q : Quests)
q._qactive = QUEST_NOTAVAIL;`，与 `SamplingBaselineTest` 从不调用 `InitQuests`
的初始状态一致），或者反过来让 `SamplingBaselineTest::SetUpTestSuite` 变成幂等
（显式清 `Quests`），后者更健壮，因为它不依赖别人清理。并实跑一次
`--gtest_shuffle --gtest_repeat=3` 作为证据。

**I2. `RosterTailDrawBounded` / `RosterQuotasSatisfied` 的层范围被静默缩到 1-15，
与简报"1-16"不一致，且缩小理由未说明**
`test/sampling_behavior_test.cpp:797`、`:825`（简报写 `level <= 16`，实现写 `level <= 15`）。

L16 走硬编码早退，`GetLevelRosterParams(16)` 却**存在**（params 表有 16 行），
所以 `ASSERT_NE(params, nullptr)` 不会失败；真正会失败的是 tail/floors 断言
（L16 只有 4 个类型且不经采样循环）。缩到 15 是**正确的**，但代码里没有注释说明，
而 `RosterCoreAlwaysPresent` 用的是 `level <= 16` 加一个 `if (level == 16) continue;`
的显式处理（`:786-788`）。两种风格不一致，后来者容易误以为漏了 L16。

**为何重要**：低风险但会掩盖"L16 的名册/params 行到底该不该存在"这个真实设计问题——
params 表给 L16 配了 `max_image=18000, tail_draw=3, class_floors=Melee=2,RangedTurret=2`，
而这些值**永远不会被读**（早退在前）。这是"无法落地的表格数据"，与宪章禁令
"不写无法落地表格"的精神冲突。

**如何修复**：在两处循环加一行注释说明"L16 硬编码早退，无尾池/floors 语义"；
并考虑在 `ValidateLevelRoster` 或规格里明确 L16 params 行为"仅登记"（如同名册行），
避免后来者按表里的 `tail_draw=3` 推断行为。

### 轻微（可选优化）

**T0. `CavesAnyClassTailBaseline` 重新钉值合法、算术推导也正确；唯一残留是 L9 的
精确分布钉值偏脆**
`test/sampling_behavior_test.cpp:192-229`（diff 441-463）。

先给裁决：这条用例**是基线参照，不是 B1 契约**。注释（改动前后都）写明
"the any-class >=3 tail is NOT a cap target"；洞穴的 B1 契约是 kite ≤ 2，由
`CavesKiteTailBaseline`（`:171-190`）以 `EXPECT_EQ(..., 0.0)` 保证，**仍在且未被削弱**。
因此"类型总数从 3-7 变成 8 导致 any-class 尾巴上升"不是放宽阈值，重新钉值**合法**。

注释里的容量推导我独立复算过一遍，**是对的**（我第一次按 2×类别数算，得到 10/8/6/8，
与注释的 8/7/6/6 不符；但注释的容量定义是 Σ min(2, 该类别候选数)，受候选数封顶，
按此复算得 L9 8 / L10 7 / L11 6 / L12 6，与注释完全一致）。含被修正的 L12 容量
7→6 也对得上。于是鸽笼论成立：L10-12 实现 8 个类型而"全类别 ≤2"的容量只有 7/6/6，
必有某类别达到 3 → 100% 是结构必然，`EXPECT_EQ(..., 100.0)` 是安全的精确断言；
L9 容量 8 = 实现 8，需要恰好唯一分配才能不越界，落在边界上，与 59.93% 的观测吻合。

驱动机制值得补一句（注释没说透）：L9-12 的 caps 只约束 kite，其余类别 cap=0（无上限），
所以 kite 被封顶后尾池溢出必然流向 Melee/Boss 等无上限类别，这才是尾巴升到 100% 的
直接原因。注释用纯计数论表达了同一结论，不算错，但读者容易误以为"所有类别都被 cap 到 2"。

**为何重要（降级后）**：唯一实际风险是 L9 的 `EXPECT_NEAR(l9, 59.93, 0.6)`——它是纯分布
钉值，10000 次迭代下 ±0.6 的窗口对任何采样顺序/候选池微调都敏感，且 L9 正好处在
容量边界，是四层里唯一"有时装得下"的层，波动性天然最大。

**如何修复**：把 L9 的容差放宽到 ±2（仍足以抓住"回落到 1.7% 量级"的真回归），并在
注释里补一句"L9-12 仅 kite 受 cap，溢出流向无上限类别"以免误读。L10-12 的
`EXPECT_EQ` 保持不动。

**T1. floors 的补足是"每次循环只补一个类别的第一个候选"，非随机，可能引入固定顺位偏置**
`Source/monster.cpp:3583-3599`。`preferred` 取 `typelist` 中该类别的**第一个**元素，
而 `typelist` 的初始顺序是 `MonstersData` 的枚举顺序（被 `typelist[i] = typelist[--nt]`
的 swap-remove 打乱但非随机化）。结果：floors 补足位总是倾向于枚举序靠前的怪。
简报给的伪代码正是如此，所以不算偏离规格，但值得记录：floors 满足的那 1-2 个槽位
在同层不同 seed 间的多样性会低于随机抽取。若后续发现"每层的远程位总是同一只怪"，
根因在此。修复方式：在该类别的候选中 `GenerateRnd` 选一个而非取首个。

**T2. `IsRosterEntryAvailableAt` 是 `IsMonsterAvailable` 的第三份复制**
`test/sampling_behavior_test.cpp:766-773`（另两份在 `Source/monster.cpp:3162` 与
`Source/tables/level_roster.cpp:39`）。三份同语义谓词，与本任务 R15 追求的
"单一真相源"精神相反。测试侧复制有其理由（`IsMonsterAvailable` 是 monster.cpp 内部
链接、且需要 level 参数化），但注释里已经承认"identical to IsMeasuredCandidate()"，
说明同一 TU 内就已经有第四份。建议把带 level 参数的版本提升为
`level_roster.h` 的导出函数（例如 `IsMonsterAvailableAt(level, type)`），
让生产与测试共用。

**T3. 逐层预算实际放大约 2 倍，本任务是它首次对玩家生效，但无用例守住上界**
`Source/monster.cpp:3521` + `assets/txtdata/monsters/level_roster_params.tsv`。
`max_image` 从旧的全局 4000 变为逐层 6000（L1-4）/9000（L5-8）/16000（L9-12）/18000（L13-16）。
我按 `monstdat.tsv` 的 `image` 列实测 core+Golem 的固定占用：L1 2691、L8 5228、L11 7656、
L15 6856 —— 也就是说**L8 起 core 自身就已超过旧的 4000 上限**，加上 `tail_draw` 个尾池
类型后总量大致落在 7000-11000，相比旧引擎的 4000 上限约为 2 倍。这是本任务首次让该表
生效（任务 2 只发布数据），属真实的 sprite 内存增长。
好消息是它不会失控：`tail_draw <= 3` 与 `MaxLvlMTypes = 24`（`Source/monster.h:39`）
共同把实现类型数钉在 ~8，`fits` 余量（我算得 L1 约 8、L15 约 11 个最小候选）远未用尽，
所以 `max_image` 实际上**不是**限制项，真正的限制项是 `tail_draw`。
建议加一条用例断言每层 `monstimgtot <= params->maxImage`（当前无任何用例检查预算上界），
并在规格里记录"`max_image` 现为宽松上界，构成由 `tail_draw` 决定"，
否则后来者调 `tail_draw` 时会误以为预算仍在兜底。

**T4. `NoParamsTailExceedsTheParameterisedCap` 的比较对象略弱**
`test/sampling_behavior_test.cpp:1041-1076`。它断言"某个无 params 层的 scatter 类型数 >
L1-16 的最大 tail_draw(=3)"。由于 L18/19/20/24 有无条件 scatter 预加，
`bestScatter` 里混入了非尾池来源。改成只在**无 scatter 预加**的层
（L17/21/22/23，注释里已经点明了这批）上取 max，断言会更贴合"尾池无上限"这一命题。

## 评估

**任务质量：** 需修复

**理由：** R15/R26/R28 与顺序陷阱四项核心裁决**都真实落地**且证据清晰（内联 caps 已删除、
core 在 `classCounts` 之前预加、fallback 为 `int::max` 而非 0），R28 的两条用例走真实引擎、
三条前提断言堵住了空断言，非同义反复；`CavesAnyClassTailBaseline` 的重新钉值与注释里的
鸽笼推导我复算后**确认正确**（容量定义是 Σmin(2,候选数)，得 8/7/6/6，与注释一致，
含 L12 的 7→6 修正），且 B1 契约仍由 `CavesKiteTailBaseline` 与
`HellL1{3,4,5}SameClassTailBaseline` 的 `EXPECT_EQ(0.0)` 原样守卫——**未放宽阈值**成立。
两处名册数据修正也已独立核对：`MT_OBLORD`@L11（Rhino/Melee，10-13 层可用，L11 非其
unique 基底层）与 `MT_SNOWWICH`@L14（Succubus/RangedTurret，13-15 层可用，unique
基底在 L13 而非 L14）均正确、最小、未触碰任何阈值。

两处必须修的问题：一是 B1 的**直接**契约断言被 `std::max(cap, exempt)` 架空（S1）——
当前数据下 `exempt` 恰好都等于 cap=2、没有掩盖真实越界，但阈值由被测数据自己提供，
一旦某层 core 在同类别放 3 个，断言会自动放宽而永不失败；今天 core 违反 caps 只能靠
某条百分比基线"恰好失败"倒推发现（本 diff 的两处名册修正正是这样倒推出来的），
A2 给 L17-24 配名册时那里既无 `CavesKiteTail*` 也无 `HellL*SameClassTail*`，将完全
失去拦截。二是 `HellfireNoParamsSamplingTest` 通过 `TestInitGame` → `InitQuests()`
把 `Quests[*]._qactive` 从零初始化的 `QUEST_NOTAVAIL` 改成 `QUEST_INIT` 且未在
`TearDownTestSuite` 恢复（I1），其中 Q_WARLORD@L13 会给 L13 多加一个 Melee，
使 `HellL13SameClassTailBaseline` 的 `EXPECT_EQ(tail, 0.0)` 在套件顺序改变时确定性失败。

**夹具重生成的独立判断**：实现者称"未重生成任何夹具"可信。仓库内只有
`sampling_behavior_test.cpp` 与 `level_roster_baseline_test.cpp` 依赖
`GetLevelMTypes`/`LevelMonsterTypeCount`（已 grep 全 `test/`），后者的断言是
**结构不变量**（`total > 0`、`total <= MaxMonsters - 10`、`mixTotal == ActiveMonsterCount`）
而非逐层硬编码数值，采样构成变化不会使其失配；`Timedemo.WarriorLevel1to2` 早在
2026-09-15 就因 Dark Expedition 掉落过滤被 `GTEST_SKIP()` 隔离
（`test/timedemo_test.cpp:114-120`），属既有决策，与本任务无关。未发现其它
"断言每层类型数/怪物数"的夹具。⚠️ 但 `drlg_*` 系列的地图夹具我未逐个核对，
控制器应以全量门禁的实跑结果为准。

**eval 缺口**：本 diff **确实**改变玩家可见行为（每层 core 必然出场、尾池上界、
逐层 sprite 预算、floors 补足），按 CLAUDE.md"行为变更必产 eval"本应配 eval 用例；
简报第 17 行明确把 eval 划给任务 4，属**已知且被计划覆盖**的缺口，不是遗忘。
控制器需确认任务 4 仍在计划中且未被裁剪。
