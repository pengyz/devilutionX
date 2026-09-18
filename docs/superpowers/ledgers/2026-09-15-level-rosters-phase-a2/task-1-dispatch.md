# A2 / Task 1 分发稿（校验层范围化 + L17-24 前置基线）

**前置**：任务 D（L14 core 重塑）必须先结束或明确搁置——**两者都会改 `level_roster_baseline_test.cpp`/TSV**，串行执行。

**控制者已核实的事实（可直接采用）**
| 事实 | 出处 |
|---|---|
| `ValidateLevelRoster(entries, params)` 是**纯函数**（无全局依赖），由 `LoadLevelRoster()` 调用 | `Source/tables/level_roster.cpp:180`（定义）／`:360`（调用） |
| HF 的数据覆盖机制＝**mod 以同路径提供同名完整表**（base `monstdat.tsv` 112 行 → `mods/hf/.../monstdat.tsv` 138 行；base 里 24 只 `Never` 的怪在 HF 下可用） | `mods/hf/txtdata/monsters/monstdat.tsv` |
| 层上限语义：`giNumberOfLevels = gbIsHellfire ? 25 : 17`（**其声明头文件执行者必须自行定位**，`diablo.h` 里没有） | `Source/diablo.cpp:2732` |
| CI 只有 `spawn.mpq` → 任何 HF 相关用例必须**贴真实依赖探测后 `GTEST_SKIP`** | 阶段 A 的 `genrl.trn` 先例；`HellfireNoParamsSamplingTest::missingHellfire_` |
| 新增测试二进制必须同时登记 `CMake/Tests.cmake` 与 `run_tests.py` 的 `TEST_TARGETS`（已有检查 F 兜住） | `tools/check_drift.py` 检查 F |

**Task 1 的交付**
1. `ValidateLevelRoster` 增加**层范围**参数：逐层检查（存在性/可用性/floors 可满足/`core` 非空）只对 `level <= maxLevel` 生效；**全局检查**（重复行、`max_image>0`、`tail_draw>=0`、`squad_*` 范围、哨兵拒绝、unique base 白名单——**更正：实为逐层检查（`IsUniqueBaseForLevel(level,type)`），由层范围约束**；其余全局检查不受影响；
2. `LoadLevelRoster()` 传入真实层上限（自行定位符号并**在报告记录来源**）；
3. **合成用例**证明范围化的两个方向（`maxLevel=16` 放行 L17-24 行；`maxLevel=24` 对其 fatal）——**且必须可失败**（反证：撤掉范围化 → 前者红）；
4. `[ A2BASELINE ]` 测量用例（HF 资产门控）：L17-24 的 placed class mix / 远程占比 / 类型数，无条件打印并写入报告——**这是后续验收 8 的基线，没有它不许改数据**；
5. 门禁（含 F）+ eval smoke + 推送后跟踪 CI 到终态；报告第一分钟落盘。

**禁止**：改任何 ceiling/floor 阈值；改 L1-16 既有数据；`DISABLED_`/占位；写规格文件（控制者维护）。
