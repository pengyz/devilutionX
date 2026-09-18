## Task 7: 验证 quest_log 重构零连带（预期零改动）

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

