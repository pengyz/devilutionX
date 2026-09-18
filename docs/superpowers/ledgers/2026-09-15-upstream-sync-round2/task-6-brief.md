## Task 6: 解决 `Source/quests.cpp` 并提交 merge

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

