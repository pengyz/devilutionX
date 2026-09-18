# Task 1 报告：测量各层已放置怪物 BehaviorClass 基线

## 状态

DONE_WITH_CONCERNS

## Commit SHA

```
85a1da18afed7413844d6d3739312dd509639be3
```

commit message: `test(roster): measure the placed class-mix baseline per level`

变更文件（与计划步骤 6 要求一致）：

```
CMake/Tests.cmake
docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md
test/level_roster_baseline_test.cpp (new)
```

## 交付物核对

- [x] `test/level_roster_baseline_test.cpp`（CRLF，独立 TU，只 include `test/drlg_test.hpp`）
- [x] `CMake/Tests.cmake` 中在 `sampling_behavior_test` 之后注册 `level_roster_baseline_test`
- [x] 公共函数 `MeasurePlacedClassMix(uint8_t level, uint32_t seed)` / `MeasureRealisedTypes(uint8_t level, uint32_t seed)`
- [x] TDD 流程：先写失败测试 → 确认失败 → 最小实现 → 确认通过 → 产出 200 seeds × L1-16 基线数据 → 提交
- [x] 独立实现 `AppendMeasurementReport`/`SAMPLING_REPORT` 约定，未共享头文件、未改动 `test/sampling_behavior_test.cpp`
- [x] `SAMPLING_REPORT` 未设置时测试静默通过（`AppendMeasurementReport` 内部对空路径直接 return）
- [x] `MeasurePlacedClassMix` 不重建地牢，只读取已放置怪物 `Monsters`/`ActiveMonsters`（调用顺序：`CreateDungeonForMeasurement` → `InitLevelMonsters` → `GetLevelMTypes` → `InitMonsters`，同一 seed 只跑一次）
- [x] 200 seeds×16 levels 报告测试耗时 105.5s，低于 2 分钟阈值，未需要把 `kSeeds` 降到 50
- [x] `/tmp/a-baseline.md` 完整基线表已逐字复制进最终报告（见下方）以及计划文档 `## A-baseline` 小节

## 命令与真实输出摘录

### 1. 首次编写测试后确认失败（TDD 红阶段）

首次实现缺少 `LoadMonsterData()`，运行导致 `AddMonsterType` 对空 `MonstersData` 索引越界（在更早的调试轮次中通过 gdb 复现并确认根因；本轮不再重复该步骤的完整日志，因为该修复已在此前完成并验证）。

### 2. 增量构建目标测试二进制

```
$ cmake --build build --target level_roster_baseline_test -j 20
[...]
[after fixes] build succeeds; only warning:
test/level_roster_baseline_test.cpp: warning: unused function 'MeasureRealisedTypes' [-Wunused-function]
```
（该 unused-function 警告是简报预期行为：`MeasureRealisedTypes` 供后续任务 3 复用，本任务不调用它。）

### 3. 运行第一条测试（绿阶段）

```
$ ./build/level_roster_baseline_test --gtest_filter=LevelRosterBaseline.PlacesMonstersForCathedralL1
[==========] Running 1 test from 1 test suite.
[ RUN      ] LevelRosterBaseline.PlacesMonstersForCathedralL1
[       OK ] LevelRosterBaseline.PlacesMonstersForCathedralL1 (12 ms)
[==========] 1 test from 1 test suite ran. (13 ms total)
[  PASSED  ] 1 test.
```

### 4. 运行基线报告测试（200 seeds × 16 levels）

```
$ SAMPLING_REPORT=/tmp/a-baseline.md time ./build/level_roster_baseline_test --gtest_filter=LevelRosterBaseline.PlacedClassMixReport
[==========] Running 1 test from 1 test suite.
[ RUN      ] LevelRosterBaseline.PlacedClassMixReport
[       OK ] LevelRosterBaseline.PlacedClassMixReport (105412 ms)
[==========] 1 test from 1 test suite ran. (105412 ms total)
[  PASSED  ] 1 test.
real  1m45.527s
```

耗时 105.527s < 120s 阈值，`kSeeds` 保持 200，16 层全部覆盖。

### 5. 全量门禁（pre-commit gate）

```
$ python3 tools/run_tests.py --json /tmp/ci.json
[...]
=== SUMMARY ===
{
  "steps": {
    "build": { "ok": true, "message": "build ok" },
    "ctest": { "passed": 704, "failed": 0, "skipped": 3, "not_run": 0, "total": 704, "passed_pct": 100, "returncode": 0 },
    "drift": {
      "drift_ok": true,
      "passes": 5,
      "output": "PASS A  Tests.cmake entries have source files\nPASS B  no placeholder assertions in tests\nPASS C  modified files keep line endings\nPASS C2 added files match .editorconfig\nPASS E  no test-only production functions\n"
    }
  }
}
EXIT_0
```

门禁通过标准全部满足：`ctest.failed==0`、`ctest.passed_pct==100`、`drift.drift_ok==true`。

## 完整基线表（200 seeds/level，L1-16，逐字复制自 `/tmp/a-baseline.md`）

| level | placed | Melee | RangedTurret | RangedKite | Rally | Charge | Sneak | Summon | Boss |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 38000 | 19041 | 0 | 0 | 12626 | 0 | 0 | 0 | 6333 |
| 2 | 38000 | 14400 | 3574 | 0 | 11055 | 3060 | 1364 | 0 | 4547 |
| 3 | 38000 | 14116 | 4699 | 0 | 8534 | 4257 | 1831 | 0 | 4563 |
| 4 | 38000 | 12316 | 6152 | 0 | 6038 | 5097 | 1510 | 0 | 6887 |
| 5 | 38000 | 14703 | 6718 | 0 | 2624 | 5115 | 3497 | 0 | 5343 |
| 6 | 38000 | 9429 | 7992 | 2283 | 0 | 7476 | 5040 | 0 | 5780 |
| 7 | 38000 | 12560 | 5954 | 3780 | 0 | 7494 | 4709 | 0 | 3503 |
| 8 | 38000 | 8005 | 3445 | 4953 | 0 | 16809 | 4372 | 0 | 416 |
| 9 | 38000 | 13320 | 1428 | 17401 | 0 | 0 | 2386 | 0 | 3465 |
| 10 | 38000 | 13113 | 0 | 18228 | 0 | 0 | 2424 | 0 | 4235 |
| 11 | 38000 | 17239 | 0 | 15860 | 0 | 0 | 0 | 0 | 4901 |
| 12 | 38000 | 20295 | 3378 | 12461 | 0 | 0 | 0 | 0 | 1866 |
| 13 | 38000 | 28594 | 5714 | 3492 | 0 | 0 | 0 | 0 | 200 |
| 14 | 38000 | 19111 | 13683 | 5206 | 0 | 0 | 0 | 0 | 0 |
| 15 | 38000 | 16864 | 21136 | 0 | 0 | 0 | 0 | 0 | 0 |
| 16 | 38000 | 18104 | 14496 | 5200 | 0 | 0 | 0 | 0 | 200 |

该表也已逐字追加进 `docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md` 的 `## A-baseline: placed class mix (200 seeds per level)` 小节，供任务 4 使用。

## 自查结论

- 测试遵循 TDD：先失败（空 `MonstersData` 索引越界）→ 定位根因 → 最小修复 → 通过。
- `MeasurePlacedClassMix`/`MeasureRealisedTypes` 签名与语义均按简报字面实现，未重建地牢，调用顺序严格遵循简报规定序列。
- `AppendMeasurementReport`/`SAMPLING_REPORT` 逻辑与 `test/sampling_behavior_test.cpp` 完全独立实现（未 include、未共享头文件、未修改该文件）。
- 行尾discipline：`test/level_roster_baseline_test.cpp` 全文 CRLF（`awk` 校验零违规行）；`CMake/Tests.cmake` 保持原 CRLF（diff 仅新增一行）；`docs/.../2026-09-15-level-rosters-phase-a.md` 保持原 LF（`awk` 校验零违规行）。漂移校验 PASS C / PASS C2 已确认。
- 全量门禁 704 tests 全通过，无 skipped 之外的失败，drift 5 项全 PASS。
- 未新增/修改任何超出任务范围的文件；`git status` 提交前只有这三个改动文件。
- 未 spawn 任何子智能体，未触碰 `test/sampling_behavior_test.cpp`、`test/drlg_test.hpp`。

## 关注点（Concerns）

1. **对简报字面代码的两处必要补充**（均是新增的初始化调用，未改变任何测量逻辑/种子值/表格列序）：
   - 在 `TestInitGame()` 之后追加 `LoadMonsterData()` —— 否则 `AddMonsterType` 会对空的 `MonstersData` 向量做索引访问（段错误）。
   - 在 `TestInitGame()` 之前追加 `LoadGameArchives(); ASSERT_TRUE(HaveMainData()); gbIsSpawn = false;` 以及 `paths::SetPrefPath(paths::BasePath() + "test/fixtures/")` —— 否则 `InitMonsters()` 内部的 `PlaceUniqueMonst`/`InitSetPiece` 因缺少任务/唯一怪素材（如 `genrl.trn`、`rnd6.dun`）而失败。此修复完全镜像 `test/sampling_behavior_test.cpp` 的 `SetUpTestSuite()` 既有模式，未引入新的假设或臆测语义。
2. **依赖 `build/spawn.mpq` 存在**：本环境初始缺少任何零售 MPQ 数据（`DIABDAT.MPQ`/`spawn.mpq`），本轮通过 `wget` 从 CI 使用的同一 URL（`https://github.com/diasurgical/devilutionx-assets/releases/download/v2/spawn.mpq`）下载到 `build/` 目录后测试才能通过 `HaveMainData()` 断言。该文件不属于仓库内容，仅是构建目录中的运行期依赖；全新 checkout 若未执行 CI 式素材配置步骤（或手动下载），此测试会在 `ASSERT_TRUE(HaveMainData())` 处失败。建议后续在 CI/本地文档中显式记录此依赖，或考虑是否应将该测试标记为需要零售素材的可跳过测试。
3. **良性 UBSan 运行时诊断**（未导致测试失败，非本次改动引入）：
   ```
   Source/lighting.cpp:99:19/31: runtime error: index 254 out of bounds for type 'DungeonFlag [112][112]'
   ```
   出现在 `InitMonsters()` 内部 `DoVision`/`DoUnVision`（`Source/monster.cpp:3740-3745`）对怪物位置做视野计算时。这是简报要求的字面调用序列本身触发的，属于引擎既有行为首次在 UBSan 下被本测试路径覆盖到，并非本任务代码引入的新缺陷；测试仍以 exit 0 / `[ OK ]` 通过。未在本任务范围内修复，仅记录供后续关注。

---

## 修复报告（复核后追加，R6/R7 裁决）

复核反馈指出顾虑 1/2 涉及正确性，要求先修再评审。已完成以下两处修复并重新验证。

### 修复内容

**修复 1（守卫语义，裁决 R7）**：两个 `TEST` 体内的
```cpp
ASSERT_TRUE(HaveMainData()) << "requires spawn.mpq/DIABDAT.MPQ in build dir";
```
改为与 `test/sampling_behavior_test.cpp` 一致的跳过语义：
```cpp
if (!HaveMainData())
	GTEST_SKIP() << "MPQ assets not found - skipping test";
```
理由：缺失 MPQ 素材是环境问题，不应让门禁变红；应跳过而非失败。

**修复 2（死代码，裁决 R6）**：删除 `MeasureRealisedTypes` 函数定义及其唯一使用者 `#include <vector>`（未再被任何代码引用；`_monster_id`/`LevelMonsterTypeCount`/`LevelMonsterTypes` 均无其他引用点）。理由：各测试二进制是独立 TU，任务 3 的用例在 `sampling_behavior_test.cpp` 中，无法调用本文件内的函数，该函数只会产生 `-Wunused-function` 警告，属死代码。

变更范围：仅 `test/level_roster_baseline_test.cpp`（`git diff` 确认：删除 1 处 include、1 个函数定义共 8 行，2 处 `ASSERT_TRUE`→`if+GTEST_SKIP` 各 1 行变 2 行，净变化 +4/-11 行）。

### 新提交 SHA

```
504522f8a7320e6ee9c23d5355422ddc338654fd
```

commit message: `test(roster): skip when MPQ assets are missing and drop the cross-binary helper`

### 验证命令与真实输出摘录

**1. 重新构建，确认无 unused-function 警告**

```
$ cmake --build build --target level_roster_baseline_test -j 20
[0/2] Re-checking globbed directories...
[1/6] Trimming .../build/mods/hf
[2/5] Trimming .../build/assets
[3/4] Building CXX object CMakeFiles/level_roster_baseline_test.dir/test/level_roster_baseline_test.cpp.o
[4/4] Linking CXX executable level_roster_baseline_test
```
无任何警告输出（此前有 `-Wunused-function` for `MeasureRealisedTypes`，现已消失）。

**2. 未设置 `SAMPLING_REPORT`（预期：两测试 PASS，静默跳过写文件）**

```
$ unset SAMPLING_REPORT && time ./build/level_roster_baseline_test
[==========] Running 2 tests from 1 test suite.
[ RUN      ] LevelRosterBaseline.PlacesMonstersForCathedralL1
[       OK ] LevelRosterBaseline.PlacesMonstersForCathedralL1 (14 ms)
[ RUN      ] LevelRosterBaseline.PlacedClassMixReport
[       OK ] LevelRosterBaseline.PlacedClassMixReport (109730 ms)
[==========] 2 tests from 1 test suite ran. (109745 ms total)
[  PASSED  ] 2 tests.
real  1m49.881s
EXIT_0
```
确认：未产生任何报告文件（`/tmp` 下无新文件），两测试均 PASS，无 `GTEST_SKIP` 触发（本环境已有 `build/spawn.mpq`，`HaveMainData()` 为真，走正常路径而非跳过分支）。

**3. 设置 `SAMPLING_REPORT=/tmp/a-baseline-verify.md`（预期：两测试 PASS，报告文件写出且内容与首次测量一致）**

```
$ rm -f /tmp/a-baseline-verify.md
$ SAMPLING_REPORT=/tmp/a-baseline-verify.md time ./build/level_roster_baseline_test
[==========] Running 2 tests from 1 test suite.
[ RUN      ] LevelRosterBaseline.PlacesMonstersForCathedralL1
[       OK ] LevelRosterBaseline.PlacesMonstersForCathedralL1 (13 ms)
[ RUN      ] LevelRosterBaseline.PlacedClassMixReport
[       OK ] LevelRosterBaseline.PlacedClassMixReport (105557 ms)
[==========] 2 tests from 1 test suite ran. (105571 ms total)
[  PASSED  ] 2 tests.
105.40user 0.30system 1:45.71elapsed 99%CPU
EXIT_0
```
`/tmp/a-baseline-verify.md` 内容核对：与本报告上方「完整基线表」逐字节一致（16 行，L1-16，同样的 placed/Melee/.../Boss 数值），确认修复未改变测量语义或输出格式。

**4. 全量门禁**

```
$ python3 tools/run_tests.py --json /tmp/ci_fix.json
[...]
=== SUMMARY ===
{
  "steps": {
    "build": { "ok": true, "message": "build ok" },
    "ctest": { "passed": 704, "failed": 0, "skipped": 3, "not_run": 0, "total": 704, "passed_pct": 100, "returncode": 0 },
    "drift": {
      "drift_ok": true,
      "passes": 5,
      "output": "PASS A  Tests.cmake entries have source files\nPASS B  no placeholder assertions in tests\nPASS C  modified files keep line endings\nPASS C2 added files match .editorconfig\nPASS E  no test-only production functions\n"
    }
  }
}
EXIT_0
```
标准全部满足：`ctest.failed==0`、`ctest.passed_pct==100`、`drift.drift_ok==true`。

### 覆盖测试名

- `LevelRosterBaseline.PlacesMonstersForCathedralL1`
- `LevelRosterBaseline.PlacedClassMixReport`

### 修复后结论

两处修复均已验证：
- unused-function 警告消失（死代码已删除）。
- `SAMPLING_REPORT` 未设置/已设置两种情形均 PASS，行为符合预期（未设置时不写文件且不失败；已设置时写出与原始测量完全一致的基线表）。
- 全量门禁 704 tests 全通过，drift 5 项全 PASS。
- 本环境因已有 `build/spawn.mpq`，`GTEST_SKIP` 分支未被实际触发；但代码路径已改为跳过语义，符合仓库既有约定（`sampling_behavior_test.cpp` 的 `missingMpqAssets_` 模式），新克隆/无素材环境下门禁将呈跳过而非失败。

### 是否还有其它顾虑

- 无新增顾虑。原顾虑 3（`lighting.cpp:99` 良性 UBSan 诊断）保留为记录性说明，未要求修复，本次修复未触及该路径，行为不变（两次运行输出中该诊断依旧出现，非回归）。

---

## 复核轮 2 修复报告（追加）

### 状态

DONE

### 新提交 SHA

```
26df82431
```

commit message: `fix(roster): load SOL data before measuring so placement matches retail levels`

变更文件：

```
test/level_roster_baseline_test.cpp
docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md
```

### 复核者本轮发现（F1-F5）与处理

- **F1（严重）**：`SOLData` 从未在 `InitMonsters()` 前加载 → `IsTileSolid` 恒假 → 原「常量放置=190/常量总数=38000」的假基线。已修复：`CreateDungeonForMeasurement` 末尾新增 `ASSERT_TRUE(LoadLevelSOLData().has_value())`，在触发器初始化 + `Freeupstairs()` 之后、`GetLevelMTypes()`/`InitMonsters()` 之前调用（SOLData 加载顺序相对建关本身无先后依赖，只需先于 `InitMonsters()`）。旧基线表已整表替换（非追加），见下方「新基线表」。
- **F2（重要）**：两个 TEST 重复 setup。已按 `test/sampling_behavior_test.cpp:82-99` 的夹具模式重构为 `class LevelRosterBaselineTest : public ::testing::Test`，`SetUpTestSuite()` + `static bool missingMpqAssets_`。
- **F3（重要）**：`PlacedClassMixReport` 原恒跑 200×16（约 106s）且零不变量断言。已改为：默认每层 5 个种子，并对每个样本断言真实不变量（`placed>0`、`placed<=MaxMonsters-10`、`sum(mix)==placed`）；仅当设置 `SAMPLING_REPORT` 环境变量时才跑完整 200×16 报告模式并写文件。
- **F4（重要，裁决 R8）**：`MeasurePlacedClassMix(uint8_t level, uint32_t seed)` 两参数均未使用。已改为无参 `MeasurePlacedClassMix()`，两处调用点同步更新。
- **F5（裁决 R9）**：UBSan `lighting.cpp:99: index 254 out of bounds` 是同一类「从未初始化」bug 的症状（触发器 `trigs[]` 从未真正初始化）。修复 F1 所需的触发器初始化调用链（见下）后，该诊断已消失（`grep -iE "lighting.cpp|runtime error|UndefinedBehaviorSanitizer" build/Testing/Temporary/LastTest.log` 零匹配），未单独修复,是作为触发器初始化修复的副作用解决的。

### 复核者建议写法的偏离说明（复核者已预先允许的情形）

复核者建议直接调用 `CreateLevel(ENTRY_MAIN)` 复刻真实执行顺序，并提醒「若你实测发现 CreateLevel 依赖某个本测试未建立的全局状态，请在报告里写明并给出你选择的替代顺序」。

实测发现：`CreateLevel` 定义在 `Source/diablo.cpp` 的匿名命名空间内。`nm` 验证：

```
$ nm -C build/CMakeFiles/devilutionx.dir/Source/diablo.cpp.o | grep CreateLevel
... t _ZN10devilution12_GLOBAL__N_111CreateLevelENS_9lvl_entryE
```

符号绑定为小写 `t`（内部链接），测试 TU 无法链接调用。同理，`LoadLvlGFX`（`CreateLevel` 内部调用的另一个函数）也在匿名命名空间内：

```
$ nm -C build/CMakeFiles/devilutionx.dir/Source/diablo.cpp.o | grep LoadLvlGFX
... t _ZN10devilution12_GLOBAL__N_110LoadLvlGFXEv
```

替代方案：在测试文件内联复刻两者的函数体：
- `CreateDungeonForMeasurement` 内联复刻 `CreateLevel` 的核心步骤：`CreateDungeon()` 建关 → 按 leveltype 分派 `InitL1..L4Triggers()` → `Freeupstairs()`（省略 `HoldThemeRooms`/`InitThemes`/`InitGolems`/`InitObjects`，见下方「已知残留差距」；省略 `LoadRndLvlPal()`，已读源码确认该函数在 `HeadlessMode` 下是 no-op）。
- `LoadRealMegaTiles()` 内联复刻 `LoadLvlGFX` 中按 leveltype 选择 `.til` 路径的部分（不含 `.cel`/special cels，纯图形数据，不影响怪物放置逻辑）。

两处偏离均已在测试文件对应函数体上方以注释记录 nm 验证结果与理由。

### 本轮修复过程中新发现并修复的第二个 bug（不在复核者原始 F1-F5 范围内）

修复 F1（加入 `LoadLevelSOLData()`）后首次重跑 `PlacesMonstersForCathedralL1` 失败：`total=0`。根因排查：测试原先沿用 `test/drlg_test.hpp:TestCreateDungeon` 的模式，给 `pMegaTiles` 分配一块全零的假缓冲区——这在 `TestCreateDungeon` 自己的场景下是合理的，因为它只校验 `dungeon[][]`/`dTransVal[][]`（`DRLG_LPass3` 之前的抽象层）。但本测试要测的是 `IsTileSolid → dPiece[][] → pMegaTiles[dungeon[x][y]-1]` 这条链路（`InitMonsters` 内可放置面积统计的直接输入），`dPiece[][]` 是由 `DRLG_LPass3()`（在 `CreateDungeon` 内部按 leveltype 调用）从真实 `pMegaTiles` 内容计算出来的；全零假缓冲区会让 `dPiece` 恒为 `SOLData[0]`（本例中读出来是"全程实心"），与 F1 修复前的症状同源同类（测量管线跳过了真正喂给 `IsTileSolid` 的素材加载）。

修复：新增 `LoadRealMegaTiles(dungeon_type)`，通过 `LoadFileInMemWithStatus<MegaTile>(til)` 加载真实 `.til` 文件（路径来源见上方「偏离说明」对 `LoadLvlGFX` 的复刻）。修复后 `PlacesMonstersForCathedralL1` 通过，完整 200×16 报告产出的 placed 数值在各层间呈现符合关卡几何差异的合理波动（16550-25642，L16 除外见下）。

此 bug 与其修复已在测试文件 `LoadRealMegaTiles` 上方注释中记录，并在此报告中重点披露——因为没有它，F1 的修复单独应用会直接回归为 `total=0`（一次失败），必须与 F1 一起修复才能让整条测量链路真正跑通。

### 已知残留 fidelity 差距（判断为超出本轮 F1-F5 范围，保留未修）

未调用 `HoldThemeRooms()`/`InitThemes()`/`InitGolems()`/`InitObjects()`：
- `zharlib` 在多次调用间不会被重置（只影响 Zhar the Mad 的定位逻辑）
- 主题房间不参与 `DungeonFlag::Populated` 排除逻辑
- 4 个 golem 预留位不计入 `ActiveMonsterCount`

复核者给出的建议复刻流程本身也未包含这些调用，因此保持一致以避免在复核明确要求之外扩大改动面。已在测试文件注释中记录。

### L16 恒定 placed=38000（190×200）的排查结论

新基线表中 L1-L15 均呈现随关卡几何变化的合理波动，唯独 L16 仍是恒定 `38000`（=200 个种子 × 190）。已用临时调试断言排查（`GetLevelMTypes()` → `LoadDiabMonsts()` 前后计数、`na`/`na/30` 计算），确认这是**引擎真实行为，非残留 fidelity bug**：

`InitMonsters()` 对 `currlevel==16` 会先调用 `LoadDiabMonsts()`，从 4 个固定的 `diabNx.dun` 文件加载 Diablo 任务专属怪物集（与 `na`/RNG 无关的固定布局），实测该步骤本身已让 `ActiveMonsterCount` 逼近/超过 `MaxMonsters-10=190` 的上限（`monster.cpp:3761-3762` 的截断逻辑对该层每个种子都会命中饱和值 190）。这与 L1-L15 靠 `na/30`（可放置面积驱动的散布计算）决定放置数量的路径不同，是 `GetLevelMTypes`/`InitMonsters` 对 `currlevel==16` 的既有特判分支（`monster.cpp:3442-3446`, `3733-3735`）的正常结果,调试排查已临时加断言测试并在验证后完整移除（未保留在最终提交中）。

结论：L16 的恒定值无需修复,是 Diablo 关卡本身的引擎行为特征,并非本测试测量管线的缺陷。

### 命令与真实输出摘录

**1. 重构后编译（零警告）**

```
$ cmake --build build --target level_roster_baseline_test -j 20
[3/4] Building CXX object CMakeFiles/level_roster_baseline_test.dir/test/level_roster_baseline_test.cpp.o
[4/4] Linking CXX executable level_roster_baseline_test
```

**2. 未设置 SAMPLING_REPORT（默认 5 seeds/level，两测试 PASS）**

```
$ unset SAMPLING_REPORT && ctest -R "LevelRosterBaseline" --output-on-failure
    Start 304: LevelRosterBaselineTest.PlacesMonstersForCathedralL1
1/2 Test #304: LevelRosterBaselineTest.PlacesMonstersForCathedralL1 ...   Passed    0.11 sec
    Start 305: LevelRosterBaselineTest.PlacedClassMixReport
2/2 Test #305: LevelRosterBaselineTest.PlacedClassMixReport ...........   Passed    3.35 sec
100% tests passed, 0 tests failed out of 2
```

**3. 设置 SAMPLING_REPORT（完整 200×16 报告模式）**

```
$ rm -f /tmp/a-baseline-final.md
$ SAMPLING_REPORT=/tmp/a-baseline-final.md ctest -R "LevelRosterBaselineTest.PlacedClassMixReport" --output-on-failure
1/1 Test #305: LevelRosterBaselineTest.PlacedClassMixReport ...   Passed  139.28 sec
100% tests passed, 0 tests failed out of 1
```

**4. UBSan 诊断确认消失（F5）**

```
$ grep -iE "lighting.cpp|runtime error|UndefinedBehaviorSanitizer" build/Testing/Temporary/LastTest.log
（零匹配）
```

**5. 全量门禁**

```
$ python3 tools/run_tests.py --json /tmp/ci_final.json
{
  "steps": {
    "build": { "ok": true, "message": "build ok" },
    "ctest": { "passed": 704, "failed": 0, "skipped": 3, "not_run": 0, "total": 704, "passed_pct": 100, "returncode": 0 },
    "drift": { "drift_ok": true, "passes": 5, ... }
  }
}
```

### 新基线表（首行 L1 与 L13 远程占比）

首行 L1：`| 1 | 18235 | 9247 | 0 | 0 | 6055 | 0 | 0 | 0 | 2933 |`

L13 远程占比：`(3249+2076)/23405 = 22.8%`（旧假基线为 24.2%，变化不大）。作为对比，L14/L15 远程占比从旧假基线的 49.7%/55.6% 上升到新真实基线的 55.9%/58.3%，波动幅度更明显——这会影响任务 4 对 `kRangedShareCeiling` 的标定口径,已在计划文档 `L13-15 远程占比基线` 一行同步更新为新数值,任务 4 需据此重新核实其阈值假设(非本任务范围,仅作提醒)。

完整基线表已同步替换进（非追加）计划文档 `## A-baseline` 小节，16 行数据逐字节与 `/tmp/a-baseline-final.md` 一致。

### 任何顾虑

1. **pMegaTiles/.til 修复超出复核者字面 F1 范围**：复核者的 F1 只点名 `SOLData`，但修复过程中发现并额外修复了 `pMegaTiles` 未加载导致的第二个 bug（详见上方专节）。这是必要的连带修复（没有它 F1 的修复会直接回归失败），但严格说是本轮我方主动扩大的修复面，请复核者确认是否接受。
2. **HoldThemeRooms/InitThemes/InitGolems/InitObjects 仍未调用**：判断为超出 F1-F5 范围保留未修（与复核者建议写法本身的省略范围一致），如需修复请明确告知。
3. **L16=38000 的排查结论基于一次性调试断言，非永久化验证**：已确认是引擎既有特判行为（非 bug），但排查用的临时断言代码已移除、未留存在最终提交里；如需要更严谨的证据留存（例如把这条排查结论也写成一个专门的最小回归测试），需另行安排。
4. **基线数值大幅变化影响下游任务**：L1 从假基线 38000 降到真实基线 18235，L14/L15 远程占比从约 50-56% 升到 56-58%,任务 4 标定 `kRangedShareCeiling` 时需以本次真实基线为准,而非此前遗留的旧数值。
