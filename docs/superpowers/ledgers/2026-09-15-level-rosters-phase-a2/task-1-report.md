# A2 / Task 1 报告：校验层范围化 + L17-24 前置基线实测

**状态**：进行中（本文件第一分钟落盘，边做边追加）

## BASE

- 起始 HEAD：`c1b92fd62 docs(spec): sync the species count and the tail_draw alternative after task D`
- 工作树：干净（`git status --short` 无输出）
- 分支：`feature/qol-upgrades`

## 进度

- [x] 步骤 1：合成用例（失败方向）
- [x] 步骤 2：确认失败
- [x] 步骤 3：实现层范围化
- [x] 步骤 4：`LoadLevelRoster()` 传真实层上限
- [x] 步骤 5：`[ A2BASELINE ]` 测量用例
- [x] 步骤 6：门禁 + eval + 提交 + CI

## 层上限符号：定位结果与一个必须绕开的坑

任务要求"`LoadLevelRoster()` 传入真实层上限"。符号定位如下：

| 项 | 值 |
|---|---|
| 符号 | `giNumberOfLevels` |
| **声明头文件** | `Source/loadsave.h:18` —— `extern DVL_API_FOR_TEST uint8_t giNumberOfLevels;` |
| 定义 | `Source/loadsave.cpp:51` |
| 赋值点 | `Source/diablo.cpp:2732`（`StartGame()` 内）、`Source/pfile.cpp:755`（`pfile_ui_save_create()`）、`Source/loadsave.cpp:2500/2505/2797/2801`（读档） |
| 语义 | **计数**（含城镇）：`gbIsHellfire ? 25 : 17` |

**坑（决定了实现取法）**：`giNumberOfLevels` 不能在名册加载时直接读。

- `LoadLevelRoster()` 在 `Source/diablo.cpp:2813`，位于 `DiabloMain()` 的启动数据加载段；
- 最早的 `giNumberOfLevels` 赋值在 `Source/diablo.cpp:2732`，位于 `StartGame()` —— 该函数由 `mainmenu_loop()` 之后才进入，**严格晚于** `:2813`。

即：`LoadLevelRoster()` 运行时 `giNumberOfLevels` 仍是零初始化的 `0`。若直接采用，`maxLevel` 恒为 0 → **所有逐层检查静默关闭**，比不做范围化更糟（校验器变哑巴，且没有任何红灯提示）。

**采用的做法**：新增 `MaxValidatedDungeonLevel()`（`Source/tables/level_roster.h` / `.cpp`），由 `gbIsHellfire` 直接推导 —— 与 `giNumberOfLevels` 同一输入，但去掉城镇槽，返回**最高地下层**（HF 24 / 否则 16）。`gbIsHellfire` 在名册加载前已就位：hf mod 的 `mods/hf/lua/mods/hf/init.lua` 调用 `hellfire.enable()`（`Source/lua/modules/hellfire.cpp:14` 置 `gbIsHellfire = true`），而 `LuaReloadActiveMods()`（`Source/lua/lua_global.cpp:243`）在跑完 mod init 之后才 `LoadMonsterData()` + `LoadLevelRoster()`（`:278-279`）。启动路径上 `LuaInitialize()`（`diablo.cpp:2797`）同样早于 `:2813`。

### 为什么用默认实参而不是重载（漂移检查 E）

既有两参调用（`level_roster.cpp` 的加载器 + `level_roster_test.cpp` 里 ~30 处）必须继续可用。做法是给第三参 `maxLevel` 一个默认实参 `= MaxValidatedDungeonLevel()`，**不新增重载**：重载会产出一个"只有测试调用者、没有生产调用者"的导出符号，正是检查 E 要拦的形状。`MaxValidatedDungeonLevel()` 自身有生产调用者（默认实参在 `LoadLevelRosterFromFiles()` 的两参…实为三参调用点展开），实测检查 E 通过（见门禁段）。

## 范围化的边界（实现事实）

**受层范围约束（逐层检查）**，`level > maxLevel` 跳过：
- 逐行可用性 `IsAvailableAt`
- unique base 白名单 `allow_unique_boost`
- `class_floors` 在 B1 caps 下可满足性
- core 非空
- core-vs-cap（R29）

**不受层范围影响（全局/结构检查）**：
- 类型存在性（越界 `_monster_id`）
- 重复 `(level, monster_id)` 行、重复 params 行
- `max_image > 0`、`tail_draw >= 0`
- `squad_chance <= 100`、`squad_size <= 3`、`squad_chance>0 && squad_size==0`
- `BehaviorClass::Count` 哨兵拒绝

## 合成用例（两方向）

`test/level_roster_test.cpp` 新增 4 个用例。载体 `MT_UNRAV`：基础 `monstdat.tsv` 为 `availability=Never`、窗口 17-18；hf overlay 同路径同名整表把它变为可用。

| 用例 | 方向 |
|---|---|
| `ValidationSkipsLevelsAboveTheActiveLevelRange` | `maxLevel=16` → L17 行**放行** |
| `ValidationStillChecksLevelsInsideTheActiveLevelRange` | `maxLevel=24` → 同一张表**fatal**（`not available` + `level 17`） |
| `GlobalChecksIgnoreTheActiveLevelRange` | 8 类全局检查在 `maxLevel=16` 下对 L20 行**仍然拒绝** |
| `CoreNonEmptyAndCoreVsCapAreScopedToTheActiveLevelRange` | core 非空 / core-vs-cap 两侧各钉一次 |

### 步骤 2：改动前确认失败

三参形式不存在 → 编译失败（`too many arguments to function ... ValidateLevelRoster(span, span)`，`level_roster_test.cpp:230` 等 8 处）。

### 反证（两次实跑，均改实现而非改测试）

**反证 1 —— 撤掉层范围化**（把 4 处范围判定改成恒真/恒假，等价于不做范围化）：

```
[  FAILED  ] LevelRosterTest.ValidationSkipsLevelsAboveTheActiveLevelRange (0 ms)
[  FAILED  ] LevelRosterTest.CoreNonEmptyAndCoreVsCapAreScopedToTheActiveLevelRange (0 ms)
 2 FAILED TESTS
```

**反证 2 —— 把全局检查也塞进范围条件**（类型存在性 / `max_image` / `tail_draw` / squad 列 / 哨兵 / 两处重复行检查都加 `level > maxLevel` continue）：

```
[  FAILED  ] LevelRosterTest.GlobalChecksIgnoreTheActiveLevelRange (0 ms)
 1 FAILED TEST
```

两次反证后均已 `cp` 还原真实实现，`level_roster_test` 41/41 绿。

## `[ A2BASELINE ]` L17-24 改动前基线（步骤 5）

新增 `HellfireLevelBaselineTest.A2PreChangeBaselineForHellfireLevels`（`test/level_roster_baseline_test.cpp`）。口径与既有 `PlacedClassMixWithinBaseline` 完全一致：**200 seeds/层**（seed base 61000）、分母＝**实际放置怪物数**、ranged ＝ `RangedTurret + RangedKite`、驱动真实生产夹具（`CreateDungeonForMeasurement` → `GetLevelMTypes` → `InitMonsters`）。

**前提断言（不是装饰）**：跑之前逐层 `ASSERT` L17-24 **仍无 params 行、仍无名册行**（即仍走 R28 legacy）。Task 2 数据一旦落地，这个用例会立刻红——因为那时它测的就不再是"改动前"基线。每个样本另断言 `ActiveMonsterCount > 0`（否则 0/0 会读成"0% ranged"，一个永远无法被超过的假基线）。

### 原始输出（本机实跑，未经加工）

```
[ A2BASELINE ] level 17 placed 16188 types 6 ranged 0 rangedShare 0 (0+0)/16188 Melee 12729 RangedTurret 0 RangedKite 0 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 3459
[ A2BASELINE ] level 18 placed 16188 types 6 ranged 0 rangedShare 0 (0+0)/16188 Melee 13782 RangedTurret 0 RangedKite 0 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 2406
[ A2BASELINE ] level 19 placed 16388 types 6 ranged 1983 rangedShare 0.121003 (0+1983)/16388 Melee 10030 RangedTurret 0 RangedKite 1983 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 4375
[ A2BASELINE ] level 20 placed 16342 types 6 ranged 3054 rangedShare 0.18688 (0+3054)/16342 Melee 7211 RangedTurret 0 RangedKite 3054 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 6077
[ A2BASELINE ] level 21 placed 22619 types 4.43 ranged 0 rangedShare 0 (0+0)/22619 Melee 8627 RangedTurret 0 RangedKite 0 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 13992
[ A2BASELINE ] level 22 placed 22712 types 4.37 ranged 0 rangedShare 0 (0+0)/22712 Melee 13228 RangedTurret 0 RangedKite 0 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 9484
[ A2BASELINE ] level 23 placed 22648 types 4.555 ranged 3337 rangedShare 0.147342 (0+3337)/22648 Melee 12699 RangedTurret 0 RangedKite 3337 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 6612
[ A2BASELINE ] level 24 placed 22653 types 5 ranged 0 rangedShare 0 (0+0)/22653 Melee 11099 RangedTurret 0 RangedKite 0 Rally 0 Charge 0 Sneak 0 Summon 0 Boss 11554
[       OK ] HellfireLevelBaselineTest.A2PreChangeBaselineForHellfireLevels (89076 ms)
```

### 供 Task 2 抄写的基线（`kRangedShareBaseline[17..24]`）

沿用既有表的"分子/分母"可审计写法：

| level | placed | 类型数(均) | ranged | rangedShare | 写法 |
|---|---|---|---|---|---|
| 17 | 16188 | 6.00 | 0 | 0.000000 | `(0.0 + 0.0) / 16188.0` |
| 18 | 16188 | 6.00 | 0 | 0.000000 | `(0.0 + 0.0) / 16188.0` |
| 19 | 16388 | 6.00 | 1983 | 0.121003 | `(0.0 + 1983.0) / 16388.0` |
| 20 | 16342 | 6.00 | 3054 | 0.186880 | `(0.0 + 3054.0) / 16342.0` |
| 21 | 22619 | 4.43 | 0 | 0.000000 | `(0.0 + 0.0) / 22619.0` |
| 22 | 22712 | 4.37 | 0 | 0.000000 | `(0.0 + 0.0) / 22712.0` |
| 23 | 22648 | 4.56 | 3337 | 0.147342 | `(0.0 + 3337.0) / 22648.0` |
| 24 | 22653 | 5.00 | 0 | 0.000000 | `(0.0 + 0.0) / 22653.0` |

### ⚠️ 必须随基线一起读的口径缺口（实测，非猜测）

`GetBehaviorClass()`（`Source/tables/monstdat.cpp:483`）只枚举 Diablo 的 AI，其余全部落进 `default: return BehaviorClass::Boss`。L17-24 上有 **10 个 AI 命中该 default**：`ArchLich`、`Diablo`、`FireBat`、`FireMan`、`Lich`、`Necromorb`、`Psychorb`、`Scavenger`、`Torchant`（`BoneDemon` 已被枚举为 `RangedKite`）。

后果，两条都要记住：
1. 上表的 `Boss` 列很大（L21 甚至 13992/22619）**不是**"这些层塞满了 boss"，而是 default 分支的聚集；
2. 其中 `Lich`/`ArchLich`/`Necromorb`/`Psychorb`/`FireMan` 等在玩法上是**远程施法者**，因此 `rangedShare` **系统性低估**了 L17-24 的真实远程压力 —— 尤其 L17/18/21/22/24 的 `0.000000` 不代表"零远程"。

**本任务不修这个缺口**：改分类会动 §4.5 的口径（控制者维护的规格），且会静默改变既有 `kRangedShareBaseline[1..15]` 的测量前提。验收 8 是**同口径**的相对比较（改动前后都用这套 `GetBehaviorClass`），在此前提下这份基线是有效参照。该说明已写入用例注释，并在用例里以 `[ A2BASELINE ] NOTE:` 无条件打印，避免只看日志的人误读 0。

### 为跑起来 L17-24 必须补的两处夹具（都是真实执行顺序，不是测试便利）

1. **`LoadRealMegaTiles` / 触发器分派补 `DTYPE_NEST` / `DTYPE_CRYPT`**：L17-20＝Nest（`nlevels\l6data\l6.til`）、L21-24＝Crypt（`nlevels\l5data\l5.til`），触发器为 `InitHiveTriggers()` / `InitCryptTriggers()`。路径与分派均照抄 `diablo.cpp:LoadLvlGFX`（`:1429-1438`）与 `CreateLevel`（`:1481-1486`）。缺这两处时 `pMegaTiles` 为空，首个 L17 建关在 `DRLG_LPass3`（`gendung.cpp:594`）直接 SEGV（已实测）。
2. **`hellfire.mpq` 直接挂载（priority 8000）**：`TestInitGame(..., hellfire=true)` 只挂 **mod** 归档 `mods/hf`（提供 L17-24 的 monstdat 覆盖），Nest/Crypt 的**图形**在 `hellfire.mpq` 里。生产路径是 mod 的 `init.lua` → `hellfire.loadData()` → `LoadHellfireArchives()`，但**不能**在测试里用它：该函数还要求 `hfmonk/hfmusic/hfvoice.mpq`，缺任一就 `DisplayFatalErrorAndExit()`——本机三者全缺（已核实），会整个测试二进制退出。本测量只需**关卡贴图**，故用 `MpqArchive::Open` + `MpqArchives` 直接挂在 8000（`LoadHellfireArchives` 自己用的值；`UnloadModArchives()` 清 8000-8999，故 TearDown 仍能卸载）。

### 资产门控（CI 安全）

`HaveHellfire()` 只说明 `hellfire.mpq` 被**找到**，不代表贴图可读。因此门控探测**真实依赖**：`nlevels\l6data\l6.til` + `nlevels\l5data\l5.til` 各开一次（两种 HF 关卡类型各一），任一不可读即 `GTEST_SKIP`。CI 只有 `spawn.mpq` → 整例跳过，不失败。

### 全局状态隔离（R30）

该套件挂 HF overlay 并置 `gbIsHellfire = true`，会污染同一二进制里以 Diablo 数据测量的既有套件。照 `HellfireNoParamsSamplingTest` 先例，`SetUpTestSuite` 快照 / `TearDownTestSuite` 还原 `Quests`、`Players`、`sgGameInitInfo`、`gbIsMultiplayer`、`gbIsHellfire`、`gbIsSpawn`、`PrefPath`，并重新 `UnloadModArchives()` + `LoadModArchives({})` + `LoadMonsterData()` + `LoadLevelRoster()`，使 `--gtest_shuffle` 下后继套件不会继承 HF 数据。

## 步骤 6：门禁 + eval

### 定向构建

```
cmake --build build --target level_roster_test level_roster_baseline_test -j8
```

退出 0，无错误无告警。

### 全量门禁

```
python3 tools/run_tests.py --json /tmp/ci.json
```

```
"ctest": { "passed": 772, "failed": 0, "skipped": 3, "not_run": 0,
           "total": 772, "failures": [], "passed_pct": 100, "returncode": 0 }
"drift": { "drift_ok": true, "passes": 6,
           "PASS A / PASS B / PASS C / PASS C2 / PASS E / PASS F" }
```

`failed == 0`、`passed_pct == 100`、`drift_ok == true`（含检查 F：两个二进制都已在 `CMake/Tests.cmake:61-62` 与 `tools/run_tests.py:53` 注册，本轮无需新增注册）。skipped 3 是既有的资产门控用例。

**中途一次真红（已修，记录以免重犯）**：首跑 `FAIL C2 added files match .editorconfig — test/level_roster_baseline_test.cpp: line endings are LF, .editorconfig wants CRLF`。原因：该文件相对 merge-base 算**新增**文件，故受 C2（整文件必须匹配 `.editorconfig` 的 CRLF）约束，而不是 C（只要求保持既有行尾）；我新写入的 26 行注释是 LF。整文件规范化为 CRLF 后 C2 转 PASS（1663/1663 行 CRLF）。四个改动文件现均为纯 CRLF：`level_roster.cpp` 408/408、`level_roster.h` 170/170、`level_roster_test.cpp` 765/765、`level_roster_baseline_test.cpp` 1663/1663。

### eval（行为变更必产，CLAUDE.md §2）

本改动是**加载期行为变更**（同一张表在非 Hellfire 下从"拒绝启动"变为"跳过 L17-24"），按 CLAUDE.md 必须产 eval。既有 case 无一绑定 `level_roster_test`（`level-rosters` 绑的是 `level_roster_baseline_test`），故新增：

- `eval/cases/rng/level-roster-validation.yaml`（id `level-roster-validation`，binary `level_roster_test`，filter `LevelRosterTest.*:ParseClassFloorsTest.*`，`passed_min: 35`、`output_contains: ["[  PASSED  ] 35 tests."]`）
- 进 **smoke**（不只 nightly）：实测 0.2s 且 `mpq_required: false`（已用 `HOME`/`XDG_DATA_HOME` 指向空目录实跑验证仍 35/35 绿），所以"非 Hellfire 安装起不来"这类回归能在提交前就被拦住
- `eval/cases/_smoke.yaml` 手加该 id；`_nightly.yaml` 由 `python3 tools/eval/sync_case_sets.py --write` 重生成（68 cases）；`--check` 输出 `OK: 68 cases, 2 suites consistent`

单跑：

```
python3 -m tools.eval.backend --run level-roster-validation
  [PASS] [rng] level-roster-validation (15/15)
```

**该 eval case 的反证（实跑，改实现不改测试）**：把范围判据全部改成恒真/恒假后重建，case 判红且报文精确：

```
[FAIL] [rng] level-roster-validation (0/15) exit_code=1, expected 0 | failed=2 > allowed 0
       | passed=33 < required 35 (vacuous pass guard)
```

即它既抓失败用例、也抓"用例数变少"的空过。恢复实现后重跑回 `[PASS] (15/15)`。

### smoke 门禁

```
python3 -m tools.eval.backend --smoke
```

37 个 case 全 PASS，各分类 1.0（`rng 1/1` 即新 case），退出码 0。

## 提交与 CI

### 提交

`dec13e7b2d5dcdd58a36209249d6bb5b5164ab91` — `feat(roster): validate rosters only up to the active level range`

改动面（`git diff --stat`，534 插入 / 7 删除）：

| 文件 | 性质 |
|---|---|
| `Source/tables/level_roster.cpp` | `MaxValidatedDungeonLevel()` + 逐层检查范围化 + 调用点传上限 |
| `Source/tables/level_roster.h` | 声明 + 「哪些检查受范围约束、哪些不受」的文档，并注明为何用默认实参而非重载（避免漂移检查 E 判为测试专用导出符号） |
| `test/level_roster_test.cpp` | 4 个新用例（两方向 + 范围边界两条） |
| `test/level_roster_baseline_test.cpp` | `HellfireLevelBaselineTest` 套件 + `[ A2BASELINE ]` 测量 |
| `eval/cases/rng/level-roster-validation.yaml` | 新增 eval case |
| `eval/cases/_smoke.yaml` / `_nightly.yaml` | 注册该 case |

**边界核实（提交前逐条对过）**：`assets/txtdata/**` 零改动（数据是 Task 2）；`docs/superpowers/specs/**` 零改动（控制器所有）；既有天花板/地板阈值（`kRangedShareCeiling`、`kSquadFormationFloor`、`class_floors`）零改动；L1-16 既有数据零改动。本报告落在 `docs/superpowers/ledgers/`，被 `docs/superpowers/ledgers/.gitignore` 忽略（`git check-ignore` 已确认），故不在提交里。

### CI

推送 `myrepo feature/qol-upgrades`：`900f5e552..dec13e7b2`。

```
gh run list -R pengyz/devilutionX
gh run watch -R pengyz/devilutionX 35195790721 --exit-status   # 退出 0
```

run **35195790721**，headSha `dec13e7b2d5dcdd58a36209249d6bb5b5164ab91`，conclusion **success**（4m24s；Build / Run full test suite / Drift check 全绿）。
<https://github.com/pengyz/devilutionX/actions/runs/35195790721>

**CI 侧核实门控确实生效**（不是"碰巧绿"）：

```
332/772 Test #332: HellfireLevelBaselineTest.A2PreChangeBaselineForHellfireLevels ***Skipped 0.01 sec
```

即 CI（只有 `spawn.mpq`）按设计 `GTEST_SKIP`，本机（有 HF 资产）真实执行并产出上面的 `[ A2BASELINE ]` 数字。

## 遗留给 Task 2 的事项

1. `kRangedShareBaseline` / `kRangedShareCeiling` 目前是 `std::array<double, 17>`（按层索引，L16 为 unconstrained 哨兵）。加 L17-24 前必须先把数组扩到 24 并按本报告的转录表填值，再把这些层纳入 `PlacedClassMixWithinBaseline` 的循环。
2. 验收 8（相对改动前基线 ≤ +5pp）的基线就是本报告 `[ A2BASELINE ]` 那张表。**没有它不许改数据。**
3. `GetBehaviorClass()` 的分类缺口（见上文 caveat）本轮**故意未修**：它会改动 §4.5 的分类口径并移动既有 L1-16 基线的测量前提。验收 8 是同口径的 like-for-like 比较。
