# Task 1 修复轮次 1 报告

状态：完成，全部验证通过

## 待修问题

- F1: `UNPACKED_MPQS` 构建下 `HellfireLevelBaselineTest::SetUpTestSuite()` 的自挂 `hellfire.mpq`
  代码段不编译（`MpqArchiveT = std::string` 时 `MpqArchive::Open`/`insert_or_assign(..., MpqArchive)`
  类型不匹配）；同配置下 `UnloadModArchives()` 清理段被整体 `#ifndef UNPACKED_MPQS` 关闭。
- F2: `LevelRosterTest` 套件的 42 处两参 `ValidateLevelRoster(entries, params)` 调用隐式依赖全局
  `gbIsHellfire`（当前二进制从不设置，故读到 false／maxLevel=16，但未来任何用例改写该全局都会静默
  改变这批断言的含义）。

## 计划

1. F1：把 `test/level_roster_baseline_test.cpp` 里 `HellfireLevelBaselineTest::SetUpTestSuite()`
   中「自挂 hellfire.mpq」的整段（含 `#include "mpq/mpq_reader.hpp"`）包进 `#ifndef UNPACKED_MPQS`；
   该配置下直接 `missingHellfire_ = true`。
2. F2：在 `test/level_roster_test.cpp` 的 `LevelRosterTest::SetUpTestSuite()` 里显式钉住
   `gbIsHellfire = false`，加注释说明原因；可选加断言证明套件运行期间恒为 false。
3. 验证：
   - 默认配置构建 `level_roster_test` + `level_roster_baseline_test`，跑通。
   - 独立目录 `build-unpacked/` 下 `UNPACKED_MPQS=ON` 构建 `level_roster_baseline_test`，
     修复前失败、修复后成功（两次输出都记录于此）。
   - `python3 tools/run_tests.py --json /tmp/ci.json`
   - `python3 -m tools.eval.backend --smoke`
4. 分块提交 + 推送 + CI 跟踪。

## 实际改动

### F1 — `test/level_roster_baseline_test.cpp`

- `#include "mpq/mpq_reader.hpp"` 改为仅在 `#ifndef UNPACKED_MPQS` 下 include。
- `HellfireLevelBaselineTest::SetUpTestSuite()` 里原来的自挂 `hellfire.mpq` 整段（搜索
  `paths::BasePath()/PrefPath()/ConfigPath()`、`MpqArchive::Open`、
  `MpqArchives.insert_or_assign(kHellfireMpqPriority, ...)`）包进 `#ifndef UNPACKED_MPQS`。
- 紧随其后的 `.til` 探测块（探测 `nlevels\l6data\l6.til` / `nlevels\l5data\l5.til` 判定
  `missingHellfire_`）同样只在 `#ifndef UNPACKED_MPQS` 下执行；`#else` 分支下直接
  `missingHellfire_ = true`（因为该配置下从未挂载 hellfire.mpq，探测毫无意义，直接判定缺失更诚实）。
- `LoadMonsterData(); LoadLevelRoster();` 保持无条件执行（不受影响）。
- 两处改动都附带注释，解释为什么 `UnloadModArchives()` 的清理段在该宏下同样被关闭
  （即挂载/卸载两端在 `UNPACKED_MPQS` 下都没有定义语义），以及为什么不为该配置伪造一个
  目录路径等价挂载（超出本任务范围）。

### F2 — `test/level_roster_test.cpp`

- `LevelRosterTest::SetUpTestSuite()` 里在既有 `gbIsSpawn = false;` 之后新增
  `gbIsHellfire = false;`，附注释说明：套件内 42 处两参 `ValidateLevelRoster(entries, params)`
  调用都隐式依赖该全局决定的默认 `maxLevel`（16 vs 24），今天读到 false 是初始化顺序的偶然结果，
  显式钉住让这批断言的含义由声明保证、不受进程内其他用例/全局改写影响。
- 额外加了 `TearDown() override`，用 `EXPECT_FALSE(gbIsHellfire)` 断言套件运行期间该全局
  始终为 false，任何未来用例意外翻转它都会在原地报错，而不是让后续用例的默认 maxLevel
  静默变成 24。
- 前置核实：grep 确认 `LevelRosterTest` 套件体内（本文件 `namespace {` 重开之前）没有任何
  用例直接触碰 `gbIsHellfire`，也没有子类化 `LevelRosterTest`，故 `TearDown` 断言不会误报。

## F1 验证：UNPACKED_MPQS 隔离构建（`build-unpacked/`）

配置命令：
```
cmake -S. -Bbuild-unpacked -G Ninja -DUNPACKED_MPQS=ON -DDEVILUTIONX_SYSTEM_BENCHMARK=OFF \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=/usr/bin/g++-13 -DCMAKE_C_COMPILER=/usr/bin/gcc-13
```

### 修复前（复现 F1）

```
cmake --build build-unpacked --target level_roster_baseline_test -j8
```
关键报错（完整日志见任务过程 `/tmp/f1-before-fix.log`）：
```
Source/engine/assets.hpp:341:15: error: 'ModManifest' does not name a type
...
no match for 'operator=' (operand types are 'std::__cxx11::basic_string<char>' and 'devilution::MpqArchive')
no matching function for call to 'std::__cxx11::basic_string<char>::basic_string(devilution::MpqArchive)'
```
后两条与 F1 的预测完全一致：`MpqArchives.insert_or_assign(kHellfireMpqPriority, std::move(*archive))`
在 `MpqArchiveT = std::string` 下类型不匹配。第一条 `ModManifest` 报错是一个**与本任务无关的、
预先存在的引擎缺陷**（见下方「发现的问题（不在本任务范围内）」），它会在到达 F1 的错误之前先
挡住整个 `libdevilutionx` 的编译——即它会挡住 `UNPACKED_MPQS` 下**任何**目标的构建，不止
`level_roster_baseline_test`。

为了在不越权修复该无关缺陷的前提下，仍然对 F1 的修复做真实编译验证，验证过程中临时应用了
一处一行改动（把 `Source/engine/assets.hpp` 里 `#include "mods/mod_identity.h"` 移出
`#ifndef UNPACKED_MPQS` 块，绕开 `ModManifest` 报错），验证完成后**已完整还原**
（`git diff --stat Source/engine/assets.hpp` 为空，见下方 git status 章节）。此举纯粹是为了让
本任务的 F1/F2 改动（均只在 `test/` 内）获得真实的编译证据，不构成对该引擎缺陷的永久修复。

### 修复后（临时绕开无关缺陷 + 应用本任务 F1 改动）

```
cmake --build build-unpacked --target level_roster_baseline_test -j8
```
结果：exit code 0，日志中 0 处 "error:"，最终产物：
```
build-unpacked/level_roster_baseline_test: ELF 64-bit LSB pie executable, x86-64, ...,
  dynamically linked, ..., with debug_info, not stripped
```
随后立即执行 `cp /tmp/assets.hpp.orig Source/engine/assets.hpp` 还原临时绕开；还原后
`git diff --stat Source/engine/assets.hpp` 无输出（确认已完全恢复原状，未混入提交）。

**`build-unpacked/` 目录处置**：保留在工作区（已被 `.gitignore` 的 `/build-*/` 规则忽略，
`git status --ignored` 确认不会被跟踪/提交），不删除，供后续如需复查该配置的编译状态时复用。

## F1+F2 验证：默认配置构建

```
cmake --build build --target level_roster_test level_roster_baseline_test -j8
```
结果：exit code 0，日志中 0 处 "error:"。

### `level_roster_test` 运行结果

```
[==========] 41 tests from 4 test suites ran. (332 ms total)
[  PASSED  ] 41 tests.
```
其中 `LevelRosterTest` 套件本身 29 个用例全部通过（每个都新经过了 F2 新增的 `TearDown()` 守卫，
无一失败），另含 `ParseClassFloorsTest`(6)、`LevelRosterFixtureLoadTest`(4)、
`LevelRosterShippedLoadTest`(2)。运行中出现两条
`ERROR: MPQ with priority 9000/9200 is already registered, skipping ...`，
为既有行为（与 F1/F2 无关，非本次改动引入的回归）。

### `level_roster_baseline_test` 运行结果

```
[==========] 11 tests from 3 test suites ran. (562874 ms total)
[  PASSED  ] 11 tests.
```
三个套件：`LevelRosterBaselineTest`(3)、`SquadPlacementTest`(7)、`HellfireLevelBaselineTest`(1)，
全部 11 个用例通过，包含 Task 1 新增的 4 个用例（`LevelRosterBaselineTest.PlacedClassMixWithinBaseline`
等，逐层 `[ MEASURED ]` 输出均在各自 ceiling 之内）。`HellfireLevelBaselineTest` 在本环境中
真实挂载了 hellfire.mpq 并测出 L17-24 的 `[ A2BASELINE ]` 数据（未 SKIP，说明本机确实具备
Hellfire 资产，F1 的 `#ifndef UNPACKED_MPQS` 分支在默认配置下按预期走的是原挂载路径，
未受本次改动影响）。运行中同样出现 `ERROR: MPQ with priority ... already registered` 提示
（既有行为）和一条 UBSan 运行时提示
（`Source/levels/drlg_l2.cpp:2072:47: runtime error: index 40 out of bounds for type 'unsigned char [40]'`，
既有代码路径，未导致用例失败，与本次改动无关，未处理）。

## 发现的问题（不在本任务范围内，仅报告）

`Source/engine/assets.hpp:341` 无条件声明
`[[nodiscard]] ModManifest ReadModManifestByName(std::string_view name);`，但定义 `ModManifest`
的 `#include "mods/mod_identity.h"` 只在 `#ifndef UNPACKED_MPQS` 下才被 include（第 30-32 行）。
后果：`UNPACKED_MPQS=ON` 时**整个引擎库 `libdevilutionx`（因而每一个测试目标）都无法编译**，
不止本任务涉及的两个文件。

- 已确认该缺陷与本次 F1/F2 改动无关、且早于本次改动存在：直接构建 `libdevilutionx` 目标
  （不涉及任何 test/ 文件）在 `UNPACKED_MPQS=ON` 下同样失败在同一行。
- 已用 `git blame -L29,31 Source/engine/assets.hpp` 确认该 `#ifndef` 保护范围由提交
  `1bf1b9886`（"Mod hashing and manifest (#8608)"）引入。
- 已用 `git show master:Source/engine/assets.hpp` 确认上游 diasurgical/DevilutionX 的
  master 分支不存在该缺陷（其保护块只包 `mpq_reader.hpp`，不包 `mod_identity.h`）——即这是
  本 fork 下游引入的缺陷，非上游遗留。

建议后续单开一个任务修复（把 `mods/mod_identity.h` 的 include 移出该 `#ifndef` 块，或改为
让 `ReadModManifestByName` 的声明本身也受该宏保护，需先确认调用方在 `UNPACKED_MPQS` 下的
预期行为）。本任务未做任何永久性改动到 `assets.hpp`。

## 行尾核查

两个改动文件均相对 merge-base（`e00b7260fff69f42a3d16b60353abced6ee04ace`）为新增文件，按
漂移规则 C2 需整文件 CRLF：

```
test/level_roster_baseline_test.cpp  CRLF: 1685  LF-only: 0
test/level_roster_test.cpp           CRLF: 788   LF-only: 0
```
（编辑前后均用 Python 逐字节核对，非仅凭 `file` 命令。）

## git status

```
 M test/level_roster_baseline_test.cpp
 M test/level_roster_test.cpp
```
无其他改动；`Source/engine/assets.hpp` 的临时验证性改动已还原并确认 diff 为空；
`build-unpacked/` 为 `.gitignore` 忽略的构建产物目录，不出现在 `git status`（非 `--ignored`）输出中。

## 全量门禁 / eval smoke

### `python3 tools/run_tests.py --json /tmp/ci.json`

```json
{
  "steps": {
    "build": { "ok": true, "message": "build ok" },
    "ctest": {
      "passed": 772, "failed": 0, "skipped": 3, "not_run": 0, "total": 772,
      "failures": [], "passed_pct": 100, "returncode": 0
    },
    "drift": {
      "drift_ok": true, "passes": 6,
      "output": "PASS A  Tests.cmake entries have source files\nPASS B  no placeholder assertions in tests\nPASS C  modified files keep line endings\nPASS C2 added files match .editorconfig\nPASS E  no test-only production functions\nPASS F  run_tests.py builds every registered test target\n\nmerge-base: e00b7260f   check E allowlist: 10 upstream symbols\n"
    }
  }
}
```
`passed_pct == 100`、`failed == 0`、`drift_ok == true`，退出码 0。全量构建了 79 个测试目标
（含本次涉及的 `level_roster_test`/`level_roster_baseline_test`，均已确认在
`tools/run_tests.py` 的 `TEST_TARGETS` 手抄清单中，不会触发
`gotcha_run_tests_target_list_misses_new_binaries` 描述的陈旧二进制问题）。

### `python3 -m tools.eval.backend --smoke`

```
D1-EVAL: running 37 case(s) (git c214b1ce3a)
  evaluated: 37  passed: 37  failed: 0  skipped: 0  pass_rate: 1.0
  by category:
    dark-expedition      4/4 (1.0)
    save-load            2/2 (1.0)
    mechanics            8/8 (1.0)
    rng                  1/1 (1.0)
    data                 6/6 (1.0)
    render               2/2 (1.0)
    combat               5/5 (1.0)
    utility              9/9 (1.0)
```
exit code 0，37/37 全过（含 `rng/level-roster-validation`）。本次改动只涉及测试基础设施
（编译期条件、fixture 全局钉住），未改变任何引擎行为，因此未新增/更新 eval case。

## 提交与 CI

分两个逻辑提交（`feature/qol-upgrades` 分支，推送 `myrepo`）：

```
23b74f5b6 test(roster): keep the hellfire fixture building under UNPACKED_MPQS
9f668b19a test(roster): pin gbIsHellfire in LevelRosterTest fixture
```

推送后 CI（`Better D1 CI`，run `35204052664`）：`build-and-test` job 全绿
（Build/Run full test suite/Drift check/Upload test report 均 ✓，耗时 5m31s），
`gh run watch -R pengyz/devilutionX 35204052664 --exit-status` 以 exit code 0 收尾，
`gh run list` 显示该 run 状态为 `completed success`。`eval-nightly`/`eval-smoke` 两个 job
未运行（工作流按 `workflow_dispatch` 配置，非本次 push 触发条件，不属于本次改动的回归）。
