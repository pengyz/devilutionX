---
name: run_tests.py 的 TEST_TARGETS 是手抄清单 — 漏登记的测试二进制会被全量门禁当陈旧产物执行
description: 全量门禁只构建 TEST_TARGETS 里的目标，但 ctest 会跑 CMake/Tests.cmake 注册的全部用例；两份清单脱同步时门禁会执行未重建的旧二进制，红/绿都不可信
type: gotcha
created: 2026-09-17
sources:
  - tools/run_tests.py（TEST_TARGETS / build_tests）
  - CMake/Tests.cmake
  - .superpowers/sdd/2026-09-15-level-rosters-phase-b/final-fix-report.md（门禁结果一节）
---

`python3 tools/run_tests.py --json <f>` 的构建阶段只 `cmake --build --target` **`tools/run_tests.py` 里 `TEST_TARGETS` 手抄的那批目标**，而随后的 `ctest --output-on-failure` 跑的是 **`CMake/Tests.cmake` 注册的全部用例**。两份清单没有任何自动校验，新增测试二进制若只注册进 `Tests.cmake`（漂移校验 A 只查"条目有源文件"，不查 `run_tests.py`），全量门禁就会**执行一个不曾重建的 `build/<name>`**。

实例（2026-09-17，阶段 B 收尾）：`level_roster_test` 与 `level_roster_baseline_test` 都不在 `TEST_TARGETS` 里。反证实验改了 `test/level_roster_baseline_test.cpp` 又改回后直接跑全量门禁，门禁未重新链接该目标，ctest 执行的是仍带变异（`kLeashedAxisBound = 6`）的旧二进制，报出一条与工作树不符的失败：

```
[ SQUADLEASH ] minions 1275 at bound 0 beyond bound 0 (bound 6)
```

而工作树里该常量是 4。指纹：**构建产物 mtime 早于源码 mtime**（`ls -l --time-style=full-iso build/<name> test/<name>.cpp`），且失败信息里的常量/输出对不上源码。已把两个目标补进 `TEST_TARGETS`（构建日志里出现 `Linking CXX executable level_roster_baseline_test` 即为已覆盖）。

**为什么：** `build_tests()` 不能简单地 `--target test`——那会连带 benchmark 目标触发 LTO 链接失败（见 `AGENTS.md`），所以只能显式列目标；显式清单就必然要手工同步，而 ctest 侧是 CMake 自动注册的，二者天然不对称。

**何时使用：** ① 新增测试二进制时，除了 `CMake/Tests.cmake` + `test/<name>_test.cpp`，**必须同步 `tools/run_tests.py` 的 `TEST_TARGETS`**；② 门禁报出的失败与你读到的源码矛盾时，先比对二进制与源码 mtime，再怀疑逻辑；③ 做完"破坏→红→恢复→绿"的反证实验后，恢复源码之后要重新构建一次再跑门禁，别指望全量门禁替你重建。校验方法：对比 `Tests.cmake` 的目标名与 `TEST_TARGETS`，差集应只剩条件目标 `text_render_integration_test`。
