# 任务 12（步骤 3-4）：规格收尾与提交 — 完成报告

**日期**：2026-09-15
**执行范围**：仅步骤 3（写回规格）与步骤 4（提交）。步骤 1-2（push 主分支 + CI 验证）由控制者完成，本次**未执行任何 `git push`**。
**改动文件**：`docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`（唯一改动文件，源码/测试/计划文件零改动）
**提交**：`5cce05c30` — `docs(spec): mark upstream sync round 2 as implemented + record findings`

---

## 1. 交付摘要

| 项 | 结果 |
|---|---|
| 改动文件数 | 1（`git status --porcelain` 仅一行 `M docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`） |
| diff 规模 | `51 insertions(+), 4 deletions(-)`（+ §7 重写） |
| 行尾自查 | `CRLF 0 LF 217`（文件原为 LF，保持 LF） |
| drift 校验 | 5 项全 PASS，`merge-base: e00b7260f`，exit 0 |
| 工作区 | 提交后干净 |
| `HEAD..origin/master` | `0` |
| push | **未执行**（按硬约束，由控制者推送） |

### 编辑位置清单

1. 头部元数据 `**状态**：` —— 已批准 → 已实施（2026-09-15）
2. §3「探测深入：上游 quest 重构的编译连带」末尾 —— 新增「结论修正（2026-09-15 实施后补记）」段
3. §4 步骤 3 `scrollrt.cpp` 条目 —— 按裁决 R8 改写
4. §5 通用红线 #4 —— 判定依据更新（原「本规格状态为『草案』」已失效）
5. §7 —— 全文重写：状态改「已实施」+ §6 十项验收逐项实测表 + 新增「执行中与规格不符/未预料之处」小节（4 点）+ 文档内部不一致说明

---

## 2. 逐处前后对照

### 2.1 头部元数据（第 5 行）

**前**
```markdown
**状态**：已批准（实施计划见 `plans/2026-09-15-upstream-sync-round2.md`）
```
**后**
```markdown
**状态**：已实施（2026-09-15；实施计划见 `plans/2026-09-15-upstream-sync-round2.md`，实施记录与验收逐项结果见 §7）
```

### 2.2 §3 末尾新增段落（原第 94 行后）

**前**：该节以「……前者被 13 个测试二进制共享，故其健康度由全量门禁兜底。」结束。

**后**（新增一段）：
```markdown
**结论修正（2026-09-15 实施后补记）**：上述「零编译连带」的推断**只对非冲突文件成立**。`Source/controls/plrctrls.cpp` 虽是这 12 个消费者文件之一，**同时又是本次 merge 的 4 个手工冲突文件之一**——冲突的 hunk 被人工解决时，不会继承上游在同一 hunk 自动合并进来的 `#include "panels/quest_log.hpp"`，于是它以「有使用、无声明」的形态在编译期报了 7 处错误（该文件用到 `QuestLogIsOpen` / `QuestlogUp` / `QuestlogDown` / `StartQuestlog`）。该 include 由提交 `91e805149` 补上（1 行，位置符合 include 字母序）。**教训：冲突文件不适用「改动不重叠即自动合并安全」的推断，必须单独列入待补 include 的检查清单。**
```

### 2.3 §4 步骤 3 `scrollrt.cpp` 条目

**前**
```markdown
   - `scrollrt.cpp` —— 取上游签名（`drawInfoBox` + `yield()`），核对 fork 的 `DrawMain` 调用点全部适配并**保留** fork 同处改动。
```
**后**
```markdown
   - `scrollrt.cpp` —— 保留上游新增的 `this_sdl_thread::yield();`，`DrawMain` 调用点保留 fork 形态（`DrawMain(hgt, false, ...)`），**不回退** fork 移除 `drawInfoBox` 脏矩形重构的改动（该处是**语义冲突**，原写「取上游签名（`drawInfoBox` + `yield()`）」并不准确，详见 §7「执行中与规格不符/未预料之处」第 1 点，裁决 R8）。
```

### 2.4 §5 通用红线 #4

**前**
```markdown
| 4 | 「已实施」需有非测试调用者且验收标准全部通过 | 本规格状态为「草案」，未标「已实施」。达成 §6 全部标准后方可改标 |
```
**后**
```markdown
| 4 | 「已实施」需有非测试调用者且验收标准全部通过 | 已于 2026-09-15 改标「已实施」：非测试调用者为仓库内全部生产代码（配置/构建/线程/渲染路径在合入后即消费新版上游实现），§6 十项验收逐项通过（逐项实测见 §7） |
```

### 2.5 §7 状态（整节重写）

**前**
```markdown
## 7. 状态

**草案（待批准）。** 批准后按 §4 执行；实施计划另立 `docs/superpowers/plans/`（本规格 §4 已给出可直接展开的步骤与门禁）。

相关规格：
```

**后**：`**已实施（2026-09-15）。**` 开头 + 合并提交/计数说明 + **§6 十项验收逐项实测表**（10 行全 PASS）+ 相关提交清单 + 「冲突实际为 4 处」及两项未触发风险，随后新增子节：

```markdown
### 执行中与规格不符/未预料之处

**1. `scrollrt.cpp` 的冲突是语义冲突，不是文本冲突（裁决 R8）**
- 现象 / 证据 / 处置-裁决（三段式）
**2. `quests.cpp` 首次解决时被整块取了一侧（Critical，裁决 R9/R10）**
- 现象 / 证据 / 处置-裁决（三段式）
**3. §3「quest_log 重构零编译连带」的结论不完整（由 `91e805149` 落实）**
- 现象 / 证据 / 处置-裁决（三段式）
**4. §4 对 `quests.h` 的处置需显式给出「保持 LF」的判据来源**
- 现象 / 证据 / 处置（三段式）

**文档内部不一致（本次一并修正）**：……
```

「相关规格：」三个条目原样保留，未改动。

### 2.6 §7 §6 十项验收逐项对应（写入规格的实测值，逐字采用任务给定数据）

| # | 验收项 | 写入规格的实测结果 |
|---|---|---|
| 1 | 无残留冲突标记 | `git grep -nE "^(<<<<<<<\|>>>>>>>\|=======)$" -- Source/ test/ CMake/ CMakeLists.txt` 无输出 |
| 2 | 已同步到上游 tip | `git rev-list --count HEAD..origin/master` = `0` |
| 3 | 构建通过 | `steps.build.ok == true` |
| 4 | 全量测试通过 | `ctest.total=698`、`failed=0`、`passed_pct=100`、`skipped=3`（与基线 698/0/3 一致） |
| 5 | eval smoke | exit 0，36/36 PASS，产物 `eval/results/20260915-183246/eval-summary.json`（`git_head=91e805149`） |
| 6 | 漂移校验 | `drift.drift_ok == true`、`passes == 5`；同步前后差异只有 `merge-base:` 行（`b3e52b1ea` → `e00b7260f`） |
| 7 | 行尾未破坏 | check C / C2 PASS；`Source/quests.h` CRLF 0 / LF 32 |
| 8 | heroname 状态 | 上游 tip 仍 5 处裸字段读取（`Source/msg.cpp:1094/1360/1368/1398`、`Source/pack.cpp:445`），fork 修复完好；`d1e122ab6` 留档 |
| 9 | timedemo quarantine | `Timedemo.WarriorLevel1to2` 仍为 `Skipped`（按用例名，`LastTest.log`） |
| 10 | CI 绿 | `gh run 34959598050`（`better-d1-ci.yml`，HEAD `d1e122ab6`）conclusion = `success` |

---

## 3. 自查输出（原样粘贴）

### 3.1 行尾自查（提交后复跑）

```
$ python3 -c "b=open('docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md','rb').read();print('CRLF',b.count(b'\r\n'),'LF',b.count(b'\n'))"
CRLF 0 LF 217
```

**结论**：CRLF = 0，满足要求；文件保持 LF。

### 3.2 drift 校验（提交后复跑）

```
$ python3 tools/check_drift.py --base origin/master
PASS A  Tests.cmake entries have source files
PASS B  no placeholder assertions in tests
PASS C  modified files keep line endings
PASS C2 added files match .editorconfig
PASS E  no test-only production functions

merge-base: e00b7260f   check E allowlist: 10 upstream symbols
$ echo $?
0
```

**结论**：5 项全 PASS，exit 0。

### 3.3 工作区与同步状态

```
$ git status --porcelain
(空)

$ git rev-list --count HEAD..origin/master
0
```

### 3.4 提交

```
$ git log --oneline -1
5cce05c30 docs(spec): mark upstream sync round 2 as implemented + record findings

$ git show --stat --format="" HEAD
 docs/.../2026-09-15-upstream-sync-round2-design.md | 55 ++++++++++++++++++++--
 1 file changed, 51 insertions(+), 4 deletions(-)
```

**提交 SHA**：`5cce05c30`（完整 `5cce05c30` 短 SHA；父提交 `d1e122ab6`）
**push**：未执行。

### 3.5 对任务给定实测数据的独立复核

为确认写入规格的数字与工作区实际状态一致，本次对可廉价复核的项做了独立验证（未新增任何推算数字）：

| 复核项 | 命令 | 结果 |
|---|---|---|
| merge 双父 | `git rev-list --parents -n1 8e92687a9` | `8e92687a9 fd2956ea8 e00b7260f` ✔ |
| 冲突标记 | `git grep -nE "^(<<<<<<<\|>>>>>>>\|=======)$" -- Source/ test/ CMake/ CMakeLists.txt` | 无匹配（exit 1）✔ |
| ctest 聚合 | `python3 -c` 读 `/tmp/ci-sync.json` | build.ok=True, total=698, failed=0, skipped=3, passed_pct=100 ✔ |
| drift JSON | 同上 | drift_ok=True, passes=5, output 含 `merge-base: e00b7260f` ✔ |
| eval 产物 | `eval/results/20260915-183246/eval-summary.json` | 36/36 passed, `git_head=91e805149…` ✔ |
| CI run | `gh run view 34959598050` | conclusion=`success`, headSha=`d1e122ab6…`, workflow=`Better D1 CI` ✔ |
| heroname 上游 | `git show origin/master:Source/msg.cpp\|pack.cpp \| grep -n heroname` | msg.cpp 1094/1360/1368/1398、pack.cpp 445 ✔ |
| timedemo | `grep Timedemo build/Testing/Temporary/LastTest.log` | `[  SKIPPED ] Timedemo.WarriorLevel1to2` ✔ |
| drawInfoBox 计数 | `git show <rev>:...scrollrt.cpp \| grep -c drawInfoBox` | merge-base 4 / HEAD 0 / origin/master 4 ✔ |
| `ed1dad939` 存在 | `git log --oneline -1 ed1dad939` | `refactor: remove drawInfoBox dirty rect` ✔ |
| quests.cpp 现状 | 读工作区文件 | 638 行、CRLF 638 / LF 638 / LF-only 0；include 含 `lua/lua_event.hpp`，**不含** `minitext.h` ✔ |
| quests.cpp 上游删除量 | `git diff --stat b3e52b1ea origin/master -- Source/quests.cpp` | `2 insertions(+), 341 deletions(-)` ✔ |
| `91e805149` 内容 | `git show 91e805149 -- Source/controls/plrctrls.cpp` | 1 行 `+#include "panels/quest_log.hpp"`，位于 `minitext.h` 与 `missiles.h` 之间 ✔ |
| quests.h 行尾 | 读工作区文件 | CRLF 0 / LF 32 ✔ |

**未发现任务给定数据与工作区实测不一致之处。**

---

## 4. 发现的文档内部不一致（如实报告）

### 4.1 状态字段三处互相矛盾（已修正）

改动前的规格对同一「状态」字段有三种不同表述：

| 位置 | 原文 |
|---|---|
| 头部元数据（第 5 行） | `**状态**：已批准（实施计划见 …）` |
| §7 状态 | `**草案（待批准）。** 批准后按 §4 执行；…` |
| §5 通用红线 #4 判定列 | `本规格状态为「草案」，未标「已实施」。达成 §6 全部标准后方可改标` |

任务只要求改 §7。但若只改 §7，头部仍写「已批准」、§5 红线 #4 仍写「草案」，文档将留下两处**在改标后即失效**的陈述。因此本次一并修正：

- 头部 → `已实施（2026-09-15；…）`
- §5 通用红线 #4 → `已于 2026-09-15 改标「已实施」：非测试调用者为仓库内全部生产代码…`
- §7 → `已实施（2026-09-15）`

**告警**：这是超出任务字面范围的两处额外改动（仍在同一 .md 文件内）。若控制者认为应保持最小改动面，可回退 `2.1`/`2.4` 两处。

### 4.2 §4 与 §3 关于 `quests.h` 的一致性核查（任务点 4）

任务点 4 要求核查 §4 是否已补「保持 LF」的判据来源。核查结果：

- §4 步骤 3 原文已写「**取上游全文（保持 LF）**……（理由与行尾判据见 §3「探测深入」）」——**判据来源已存在**，无需新增。
- §4 风险表对应行原文已写「被上游重写为 LF 的文件**保持 LF**（判据见 §3「探测深入」）」——**亦已存在**。
- §3「探测深入」给出判据本体（`.gitattributes` = `* -text`；门禁 C 以新 merge-base 为基线；写回 CRLF 会 FAIL 且每次同步都整文件冲突）；§6 第 7 项亦引用该判据。

**结论**：§3 与 §4 之间**不存在与实测矛盾的陈述**，四处（§3 / §4 步骤 3 / §4 风险表 / §6 第 7 项）判据来源已闭环。规格中已按此写入「处置」段。

### 4.3 非矛盾但值得记录的既有表述

- §4 步骤 3 `quests.cpp` 条目写「保留 fork 的两个 include（`lua/lua_event.hpp`、`minitext.h`），**除非确认其消费者已随上游改动消失**」。实测正是触发了该条件分支（`InitQTextMsg` 的消费者随上游迁移消失），最终只保留 `lua/lua_event.hpp`。原句是条件式写法，**不构成错误**，故未改写；实际处置已记入 §7 第 2 点。
- §3 该节标题原文为「探测深入：上游 quest 重构的编译连带」，但事实表 / §7 中引用为「探测深入：quest_log 重构」。两处指同一节（§3 该节正文即讲 quest_log 的 include 连带）。规格 §7 第 3 点标题使用了「quest_log 重构」以确保与 §3 本体引用一致，**未修改 §3 原标题**（避免重写无关段落）。

---

## 5. 硬约束遵守情况

| 约束 | 状态 |
|---|---|
| 只改 `docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md` | ✔ `git status --porcelain` 仅该文件 |
| 不改源码 / 测试 / 计划文件 | ✔ |
| 不执行 `git push` | ✔ 未调用任何 push |
| 不分发子代理 | ✔ |
| 不编造数字 | ✔ 全部数字来自任务给定实测值，且经 §3.5 独立复核 |
| 行尾 LF | ✔ CRLF 0 / LF 217 |

## 6. 顾虑 / 待控制者裁决

1. **额外改动面**（见 §4.1）：头部状态行与 §5 红线 #4 超出任务字面范围。理由是避免改标后留下两处自相矛盾的陈述；若要求最小改动，可单独回退这两处而不影响 §7 完整性。
2. **§3 该节标题命名**（见 §4.3）：§3 原标题「上游 quest 重构的编译连带」与事实表 / §7 引用的「quest_log 重构」字面不同但指同一节，本次未改原标题。
3. **`plrctrls.cpp` 的 7 处编译错误**：任务给定为 7 处，本次未在仓库中找到留存该编译输出的日志文件（`91e805149` 的提交信息只列出 4 个被使用但未声明的符号），故规格按给定数字写入而未附加符号级映射。