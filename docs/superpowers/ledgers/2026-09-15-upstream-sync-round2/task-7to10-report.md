# Task 7-10 报告：合并结果编译验证 + 四道门禁

**执行时间**：2026-09-15 18:27 - 18:32
**分支**：`feature/qol-upgrades`，起点 HEAD `8e92687a9`（merge commit：pre-merge `fd2956ea8` + 上游 `e00b7260f`）
**结果**：4 项任务全部完成；1 处必需的最小修复（commit `91e805149`），已单独提交。最终 HEAD `91e805149`，工作区干净。

---

## Task 7：验证 quest_log 重构零连带

### 步骤 1：编译 libdevilutionx，检查 error（修复前）

命令：
```bash
cmake --build build --target libdevilutionx -j 20 2>&1 | grep -E "error:" | head -30
```

**实际输出**（与简报预期「无输出」不符，出现编译错误）：
```
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:1777:17: error: ‘QuestlogUp’ was not declared in this scope; did you mean ‘QuestLogMove’?
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:1779:17: error: ‘QuestlogDown’ was not declared in this scope; did you mean ‘QuestLogMove’?
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:1811:13: error: ‘QuestLogIsOpen’ was not declared in this scope; did you mean ‘QuestLogMove’?
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:2130:25: error: ‘QuestLogIsOpen’ was not declared in this scope
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:2146:22: error: ‘QuestLogIsOpen’ was not declared in this scope
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:2147:25: error: ‘StartQuestlog’ was not declared in this scope
/home/peng/workspace/DevilutionX/Source/controls/plrctrls.cpp:2434:13: error: ‘QuestLogIsOpen’ was not declared in this scope
```

### 步骤 2：12 消费者文件 include 计数（修复前，诊断用）

命令：
```bash
for f in Source/minitext.cpp Source/qol/chatlog.cpp Source/diablo.cpp Source/stores.cpp \
         Source/control/control_panel.cpp Source/controls/game_controls.cpp \
         Source/controls/touch/gamepad.cpp Source/controls/touch/renderers.cpp \
         Source/controls/plrctrls.cpp Source/engine/render/scrollrt.cpp \
         test/panel_state_test.cpp test/ui_test.hpp; do
  printf '%-45s %s\n' "$f" "$(grep -c 'panels/quest_log.hpp' "$f")"
done
grep -c "QuestLogIsOpen\|pQLogCel\|DrawQuestLog" Source/quests.cpp
```

**实际输出**（修复前）：
```
Source/minitext.cpp                           1
Source/qol/chatlog.cpp                        1
Source/diablo.cpp                             1
Source/stores.cpp                             1
Source/control/control_panel.cpp              1
Source/controls/game_controls.cpp             1
Source/controls/touch/gamepad.cpp             1
Source/controls/touch/renderers.cpp           1
Source/controls/plrctrls.cpp                  0   <-- 唯一缺失项
Source/engine/render/scrollrt.cpp             1
test/panel_state_test.cpp                     1
test/ui_test.hpp                              1
0   （quests.cpp 符号引用计数，符合预期）
```

**诊断**：11/12 消费者文件的 `#include "panels/quest_log.hpp"` 已由上游合并自动补齐；唯独 `Source/controls/plrctrls.cpp` 缺失。这是简报预判的编译错误类型（步骤 3 判据命中）：`QuestLogIsOpen`/`QuestlogUp`/`QuestlogDown`/`StartQuestlog` 四个符号在该文件被使用但未声明。

### 步骤 3：现场诊断与最小修复（按简报判据执行）

在 `Source/controls/plrctrls.cpp` 的 `panels/` 分组（按字母序）插入一行：
```diff
+#include "panels/quest_log.hpp"
 #include "panels/spell_icons.hpp"
 #include "panels/spell_list.hpp"
 #include "panels/ui_panels.hpp"
```

行尾确认（未破坏 CRLF）：
```
$ file Source/controls/plrctrls.cpp
Source/controls/plrctrls.cpp: C++ source, ASCII text, with CRLF line terminators
```

提交：
```
commit 91e805149
fix(build): add panels/quest_log.hpp include after the upstream quest split

plrctrls.cpp used QuestLogIsOpen/QuestlogUp/QuestlogDown/StartQuestlog
but was missing the panels/quest_log.hpp include that the other 11
consumer files received automatically during the upstream quest_log
split merge. This is the sole build fallout from task 6's merge
(task-7-brief.md step 3 fallback path).

 Source/controls/plrctrls.cpp | 1 +
 1 file changed, 1 insertion(+)
```

**注**：这与简报正文「背景」部分描述的「已在探测阶段复验、预判已被证伪」的说法不符——实测中该连带确实发生了（`plrctrls.cpp` 是探测阶段未覆盖或漏检的第 12 个消费者文件）。已按简报步骤 3 给出的允许回退路径处理，非缺陷，已如实记录。

### 步骤 1/2 复验（修复后）

命令：
```bash
cmake --build build --target libdevilutionx -j 20 2>&1 | grep -E "error:" | head -30
```
**输出**：无（零编译错误，编译日志末尾 `[88/88] Building CXX object ... items.cpp.o`，无 error）。

12 文件计数复验：
```
Source/minitext.cpp                           1
Source/qol/chatlog.cpp                        1
Source/diablo.cpp                             1
Source/stores.cpp                             1
Source/control/control_panel.cpp              1
Source/controls/game_controls.cpp             1
Source/controls/touch/gamepad.cpp             1
Source/controls/touch/renderers.cpp           1
Source/controls/plrctrls.cpp                  1   <-- 修复后已为 1
Source/engine/render/scrollrt.cpp             1
test/panel_state_test.cpp                     1
test/ui_test.hpp                              1
0
```
判据全部满足：前 12 行均为 1，`quests.cpp` 符号引用计数为 0。

**R10 minitext.h 回退检查**：整个编译过程中未出现涉及 `minitext.h` 提供符号（如 `InitQTextMsg`）的未声明错误，说明 `Source/quests.cpp` 删除 `#include "minitext.h"` 的裁决 R10 无需回退。

**Task 7 结论**：库编译通过，唯一连带已修复并单独提交，`quests.cpp` 无 R10 回退需要。

---

## Task 8：门禁 1-2（CMake 配置 + 全目标编译）

### 步骤 1：配置

命令：
```bash
cmake -S. -Bbuild -G Ninja -DDEVILUTIONX_SYSTEM_BENCHMARK=OFF 2>&1 | tail -5
```
**实际输出**（tail）：
```
-- Performing Test HAVE_STEADY_CLOCK -- success
-- Performing Test HAVE_PTHREAD_AFFINITY -- success
-- Configuring done
-- Generating done
-- Build files have been written to: /home/peng/workspace/DevilutionX/build
```
退出码：0。无「找不到目标」类错误。

### 步骤 2：库与主程序编译

命令：
```bash
cmake --build build --target devilutionx -j 20 2>&1 | tail -5
```
**实际输出**（tail）：
```
[77/79] Building CXX object Source/CMakeFiles/libdevilutionx_text_render.dir/engine/render/text_render.cpp.o
[78/79] Building CXX object Source/CMakeFiles/libdevilutionx_dun_render.dir/engine/render/dun_render.cpp.o
[79/79] Linking CXX executable devilutionx
```
退出码：0。仅一条无害警告（`stores.cpp:2062 TakeGold defined but not used`，非 error，不影响判据）。

二进制确认：
```
$ ls -la build/devilutionx
-rwxrwxr-x 1 peng peng 351401048 Sep 15 18:29 build/devilutionx
```

### 步骤 3：无需追加提交（Task 7 的修复已覆盖唯一的编译问题；CMake 层面无配置错误）。

**Task 8 结论**：`build.ok == true`，配置与链接均成功。

---

## Task 9：门禁 3-4（测试目标 + 全量 ctest）

### 步骤 1：全部测试目标构建 + 全量 ctest

命令：
```bash
python3 tools/run_tests.py --json /tmp/ci-sync.json 2>&1 | tail -30
```
**实际输出**（尾部 SUMMARY JSON）：
```
=== Running full ctest ===
+ ctest --output-on-failure
=== Drift check ===

Report written to /tmp/ci-sync.json

=== SUMMARY ===
{
  "timestamp": "2026-09-15 18:29:15",
  "build_dir": "/home/peng/workspace/DevilutionX/build",
  "steps": {
    "build": {
      "ok": true,
      "message": "build ok"
    },
    "ctest": {
      "passed": 698,
      "failed": 0,
      "skipped": 3,
      "not_run": 0,
      "total": 698,
      "failures": [],
      "passed_pct": 100,
      "returncode": 0
    },
    "drift": {
      "drift_ok": true,
      "passes": 5,
      "output": "PASS A  Tests.cmake entries have source files\nPASS B  no placeholder assertions in tests\nPASS C  modified files keep line endings\nPASS C2 added files match .editorconfig\nPASS E  no test-only production functions\n\nmerge-base: e00b7260f   check E allowlist: 10 upstream symbols\n"
    }
  }
}
```
退出码：0。

### 步骤 2：JSON 关键字段判定

命令：
```bash
python3 -c "
import json; d=json.load(open('/tmp/ci-sync.json'))
print('ctest', d['steps']['ctest']); print('drift_ok', d['steps']['drift']['drift_ok'])"
```
**实际输出**：
```
ctest {'passed': 698, 'failed': 0, 'skipped': 3, 'not_run': 0, 'total': 698, 'failures': [], 'passed_pct': 100, 'returncode': 0}
drift_ok True
```

判据：`failed == 0` ✅，`passed_pct == 100` ✅，`drift.drift_ok == True`（5 项 PASS）✅。测试总数 698，与基线（698 total / 3 skipped）**完全一致，无变化**，无需记录数量差异原因。

### 步骤 2b：按名字确认 timedemo 仍为 Skipped

命令：
```bash
grep -n "WarriorLevel1to2" build/Testing/Temporary/LastTest.log
```
**实际输出**（节选关键行）：
```
5879:227/698 Testing: Timedemo.WarriorLevel1to2
5880:227/698 Test: Timedemo.WarriorLevel1to2
5890:[ RUN      ] Timedemo.WarriorLevel1to2
5892:quarantined: upstream-recorded demo desyncs under the Dark Expedition drop filter; re-record test/fixtures/timedemo/WarriorLevel1to2 (see docs/knowledge/gotcha_timedemo_isOnActiveLevel_failure.md)
5893:[  SKIPPED ] Timedemo.WarriorLevel1to2 (0 ms)
5900:[  SKIPPED ] Timedemo.WarriorLevel1to2
5907:  • 1 test skipped: quarantined: upstream-recorded demo desyncs under the Dark Expedition drop filter; re-record test/fixtures/timedemo/WarriorLevel1to2 (see docs/knowledge/gotcha_timedemo_isOnActiveLevel_failure.md)
```
**结论**：`Timedemo.WarriorLevel1to2` 明确出现 `(Skipped)` 状态，未发生状态翻转，与基线一致（规格 §6 验收项 9 满足）。

### 步骤 2c：确认 TEST_TARGETS 未与 Tests.cmake 脱钩

命令：
```bash
git diff b3e52b1ea origin/master -- CMake/Tests.cmake | grep -E '^[+-]  [a-z_0-9]+_test'
```
**实际输出**：
```
+  app_fatal_for_testing
+  language_for_testing
```

**分析**：这两条命中是因为 grep 模式 `_test$` 匹配了 `_for_testing` 结尾字符串，并非新增的 gtest `_test`/`_benchmark` 目标。进一步确认变更上下文：

```bash
git diff b3e52b1ea origin/master -- CMake/Tests.cmake
```
显示唯一实质差异是 `dun_render_benchmark` 的 `target_link_dependencies` 从
```
target_link_dependencies(dun_render_benchmark PRIVATE libdevilutionx_so)
```
变为增补 `libdevilutionx_assets`、`libdevilutionx_dun_tile_data`、`libdevilutionx_light_tables`、`libdevilutionx_dun_render`、`libdevilutionx_options`、`app_fatal_for_testing`、`language_for_testing`、`tl`、`unordered_dense::unordered_dense` 等链接依赖。

另外确认 `tests`/`standalone_tests` 成员列表本身无任何增删：
```bash
git diff b3e52b1ea origin/master -- CMake/Tests.cmake | grep -B5 -A30 "set(tests\|set(standalone_tests"
```
无输出（该 diff 区间不涉及这两个 set 声明块）。

**结论**：与简报预期完全一致——本轮上游对 `Tests.cmake` 的改动只涉及 `dun_render_benchmark`（性能基准，非正确性测试）的链接依赖，未增删改任何 `tests`/`standalone_tests` 成员。`tools/run_tests.py` 的 `TEST_TARGETS` **无需更新**。

**Task 9 结论**：`ctest.failed == 0`，与基线 698/0/3-skipped 完全一致，timedemo 名字级确认 Skipped，TEST_TARGETS 未脱钩。

---

## Task 10：eval smoke + 漂移校验，并与基线对比

### 步骤 1：eval smoke 门禁

命令：
```bash
python3 -m tools.eval.backend --smoke; echo "exit=$?"
```
**实际输出**（关键部分）：
```
  [PASS] [combat] combat-missiles (15/15)
  [PASS] [mechanics] mechanics-inventory_ui (15/15)
  [PASS] [mechanics] mechanics-panel_state (15/15)
  [PASS] [mechanics] mechanics-game_menu (15/15)
  [PASS] [mechanics] mechanics-char_panel (15/15)
  [PASS] [data] data-townerdat (15/15)
  [PASS] [data] data-mod_identity (15/15)
  [PASS] [data] data-ini (15/15)
  [PASS] [data] data-appfat (15/15)
  [PASS] [combat] combat-dead (15/15)
  [PASS] [combat] combat-effects (15/15)
  [PASS] [combat] combat-cursor (15/15)
  [PASS] [combat] combat-ai_registry (15/15)
  [PASS] [utility] utility-random (15/15)
  [PASS] [utility] utility-vision (15/15)
  [PASS] [utility] utility-utf8 (15/15)
  [PASS] [utility] utility-static_vector (15/15)
  [PASS] [utility] utility-rectangle (15/15)
  [PASS] [utility] utility-str_cat (15/15)
  [PASS] [utility] utility-parse_int (15/15)
  [PASS] [utility] utility-format_int (15/15)
  [PASS] [utility] utility-math (15/15)
  by category:
    dark-expedition      4/4 (1.0)
    save-load            2/2 (1.0)
    mechanics            8/8 (1.0)
    data                 6/6 (1.0)
    render               2/2 (1.0)
    combat               5/5 (1.0)
    utility              9/9 (1.0)
exit=0
```
汇总行（另行 grep 确认）：
```
evaluated: 36  passed: 36  failed: 0  skipped: 0  pass_rate: 1.0
```
判据：exit 0，36/36 PASS ✅。

### 步骤 2：漂移校验并与基线对比

命令：
```bash
python3 tools/check_drift.py --base origin/master 2>&1 | tee /tmp/drift-after-sync.txt
diff /tmp/drift-before-sync.txt /tmp/drift-after-sync.txt
```
**实际输出**（`/tmp/drift-after-sync.txt`）：
```
PASS A  Tests.cmake entries have source files
PASS B  no placeholder assertions in tests
PASS C  modified files keep line endings
PASS C2 added files match .editorconfig
PASS E  no test-only production functions

merge-base: e00b7260f   check E allowlist: 10 upstream symbols
```
**diff 结果**：
```
7c7
< merge-base: b3e52b1ea   check E allowlist: 10 upstream symbols
---
> merge-base: e00b7260f   check E allowlist: 10 upstream symbols
```

**结论**：与基线的唯一差异正是 `merge-base:` 行从 `b3e52b1ea` 变为 `e00b7260f`（本轮同步的上游 tip），PASS/FAIL 集合（5 项全 PASS）**完全一致**，无需记录任何行为差异到实施记录。

### 步骤 3：确认没有行尾被改动

命令：
```bash
python3 tools/check_drift.py --base origin/master 2>&1 | grep -E "PASS C |PASS C2 "
```
**实际输出**：
```
PASS C  modified files keep line endings
PASS C2 added files match .editorconfig
```
两项均 PASS，`Source/quests.h`/`Source/controls/plrctrls.cpp` 等本轮改动的行尾均未被破坏。

**Task 10 结论**：eval smoke 36/36 PASS exit 0；drift 5/5 PASS，与基线对比仅 merge-base 值符合预期变化，无回归。

---

## 汇总判据对照表

| 判据 | 基线（b3e52b1ea） | 本次（e00b7260f） | 结果 |
|---|---|---|---|
| ctest.total | 698 | 698 | 一致 |
| ctest.failed | 0 | 0 | ✅ 一致 |
| ctest.skipped | 3 | 3 | 一致 |
| ctest.passed_pct | 100 | 100 | ✅ 一致 |
| drift.drift_ok | true | true | ✅ 一致 |
| drift.passes | 5 | 5 | ✅ 一致 |
| Timedemo.WarriorLevel1to2 | Skipped | Skipped | ✅ 一致（名字级确认） |
| TEST_TARGETS 脱钩 | - | 未脱钩（仅 benchmark 链接依赖变化） | ✅ |
| eval smoke | - | 36/36, exit 0 | ✅ |
| drift PASS C / C2（行尾） | PASS/PASS | PASS/PASS | ✅ 一致 |

## 修复/回退记录

| 项 | 内容 | 提交 SHA |
|---|---|---|
| 必需修复 | `Source/controls/plrctrls.cpp` 缺失 `#include "panels/quest_log.hpp"`，补齐 1 行（CRLF 行尾保留） | `91e805149` |
| R10 minitext.h 回退 | 未触发（编译全程无相关未声明符号错误），`Source/quests.cpp` 保持已删除 `#include "minitext.h"` 的状态 | 无需回退 |

## 最终状态

```
$ git log --oneline -3
91e805149 fix(build): add panels/quest_log.hpp include after the upstream quest split
8e92687a9 merge: sync upstream master (28 commits, tip e00b7260f)
fd2956ea8 docs(plan): use Task N headings so task-brief can extract tasks

$ git status --short
（空，工作区干净）
```

未推送，未 rebase，除 `Source/controls/plrctrls.cpp` 的必需 1 行修复外未修改任何其他文件。
