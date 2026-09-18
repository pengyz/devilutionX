# Task 4 评审（阶段 A 收尾：阈值 + eval + 台账 + O1/格式）

Base `9095a2397` → Head `a1de8dcda`（工作区 HEAD 实为 `f9fa34bb6`，多一个 spec 附录 E 提交，不在本次 diff 内）。
只读评审：未改动工作区/暂存区/HEAD（`git status --porcelain` 为空）。
所有反事实数据实验都在 `/tmp/pr*` 沙箱副本内做（symlink 到 mpq、复制 `build/assets`），仓库资产未被触碰。

## 规格符合度

- ✅ **交付项 1（`PlacedClassMixWithinBaseline`）符合规格**：走真实建关 + 真实放置，不自证；阈值 = 基线 + 5pp，
  分子分母照抄计划 A-baseline 表（逐位核对 `plans/...phase-a.md:681-683`：L13 `3249/2076/23405`、
  L14 `9674/3425/23443`、L15 `13533/0/23193` —— 与 `test/level_roster_baseline_test.cpp:146-148` **完全一致**）。
- ✅ **交付项 2-3（eval / 台账）符合规格**。
- ✅ **交付项 4（O1 + include 顺序）符合规格**。
- ❌ **发现问题：R4 的"唯一数据解"主张不成立** —— 存在一个既满足 ceiling、又比现状多样的解。
  见「问题 / 重要 1」。这是本轮最重要的发现。
- ⚠️ **无法仅凭 diff 验证**：全量门禁 743 passed（报告 3.1）我未重跑（约十几分钟 + 会写 `/tmp/ci.json`）；
  控制器如需确认，跑 `python3 tools/run_tests.py --json /tmp/ci.json` 并核 `steps.ctest.passed_pct==100`
  / `steps.drift.drift_ok==true`。我已单独复核了三个直接相关二进制（见下）。

## 独立验证到的事实（不是引用报告）

| 项 | 我的实测 | 报告声称 | 一致？ |
|---|---|---|---|
| `PlacedClassMixWithinBaseline` | L13 `0.236493 (5537/23413)` / L14 `0.491774 (11269/22915)` / L15 `0.505309 (11611/22978)` | 同 | ✅ 逐位一致 |
| `sampling_behavior_test` | 26 passed | 26 | ✅ |
| `level_roster_test` | 29 passed | 29 | ✅ |
| `sync_case_sets --check` | `OK: 67 cases, 2 suites consistent` | 同 | ✅ |
| `backend --check level-rosters.yaml` | OK | 同 | ✅ |
| `clang-format`（本机仅 14，CI 用 18） | 两个 test 文件均无 diff | FORMAT_CLEAN | ✅（版本差异见「轻微 1」） |

补充核实的事实：
- `test/.clang-format` 确有 `SortIncludes: true`；`test/sampling_behavior_test.cpp:402-405` 现为
  `level_roster.h → monstdat.h → questdat.hpp` 字典序，归位正确。CI 检查路径确为 `Source` 与 `test`
  （`.github/workflows/clang-format-check.yml:29-38`）。
- 台账行已落在 `docs/knowledge/decision_save_format_policy.md:31`，四栏（日期/变更/触碰/兼容处置）齐全，
  位置符合简报（2026-09-15 两行之后、"未合入主线"那行之前），并显式引用决策 35。`.md` 仍为 LF。
- 行尾：两个 TSV 与两个 `.cpp` 仍是 CRLF，`.yaml`/`.md` 为 LF —— 符合禁令 7。

## 优点

1. **阈值用例确实驱动生产夹具，不是自证**。`test/level_roster_baseline_test.cpp:296-304` 走
   `CreateDungeonForMeasurement → GetLevelMTypes() → InitMonsters()`，分母是引擎自己的
   `ActiveMonsterCount`，分子从 `Monsters[ActiveMonsters[i]].ai` 反查 class —— 统计的是**实际放置的怪**，
   没有在测试里重算一遍采样规则。`ASSERT_GT(total, 0u)`（:305）挡住"空测通过"。
2. **阈值以分数形式而非四舍五入字面量落地**（:146-148），出处注释指到 A-baseline 表，可审计性明显好于写 `0.278`。
3. **无条件打印 `[ MEASURED ]`**（:309-311）+ eval 用例把这三行钉进 `output_contains`：门禁失败时能直接读到数字，
   而不是只知道红。这是一个比"仅 EXPECT_LE"更有价值的设计。
4. **O1 收紧确实有代码依据且方向正确**。`Source/monster.cpp:3464-3475` 证实引擎在 `GetLevelMTypes()` 里
   **只**无条件预加 Golem 与 6 个 quest 相关 base（`MT_CLEAVER` + 5 个 quest unique），其余 unique 由
   `PlaceUniqueMonsters()` 在已有类型中挑；旧实现把"该层全部 `mlevel` 匹配的 unique"计入豁免，
   L13 有 9 个 unique base 横跨多类，确实会让格子恒真。收紧是真的提高了判别力。
5. **新增两条用例不是自证**。`RosterQuotaAllowanceIsBinding`（:529-584）不硬编码层号，而是从出厂名册
   搜索"core 恰好压满 cap"的格子；我独立算了一遍饱和格子集合 = `{L10/L11/L12 RangedKite, L13 Melee,
   L15 Melee, L15 RangedTurret}`，**非空**，所以 `ASSERT_FALSE(saturated.empty())` 不空转，
   `EXPECT_EQ(maxRealised, allowance)` 是能失败的紧断言。`EXPECT_LE` 没被放宽（仍是 `cap + preAdds`）。
6. **`EnginePreAddExemptionTracksQuestAvailability` 用可证伪的三段式**（:589-630）：无 quest 只有 Boss 豁免 →
   挂 Q_VEIL 到 L14 只让 Melee +1 → 同 quest 挂到 L13 对 L14 无额度。第三段是真正的判别力证据。
   收尾 `Quests[Q_VEIL] = {}` + `QuestsData.clear()` 恢复了套件前置条件（该套件 `SetUpTestSuite` 故意不调
   `LoadQuestData()`），这一点做得细致。
7. **`QuestPreAddAvailableAt` 复用引擎自己的 `Quest::IsAvailable()`**（`Source/tables/questdat.cpp:64-76`）
   而不是重写一份判定，避免 harness 与引擎漂移；对 `QuestsData.empty()` 的早退保护有真实必要
   （`IsAvailable()` 会索引 `QuestsData[_qidx]`）。
8. **B1 契约未被削弱**：我在选项 E 的反事实数据下重跑，`HellL13/14SameClassTailBaseline` 的
   `EXPECT_EQ(tail, 0.0)`、`CavesKiteTailBaseline`、`RosterQuotasSatisfied`、`QuestPreAddRePickDoesNotDoubleCount`
   全部仍然 OK —— 这些断言没有为本次数据改动让路，也没有被改动。
9. **R28 legacy 分支未受影响**：`tail_draw` 改动只落在 L13/L14 的**已有** params 行；L17-24 依旧无 params 行，
   走 `tailDraw = numeric_limits<int>::max()`（`Source/monster.cpp:3522`）。
   `HellfireNoParamsSamplingTest` 两条用例在出厂数据下 OK（亲自跑到）。
10. **顺带修的 eval 记账问题是真问题，修得对**（R33 已接受）：`sampling-anti-monopoly.yaml` 的
    `passed_min: 13` / `[  PASSED  ] 13 tests.` 确已过期（`SamplingBaselineTest` 现为 24 条，我 `grep -c` 核过），
    `passed_min` 是"vacuous pass guard"（`tools/eval/assertions.py:38-41`），过期会让门禁**恒松**而非恒紧，
    所以这不是无害的陈旧数字。`sync_case_sets` 规则 3 要求每个 case 至少属于一个 suite，
    `sampling-anti-monopoly-cap` 此前确实不在任何 suite（该 id 与文件名不同名，容易漏），现已随
    `--write` 重生成的 `_nightly.yaml` 归位；`_smoke.yaml` 是人工策展的"快 + 不依赖 MPQ"集合，
    把这两个分钟级 `mpq_required` 用例排除在 smoke 外符合该文件自身约定。

## 问题

### 严重（必须修复）

无。本次改动没有引入 Critical 级破坏：三个直接相关二进制全绿，B1 契约与 R28 分支均未被削弱，
台账/格式/行尾合规，阈值用例走真实放置路径且能失败。

### 重要（建议修复）

**1. 「唯一数据解」不成立：`L14 tail_draw=2` 是一个更优解（多样性 +1 且仍在带内）。**
`assets/txtdata/monsters/level_roster_params.tsv:14`（`14  18000  1  Melee=2,RangedKite=1`）

- **问题**：报告 1.4 与「遗留关注点 1」都主张现状是"cap=2 + 候选池只有三类"下满足 R4 的**唯一**数据解。
  我按控制者要求逐个算/跑了 7 组候选（每组都是真跑 200 seed × 3 层的真实放置，不是纸上推演）：

  | 选项 | L13 core / tail | L14 core / tail | L13 实测 | L14 实测 | 结论 |
  |---|---|---|---|---|---|
  | **现状** | 2 Melee / 1 | 2M+1T+1K→3 / 1 | **0.2365** ✅ | **0.4918** ✅ | 通过 |
  | A | +MT_XACID → 3 / 1 | 3 / 1 | 0.3792 ❌ | 0.4918 ✅ | L13 超 |
  | B | 3 / 2 | 3 / 2 | 0.4767 ❌ | **0.5964** ✅ | L13 超 |
  | C | 4（含 SUCCUBUS）/ 1 | 4（含 NBLACK）/ 1 | 0.4632 ❌ | 0.6005 ✅ | L13 超 |
  | D | 2 / 3（改回原值） | 3 / 3（改回原值） | 0.4687 ❌ | 0.6820 ❌ | 两层都超 |
  | **E** | 2 / **2** | 3 / **2** | 0.3717 ❌ | **0.5964** ✅ | **L14 侧可行** |
  | F | 2 / 2（去 Kite floor） | 3 / 1 | 0.3827 ❌ | 0.4918 ✅ | L13 超 |

- **核心发现**：**L13 侧报告是对的**（我试的 6 种放松全部超顶，最小的 A 也到 0.3792 vs ceiling 0.2775，
  超出 10pp 不是贴边），但 **L14 侧不是唯一解**：把 L14 的 `tail_draw` 从 1 恢复到 **2**，
  L14 实测 `0.5964`，仍低于 ceiling `0.60876`，而该层的怪种数从 4 提到 **5**。
  我进一步在该数据下重跑了 `level_roster_test`（29 passed）与 `sampling_behavior_test` 的 B1 契约用例
  （`HellL13/L14SameClassTailBaseline`、`CavesKiteTailBaseline`、`RosterQuotasSatisfied`、
  `RosterQuotaAllowanceIsBinding`、`QuestPreAddRePickDoesNotDoubleCount`）—— **全部仍 OK**。
  也就是说这是一个"零代码改动、只动一个数字、多一个怪种、所有既有保证不变"的严格改进。
- **为何重要**：报告的"唯一解"结论正被用来把手感损失上报给规格作者（遗留关注点 1），并作为
  "只能等阶段 B 放宽 cap"的论据。若结论被误信，L14 会白白少一个怪种，且阶段 B 的讨论会建立在
  一个偏悲观的可行域描述上。R32 已裁定"阶段 A 不为多样性放宽两个守卫"—— 而这个改进**恰好不需要**
  放宽任何守卫，属于 R32 允许范围内的纯数据优化。
- **注意（诚实报告余量）**：`0.5964` 距 ceiling `0.60876` 只有 **1.2pp** 余量，比现状的 11.7pp 薄得多。
  报告在 1.4 节尾注里正是以"太贴边"为由否掉过另一个 1.2pp 方案（`MT_BALROG` 换 `MT_SUCCUBUS`）。
  所以这是一个**取舍**而非无脑改进：+1 怪种 vs 余量从 11.7pp 降到 1.2pp。
- **如何修复**：不要求必须改数据，但要求**修正结论并让控制者/规格作者在知情下决定**：
  (a) 把报告与计划「实施记录」里的"唯一数据解"改为"L13 侧穷举 6 组均超顶；L14 侧存在
  `tail_draw=2` 的可行解，余量 1.2pp，因贴边风险未采用"；
  (b) 若接受 1.2pp 余量，则 `level_roster_params.tsv` L14 的 `tail_draw` 改回 `2` 并重跑三件套。

**2. `kRangedShareBaseline`/`kRangedShareCeiling` 用 `std::array<double,16>` 但按 `[level]` 索引到 15，
数组尺寸没有为"层号即索引"留出余量，且 L1-12 被填成 `0.0` 而非"不适用"哨兵。**
`test/level_roster_baseline_test.cpp:144-168`

- **问题**：数组长度 16、合法索引 0-15，当前循环 `level = 13..15` 恰好不越界。但语义上
  "用层号直接索引"的表通常要 `NUMLEVELS`/17 长度（简报给的示例正是 `std::array<double,17>`）。
  一旦有人把循环扩到 L16（例如阶段 A2 接手 L16-24），`kRangedShareCeiling[16]` 就是**越界读**，
  且 `constexpr` 数组的越界不会被 `EXPECT_*` 抓到。此外 L1-12 填 `0.0` 意味着"ceiling 0%"，
  如果误用到那些层，断言会以看似合理的方式恒失败，而不是明确报"该层无阈值"。
- **为何重要**：这是一个安静的未来越界 + 误用即静默错解的组合，成本极低但要等到别人扩层时才炸。
- **如何修复**：数组尺寸改 17（或 `NUMLEVELS`），并把不适用的层填 `1.0`（"无上限"）或加
  `static_assert` + 循环内 `ASSERT_LT(level, kRangedShareCeiling.size())`。

**3. eval 用例 `passed_min` 仍是硬编码计数，本轮修了一次症状但没解决机制。**
`eval/cases/rng/level-rosters.yaml:20`（`passed_min: 3`）、`eval/cases/rng/sampling-anti-monopoly.yaml:20`（`24`）

- **问题**：`passed_min` 与 `output_contains: "[  PASSED  ] N tests."` 两处都要随用例数手工同步。
  本次过期正是这个机制导致的（R33 已接受修复本身），但修完之后**同一个坑仍然在**：
  `level-rosters.yaml` 又新增了一个 `passed_min: 3` + `[  PASSED  ] 3 tests.`，
  下次谁往 `LevelRosterBaselineTest` 加一条用例，`output_contains` 就会**直接失败**（不是变松，是变红），
  而 `passed_min` 会变松。报告「遗留关注点 3」自己也点出了这一点，但只是记录，未处置。
- **为何重要**：`passed_min` 的设计目的是"vacuous pass guard"（`tools/eval/assertions.py:38-41`）。
  硬编码等值 `[  PASSED  ] N tests.` 把守卫变成了脆性锁：正常增测会红，只能靠改数字消红，
  长期会训练出"看到这行红就改数字"的坏反射，从而真正削弱守卫。
- **如何修复**（择一，属于机制决定，可由控制者裁决后另开任务）：
  把 `output_contains` 里的 `[  PASSED  ] N tests.` 去掉，只保留 `failed: 0` + 有语义的
  `[ MEASURED ]` 行 + `passed_min` 取一个**下界**（如 3）而非等值；或让 `sync_case_sets`
  顺带校验/回填 `passed_min`。

### 轻微（可选优化）

**1. 本机无 clang-format 18，格式结论只在 14 上复核过。**
本机 `clang-format --version` 为 `14.0.0`；CI 用 18（`clang-format-check.yml:32,38`）。
我用 14 对两个 test 文件做了 `--dry-run -Werror`，**无 diff**；报告声称 18 下 FORMAT_CLEAN。
14 与 18 在 `webkit` 基线上对花括号初始化列表/尾随注释的换行策略有已知差异，所以我**不能**
把"18 也干净"当成已验证。控制器若要确认，在有 18 的环境跑
`clang-format-18 --dry-run -Werror test/sampling_behavior_test.cpp test/level_roster_baseline_test.cpp`。

**2. 报告 1.1 承认顺带修了两处"与本次改动无关的既有格式违规"（旧 939 行花括号初始化列表、958 行附近空行），
diff 里对应 `A1A3VariantsAreCore` 的 carriers 列表展开（`test/sampling_behavior_test.cpp:653-658`）
与删掉一个空行（:676）。**
这是无害的，但严格说超出了简报「把 `:38` 的 include 归位」的范围，让本任务的 diff 混入了与交付项无关的行。
不建议回退（回退会让 CI 18 变红），只建议今后此类顺带格式修单独成 commit 以便 review。

**3. `kQuestUniquePreAdds` 只列 5 个 quest unique + 单独处理 `MT_CLEAVER`，注释说"六个 QUEST uniques"。**
`test/sampling_behavior_test.cpp:730-731` 注释写 "the base types of the six QUEST uniques"，
但表里是 5 项，第 6 个（Butcher/`MT_CLEAVER`）在 `EnginePreAddClassCount` 里单独 `if`（:472-474），
因为它预加的是普通怪 type 而非 unique base。代码是对的（我对着 `Source/monster.cpp:3464-3475` 核过），
注释的"six ... uniques"与表的 5 项容易让下个人以为漏了一条。建议注释改为
"Golem + Q_BUTCHER 的 MT_CLEAVER + 5 个 quest unique 的 base（共 6 个 quest 触发的预加）"。

**4. `EnginePreAddExemptionTracksQuestAvailability` 依赖 `Q_VEIL` 的 `_qlevel` 可写与 `MT_RBLACK` 的 class。**
用例内 `ASSERT_EQ(questClass, BehaviorClass::Melee)`（:600）已经把"MT_RBLACK 是 Melee"这个前提断言出来，
做法正确。仅提示：该用例通过直接写 `Quests[Q_VEIL]._qlevel = 13/14` 绕过了 `InitQuests()`，
与生产路径（`_qlevel` 来自 `questdat.tsv` 的 `qdlvl`，L14 确实是 Q_VEIL）不同源；
若将来 `questdat.tsv` 把 Q_VEIL 挪层，用例仍会绿而生产语义已变。属可接受的 harness 简化。

## 评估

**任务质量：** 需修复

**理由：** 交付项 1-5 都真实落地且质量偏高（阈值用例走真实放置、O1 收紧有代码依据、两条新用例可证伪、
eval 记账修得对），我独立复核的数字与报告逐位一致；但报告的核心结论「R4 下现状是唯一数据解」
经我实测被证伪——L14 `tail_draw=2` 同样在带内且多一个怪种（代价是余量从 11.7pp 降到 1.2pp），
这个结论正被用于向规格作者上报手感损失，必须先纠正再决策。
