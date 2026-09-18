# Task 4 报告：逐层名册 Phase A 收尾（阈值用例 / eval / 台账 / review 遗留）

状态：进行中（增量提交 + 增量报告）

分支：`feature/qol-upgrades`；起点 HEAD `9095a2397`

## 交付项对照

| # | 交付项 | 状态 |
|---|---|---|
| 1 | `PlacedClassMixWithinBaseline` 阈值用例 | ✅ 完成 |
| 2 | `eval/cases/rng/level-rosters.yaml` + `--smoke` | ⏳ |
| 3 | 存档格式台账追加一行 | ⏳ |
| 4 | review 遗留 O1 + include 顺序 | ✅ 完成 |
| 5 | 全量门禁 `run_tests.py --json /tmp/ci.json` | ⏳ |

---

## 增量 1：review 遗留 + 阈值用例 + R4 名册调整

### 1.1 include 顺序（`test/sampling_behavior_test.cpp:38`）

原顺序把 `tables/questdat.hpp` 排在 `tables/level_roster.h` 之前。改为字典序：
`tables/level_roster.h` → `tables/monstdat.h` → `tables/questdat.hpp`。

顺带对该文件整体跑了 clang-format 18（CI 用的版本，`clang-format-check.yml:32`），
修掉 2 处与本次改动无关的既有违规（旧 939 行的花括号初始化列表、958 行附近的多余空行）。
CRLF 保持不变。

### 1.2 O1：收紧 `EnginePreAddClassCount` 豁免

原实现把「引擎无条件预加」这件事近似成一个宽豁免，导致 `RosterQuotasSatisfied`
的配额断言比实际可达上界松。现在豁免精确到两类：

- Golem（`MT_GOLEM`，`PLACE_UNIQUE`，永远预加）
- `MT_CLEAVER`（Q_BUTCHER）+ 5 个任务唯一怪的 base type

任务唯一怪用具名表驱动，替代散落的 if：

```cpp
constexpr std::array<std::pair<quest_id, UniqueMonsterType>, 5> kQuestUniquePreAdds {
	{ { Q_GARBUD, UniqueMonsterType::Garbud },
	  { Q_ZHAR, UniqueMonsterType::Zhar },
	  { Q_LTBANNER, UniqueMonsterType::SnotSpill },
	  { Q_VEIL, UniqueMonsterType::Lachdan },
	  { Q_WARLORD, UniqueMonsterType::WarlordOfBlood } }
};
```

新增 `QuestPreAddAvailableAt(level, quest)`：`QuestsData` 为空时返回 false
（`SamplingBaselineTest::SetUpTestSuite` 故意不调 `LoadQuestData()`），否则要求
`Quests[quest]._qlevel == level && Quests[quest].IsAvailable()`。这一点有代码依据：
`PlaceUniqueMonsters()`（`Source/monster.cpp:505-530`）在 `minionType == LevelMonsterTypeCount`
时直接跳过该唯一怪，说明预加确实是按 quest 可用性发生的，不是无条件的。

配套新增 2 个用例（不是占位测试，都从出厂名册/真实 quest 状态推断期望）：

- `SamplingBaselineTest.RosterQuotaAllowanceIsBinding`：从出厂名册里发现所有
  「饱和的 (level, 受限 class)」组合，断言非 Boss class 没有余量，且
  `maxRealised == allowance`（配额是紧的，不是上界虚高）。
- `SamplingBaselineTest.EnginePreAddExemptionTracksQuestAvailability`：调
  `LoadQuestData()`，验证无 quest 时只有 Boss 被豁免；把 Q_VEIL 放到 L14 后
  只有 Melee 的额度 +1（`MT_RBLACK`）；同一 quest 挂到别的层不产生额度；
  最后 `Quests[Q_VEIL] = {}` + `QuestsData.clear()` 复原前置条件。

`sampling_behavior_test` 用例数 24 → 26。

### 1.3 阈值用例 `PlacedClassMixWithinBaseline`

`test/level_roster_baseline_test.cpp`：

- `constexpr double kRangedShareTolerance = 0.05;`
- `constexpr std::array<double, 16> kRangedShareBaseline`，13/14/15 位写成
  `(3249.0+2076.0)/23405.0`、`(9674.0+3425.0)/23443.0`、`(13533.0+0.0)/23193.0`
  ——分子分母照抄计划「A-baseline」表（`docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md:658`
  的「实施记录」与 663-682 的基线表），不落成四舍五入后的字面量，出处可审。
- `kRangedShareCeiling[i] = kRangedShareBaseline[i] + kRangedShareTolerance`。
- 用例本体：L13-15 各 200 seed，`CreateDungeonForMeasurement(level, 9000 + seed)` →
  `GetLevelMTypes()` → `InitMonsters()` → `MeasurePlacedClassMix()`；分母是
  `ActiveMonsterCount`（放置怪总数），分子是 `RangedTurret + RangedKite`；
  `ASSERT_GT(total, 0u)` 防空测；无条件 `std::cout` 打印实测值，
  `EXPECT_LE(share, kRangedShareCeiling[level])`。

复用既有 fixture 的采样/放置路径，不重新实现采样规则（禁令 6）。

### 1.4 R4：阈值超标 → 改名册数据，不动阈值

首跑即失败（实测，非推测）：

```
level 13 ranged share 0.534692 (12623/23608), ceiling 0.277515   FAIL
level 14 ranged share 0.657604 (15069/22915), ceiling 0.60876    FAIL
level 15 ranged share 0.505309,               ceiling 0.633495   PASS
```

定界（临时诊断用例测出来的，之后已删除）：

1. `PLACE_SCATTER` 放置在 `scattertypes` 上是均匀的（`Source/monster.cpp:3822-3841`），
   所以「放置后的远程占比」≈「远程 type 占 type 表的比例」。
2. L13-15 的候选池实际只有 Melee / RangedTurret / RangedKite 三类。
3. `BehaviorClassCapForLevel` 在 L13-16 对**任意** class 都是 2
   （`Source/tables/level_roster.h:118`）。于是一旦 type 多样性超过 2 个 Melee，
   多出来的必然是远程。

所以超标的根因是「core 里远程太多 + tail_draw 还要再抽 3 次」，而 cap 又不允许用
更多 Melee 去稀释。按 R4 只能改名册数据：

`assets/txtdata/monsters/level_rosters.tsv`

| level | 改前 core | 改后 core |
|---|---|---|
| 13 | `MT_STORML`(Kite), `MT_NBLACK`(Melee), `MT_SUCCUBUS`(Turret), `MT_GUARD`(Melee) | `MT_NBLACK`, `MT_GUARD` |
| 14 | `MT_VTEXLRD`(Melee), `MT_SNOWWICH`(Turret), `MT_SUCCUBUS`(Turret), `MT_XACID`(Kite) | `MT_VTEXLRD`, `MT_SNOWWICH`, `MT_XACID` |
| 15 | 不变 | 不变 |

`assets/txtdata/monsters/level_roster_params.tsv`

| level | 改前 | 改后 |
|---|---|---|
| 13 | `18000  3  Melee=2,RangedKite=1` | `18000  1  Melee=2,RangedKite=1` |
| 14 | `18000  3  Melee=2,RangedKite=1` | `18000  1  Melee=2,RangedKite=1` |
| 15 | `18000  3  Melee=2,RangedTurret=2` | 不变 |

理由：

- 真正压低占比的是**删远程 core**（L13 去掉 Storm Lord + Succubus，L14 去掉一个 Succubus）。
- `tail_draw` 降到 **1** 而不是 0：0 会让这层完全没有随机尾部；保留 1 次抽取，
  再让 `RangedKite=1` 的 floor 去引导这一次抽取，层里就仍有风筝存在感，
  但不会同时出现第二个远程 type。
- L15 本来就在带内，不动。

L13 的 core 只剩 2 个 Melee，与 `RangedKite=1` 的 floor 不冲突：floor 由 tail 那一次抽取满足。
`level_roster_test.cpp` 里对出厂名册的硬断言只钉 L9（`maxImage 16000` / `tailDraw 3` / `MT_BMAGMA` core），
不与 L13/L14 改动冲突（已确认）。

改完实测：

```
[ MEASURED ] level 13 ranged share 0.236493 (5537/23413), baseline 0.227515, ceiling 0.277515
[ MEASURED ] level 14 ranged share 0.491774 (11269/22915), baseline 0.55876, ceiling 0.60876
[ MEASURED ] level 15 ranged share 0.505309 (11611/22978), baseline 0.583495, ceiling 0.633495
[  PASSED  ] 3 tests.
```

三层都在带内，且余量分别为 4.1pp / 11.7pp / 12.8pp。

（曾试过 L14 用 `MT_BALROG` 换 `MT_SUCCUBUS` 的方案，L14 只到 0.597389，
距 ceiling 仅 1.2pp，太贴边；且该方案让 `QuestPreAddRePickDoesNotDoubleCount`
失败——L14 core 变成 3 个 Melee 后加上 Q_VEIL 的 `MT_RBLACK` 预加就有 3 个 Melee，
撞破 cap=2。最终方案改回 `MT_XACID`，core 只有 2 个 Melee，两个约束同时满足。）

### 1.5 增量 1 验证

```
$ cmake --build build --target level_roster_baseline_test level_roster_test sampling_behavior_test -j8
$ cd build && ./level_roster_test
[==========] 29 tests from 4 test suites ran. (226 ms total)
[  PASSED  ] 29 tests.
$ ./sampling_behavior_test
[==========] 26 tests from 2 test suites ran. (4539 ms total)
[  PASSED  ] 26 tests.
$ ./level_roster_baseline_test
[==========] 3 tests from 1 test suite ran. (23843 ms total)
[  PASSED  ] 3 tests.
$ clang-format-18 --dry-run -Werror test/level_roster_baseline_test.cpp test/sampling_behavior_test.cpp
FORMAT_CLEAN
```

注意（坑）：`python3 tools/run_tests.py --test <name>` **不构建**
（`tools/run_tests.py:208` 只在 `not args.no_build and not args.test` 时构建），
所以必须先 `cmake --build build --target <name> -j8`，否则跑的是旧二进制。
TSV 资产会被复制进 `build/assets/txtdata/monsters/`，改表同样必须重新构建。

提交：`85471bee0 test(roster): pin L13-15 placed ranged share to baseline + 5pp`

（报告文件本身在 `.gitignore` 覆盖范围内，未纳入提交。）

---

## 增量 2：eval 用例 + 存档格式台账

### 2.1 `eval/cases/rng/level-rosters.yaml`（新增，LF）

结构对齐 `eval/cases/rng/sampling-anti-monopoly.yaml`：`setup.mpq_required: true`、
`run.backend: gtest` / `binary: level_roster_baseline_test` / `filter: LevelRosterBaselineTest.*`、
`expected` 里 `exit_code: 0`、`failed: 0`、`passed_min: 3`、`skipped_max: 1`。

`output_contains` 除了 `[  PASSED  ] 3 tests.` 还钉了三行 `[ MEASURED ] level N ranged share`
——用例无条件打印实测占比，所以这个断言保证「数字确实被算出来并暴露到日志」，
将来某层被推向远程画廊时，门禁不只是红，还能直接读到具体数值。
`timeout: 300`（本地实测该二进制约 24s，留足 CI 余量）。

### 2.2 顺带修掉的两处既有不一致

1. `sampling-anti-monopoly.yaml` 的 `passed_min: 13` / `[  PASSED  ] 13 tests.` 已经过期：
   `SamplingBaselineTest.*` 现在是 24 个用例（Task 3 加了一批，本任务 O1 又加了 2 个）。
   已改为 24，并在 description 里补上 O1 两个新用例的说明。
2. `sampling-anti-monopoly-cap` 此前不在任何 suite 里，`sync_case_sets --check`
   一直是 FAIL（先于本任务存在）。跑 `sync_case_sets --write` 重生成 `_nightly.yaml`
   （该文件本身标注为自动生成），新用例与它一起进入 nightly。
   `_smoke.yaml` 是人工策展的「快、且不依赖 MPQ」集合，这两个用例都 `mpq_required: true`
   且分钟级，按该文件自身的约定不放进 smoke。

验证：

```
$ python3 -m tools.eval.backend --check --path eval/cases/rng/level-rosters.yaml
OK: 1 case(s) valid
$ python3 -m tools.eval.sync_case_sets --check
OK: 67 cases, 2 suites consistent          # 改前：FAIL: cases not in any suite: [...]
$ python3 -m tools.eval.backend --run level-rosters
  evaluated: 1  passed: 1  failed: 0  skipped: 0  pass_rate: 1.0
  [PASS] [rng] level-rosters (30/30)
$ python3 -m tools.eval.backend --run sampling-anti-monopoly-cap
  [PASS] [rng] sampling-anti-monopoly-cap (15/15)
$ python3 -m tools.eval.backend --smoke
  ... dark-expedition 4/4, save-load 2/2, mechanics 8/8, data 6/6,
      render 2/2, combat 5/5, utility 9/9   (全 PASS)
smoke_exit=0
```

### 2.3 存档格式台账

`docs/knowledge/decision_save_format_policy.md` 的「格式变更台账」表追加一行
（放在 2026-09-15「计划中」两行之后、未合入主线那行之前）：

- 日期：2026-09-16（**已落地**，Phase A）
- 变更：逐层名册改变采样顺序 → `monster.levelType`（`LevelMonsterTypes` 索引）语义变化；
  core 预加先入表，尾部抽取受 B1 cap 与 `class_floors` 约束，同层同 seed 的索引与旧档不再对应
- 触碰：`Source/monster.cpp`、`assets/txtdata/monsters/level_rosters.tsv`、
  `assets/txtdata/monsters/level_roster_params.tsv`
- 兼容处置：**不做兼容**（宪章决策 35）；旧档 `levelType` 会被重新解释成另一种怪，需重开新档

行尾：`.md` / `.yaml` 均为 LF（已 `file` 确认）。

提交：`5171fb9fb test(eval): add level-rosters case and record the levelType format change`

---

## 增量 3：全量门禁 + 计划实施记录

### 3.1 全量门禁

```
$ python3 tools/run_tests.py --json /tmp/ci.json
  build:  ok
  ctest:  passed 743  failed 0  skipped 3  not_run 0  passed_pct 100  returncode 0
  drift:  drift_ok true
          PASS A  Tests.cmake entries have source files
          PASS B  no placeholder assertions in tests
          PASS C  modified files keep line endings
          PASS C2 added files match .editorconfig
          PASS E  no test-only production functions
gate_exit=0
```

`failed == 0`、`passed_pct == 100`、`drift_ok == true`，退出码 0，三项达标。
（JSON 的实际路径是 `steps.ctest.*` / `steps.drift.drift_ok`，不是 `checks.*`。）

### 3.2 计划实施记录回填

`docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md` 的「实施记录」表：

- `kRangedShareCeiling` 行补上落地值 L13 27.8% / L14 60.9% / L15 63.3%，并指明它以具名
  constexpr 落在 `test/level_roster_baseline_test.cpp`、分子分母照抄 A-baseline 表。
- 新增「任务 4 阈值实测（改后）」行：L13 23.6% / L14 49.2% / L15 50.5%。
- 新增「任务 4 名册调整（R4）」行：记录首跑超标值、cap 导致「多样性溢出必为远程」的根因，
  以及改数据（删远程 core + `tail_draw` 3→1 保留 `RangedKite=1` floor）的取舍。

## 交付项最终状态

| # | 交付项 | 状态 |
|---|---|---|
| 1 | `PlacedClassMixWithinBaseline` 阈值用例 | ✅ |
| 2 | `eval/cases/rng/level-rosters.yaml` + `--smoke` exit 0 | ✅ |
| 3 | 存档格式台账追加一行 | ✅ |
| 4 | review 遗留 O1 + include 顺序 | ✅ |
| 5 | 全量门禁 743 passed / 0 failed / 100% / drift_ok | ✅ |

## 遗留关注点

1. **名册数据变更影响手感，不只是数字。** L13 的 core 从 4 个缩到 2 个、L13/L14 的
   `tail_draw` 从 3 降到 1，意味着这两层的怪种多样性明显下降（L13 稳定只有 2 core + 1 尾）。
   这是在「cap=2 + 只有三个 class 可用」的约束下满足 R4 的唯一数据解，但值不值这个手感代价，
   建议由规格作者确认。若希望恢复多样性，需要的是**放宽 L13-16 的 class cap 或扩充 Melee 候选池**，
   而这属于规格层面的改动，不在本任务范围。
2. **观察 O2 未处理**（加载期校验是否应把 quest 预加计入 cap）——按 brief 归入 Phase B，本次未动。
3. `sampling-anti-monopoly-cap` 的 `passed_min` 是按当前用例数硬编码的，后续再往
   `SamplingBaselineTest` 加用例时同样要同步这个数字（这次就是因为它没同步才过期）。
   若嫌脆，可以考虑把该断言改成只钉 `failed: 0` + 关键 `output_contains`，但那是另一个决定。
4. Phase A2（HF overlay L17-24）仍未做，L17-24 目前走 R28 legacy fallback。

提交：`a1de8dcda docs(plan): record task 4 threshold values and the R4 roster adjustment`

---

## 增量 4：评审裁决修复（F1-F5）

### F1（裁决 R34）措辞更正 + 替代解入档，**不改数据**

评审用 7 组反事实名册（每组 200 seeds × 3 层真实放置）证伪了我上一版
「唯一数据解」的说法。接受这个更正：

- L13 侧我的结论**站得住**：最小放松实测 0.3792 vs ceiling 0.2775，超 10pp，不是贴边问题。
- L14 侧**不唯一**：`tail_draw` 1→2 实测 **0.5964 < ceiling 0.60876**，物种 4→5，
  零代码改动，全测试绿。

按控制者裁决**保持 L14 `tail_draw = 1`**，并把措辞改成准确表述。计划「实施记录」
的「任务 4 名册调整（R4）」行现在写的是：这是「在我们接受的余量标准（≥5pp）下唯一」的解，
并明确记下 `tail_draw=2` 是可行替代解（0.5964 / 60.876% / 余量 1.2pp / 物种 4→5），
本阶段**有意不采纳**——余量 1.2pp 会让守卫变脆，且我此前正是以「太贴边」否掉过等余量的
`MT_BALROG` 方案，采纳会自相矛盾。

规格 `docs/superpowers/specs/2026-09-15-level-rosters-design.md` 附录 E 新增第 5 条，
记录该替代解的实测数据、R34 三条不采纳理由，以及留给阶段 B 的取舍问题
（「是否值得用 1.2pp 守卫余量换 L14 一个物种」），并写明两种收敛路径：
若阶段 B 按附录 E 第 1 条放宽 cap 或占比上限，本项自然消解；若阶段 B 确认守卫余量必须 ≥5pp，
本项应正式关闭为「不采纳」。

### F2 `kRangedShareCeiling` 越界与静默误判

两张表都从 `std::array<double, 16>` 改成 **`std::array<double, 17>`**（按层号索引，
L16 现在是合法下标而不是越界读）；未设基线的层从 `0.0` 改为具名哨兵
**`kRangedShareUnconstrained = 1.0`**（占比不可能超过 1.0，等价于「无约束」）。
注释写明只有 L13-15 有基线、其余为无约束，且循环范围将来扩展**必须先补基线**。

`0.0` 的危险正如评审所说：它读起来是「ceiling 为 0」，一旦有人放宽被测层范围，
「没有基线」就会被静默当成「远程占比必须为 0」——要么必失败，要么更糟，
过了也不代表任何东西。哨兵让这个遗漏是显式的。

ceiling 表里哨兵层直接写哨兵值本身、不加 tolerance——否则会变成 1.05，反而模糊了
「无约束」这个读法。

### F3（裁决 R35）`passed_min` 机制：不重构，但标注 + 复核

复核了两份 case 的计数与真实用例数（用 `--gtest_list_tests` 实数）：

```
LevelRosterBaselineTest.* = 3      -> level-rosters 的 passed_min 3        ✅ 一致
SamplingBaselineTest.*    = 24     -> sampling-anti-monopoly-cap 的 24     ✅ 一致
```

两个 YAML 顶部各加了 4 行维护提醒，写明 `passed_min` 与
`[  PASSED  ] N tests.` 是硬编码计数，增删用例必须同步，否则门禁会静默失效
（N 偏小 → 少跑也算过）或误红（N 偏大 → 全绿也判失败），并记下当前值与核对日期。
按裁决**不在本任务重构 eval 工具层**；机制缺陷已写进下方「遗留关注点」。

### F4 全量门禁实跑证据 + clang-format 版本

```
$ python3 tools/run_tests.py --json /tmp/ci.json
gate_exit=0
build.ok             True
steps.ctest.passed       743
steps.ctest.failed       0
steps.ctest.skipped      3
steps.ctest.passed_pct   100
steps.ctest.returncode   0
steps.drift.drift_ok     True
    PASS A  Tests.cmake entries have source files
    PASS B  no placeholder assertions in tests
    PASS C  modified files keep line endings
    PASS C2 added files match .editorconfig
    PASS E  no test-only production functions
    merge-base: e00b7260f   check E allowlist: 10 upstream symbols
```

clang-format：系统装的是 **14.0.0**（`Ubuntu clang-format version 14.0.0-1ubuntu1.1`），
CI 用 **18**（`.github/workflows/clang-format-check.yml:32`）。为避免版本差异，
我没有用系统的 14，而是本地解出了一份 **18.1.8**（`clang-format version 18.1.8`）
并全程用它格式化与校验，两个改动文件当前 `--dry-run -Werror` 干净。

关于 include 顺序修正：`test/.clang-format` 是 `BasedOnStyle: webkit` + `SortIncludes: true`，
没有 `IncludeCategories` / `IncludeBlocks` 自定义分组，所以排序退化为块内字典序——
这个改动是纯机械移动（`level_roster.h` / `monstdat.h` / `questdat.hpp` 三行重排，
无增删、无内容变化），不依赖任何版本特有的分组启发式。即便如此，
校验仍是用与 CI 同版本的 18.1.8 做的，不是靠推理。

### F5 `kQuestUniquePreAdds` 注释与表不一致

按要求以 `Source/monster.cpp:3464-3475` 的实际代码为准核对，结论是
**注释和表都没错，是注释表述有歧义**：那段代码确实有 **6 处** quest 门控的 type 预加，
但其中 `Q_BUTCHER` 加的是**普通怪 type** `MT_CLEAVER`（`AddMonsterType(MT_CLEAVER, PLACE_SPECIAL)`），
只有其余 5 个（`Q_GARBUD`/`Q_ZHAR`/`Q_LTBANNER`/`Q_VEIL`/`Q_WARLORD`）加的是
**unique 的 base type**（`AddMonsterType(UniqueMonsterType::..., PLACE_UNIQUE)`）。

所以「六个 quest unique」是错的（`MT_CLEAVER` 不是 unique），但表里 5 项也不该补成 6——
`MT_CLEAVER` 在 `EnginePreAddClassCount` 里是单独一行处理的。注释已改成：
「MT_GOLEM 加上 monster.cpp:3464-3475 的**六处 quest 门控 type 预加**；这六处不是六个 unique：
Q_BUTCHER 加普通 type MT_CLEAVER，其余五个加 unique 的 base type，所以下面这张表是 5 项，
MT_CLEAVER 在 EnginePreAddClassCount 里单独计数」。行号也从 `3463-3475` 校正为 `3464-3475`。

### 增量 4 验证

```
$ cmake --build build --target level_roster_baseline_test sampling_behavior_test -j8
[5/6] Linking CXX executable level_roster_baseline_test
[6/6] Linking CXX executable sampling_behavior_test
$ ./sampling_behavior_test
[==========] 26 tests from 2 test suites ran. (4310 ms total)
[  PASSED  ] 26 tests.
$ ./level_roster_baseline_test
[ MEASURED ] level 13 ranged share 0.236493 (5537/23413), baseline 0.227515, ceiling 0.277515
[ MEASURED ] level 14 ranged share 0.491774 (11269/22915), baseline 0.55876, ceiling 0.60876
[ MEASURED ] level 15 ranged share 0.505309 (11611/22978), baseline 0.583495, ceiling 0.633495
[==========] 3 tests from 1 test suite ran. (22243 ms total)
[  PASSED  ] 3 tests.
$ clang-format-18.1.8 --dry-run -Werror test/level_roster_baseline_test.cpp test/sampling_behavior_test.cpp
FORMAT_CLEAN
$ python3 -m tools.eval.backend --check --path eval/cases/rng   ->  OK: 3 case(s) valid
$ python3 -m tools.eval.sync_case_sets --check                  ->  OK: 67 cases, 2 suites consistent
$ python3 -m tools.eval.backend --run level-rosters             ->  [PASS] (30/30)
$ python3 -m tools.eval.backend --run sampling-anti-monopoly-cap ->  [PASS] (15/15)
$ python3 -m tools.eval.backend --smoke                         ->  36 PASS / 0 FAIL, exit 0
```

阈值实测值三层均未变（哨兵改动只影响未设基线的层），说明 F2 是纯加固、无行为影响。

## 遗留关注点（更新）

1. **`passed_min` 硬编码计数机制是脆的**（裁决 R35 本任务不重构，记此备最终评审分拣）：
   `expected.passed_min` + `output_contains: "[  PASSED  ] N tests."` 把用例数写死在 YAML 里，
   两种失效方向都不响：N 偏小则「少跑也算过」——门禁静默失效；N 偏大则「全绿也判失败」——误红。
   本轮就是撞上第一种（13 vs 实际 24，已修）。可选出路：让 runner 从
   `--gtest_list_tests` 自动取期望数，或把断言改成只钉 `failed: 0` + 关键 `output_contains`
   而放弃计数。属 eval 工具层 Infra，需单独立项。
2. **L14 的替代解已入档但未采纳**（附录 E 第 5 条）：现状是拿 10.5pp 的额外守卫余量
   换掉 1 个物种。阶段 B 若放宽 cap / 占比上限，应回头重评这一项。
3. **名册数据变更影响手感**：L13 稳定只有 2 core + 1 尾抽，L13/L14 物种多样性下降。
   已归入附录 E 第 1 条由阶段 B 权衡（裁决 R32），本阶段不动守卫。
4. **观察 O2 未处理**（加载期校验是否把 quest 预加计入 cap）——附录 E 第 2 条，Phase B。
5. **Phase A2（HF overlay L17-24）未做**，L17-24 走 R28 legacy fallback。
6. 本地系统 clang-format 是 14，与 CI 的 18 不同；本任务全程用本地解出的 18.1.8 校验，
   但**这份 18 不在仓库工具链里**，后续改 `Source/` 或 `test/` 的人若只用系统 14，
   仍可能提交出 CI 报格式错的代码。值得考虑固化一个版本获取方式（同属工具层，未在本任务处理）。
