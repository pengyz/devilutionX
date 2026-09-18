# Task 1 修复轮 R1 重发复审

复审基线 `504522f8` → Head `26df82431`（仅一个提交）。范围：F1-F5 裁决 + 修复自身引入的新破坏。只读复审，未改动工作区/暂存区/HEAD（`git status --porcelain` 空）。

### 问题裁决

- **F1【严重】SOLData 未加载 → 基线伪数据 — ADDRESSED**
  - SOL 加载已加入：`test/level_roster_baseline_test.cpp:121` `ASSERT_TRUE(LoadLevelSOLData().has_value())`，位于 `CreateDungeonForMeasurement()` 末尾；两个 TEST 都在该函数返回后才调 `GetLevelMTypes()`/`InitMonsters()`（`:166-168`、`:201-203`），故**确实在 `InitMonsters` 之前**。顺序放在 `CreateDungeon` 之后而非之前，与引擎 `Source/diablo.cpp:3433`（SOL）→`:3472`(`LoadGameLevelStandardLevel`→`CreateLevel`→`InitMonsters`) 的相对次序不同，但已核实 `CreateDungeon`/DRLG 路径不读 `SOLData`（全仓 `IsTileSolid`/`SOLData[` 调用点：`monster.cpp`、`themes.cpp`、`items.cpp`、`player.cpp`、`inv.cpp`、`tile_properties.cpp`、`scrollrt.cpp`；`Source/levels/gendung.cpp`、`drlg_l1.cpp` 零命中），且本测试不调 `CreateThemeRooms()`，因此该次序差异不影响 `InitMonsters` 里 `na` 的计算（`Source/monster.cpp:3751-3756`）。
  - 新基线逐层不同：计划文档 `docs/superpowers/plans/2026-09-15-level-rosters-phase-a.md:660-677`，L1=18235（均值 91.2）… L15=23193，不再恒定 38000。仅 L16 仍为 38000（=190×200），已核实是引擎 `currlevel==16` 特判：`Source/monster.cpp:3733-3735` 先 `LoadDiabMonsts()` 加载 4 个固定 `diabNx.dun`，配合 `:3761-3762` 的 `MaxMonsters-10` 截断而饱和，报告的解释与代码一致。
  - 文档回填已核实：旧数字**零残留**（`grep 19041|5714|21136|13683|12626|6333|16809|17401|18228|15860|20295|28594|19111|16864|18104|14496` 在 `docs/superpowers/plans/`、`docs/superpowers/specs/` 全库零命中）；`38000` 仅剩 L16 一行。「实施记录」百分比已替换为 `L13 22.8%/L14 55.9%/L15 58.3%`（:648），与新表行内数字一致（`(3249+2076)/23405=22.75%`、`(9674+3425)/23443=55.87%`、`13533/23193=58.35%`）。全 16 行 placed == 类别计数之和（脚本核对，全 OK）。
  - 独立复跑核实：`SAMPLING_REPORT=/tmp/rereview-baseline.md ./level_roster_baseline_test --gtest_filter=...PlacedClassMixReport` → PASS(139.3s)，产出的 16 行与文档表**逐字节一致**，报告声明「L1 placed=18235」为真。

- **F2【重要】setup 逐字重复 / 各自 LoadGameArchives — ADDRESSED**
  - 已改为 fixture：`test/level_roster_baseline_test.cpp:136-159` `class LevelRosterBaselineTest : public ::testing::Test` + `SetUpTestSuite()`（内含唯一一次 `LoadGameArchives()`，:140）+ 静态 `missingMpqAssets_`（:156、:159），两个 TEST 改为 `TEST_F` 并只留 3 行跳过判断（:161-164、:180-183）。形态与 `test/sampling_behavior_test.cpp:80-99` 一致。全文件 `LoadGameArchives` 仅 1 次。

- **F3【重要】零断言 106s 用例进门禁 — ADDRESSED**
  - 默认小样本：`:187-188` `fullReport = getenv("SAMPLING_REPORT") != nullptr`，`seedsPerLevel = fullReport ? 200 : 5`；写文件与表头也全部由 `fullReport` 守卫（:191-195、:220-228）。
  - 真实不变量：`:207-214` 每个样本断言 `ActiveMonsterCount > 0`、`<= MaxMonsters-10`（`MaxMonsters=200`，`Source/monster.h:38`）、`sum(mix) == ActiveMonsterCount`；单层用例另加 `EXPECT_LE(total, MaxMonsters-10)`（:176）。
  - 实测核实「默认不跑满 200」：未设环境变量整二进制 3.30s（2 tests PASS），设置后单测 139s，约 42× 差距，与 5/200 比例吻合；ctest 记录同样为 3.18s（`build/Testing/Temporary/LastTest.log:7922+`）。

- **F4【重要，R8】MeasurePlacedClassMix 形参未使用 — ADDRESSED**
  - `:124` 改为无参 `MeasurePlacedClassMix()`；两处调用点同步更新（`:170`、`:204`），全文件无残留带参调用。单 TU 重编译零警告（复用 `compile_commands.json` 原命令，exit 0，无 `-Wunused` 输出）。

- **F5【重要，R9】UBSan lighting.cpp:99 越界必须消失 — ADDRESSED**
  - 根因已按要求修：`:102-118` 按 `leveltype` 分派 `InitL1..L4Triggers()`，`:119` `Freeupstairs()`，均在 SOL 与 `InitMonsters` 之前；与 `Source/diablo.cpp:1461-1490` `CreateLevel` 的真实分派一致（本测试 L1-16 只覆盖 CATHEDRAL/CATACOMBS/CAVES/HELL，`default` 分支 `FAIL()`，`GetLevelType` 见 `Source/levels/gendung.cpp:336-354`，覆盖完备）。`InitL1..L4Triggers` 均以 `numtrigs = 0` 开头并递增（`Source/levels/trigs.cpp:111/154/174/202`），因此 `monster.cpp:3737-3744` 的 `trigs[i]` 循环不再读未初始化项。
  - 实测核实：构建为 Debug + `-fsanitize=address,undefined`（`build/compile_commands.json`；测试二进制含 `__ubsan` 符号），单层测试与默认 16 层测试各自输出 `grep -icE "runtime error|lighting.cpp|out of bounds"` = **0**；`build/Testing/Temporary/LastTest.log` 全量门禁日志同样 0 命中。报告的「零匹配」声明为真。

### 修复 Diff 中的新增破坏

无 Critical/Important 新增破坏。

低（非阻塞，记录）：`CreateDungeonForMeasurement` 现在把 `InitLevelMonsters()`（`:99`）吸收进建关辅助函数内部，函数名不再反映其职责（建关 + 重置怪物表 + 加载 SOL）；纯可读性问题，不影响正确性。

### 范围外观察

- **R10（pMegaTiles/.til）披露准确**：`LoadRealMegaTiles()`（`:49-72`）加载的 4 条 `.til` 路径与引擎 `Source/diablo.cpp:1408-1427` `LoadLvlGFX` 的 CATHEDRAL/CATACOMBS/CAVES/HELL 分支**逐字符一致**；报告称「不加载 .cel/special cels」与代码一致。必要性说明也成立：`TileHasAny` 读 `SOLData[dPiece[x][y]]`（`Source/levels/dun_tile_data.hpp:204-207`），`dPiece` 由 `DRLG_LPass3` 从 `pMegaTiles` 计算，旧代码的全零假缓冲（对比 `test/drlg_test.hpp:83`）确实会让 `dPiece` 恒为 0；报告「只修 F1 会 total=0」的因果链可信。
- **R11（HoldThemeRooms/InitThemes/InitGolems/InitObjects 暂缓）不影响 F1-F5 判定**。理由：`InitGolems` 的 4 个预留位（`Source/monster.cpp:3725-3728`）与 `InitObjects`/主题房间的 `Populated` 排除只会让 placed 系统性偏低若干个，不会把它变回与几何无关的常量；F1 要求的「逐层不同」已成立。仅提醒下游：当前基线相对引擎真值略低（缺 golem 占位 + 未排除主题房/物件占格），任务 4 标定 `kRangedShareCeiling` 用的是**占比**，受此影响很小，但绝对 placed 数不宜当作引擎真值引用。
- 简报第 9 行要求的 `MeasureRealisedTypes` 至今未实现（全 `test/` 零命中）。该缺口在修复前即存在、本轮 diff 未触及，归入范围外；任务 3 若指望复用同一夹具会缺这个接口。
- L16 恒定值的排查结论只在报告文字中，代码里无回归保护；后续若 `MaxMonsters` 或 diab*.dun 变化，L16 行会静默改变。

### 裁决

**修复轮次：** 全部问题已处理且无新增 Critical/Important 破坏 —— 无未关闭项。F1/F2/F3/F4/F5 全部 ADDRESSED；报告三项关键声明（L1 placed=18235、默认 5 seeds 不跑满 200、UBSan 消失）均已独立复跑核实为真；R10 披露准确。
