# 上游同步（第二轮）实施计划

> **面向 Agent 执行者：** 必需子技能：使用 superpower-subagent-driven-development（推荐）或 superpower-executing-plans 按任务逐项执行本计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 把上游 `origin/master`（28 个提交，tip `e00b7260f`）合入 `feature/qol-upgrades`，解 4 处冲突，四道门禁全绿，并把 fork-only 的 heroname 修复状态复核留档。

**架构：** 一次 `git merge`（非 rebase）产生 merge commit；4 处冲突逐个人工判定；其中 `quests.h` 的整文件冲突源于「上游把该文件重写为 LF 并把内容搬到新建的 `tables/questdat.hpp`」——取上游全文，再把 fork 的唯一增量（`QuestData::scriptName`）落到它的新家。合并后预期有一类编译连带（上游把 `QuestLogIsOpen`/`pQLogCel`/`DrawQuestLog` 迁到 `panels/quest_log.hpp`），由编译门禁暴露并逐个补 include。门禁顺序与判据见规格 §6。

**技术栈：** C++23 / CMake+Ninja / GoogleTest + ctest / Python 工具链（`tools/run_tests.py`、`tools/check_drift.py`、`tools/eval/backend.py`）

**规格：** `docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`（裁决基准；本计划的论证以规格为准，执行者需同时阅读两者）

## 全局约束

- 集成方式：**merge，禁止 rebase**（`feature/qol-upgrades` 的 224 个提交已推送到 `myrepo`）。
- 基线锁定：合并对象是 `origin/master` 的 `e00b7260f`；执行前先 `git fetch origin master` 并确认 tip 未变（变了则停止并回报，重新探测冲突面）。
- 行尾：`.gitattributes` 是 `* -text`（git 不改行尾）。`.editorconfig` 默认 CRLF，C++ 属默认。**被上游重写为 LF 的文件（`Source/quests.h` 及上游新增文件）保持 LF**——门禁 C 以「新的 merge-base（= 上游 tip）」为基线比对，把它改成 CRLF 会直接 FAIL，且会保证以后每次同步都整文件冲突。
- 冲突解决禁止整块取一侧（`quests.h` 除外——它已被上游整文件重写，见任务 3）。
- 门禁判据（与规格 §6 一致）：`ctest.failed == 0`、`ctest.passed_pct == 100`、`drift.drift_ok == true`（5 项 PASS）、eval smoke exit 0。测试数量以零失败为准；数量变化必须记录原因。
- 不推送 `master`；只推 `myrepo feature/qol-upgrades`。
- 每个任务一个提交（提交信息用 conventional commits 前缀，正文写清依据与实测）。

---

## 探测阶段已确认的事实（2026-09-15 实测，实施时直接引用）

| 项 | 值 | 出处 |
|---|---|---|
| merge-base | `b3e52b1ea`（2026-07-26） | `git merge-base HEAD origin/master` |
| 上游 tip | `e00b7260f` | `git fetch origin master` |
| 落后提交 | 28 | `git rev-list --count HEAD..origin/master` |
| 上游触及 / fork 改动 / 交集 | 106 / **265** / 24 文件 | `git log --name-only`、`git diff --name-only`、`comm -12` |
| 探测性 merge 冲突 | **4 文件 / 4 块** | worktree + `--no-commit` + `--abort` |
| 测试基线 | 698（0 failed / 3 skipped） | `python3 tools/run_tests.py` |

**4 处冲突的完整内容**（已在探测性 merge 中逐字取出）：

```cpp
// Source/controls/plrctrls.cpp —— 双方各加了一个 include
<<<<<<< HEAD
#include "levels/gendung.h"
=======
#include "cursor_defs.hpp"
>>>>>>> origin/master
```

```cpp
// Source/engine/render/scrollrt.cpp —— 上游改了 DrawMain 签名并插入 yield
<<<<<<< HEAD
	DrawMain(hgt, false, drawHealth, drawMana, drawBelt, drawControlButtons);
=======
	this_sdl_thread::yield();
	DrawMain(hgt, drawInfoBox, drawHealth, drawMana, drawBelt, drawControlButtons);
>>>>>>> origin/master
```

```cpp
// Source/quests.cpp —— fork 侧多两个 include，上游删掉了这一块
<<<<<<< HEAD
#include "lua/lua_event.hpp"
#include "minitext.h"
=======
>>>>>>> origin/master
```

`Source/quests.h` 是**整文件冲突**（`<<<<<<<` 在第 1 行、`=======` 在第 151 行）：fork 侧 149 行（CRLF），上游侧 32 行（LF）。原因不是内容改写而是**上游把该文件重写为 LF 并把 148 行内容搬到新建的 `Source/tables/questdat.hpp`**（`git diff --stat b3e52b1ea origin/master -- Source/quests.h` = `32 insertions(+), 148 deletions(-)`）；fork 侧相对 merge-base 只加了 **1 行**：

```cpp
// fork 在 struct QuestData 末尾新增（quests.h:107）
	std::string scriptName;
```

上游 `Source/tables/questdat.hpp:117-127` 的 `struct QuestData` 字段与 fork 完全一致（`_qdlvl` … `_qlstr`），**只缺这一行**。

**quest_log 重构的编译连带：经复验为「零连带」**（初稿按「预期编译失败」写过，已被探测性 merge 证伪）。上游把 `QuestLogIsOpen` / `pQLogCel` / `DrawQuestLog` 从 `quests.h`+`quests.cpp` 迁到新建的 `Source/panels/quest_log.{hpp,cpp}`，但**上游同时给每个消费者文件补了 `#include "panels/quest_log.hpp"`**，而 fork 对这些文件的本地改动不与那一行重叠，因此 git 自动合并即完成迁移。`git -c rerere.enabled=false merge --no-commit --no-ff origin/master` 后实测：下列 12 个文件**各含 1 个**该 include，且 `Source/quests.cpp` 对这三个符号的引用为 **0**（上游对该文件的删除也与 fork 的 2 行 include 冲突不重叠，自动合并已生效）——既无「未声明」编译错误，也无与 `panels/quest_log.cpp` 的重复定义。

| 已自动带上 include 的文件（复验计数均为 1） |
|---|
| `Source/minitext.cpp`、`Source/qol/chatlog.cpp`、`Source/diablo.cpp`、`Source/stores.cpp`、`Source/control/control_panel.cpp`、`Source/controls/game_controls.cpp`、`Source/controls/touch/gamepad.cpp`、`Source/controls/touch/renderers.cpp`、`Source/controls/plrctrls.cpp`、`Source/engine/render/scrollrt.cpp`、`test/panel_state_test.cpp`、`test/ui_test.hpp` |

**另一处需注意的交集文件**：`test/ui_test.hpp` 与 `test/panel_state_test.cpp` 也在 24 文件交集内（上游 `53b91fd7a` 改动过），探测性 merge 中**未产生冲突**（自动合并成功，并各自带上该 include）。它们是 13 个测试二进制共享的头（`test/ui_test.hpp`），因此它们的健康度影响面比单文件更大——任务 9 的全量门禁是其验收依据。

---

## 文件结构

**新建（由 merge 自动带入，本计划不手写）：** `Source/tables/questdat.{hpp,cpp}`、`Source/panels/quest_log.{hpp,cpp}`、`Source/cursor_defs.hpp`、`Source/levels/parse_dungeon_type.*`、`Source/engine/light_tables.*`、`Source/levels/drlg_quests.*`、`Source/levels/dun_tile_data.*`、`Source/parse_sfx_id.*`、`CMake/finders/FindSDL.cmake`、`Source/platform/amiga/strerror_r.c`（共 17 个）

**修改（人工参与）：**

| 文件 | 动作 |
|---|---|
| `Source/quests.h` | 取上游全文（32 行 / LF），丢弃 fork 侧 149 行版本 |
| `Source/tables/questdat.hpp` | 在 `struct QuestData` 末尾补 `std::string scriptName;`（fork 增量迁移） |
| `Source/engine/render/scrollrt.cpp` | 采用上游两行（`yield()` + `drawInfoBox` 参数） |
| `Source/controls/plrctrls.cpp` | 两个 include 都保留 |
| `Source/quests.cpp` | 保留 fork 的两个 include |
| `test/ui_test.hpp`、`test/panel_state_test.cpp` | 交集文件（上游改过），探测 merge 自动合并成功、无需人工改动 —— 但由任务 9 全量门禁验收 |

---

### 任务 1：预检与基线固化

**文件：**
- 修改：无（只读 + 记录）

**接口：**
- 依赖输入：无
- 对外产出：确认的 `UPSTREAM_TIP`（供任务 2、11 使用的 SHA）、基线门禁 JSON（供任务 10 对比）

- [ ] **步骤 1：确认上游 tip 未变**

```bash
git fetch origin master
git log -1 --format='%H %ad %s' --date=iso origin/master
```

预期：`e00b7260f`，提交信息 `Prevent inlining of blitter operations`。**若 SHA 变了：停止，重新跑探测性 merge 并回报新的冲突面。**

- [ ] **步骤 2：确认工作区干净且分支正确**

```bash
git status --short          # 预期：无输出
git branch --show-current   # 预期：feature/qol-upgrades
git worktree list           # 预期：只有主工作区
```

- [ ] **步骤 3：记录基线门禁**

```bash
python3 tools/run_tests.py --json /tmp/ci-before-sync.json 2>&1 | tail -20
```

预期：`ctest.failed == 0`、`ctest.passed_pct == 100`、`ctest.total == 698`、`drift.drift_ok == true`。**若不满足：先停下来回报——不要在有红灯的基线上做 merge。**

- [ ] **步骤 4：记录基线 drift 输出（供任务 10 对比口径变化）**

```bash
python3 tools/check_drift.py --base origin/master 2>&1 | tee /tmp/drift-before-sync.txt
```

预期：5 项 PASS，`merge-base: b3e52b1ea`。

- [ ] **步骤 5：无需提交（本任务不产生代码变更）**

---

### 任务 2：执行 merge 并复现 4 处冲突

**文件：**
- 修改：由 merge 自动（100 个文件自动合并 + 4 个冲突文件）

**接口：**
- 依赖输入：任务 1 确认的 `origin/master` tip
- 对外产出：处于冲突待解决状态的索引（供任务 3-6 使用）

- [ ] **步骤 1：执行 merge（不自动提交）**

```bash
git merge --no-commit --no-ff origin/master
```

预期：退出码 1，输出「自动合并失败，修正冲突然后提交修正的结果」。

- [ ] **步骤 2：确认冲突集合与探测结果一致**

```bash
git diff --name-only --diff-filter=U | sort
```

预期恰好 4 个文件：

```
Source/controls/plrctrls.cpp
Source/engine/render/scrollrt.cpp
Source/quests.cpp
Source/quests.h
```

**若出现第 5 个冲突文件：停止并回报**（探测结果不成立，说明上游 tip 已变或 fork 侧状态变了）。

- [ ] **步骤 3：确认无冲突标记遗留在自动合并的文件里**

```bash
git grep -nE "^(<<<<<<<|>>>>>>>|=======)$" -- Source/ test/ CMake/ CMakeLists.txt | grep -v -E "quests\.h|plrctrls\.cpp|scrollrt\.cpp|quests\.cpp"
```

预期：无输出。

- [ ] **步骤 4：本任务不单独提交**（与任务 3-6 的冲突解决一起提交为一个 merge commit）

---

### 任务 3：解决 `Source/quests.h`（整文件冲突 + fork 增量迁移）

**文件：**
- 修改：`Source/quests.h`（改为上游版本）、`Source/tables/questdat.hpp`（补 fork 增量）
- 测试：`Source/quests.cpp:286-287`（`questData.scriptName` 的消费者，编译即验证）

**接口：**
- 依赖输入：任务 2 的冲突状态
- 对外产出：`QuestData`（现在声明于 `Source/tables/questdat.hpp`）含成员 `std::string scriptName;`，供 `Source/quests.cpp` 的 `lua::OnQuestCheck(questData.scriptName, &quest)` 使用

- [ ] **步骤 1：取上游版本覆盖冲突文件**

```bash
git checkout --theirs -- Source/quests.h
git add Source/quests.h
```

- [ ] **步骤 2：确认取到的是上游版本且行尾为 LF**

```bash
wc -l Source/quests.h                      # 预期：32
grep -c $'\r' Source/quests.h              # 预期：0（LF）
grep -n "questdat.hpp" Source/quests.h     # 预期：命中 include 行
```

- [ ] **步骤 3：确认上游 `QuestData` 缺的正是 fork 的那一行**

```bash
sed -n '/struct QuestData/,/};/p' Source/tables/questdat.hpp
grep -n "scriptName" Source/tables/questdat.hpp || echo "EXPECTED-ABSENT"
```

预期：结构体末尾是 `std::string _qlstr;`，且 `scriptName` 不存在。

- [ ] **步骤 4：把 fork 增量落到它的新家**

在 `Source/tables/questdat.hpp` 的 `struct QuestData` 中，`std::string _qlstr;` 之后加一行：

```cpp
	std::string scriptName;
```

- [ ] **步骤 5：确认落地正确且行尾未变**

```bash
sed -n '/struct QuestData/,/};/p' Source/tables/questdat.hpp   # 预期：末行为 std::string scriptName;
python3 -c "b=open('Source/tables/questdat.hpp','rb').read(); print('CRLF', b.count(b'\r\n'), 'LF', b.count(b'\n'))"
```

预期：`CRLF 141 LF 141`（**上游该文件本身就是 CRLF**——本轮唯一的 LF 文件是 `Source/quests.h` 与上游新增的 7 个文件，`questdat.hpp` 不在其中）。判据是「与该文件在上游的行尾一致」，不是固定值；本例的一致值就是 CRLF。

- [ ] **步骤 6：确认消费者的引用链完整**

```bash
grep -n "scriptName" Source/quests.cpp Source/quests.h Source/tables/questdat.hpp
```

预期：`questdat.hpp` 定义、`quests.cpp:286-287` 使用；`quests.h` 不再出现（内容已迁走）。

- [ ] **步骤 7：提交（与其余冲突解决合并提交，见任务 6 步骤 3）**

---

### 任务 4：解决 `Source/engine/render/scrollrt.cpp`

**文件：**
- 修改：`Source/engine/render/scrollrt.cpp`（1 处冲突）

**接口：**
- 依赖输入：任务 2 的冲突状态
- 对外产出：`DrawMain` 调用点与上游新签名一致（含 `drawInfoBox` 参数），并保留上游插入的 `this_sdl_thread::yield();`

- [ ] **步骤 1：确认上游的 `DrawMain` 签名与 `drawInfoBox` 变量来源**

```bash
grep -n "drawInfoBox" Source/engine/render/scrollrt.cpp | head
git show origin/master:Source/engine/render/scrollrt.cpp | grep -n "drawInfoBox" | head
```

预期：上游文件里 `drawInfoBox` 在同一函数作用域内已有定义（若没有，说明签名改动还有别的调用点要改——**停止并回报**）。

- [ ] **步骤 2：采用上游两行**

把冲突块替换为：

```cpp
	this_sdl_thread::yield();
	DrawMain(hgt, drawInfoBox, drawHealth, drawMana, drawBelt, drawControlButtons);
```

- [ ] **步骤 3：确认无残留标记、且 fork 侧该文件的其它改动仍在**

```bash
grep -nE "^(<<<<<<<|>>>>>>>|=======)$" Source/engine/render/scrollrt.cpp   # 预期：无输出
git diff origin/master -- Source/engine/render/scrollrt.cpp | head -40
```

预期：与上游的差异只包含 fork 侧既有改动（不应出现 `DrawMain(hgt, false, ...)` 这一旧形态）。

- [ ] **步骤 4：提交（与其余冲突解决合并提交，见任务 6 步骤 3）**

---

### 任务 5：解决 `Source/controls/plrctrls.cpp`

**文件：**
- 修改：`Source/controls/plrctrls.cpp`（1 处冲突，双方各加一个 include）

**接口：**
- 依赖输入：任务 2 的冲突状态
- 对外产出：两个 include 同时存在

- [ ] **步骤 1：采用双方**

把冲突块替换为（保持文件既有的 include 排序风格，`cursor_defs.hpp` 与 `levels/gendung.h` 都在此保留）：

```cpp
#include "cursor_defs.hpp"
#include "levels/gendung.h"
```

- [ ] **步骤 2：确认无残留标记**

```bash
grep -nE "^(<<<<<<<|>>>>>>>|=======)$" Source/controls/plrctrls.cpp   # 预期：无输出
grep -n "cursor_defs.hpp\|levels/gendung.h" Source/controls/plrctrls.cpp
```

- [ ] **步骤 3：提交（与其余冲突解决合并提交，见任务 6 步骤 3）**

---

### 任务 6：解决 `Source/quests.cpp` 并提交 merge

**文件：**
- 修改：`Source/quests.cpp`（1 处冲突，fork 侧两个 include 对上游的删除）

**接口：**
- 依赖输入：任务 2 的冲突状态；任务 3-5 的解决结果
- 对外产出：一个完整的 merge commit，含全部 4 处冲突解决

- [ ] **步骤 1：确认这两个 include 在 fork 侧确有消费者**

```bash
grep -rn "lua::OnQuestCheck\|minitext" Source/quests.cpp | head
```

预期：`lua::OnQuestCheck`（`questData.scriptName` 那条）与 minitext 相关调用都在 `quests.cpp` 内。**若发现消费者已随上游改动消失：改为取上游（删除该 include 块），并在提交信息里写明依据。**

- [ ] **步骤 2：保留 fork 的两个 include**

把冲突块替换为：

```cpp
#include "lua/lua_event.hpp"
#include "minitext.h"
```

- [ ] **步骤 3：确认没有残留冲突标记，然后提交 merge**

```bash
git grep -nE "^(<<<<<<<|>>>>>>>|=======)$" -- Source/ test/ CMake/ CMakeLists.txt
```

预期：无输出。然后：

```bash
git add Source/quests.cpp Source/quests.h Source/tables/questdat.hpp \
        Source/engine/render/scrollrt.cpp Source/controls/plrctrls.cpp
git commit -F - <<'MSG'
merge: sync upstream master (28 commits, tip e00b7260f)

Merge upstream master into feature/qol-upgrades: merge-base had been stuck
at b3e52b1ea (2026-07-26) for 28 upstream commits and 224 fork commits.

Conflicts (4 files, all resolved by hand):
- quests.h: upstream rewrote the file to LF and moved its contents to the
  new tables/questdat.hpp. Took upstream's version verbatim (keeping LF -
  check C compares against the new merge-base, and CRLF would guarantee a
  whole-file conflict on every future sync) and re-applied the fork's only
  local addition, QuestData::scriptName, in its new home so that
  quests.cpp's lua::OnQuestCheck(questData.scriptName, ...) keeps working.
- scrollrt.cpp: took upstream's DrawMain(..., drawInfoBox, ...) plus the
  this_sdl_thread::yield() line.
- plrctrls.cpp: kept both sides' includes (cursor_defs.hpp + levels/gendung.h).
- quests.cpp: kept the fork's includes (lua/lua_event.hpp, minitext.h).

Expected fallout handled in the following commits: upstream moved
QuestLogIsOpen/pQLogCel/DrawQuestLog into the new panels/quest_log.hpp.
MSG
```

- [ ] **步骤 4：确认 merge commit 形态正确**

```bash
git log --oneline -1                       # 预期：merge 提交
git rev-list --count HEAD..origin/master   # 预期：0
git status --short                         # 预期：无输出
```

---

### 任务 7：验证 quest_log 重构零连带（预期零改动）

**文件：**
- 修改：无（本任务预期不产生代码变更，只验证）
- 测试：`cmake --build build --target libdevilutionx`（编译即验证）

**接口：**
- 依赖输入：任务 6 的 merge commit
- 对外产出：库编译通过的结论（任务 8 的前置）

**背景（已在探测阶段复验）**：初稿曾预判此步会产生「`QuestLogIsOpen` 未声明」编译错误，**该预判已被证伪**。上游迁移这三个符号时**同时给每个消费者文件补了 `#include "panels/quest_log.hpp"`**，且 fork 对这些文件的本地改动不与那一行重叠 → git 自动合并即完成；`Source/quests.cpp` 对这三个符号的引用计数已为 0（无重复定义）。复验命令与计数见本计划「探测阶段已确认的事实」。

- [ ] **步骤 1：编译，确认零连带**

```bash
cmake --build build --target libdevilutionx -j 20 2>&1 | grep -E "error:" | head -30
```

预期：**无输出**（零编译错误）。

- [ ] **步骤 2：确认 12 个消费者文件的 include 已就位（机器判据，不靠肉眼）**

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

预期：前 12 行计数**均为 1**；最后一行输出 `0`。

- [ ] **步骤 3：仅当步骤 1 出现编译错误时才现场诊断**

判据：读错误原文，确认是哪个符号在哪个文件未被声明；若确为该类，给该文件补 `#include "panels/quest_log.hpp"`，重编译至零错误，然后提交：

```bash
git add -A Source/
git commit -m "fix(build): add panels/quest_log.hpp include after the upstream quest split"
```

- [ ] **步骤 4：未出现错误时不提交（预期路径）**

### 任务 8：门禁 1-2（CMake 配置 + 全目标编译）

**文件：**
- 修改：无（除非编译再暴露问题，按任务 7 的判据处理并追加提交）

**接口：**
- 依赖输入：任务 7 的编译通过
- 对外产出：`build.ok == true`

- [ ] **步骤 1：配置通过**

```bash
cmake -S. -Bbuild -G Ninja -DDEVILUTIONX_SYSTEM_BENCHMARK=OFF 2>&1 | tail -5
```

预期：退出码 0，无「找不到目标」类错误。

- [ ] **步骤 2：库与主程序编译通过**

```bash
cmake --build build --target devilutionx -j 20 2>&1 | tail -5
```

预期：链接成功，生成 `build/devilutionx`。

- [ ] **步骤 3：若上游 CMake 改动引发配置错误，按错误原文处理并追加提交**

```bash
git commit -am "fix(build): adapt to upstream CMake changes"
```

（仅在步骤 1/2 失败时执行；提交信息里贴错误原文与判据。）

---

### 任务 9：门禁 3-4（测试目标 + 全量测试）

**文件：**
- 修改：无（除非测试注册漂移，按判据处理）

**接口：**
- 依赖输入：任务 8 的编译通过
- 对外产出：`ctest.failed == 0`

- [ ] **步骤 1：全部测试目标构建**

```bash
python3 tools/run_tests.py --json /tmp/ci-sync.json 2>&1 | tail -30
```

预期：`build.ok == true`；随后 ctest 全量执行。

- [ ] **步骤 2：判定并记录**

```bash
python3 -c "
import json; d=json.load(open('/tmp/ci-sync.json'))
print('ctest', d['steps']['ctest']); print('drift_ok', d['steps']['drift']['drift_ok'])"
```

预期：`failed == 0`、`passed_pct == 100`。测试数量若不再是 698：**必须记录变化原因**（上游新增/删除测试），判据仍是零失败。

- [ ] **步骤 2b：按名字确认 timedemo 仍为 Skipped（不能只看聚合计数）**

`run_tests.py` 的 `summary` 只记录 skipped **计数**，不记录被跳过用例的名字，因此聚合数字不变也可能掩盖状态翻转：

```bash
grep -n "WarriorLevel1to2" build/Testing/Temporary/LastTest.log
```

预期：出现 `Timedemo.WarriorLevel1to2 (Skipped)`（规格 §6 验收项 9）。

- [ ] **步骤 2c：确认本地测试目标列表未与 `CMake/Tests.cmake` 脱钩**

`tools/run_tests.py` 的 `TEST_TARGETS` 是手工列表，`check_drift.py` 的 A 项只校验 `Tests.cmake` ↔ `test/*.cpp`，**不校验这个列表**：

```bash
git diff b3e52b1ea origin/master -- CMake/Tests.cmake | grep -E '^[+-]  [a-z_0-9]+_test'
```

预期：无输出（本轮上游对 `Tests.cmake` 的改动只涉及 `dun_render_benchmark` 的链接依赖与 `test_main`，未增删改任何 `tests`/`standalone_tests` 成员）。**若有输出：必须同步更新 `tools/run_tests.py` 的 `TEST_TARGETS`**，否则该工具会静默漏跑新测试。

- [ ] **步骤 3：失败处理（若发生）**

```bash
cd build && ctest --output-on-failure -R "<失败测试名>"
```

单测失败 → 在 `build/Testing/Temporary/LastTest.log` 读栈与断言 → 判断是「上游行为改变」还是「fork 适配缺失」，**不要用改测试断言的方式让门禁变绿**；若确认是上游的既有行为变化，按 `docs/knowledge/` 规则记录并在此计划末尾追加「执行中暴露的问题」小节。

---

### 任务 10：eval smoke 与漂移校验

**文件：**
- 修改：无

**接口：**
- 依赖输入：任务 9 的全量门禁
- 对外产出：`eval smoke exit 0`、drift 5 项 PASS 及与基线的口径对比结论

- [ ] **步骤 1：eval smoke 门禁**

```bash
python3 -m tools.eval.backend --smoke; echo "exit=$?"
```

预期：exit 0，36/36 PASS。

- [ ] **步骤 2：漂移校验并与基线对比**

```bash
python3 tools/check_drift.py --base origin/master 2>&1 | tee /tmp/drift-after-sync.txt
diff /tmp/drift-before-sync.txt /tmp/drift-after-sync.txt
```

预期：5 项 PASS；与基线的唯一差异应是 `merge-base:` 行从 `b3e52b1ea` 变为 `e00b7260f`。**若 PASS/FAIL 集合发生变化，逐项说明原因并追加到实施记录**（规格 §4 风险表第 5 行）。

- [ ] **步骤 3：确认没有行尾被改动**

```bash
python3 tools/check_drift.py --base origin/master 2>&1 | grep -E "PASS C |PASS C2 "
```

预期：两项都 PASS。（若 FAIL，最常见原因是把 `Source/quests.h` 写回了 CRLF——回到任务 3 步骤 5 的判据修正。）

---

### 任务 11：复核 heroname 分叉状态并留档

**文件：**
- 修改：`docs/knowledge/pattern_fixed_width_field_reads.md`（追加同步后的分叉状态），必要时新建实施记录

**接口：**
- 依赖输入：任务 6 的 merge 完成
- 对外产出：heroname 修复在上游是否已修的明确结论（规格 §6 验收项 8）

- [ ] **步骤 1：在上游 tip 上复核（这是判据，不看本地）**

```bash
git show origin/master:Source/msg.cpp | grep -n "heroname"
git show origin/master:Source/pack.cpp | grep -n "heroname"
```

预期（2026-09-15 探测结果）：`msg.cpp:1094/1360/1368/1398`、`pack.cpp:445` 仍是裸字段（未修）。

- [ ] **步骤 2：按结论分支处理**

- 若**仍未修**：保留 fork 修复，在 `docs/knowledge/pattern_fixed_width_field_reads.md` 的正文追加一行状态（格式：`**上游状态（2026-09-15 同步后复核）**：仍保留 5 处裸字段读取，fork 修复见 a82c0a827`），并把上游 PR 的文案与 ASan 复现步骤整理成 `docs/superpowers/plans/` 下的一个独立小节或单独文件（不在本计划内提交 PR——对外动作需单独确认）。
- 若**已修**：比对上游实现与 fork 修复（`git show origin/master:Source/pack.cpp | grep -n "pName"`），若上游用了同样的定长读取则以**上游为准**并追加提交：

```bash
git checkout origin/master -- Source/pack.cpp Source/pfile.cpp Source/msg.cpp
git commit -am "fix: take upstream's bounded name reads (upstream caught up)"
```

- [ ] **步骤 3：提交留档**

```bash
git add docs/knowledge/pattern_fixed_width_field_reads.md
git commit -m "docs(knowledge): record heroname divergence state after the upstream sync"
```

---

### 任务 12：推送、CI 与规格状态更新

**文件：**
- 修改：`docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md`（§7 状态改为「已实施」+ 实施记录）

**接口：**
- 依赖输入：任务 1-11 全部完成
- 对外产出：远端同步、CI 绿、规格状态闭环

- [ ] **步骤 1：推送**

```bash
git push myrepo feature/qol-upgrades
```

预期：推送成功（**不是** `master`）。

- [ ] **步骤 2：等 CI 并确认结论**

```bash
gh run list --repo pengyz/devilutionX --workflow=better-d1-ci.yml --limit 3 \
  --json databaseId,headSha,conclusion --jq '.[] | "\(.databaseId) \(.conclusion) \(.headSha[0:9])"'
```

预期：对应本次 HEAD 的运行 conclusion 为 `success`。

- [ ] **步骤 3：更新规格状态并把实测结论写进去**

在 `docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md` 的 §7 追加（内容必须来自实测，不得照抄本计划）：

```markdown
## 7. 状态

**已实施（<YYYY-MM-DD>）。** §6 全部验收项通过：`ctest` <N> 项零失败、drift 5 项 PASS、eval smoke 36/36、CI <run id> success。

冲突实际为 4 处（与探测一致）。执行中与规格不符/未预料之处：

- <逐条记录，例如：上游把 quests.h 重写为 LF 导致整文件冲突——规格未预料，已按「保持与上游一致」处置并写明判据>
```

- [ ] **步骤 4：提交并推送文档更新**

```bash
git add docs/superpowers/specs/2026-09-15-upstream-sync-round2-design.md
git commit -m "docs(spec): mark upstream sync round 2 as implemented + record findings"
git push myrepo feature/qol-upgrades
```

---

## 对抗性复核记录（2026-09-15，kiro 子代理 + 主 agent 复验）

对初稿做过一次独立对抗性复核（要求只读、只写可验证结论）。发现与处置：

| id | 严重度 | 问题 | 处置 |
|---|---|---|---|
| F1 | blocker | 任务 7 的「预期编译连带」是**假问题**：探测性 merge（rerere 关闭）显示 12 个消费者文件已由上游改动自动带上 `#include "panels/quest_log.hpp"`，`quests.cpp` 对三个符号的引用计数为 0 → 既无未声明错误也无重复定义 | 任务 7 重写为「验证零连带」（机器判据：12 文件各 1 个 include、引用计数 0），仅在编译真报错时进入诊断分支；规格 §3 对应段落同步改写 |
| F2 | high | 事实表「本分支侧改动文件 = 263」错误，实测 **265** | 计划与规格两处均改为 265（复验命令 `git diff --name-only $(git merge-base HEAD origin/master) HEAD \| wc -l`） |
| F3 | high | `test/ui_test.hpp`、`test/panel_state_test.cpp` 在 24 文件交集内（上游 `53b91fd7a` 改过）却未被文档提及 | 已在「探测阶段已确认的事实」与规格 §3 写明：自动合并成功、无需人工改动，由任务 9 全量门禁兜底（`ui_test.hpp` 被 13 个测试二进制共享） |
| F4 | medium | 任务 3 步骤 5 对 `questdat.hpp` 行尾的预期写成 LF，实际上游是 **CRLF**（141/141）；本轮唯一 LF 的是 `Source/quests.h` 与上游新增的 7 个文件 | 预期输出改为 `CRLF 141 LF 141` 并说明原因 |
| F5 | medium | `tools/run_tests.py` 的 `TEST_TARGETS` 手工列表与 `CMake/Tests.cmake` 存在结构性脱钩风险（`check_drift.py` A 项不校验该列表）。本轮未触发，但计划对此沉默 | 任务 9 增加步骤 2c：用 `git diff <merge-base> origin/master -- CMake/Tests.cmake` 过滤集合成员增删，有输出则必须同步 `TEST_TARGETS` |
| F8 | low | 任务 9 只校验聚合 `skipped` 计数，无法发现 timedemo 从 Skipped 翻转成别的状态（`run_tests.py` 不记录被跳过用例名） | 任务 9 增加步骤 2b：按名字查 `LastTest.log` 确认 `Timedemo.WarriorLevel1to2 (Skipped)` |

复核同时确认以下断言为真（不再展开）：merge-base/tip/28 提交；4 个冲突文件与冲突块行数、三处小冲突原文逐字一致；`quests.h` 149 CRLF vs 32 LF 与 markers 位置；`questdat.hpp` 的 `QuestData` 字段只缺 `scriptName`；`git checkout --theirs` 与 `git commit -F -` 在 merge 状态下的语义；`libdevilutionx`/`devilutionx` 目标名存在；heroname 上游 5 处仍未修；24 个交集文件中仅 `quests.h` 行尾不一致；`.editorconfig`/`.gitattributes`/`CMakePresets.json` 未被上游改动。

复核也**自我纠正**了两条早期误判（一度以为 `quests.cpp` 会与 `panels/quest_log.cpp` 重复定义而构成 blocker；实测该定义已被自动合并删除），这也说明本节的结论以复验命令为准。

## 自审

**1. 规格覆盖度（规格 §6 十项验收 → 本计划任务）**

| 规格验收项 | 对应任务 |
|---|---|
| 1 无残留冲突标记 | 任务 6 步骤 3 |
| 2 `HEAD..origin/master` 计数为 0 | 任务 6 步骤 4 |
| 3 构建通过 | 任务 8 |
| 4 全量测试零失败 | 任务 9 |
| 5 eval smoke exit 0 | 任务 10 步骤 1 |
| 6 漂移校验 5 项 PASS | 任务 10 步骤 2 |
| 7 行尾未破坏 | 任务 10 步骤 3 |
| 8 heroname 状态有结论并留档 | 任务 11 |
| 9 timedemo quarantine 状态 | 任务 9 步骤 2（全量 ctest 输出中确认 `Timedemo.WarriorLevel1to2` 仍为 `Skipped`；若变化，记录原因） |
| 10 CI 绿 | 任务 12 步骤 2 |

**2. 占位符扫描**

本计划不含 TBD/TODO/「后续补充」。任务 7 经对抗性复核后从「处理预期编译连带」改为**「验证零连带」**——原写法让执行者去修一个 git 自动合并已经解决的问题（假问题），属于会误导执行者的缺陷；现版本给出机器可判的计数判据（12 个文件各 1 个 include、`quests.cpp` 引用计数 0），仅在编译真的报错时才进入现场诊断分支。任务 8 步骤 3、任务 9 步骤 3 是条件分支，各自写明触发条件、命令与提交信息。

**3. 类型与命名一致性**

- `QuestData::scriptName`（任务 3 定义）与 `Source/quests.cpp:286-287` 的 `questData.scriptName` 一致。
- `panels/quest_log.hpp`（任务 7）与上游 `Source/panels/quest_log.hpp:9-12` 的声明一致。
- 冲突文件的路径与 4 处探测结果逐字一致。

**4. 规格缺口（本计划新发现，需回写规格）**

规格 §4 的风险表未预料「行尾差异导致整文件冲突」这一项，§6 也未给出对应的判据。执行时应把以下内容回写到规格（任务 12 步骤 3 一并处理）：

- 事实：`.gitattributes` 为 `* -text`，git 不做行尾归一化；上游把 `Source/quests.h` 重写为 LF，而 fork 侧是 CRLF（24 个交集文件中**唯一**一个行尾不同的）。
- 判据：被上游重写为 LF 的文件**保持 LF**——门禁 C 以新的 merge-base（= 上游 tip）为基线比对，改成 CRLF 会 FAIL，且会保证以后每次同步都整文件冲突。