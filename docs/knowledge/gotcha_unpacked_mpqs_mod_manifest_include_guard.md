---
name: UNPACKED_MPQS 下 assets.hpp 必须无条件 include mod_identity.h（下游曾把引擎编坏）
description: Source/engine/assets.hpp 无条件声明 ReadModManifestByName(...)（返回 ModManifest），但 mod_identity.h 一度被并进 #ifndef UNPACKED_MPQS guard → 该配置下整个 libdevilutionx 编不过。修法是让该 include 留在 guard 外；改这个头文件时两种配置都要编一次。
type: gotcha
created: 2026-09-16
sources:
  - Source/engine/assets.hpp（ReadModManifestByName 的无条件声明；mod_identity.h 与 mpq_reader.hpp 的 guard 归属）
  - 下游引入提交 1bf1b9886（"Mod hashing and manifest #8608"）；上游 master 无此问题（只 guard mpq_reader.hpp）
  - docs/superpowers/ledgers/2026-09-15-level-rosters-phase-a2/task-unpacked-mpqs-fix-report.md（修复前后完整构建输出）
---

**事实**：`Source/engine/assets.hpp` **无条件**声明
`[[nodiscard]] ModManifest ReadModManifestByName(std::string_view name);`
而 `ModManifest` 由 `#include "mods/mod_identity.h"` 提供。某个下游提交（`1bf1b9886`）把 `mod_identity.h` 和 `mpq/mpq_reader.hpp` **一起**并进了 `#ifndef UNPACKED_MPQS` guard → 结果 `-DUNPACKED_MPQS=ON` 时 `ModManifest` 未声明，**整个 `libdevilutionx`（10+ 编译单元）编不过**，与测试代码无关。

**修法**：`mod_identity.h` 留在 guard **外**（`mpq_reader.hpp` 继续受 guard，因为只有 MPQ 路径需要它），并在源文件里留一行注释说明"为何该 include 不能进 guard"。

**为什么值得记**：①该缺陷是**配置相关**的——CI 默认 `UNPACKED_MPQS=OFF`，所以它不会在常规门禁里暴露（`CMakeLists.txt:168` 默认 OFF），只在真正切到该配置时炸；②上游 master 没有这个问题（那里只 guard `mpq_reader.hpp`），**下次同步上游或合并该头文件时极易再次引入**。

**何时使用**：改动 `Source/engine/assets.hpp`（或任何"声明无条件、定义被 guard"的头文件）时；以及任何声称"支持 `UNPACKED_MPQS`"的验证——**必须真的用 `-DUNPACKED_MPQS=ON` 配一遍并编 `libdevilutionx`**，光看测试目标不够。
