# 最终全分支评审必修发现修复报告（F1/F2/F5）

- 分支：`feature/qol-upgrades`
- 修复前 HEAD：`5cce05c30`
- 修复后 HEAD：`14bd27503`
- 提交：
  - F1：`88035420d` — `fix(tools): build sampling_behavior_test in the gate`
  - F2+F5：`14bd27503` — `docs(spec+knowledge): correct TEST_TARGETS claim, add conflict-file gotcha`

---

## F1（Important）：门禁漏建 `sampling_behavior_test`

### Before

`tools/run_tests.py` 的 `TEST_TARGETS` 列表（第 42-62 行）不含 `sampling_behavior_test`，而 `CMake/Tests.cmake` 的 `tests` 列表第 60 行已注册它，CTest 也已注册（`build/CTestTestfile.cmake` 第 42 行 `include(".../sampling_behavior_test[1]_include.cmake")`）。

`build/sampling_behavior_test` 修复前 mtime：

```
2026-08-13 22:30:58.093858649 +0800
```

### 交叉比对脚本与输出（修复前）

一次性脚本 `/tmp/crosscheck.py`（未提交），解析 `CMake/Tests.cmake` 的 `tests` + `standalone_tests`（含 `if(NOT USE_SDL1) list(APPEND standalone_tests text_render_integration_test)` 条件追加，排除 `benchmarks`），与 `tools/run_tests.py` 的 `TEST_TARGETS` 做双向差集：

```python
import re
from pathlib import Path

cmake_text = Path("CMake/Tests.cmake").read_text()

def extract_list(varname, text):
    m = re.search(r'set\(' + varname + r'\s*(.*?)\)', text, re.S)
    items = [l.strip() for l in m.group(1).splitlines() if l.strip() and not l.strip().startswith('#')]
    return items

tests = extract_list("tests", cmake_text)
standalone_tests = extract_list("standalone_tests", cmake_text)
for m in re.finditer(r'list\(APPEND\s+standalone_tests\s+([^\)]+)\)', cmake_text):
    for tok in m.group(1).split():
        if tok not in standalone_tests:
            standalone_tests.append(tok)

cmake_all = set(tests) | set(standalone_tests)

run_tests_text = Path("tools/run_tests.py").read_text()
m = re.search(r'TEST_TARGETS\s*=\s*\[(.*?)\]', run_tests_text, re.S)
targets = re.findall(r'"([^"]+)"', m.group(1))
targets_set = set(targets)

print("In Tests.cmake but NOT in TEST_TARGETS:", sorted(cmake_all - targets_set))
print("In TEST_TARGETS but NOT in Tests.cmake:", sorted(targets_set - cmake_all))
```

**修复前输出：**

```
tests count: 48
standalone_tests count: 20
cmake_all (tests+standalone, excl benchmarks) count: 68
TEST_TARGETS count: 67 unique: 67

In Tests.cmake but NOT in TEST_TARGETS: ['sampling_behavior_test']
In TEST_TARGETS but NOT in Tests.cmake: []
```

结论：唯一脱钩项就是 `sampling_behavior_test`，反向（TEST_TARGETS 有陈旧条目）为空，无需额外处理。

### 修复

`tools/run_tests.py` 的 `TEST_TARGETS` 里，紧跟 `panel_state_test` 之后加入 `sampling_behavior_test`（该条目在 `Tests.cmake` 里正好也紧邻 `panel_state_test` 之后，保持位置对应；未改动列表既有格式/分组风格）：

```diff
     "townerdat_test", "writehero_test", "vendor_test", "panel_state_test",
+    "sampling_behavior_test",
     "store_transaction_test", "visual_store_test", "stash_test",
```

### 修复后交叉比对（同一脚本重跑）

```
tests count: 48
standalone_tests count: 20
cmake_all (tests+standalone, excl benchmarks) count: 68
TEST_TARGETS count: 68 unique: 68

In Tests.cmake but NOT in TEST_TARGETS: []
In TEST_TARGETS but NOT in Tests.cmake: []
```

双向差集均为空，交叉比对通过。

### 重跑全量门禁

命令：`python3 tools/run_tests.py --json /tmp/ci-sync2.json`

构建阶段可见 `sampling_behavior_test` 被实际重建：

```
[3/4] Building CXX object CMakeFiles/sampling_behavior_test.dir/test/sampling_behavior_test.cpp.o
[4/4] Linking CXX executable sampling_behavior_test
```

`/tmp/ci-sync2.json` 关键字段：

```json
{
  "steps": {
    "build": { "ok": true, "message": "build ok" },
    "ctest": {
      "passed": 698, "failed": 0, "skipped": 3, "not_run": 0,
      "total": 698, "failures": [], "passed_pct": 100, "returncode": 0
    },
    "drift": { "drift_ok": true, "passes": 5 }
  }
}
```

### Before / After 对照

| 检查项 | 修复前 | 修复后 |
|---|---|---|
| `TEST_TARGETS` 含 `sampling_behavior_test` | 否 | 是 |
| `build/sampling_behavior_test` mtime | `2026-08-13 22:30:58`（陈旧） | `2026-09-15 19:10:33`（本轮重建） |
| Tests.cmake↔TEST_TARGETS 差集 | `['sampling_behavior_test']` | `[]` / `[]` |
| ctest.failed / passed_pct | 未知（旧二进制跑的用例未反映当前代码） | `0` / `100` |

### mtime 验证

```
$ stat -c '%y' build/sampling_behavior_test
2026-09-15 19:10:33.281282419 +0800
```

晚于本轮构建触发时刻，确认已重建。

### SamplingBaselineTest 抽样结果

`grep -n "SamplingBaselineTest" build/Testing/Temporary/LastTest.log | head`：

```
7452:287/698 Testing: SamplingBaselineTest.HellL15SameClassTailBaseline
7453:287/698 Test: SamplingBaselineTest.HellL15SameClassTailBaseline
7454:Command: ".../build/sampling_behavior_test" "--gtest_filter=SamplingBaselineTest.HellL15SameClassTailBaseline" ...
7456:"SamplingBaselineTest.HellL15SameClassTailBaseline" start time: Sep 15 19:11 CST
7459:Note: Google Test filter = SamplingBaselineTest.HellL15SameClassTailBaseline
7462:[----------] 1 test from SamplingBaselineTest
7463:[ RUN      ] SamplingBaselineTest.HellL15SameClassTailBaseline
7464:[       OK ] SamplingBaselineTest.HellL15SameClassTailBaseline (201 ms)
7465:[----------] 1 test from SamplingBaselineTest (201 ms total)
7474:"SamplingBaselineTest.HellL15SameClassTailBaseline" end time: Sep 15 19:11 CST
7475:"SamplingBaselineTest.HellL15SameClassTailBaseline" time elapsed: 00:00:00
7478:288/698 Testing: SamplingBaselineTest.HellL14SameClassTailBaseline
...
```

全量核对（13 个唯一用例，均为 `[ OK ]`）：

```bash
$ grep -oP 'SamplingBaselineTest\.\w+' build/Testing/Temporary/LastTest.log | sort -u | wc -l
13
$ grep -oP 'SamplingBaselineTest\.\w+' build/Testing/Temporary/LastTest.log | sort -u
SamplingBaselineTest.CatacombsUnconstrainedByCap
SamplingBaselineTest.CavesAnyClassTailBaseline
SamplingBaselineTest.CavesKiteTailBaseline
SamplingBaselineTest.ClassifyCoversAllAiIds
SamplingBaselineTest.CounselorIsRangedTurret
SamplingBaselineTest.HellL13SameClassTailBaseline
SamplingBaselineTest.HellL14SameClassTailBaseline
SamplingBaselineTest.HellL15SameClassTailBaseline
SamplingBaselineTest.Level16HardcodedTypes
SamplingBaselineTest.MPSameSeedSameComposition
SamplingBaselineTest.QuestPreAddRePickDoesNotDoubleCount
SamplingBaselineTest.SampleCountWithinBudget
SamplingBaselineTest.SamplingTerminates
$ grep "\[       OK \] SamplingBaselineTest" build/Testing/Temporary/LastTest.log | wc -l
13
```

### `git rev-list --count HEAD..origin/master`

```
0
```

无回退。

### 提交

```
88035420d fix(tools): build sampling_behavior_test in the gate
```

正文：说明 `sampling_behavior_test` 已注册于 `Tests.cmake`/CTest 但缺席 `TEST_TARGETS`，导致 13 个用例长期跑旧二进制；本轮最终评审（F1）发现并修复。

---

## F2（Important）：规格陈述与实测相反

### Before

`docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md` 第 184 行（修复前）：

> 冲突实际为 **4 处**（与探测一致）。另两项探测期风险本轮**未触发**：`tools/run_tests.py` 的 `TEST_TARGETS` 未与 `CMake/Tests.cmake` 脱钩（上游本轮只改 benchmark 的链接依赖）；`questdat.hpp` 行尾维持 CRLF，本轮唯一 LF 的仍是 `Source/quests.h` 与上游新增的 7 个文件。

此处「未脱钩」的陈述与 F1 的实测结果相反——脱钩客观存在，且不是本轮引入（`git show 5cce05c30:CMake/Tests.cmake` 与更早历史比对，`sampling_behavior_test` 早在合并前就已在 `tests` 列表里，非本轮新增）。

### After

替换为两段：

1. 保留「未触发」的表述，但只保留客观仍成立的那一半（`questdat.hpp` 行尾维持 CRLF）——把 `TEST_TARGETS`/`Tests.cmake` 脱钩的表述整段移出，单独更正为准确陈述：

> `tools/run_tests.py` 的 `TEST_TARGETS` 与 `CMake/Tests.cmake` 的脱钩需要更正表述：本轮上游只改了 benchmark 的链接依赖，**未新增/删除/改名任何 `tests`/`standalone_tests` 成员**——这一点属实；但**既有的 `sampling_behavior_test`**（在合并前就已注册于 `Tests.cmake` 的 `tests` 列表与 CTest）**早已处于脱钩状态**：它未被列入 `TEST_TARGETS`，导致 `run_tests.py` 的构建阶段从不重建它，ctest 长期用旧二进制跑它的 13 个 `SamplingBaselineTest.*` 用例（合并本身未引入或加重这个脱钩，pre-merge 的 `Tests.cmake` 同样含 `sampling_behavior_test`）。该缺口由最终全分支评审发现（F1），已由本轮提交 `88035420d`（`fix(tools): build sampling_behavior_test in the gate`）修复：`sampling_behavior_test` 加入 `TEST_TARGETS` 后重跑门禁验证，见下方补充验收数据。

2. 在 §7「状态」区块（本轮新增/相关提交列表之后）追加「§7 补充验收（F1 修复后重跑门禁，2026-09-15）」表格，记录本次重跑的关键字段：

```markdown
| 检查项 | 结果 |
|---|---|
| `ctest.total` / `failed` / `passed_pct` | `698` / `0` / `100` |
| `drift.drift_ok` / `drift.passes` | `true` / `5` |
| `build/sampling_behavior_test` mtime | `2026-09-15 19:10:33`（晚于本轮构建，此前为 `2026-08-13 22:30:58` 的旧二进制） |
| `SamplingBaselineTest.*` 用例数与结果 | 13 个用例全部出现在 `LastTest.log` 且均为 `[ OK ]` |
| `git rev-list --count HEAD..origin/master` | `0` |
```

以及在提交列表中追加：

> `88035420d`（`fix(tools): build sampling_behavior_test in the gate`，最终全分支评审 F1 修复）。

### 行尾自查

```
$ grep -c $'\r' docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md || echo "no CR (good, LF)"
no CR (good, LF)
```

`.md` 保持既有 LF，未引入 CRLF。

### 提交

随 F5 一并提交在 `14bd27503`（见下）。

---

## F5（Minor 但必修）：新增知识库文档记录冲突文件漏带自动合并行的根因

### 新增文件

`docs/knowledge/gotcha_conflict_file_loses_auto_merged_lines.md`（新建，LF 行尾）

内容结构（对照要求逐项核对）：

- **frontmatter**：参考 `docs/knowledge/pattern_fixed_width_field_reads.md` 的既有格式（`name`/`description`/`type`/`created`/`sources` 五键，`sources` 为数组，每项括号注释）。
- **症状**：解冲突后编译报符号未声明；本轮实例 `Source/controls/plrctrls.cpp` 缺 `#include "panels/quest_log.hpp"`，7 处编译错误，由 `91e805149` 修复。
- **根因**：手工解冲突文件不继承上游同一 hunk 里自动合并进来的新增行；`plrctrls.cpp` 恰好同时是消费者集合与 4 个手工冲突文件集合的交集，「无重叠即安全」推断对冲突文件集合失效。
- **检查清单**：
  1. 取「手工冲突文件集合」∩「上游新增 include/声明的消费者集合」交集，逐个核对；
  2. 整文件三方核对（`git merge-file -p ours base theirs` vs 最终文件逐字节比），而非只看冲突块——引用 `quests.cpp` 的 Critical 缺陷（整块取一侧丢弃 341 行迁移，任务评审发现后 amend 进 `8e92687a9`）作为反例；
  3. 编译门禁是最后防线，冲突文件集合里的编译错误优先怀疑漏带上游新增行。
- **sources**：`91e805149`（漏 include 的修复）、`8e92687a9`（merge commit，quests.cpp 修复后 amend）、本轮上游同步（tip `e00b7260f`）。

### MEMORY.md 索引

在 `docs/knowledge/MEMORY.md` 的 **## Gotchas** 分组末尾追加一行（风格与既有条目一致：`[标题](文件名) — 一句话摘要`）：

```
- [手工解冲突的文件不会继承上游同一 hunk 里自动合并进来的新增行](gotcha_conflict_file_loses_auto_merged_lines.md) — `plrctrls.cpp` 是本轮 4 个手工冲突文件之一，冲突块只处理两行 include 取舍，漏带上游同一次 quest_log 迁移新增的 `panels/quest_log.hpp` include → 7 处编译错误（`91e805149` 修复）；「不重叠即安全」的推断对冲突文件集合失效，需整文件三方核对
```

### 行尾自查

```
$ grep -c $'\r' docs/knowledge/gotcha_conflict_file_loses_auto_merged_lines.md || echo "no CR (good, LF)"
no CR (good, LF)
$ file docs/knowledge/gotcha_conflict_file_loses_auto_merged_lines.md
docs/knowledge/gotcha_conflict_file_loses_auto_merged_lines.md: Unicode text, UTF-8 text
$ grep -c $'\r' docs/knowledge/MEMORY.md || echo "no CR (good)"
no CR (good)
```

新文件、被追加的 `MEMORY.md` 均为纯 LF，无 CRLF 混入。

### 提交

```
14bd27503 docs(spec+knowledge): correct TEST_TARGETS claim, add conflict-file gotcha
```

（同一提交包含 F2 的规格更正与 F5 的知识库新文件+索引，均为纯文档改动，不涉 `Source/`。）

---

## 硬约束核对

- 改动文件范围：`tools/run_tests.py`、`docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`、新增 `docs/knowledge/gotcha_conflict_file_loses_auto_merged_lines.md`、`docs/knowledge/MEMORY.md` 索引行——四项之外无其它改动（`git status`/`git diff --stat` 已核对，未涉及 `Source/`、测试断言或计划文件）。
- 未 `git push`，未 `rebase`。
- 提交历史：

```
14bd27503 docs(spec+knowledge): correct TEST_TARGETS claim, add conflict-file gotcha
88035420d fix(tools): build sampling_behavior_test in the gate
5cce05c30 docs(spec): mark upstream sync round 2 as implemented + record findings
```

## 汇总验证

| 项目 | 结果 |
|---|---|
| Tests.cmake ↔ TEST_TARGETS 双向差集（修复后） | `[]` / `[]` |
| `ctest.total` / `failed` / `passed_pct` | `698` / `0` / `100` |
| `drift.drift_ok` / `passes` | `true` / `5` |
| `sampling_behavior_test` mtime | `2026-09-15 19:10:33`（本轮重建） |
| 13 个 `SamplingBaselineTest.*` 用例 | 全部 `[ OK ]` |
| `git rev-list --count HEAD..origin/master` | `0` |
| 三份文档行尾 | 均 LF，无 CRLF 混入 |
