# Task 2b 复审（修复轮次 1）——裁决

**基线**：`1e07b0a9f` → **Head**：`33a656a7e`（单提交 `test(roster): assert parsed values and make the assets path switch order-independent`）
**范围**：Imp-1、Imp-2、Minor 5、Minor 3 doc + 修复 diff 的新增破坏。Minor 4 按控制者裁决不核实。
**改动面**：`Source/tables/level_roster.cpp`（+11/-8）、`Source/tables/level_roster.h`（+7）、`test/level_roster_test.cpp`（+69/-11）。TSV 未被触碰（diff 的 files changed 仅 3 个文件，两张 shipped 表与 fixture 表均不在内）。

## 问题裁决

### Imp-1【重要】shipped 表用例缺真值断言 —— ADDRESSED

`test/level_roster_test.cpp:186-200`（params9 真值断言 + `MT_BMAGMA` 行四元组）、`test/level_roster_test.cpp:163-172`（交错 fixture 用例补 `role`/`allowUniqueBoost`）。

- **断言是否反映表里的真实值（而非改 TSV 迁就测试）**：是。`assets/txtdata/monsters/level_roster_params.tsv` L9 行实测为 `9\t16000\t3\tRangedKite=1`，`assets/txtdata/monsters/level_rosters.tsv:34` 实测为 `9\tMT_BMAGMA\tcore\t-`；两张 TSV 均**不在** diff 的 files changed 内，行尾仍是 CRLF（`cat -A` 确认 `^M`）。测试断言 `maxImage==16000`、`tailDraw==3`、`role==Core`、`allowUniqueBoost==false` 与表内容逐项一致。
- **两列对调是否会让用例失败**：会。`RecordReader` 是**位置读取**（`Source/data/record_reader.hpp:29-33`，`readInt(name, out)` 内部 `nextField()`，`name` 仅用于报错文案），`LoadLevelRosterParamsFromFile()` 按 `level → max_image → tail_draw → class_floors` 顺序取字段（`Source/tables/level_roster.cpp:126-131`）。列对调后 L9 解析为 `maxImage=3 / tailDraw=16000`，`ValidateLevelRoster` 不会拦（`maxImage>0`、`tailDraw>=0` 均满足，`level_roster.cpp:179-182`），因此唯一拦截点就是新断言 `EXPECT_EQ(params9->maxImage, 16000)` —— 必然失败。修复前的 `ASSERT_NE(..., nullptr)` 则会放过。
- 四元组覆盖：`level` 由 `GetLevelRoster(9)` 隐含、`type` 由 `find_if(e.type == MT_BMAGMA)` 锚定、`role`/`allowUniqueBoost` 显式断言。交错 fixture 两行（`1 MT_WSKELAX core -`、`1 MT_NZOMBIE core -`，与 `test/fixtures/txtdata/monsters/level_rosters_interleaved.tsv` 实测一致）同样补齐，并从 `any_of` 改为 `find_if` + `ASSERT_NE(end)`，使定位失败与字段失败可区分。

### Imp-2【重要】测试路径切换依赖顺序 + 拼接不一致 —— ADDRESSED

`test/level_roster_test.cpp:326-364`。

- **保存-恢复是否在生命周期钩子里**：是。`LevelRosterFixtureLoadTestBase::SetUp()`（:334-337）保存 `paths::AssetsPath()` 到 `savedAssetsPath_`，`TearDown()`（:339-342）恢复；两个具体固件 `LevelRosterFixtureLoadTest`（:348-355）与 `LevelRosterShippedLoadTest`（:357-364）各自 `SetUp()` 先调基类再设自己的路径。**TEST 体内已无任何 `SetAssetsPath`**（原 `LoadsTheShippedRosterAndValidatesIt` 体内的那行已删除，见 diff:178-180）。模式与 `test/assets_test.cpp:20,38`（`HasLooseLogicAssetsTest` 的 `savedAssetsPath_` 保存/恢复）一致。派生类只覆写 `SetUp()`，基类 `TearDown()` 仍生效，恢复不会被跳过。
- **拼接统一**：两处均为 `paths::BasePath() + "test/fixtures/"` / `+ "assets/"`，无多余前导 `/`（原先一处带 `/`、一处不带）。与仓库既有写法 `test/timedemo_test.cpp:55`、`test/drlg_test.hpp:48`、`test/level_roster_baseline_test.cpp:151`、`test/main.cpp:93` 一致（`BasePath()` 来自 `SDL_GetBasePath()`，自带尾分隔符，`Source/utils/paths.cpp:96-102`）。
- **顺序无关性证据**：报告给出了 filter 单跑（`LevelRosterFixtureLoadTest.*` 1/1、`LevelRosterShippedLoadTest.*` 1/1、`LevelRosterTest.*` 19/19）与 3 组 shuffle 种子（111/222/777，各 27/27）。我**独立复跑核实**（`build/level_roster_test` mtime 07:15:08 晚于全部三个源文件，二进制是修复后产物）：
  - `--gtest_filter='LevelRosterShippedLoadTest.*'` → 1 PASSED
  - `--gtest_filter='LevelRosterFixtureLoadTest.*'` → 1 PASSED
  - `--gtest_shuffle --gtest_random_seed=4242`（报告未用的第 4 个种子）→ 27 PASSED
  两个 e2e 用例各自单跑均通过，证明不再依赖"fixture 用例先跑、shipped 用例负责改回路径"的隐式顺序。

### Minor 5 `LogLoadedRosterSummary()` 硬编码 `level <= 16` —— ADDRESSED

`Source/tables/level_roster.cpp:271-282`。`for (uint8_t level = 1; level <= 16; level++)` 已改为 `for (const LevelRosterParams &param : Params)`，并带注释说明动机（阶段 A2 加 L17-24 后无需改此处）。`GetLevelRoster()`/`GetLevelRosterParams()` 两个调用均保留，漂移检查 E 的生产消费者不减。遍历中只读 `Params`、`GetLevelRosterParams()` 内部 `find_if` 亦只读，无边遍历边改的失效风险。

### Minor 3 doc（spawn 语义提示）—— ADDRESSED（含一处残留缺口，非阻塞）

`Source/tables/level_roster.h:91-98`：`GetLevelRoster()` 声明上方新增 doc，明确①本函数不按 availability（spawn/retail/hellfire）过滤；②spawn 数据下多数 core 行是 `availability=Retail` 故不可用；③采样调用方（任务 3 的采样循环）**必须**自行对照当前加载的 `MonstersData` 过滤；④非空 span 不构成 spawn 可用性保证。要求的语义提示（"调用方/采样循环必须自行按可用性过滤"）已落在消费点上。

残留：`LoadLevelRosterFromFiles()` 的 doc（`level_roster.h:51-59`）**未**提 spawn 下 `ValidateLevelRoster` 走宽松分支（`level_roster.cpp:230-236` 直接 `return nullopt`，跳过逐行 availability / unique 白名单 / class floor 可满足性），即"加载成功 ≠ 这些行在当前模式可用"。`grep -i spawn Source/tables/level_roster.h` 只命中 92-96 行。属 Minor 的补充面，不改变裁决。

## 修复 Diff 中的新增破坏

无 Critical / Important。以下为 Low：

- **Low** `test/level_roster_test.cpp:316-320`（基类注释）：注释称 `LoadCoreArchives()`/`LoadMonsterData()` "only need to run once per binary … so they stay in SetUpTestSuite()"，但 `SetUpTestSuite()` 是**每套件**一次，拆出两个具体固件后它实际会跑两次（原先一次）。功能无害（同注释所述重复调用只是记 benign warning，`LoadCoreArchives()` 走 `LoadMPQ` 幂等登记，27 用例全绿且两个套件单跑均 PASS），但注释措辞与拆分后的实际行为不符。
- **Low** `Source/tables/level_roster.cpp:276`：改为遍历 `Params` 后，只出现在 `Entries` 而无对应 params 行的 level 不再被诊断打印（校验并不强制每个 entries level 都有 params 行，`level_roster.cpp:213-228` 只要求 core 非空）。这类畸形表原先会被 `1..16` 循环报成 `params missing`，现在会被静默跳过。修复严格符合复审要求的两个选项之一（"遍历 Params 实际出现的 level"），故不计为未关闭项。

## 范围外观察

- `LevelRosterFixtureLoadTestBase::TearDown()` 恢复了 `AssetsPath`，但**未**恢复 loader 的全局 `Entries`/`Params`（跑完后残留最后一个 e2e 用例加载的表）与 `SetUpTestSuite()` 里设置的 `gbIsSpawn = false`。当前无用例依赖这两者的初始值（shuffle 4242 + 全量 27 均绿），后续若加"未加载时 `GetLevelRoster()` 应为空"之类用例需注意。
- 报告"全量 731/0/100% + drift ok（PASS A/B/C/C2/E）"按指示未重跑；diff 不含 `CMake/`、`assets/`、`tools/` 改动，无跨二进制影响面，陈述可信但仍属未独立核实。
- 工作区在本次复审中保持干净（`git status --porcelain` 空），未做任何改动。

## 裁决

**修复轮次：全部问题已处理且无新增 Critical/Important 破坏。**

未关闭项：无。
可选跟进（均非阻塞）：①`LoadLevelRosterFromFiles()` doc 补一句 spawn 宽松校验语义；②修正基类注释中 "once per binary" 的措辞；③如在意畸形表可观测性，诊断可遍历 `Entries ∪ Params` 的 level 并集。
