# UNPACKED_MPQS 编译阻塞修复报告

## 问题归属

- 上游 `master` 无此问题：`git show master:Source/engine/assets.hpp` 中，`#ifndef UNPACKED_MPQS` 只
  guard 了 `mpq/mpq_reader.hpp`。
- 下游提交 `1bf1b9886`（"Mod hashing and manifest #8608"）在同一个 guard 里追加了
  `#include "mods/mod_identity.h"`，同时在文件末尾（原 `:341` 一带）**无条件**声明了
  `[[nodiscard]] ModManifest ReadModManifestByName(std::string_view name);`。
  `ModManifest` 的定义来自 `mods/mod_identity.h`，一旦该 include 被 guard 掉
  （即 `-DUNPACKED_MPQS=ON` 时），`ModManifest` 未声明 → `libdevilutionx` 的每个编译单元
  在 include `assets.hpp` 时都报 `'ModManifest' does not name a type`，导致整个库编不过，
  与 `test/` 目录无关。

## 修复

`Source/engine/assets.hpp`：把 `#include "mods/mod_identity.h"` 移出
`#ifndef UNPACKED_MPQS` guard，保持 `#include "mpq/mpq_reader.hpp"` 仍受 guard
（`MpqArchive` 仅在非 `UNPACKED_MPQS` 分支被使用，guard 是正确的）：

```cpp
// `ModManifest` (from mod_identity.h) is used unconditionally below by
// `ReadModManifestByName`, so this include must stay outside the guard even
// though it is otherwise only needed for the packed-MPQ mod pipeline.
#include "mods/mod_identity.h"
#ifndef UNPACKED_MPQS
#include "mpq/mpq_reader.hpp"
#endif
```

纯 include/编译期改动，不改变任何运行时行为。

## 同类符号排查（交底）

在 `assets.hpp` / `assets.cpp` 中搜索了 `UNPACKED_MPQS`、`MpqArchive`、`ModManifest`、
`mod_identity` 的所有出现位置：

- `assets.hpp` 里唯一"声明无条件、定义受 guard 影响"的符号就是 `ReadModManifestByName`
  （已修复其依赖）。
- `assets.hpp:122` 的 `AssetRef::archive` 成员（`MpqArchive *archive`）位于
  `#else`（即非 `UNPACKED_MPQS`）分支的 `struct AssetRef` 定义内，本身受
  `#ifdef UNPACKED_MPQS ... #else ... #endif` 整体切换保护，不受影响。
- `assets.hpp:305/307` 的 `MpqArchiveT` 用 `#ifdef UNPACKED_MPQS` 分别定义为
  `std::string` 或 `MpqArchive`，同样是整体切换，不是本次 guard 泄漏的问题模式。
- `assets.cpp` 内已经用 `#include "mods/mod_identity.h"`（无 guard，第 22 行）+
  大量 `#ifdef/#ifndef UNPACKED_MPQS` 分支包裹具体实现（`ReadPackedModManifestFrom`、
  `ReadLoadedModManifest`、`ReadModManifestByName` 内部逻辑等），没有发现同类"无条件声明
  但依赖被 guard 的类型"的问题。

**结论**：未发现除 `ReadModManifestByName`/`ModManifest` 外的同类问题；本次修复已覆盖
全部已知阻塞点。

## 验证

### 1. UNPACKED_MPQS=ON 配置（`build-unpacked/`）

配置：
```
cmake -S. -Bbuild-unpacked -G Ninja -DUNPACKED_MPQS=ON -DDEVILUTIONX_SYSTEM_BENCHMARK=OFF
```
→ Configuring done / Generating done，无错误。

**修复前**（临时 `git stash` 掉 `assets.hpp` 的改动后复现）：
```
cmake --build build-unpacked --target libdevilutionx -j8
...
In file included from .../towners.cpp:11:
Source/engine/assets.hpp:341:15: error: 'ModManifest' does not name a type
  341 | [[nodiscard]] ModManifest ReadModManifestByName(std::string_view name);
      |               ^~~~~~~~~~~
```
（同样错误在 `menu.cpp`、`automap.cpp`、`selhero.cpp`、`diablo.cpp`、`pfile.cpp` 等
十余个编译单元中重复出现，10 个目标失败后 ninja 停止）。

**修复后**（`git stash pop` 恢复改动）：
```
cmake --build build-unpacked --target libdevilutionx -j8
...
[30/30] Building CXX object Source/CMakeFiles/libdevilutionx.dir/lua/lua_global.cpp.o
```
→ 全部 30 个目标编译成功，无错误。

```
cmake --build build-unpacked --target level_roster_test level_roster_baseline_test -j8
...
[31/31] Linking CXX executable level_roster_test
```
→ 成功链接。运行结果：

- `level_roster_test`：41 tests from 4 test suites，全部 `PASSED`。
- `level_roster_baseline_test`：11 个测试因缺少 `spawn.mpq`/`DIABDAT.MPQ` 而 `SKIPPED`
  （build-unpacked 目录下没有真实游戏资产，这是预期行为，不是回归）。

### 2. 默认配置（`build/`，不受影响）

```
cmake --build build --target level_roster_test level_roster_baseline_test -j8
```
→ 56/56 编译+链接成功。

- `level_roster_test`（`build/`）：41 tests, 全部 `PASSED`。
- `level_roster_baseline_test`（`build/`）：11 tests, 全部 `PASSED`（耗时约 10 分钟，
  含大量统计性 squad-formation / rate 测试，运行时长与本次改动无关，是既有测试特性）。

全量门禁：
```
python3 tools/run_tests.py --json /tmp/ci.json
```
结果（`/tmp/ci.json`）：
```json
{
  "ctest": { "passed": 772, "failed": 0, "skipped": 3, "total": 772, "passed_pct": 100, "returncode": 0 },
  "drift": {
    "drift_ok": true,
    "passes": 6,
    "output": "PASS A ... PASS B ... PASS C ... PASS C2 ... PASS E ... PASS F ..."
  }
}
```
→ `failed == 0 && passed_pct == 100 && drift_ok == true`（含检查 F：run_tests.py 能构建
每个注册测试目标）。

Eval smoke：
```
python3 -m tools.eval.backend --smoke
```
→ `evaluated: 37  passed: 37  failed: 0  skipped: 0  pass_rate: 1.0`，exit code 0。

### 3. Git 状态

`git status` 干净，只有 `Source/engine/assets.hpp` 被修改（4 行插入 / 1 行删除），
无其它文件受影响，无残留构建产物纳入版本控制。

## 顾虑

- `build-unpacked/` 目录下没有真实的 `spawn.mpq`/`DIABDAT.MPQ`，因此
  `level_roster_baseline_test` 在该配置下的 11 个测试被跳过而非真正跑过。这与
  `UNPACKED_MPQS` 修复本身无关（该测试需要真实资产文件才能跑，`build/`（默认配置）下
  已有资产并全部通过），但如果后续需要在 CI 里对 `UNPACKED_MPQS=ON` 配置做同等深度的
  baseline 验证，需要额外准备 unpacked 资产目录。
- `level_roster_baseline_test` 在默认配置下运行耗时约 10 分钟（`SquadFormationRate`
  单测就有 289655 ms），属于既有测试的统计特性，不是本次改动引入的新问题，但如后续
  CI 超时收紧，可能需要单独关注这个测试的时长。
