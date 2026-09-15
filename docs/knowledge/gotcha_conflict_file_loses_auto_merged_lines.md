---
name: "手工解冲突的文件不会继承上游同一 hunk 里自动合并进来的新增行"
description: "上游重构把符号迁到新头文件并给每个消费者补 include，非冲突文件靠 git 自动三方合并带上新 include；但恰好落在本轮手工冲突文件集合里的消费者，人工解冲突时只处理冲突块内的取舍，不会把上游在同一 hunk 附近新增的 include/声明带进来——'无重叠即安全'的推断对冲突文件集合失效"
type: "gotcha"
created: "2026-09-15"
sources:
  - "91e805149 (fix(build): add panels/quest_log.hpp include after the upstream quest split)"
  - "8e92687a9 (merge commit：quests.cpp 修复后 amend 进此提交)"
  - "本轮上游同步第二轮（tip e00b7260f）"
---

## 症状

解完 merge 冲突后编译报「某个符号未声明」，而该符号的声明看起来"上游明明加了"。本轮实例：`Source/controls/plrctrls.cpp` 缺 `#include "panels/quest_log.hpp"`，导致 7 处编译错误（`QuestLogIsOpen` / `QuestlogUp` / `QuestlogDown` / `StartQuestlog` 等无声明），由 `91e805149` 修复。

## 根因

**手工解冲突的文件不会继承上游在同一 hunk 里自动合并进来的新增行。**

上游把 `QuestLogIsOpen` / `pQLogCel` / `DrawQuestLog` 等符号迁到新的 `Source/panels/quest_log.hpp`，并给每个消费者补了对应 include。这些 include 对**非冲突文件**是随 git 自动三方合并自然带入 fork 的——本项目在同步前的探测复验里验证过这条链路，结论是「12 个消费者文件都已带上 include」。

但 `plrctrls.cpp` 恰好同时落在两个集合的交集里：它既是 quest_log 重构的消费者，又是本次 merge 的 **4 个手工冲突文件之一**。它的冲突块只包含两行 include 的取舍（哪一侧的 include 列表生效），人工解决冲突时视线只落在冲突块本身，不会去看上游在**同一 hunk 附近、冲突块之外**新增的 `#include "panels/quest_log.hpp"`——那行对人工解冲突流程是不可见的。于是「改动不重叠即自动合并安全」这条对非冲突文件成立的推断，对**冲突文件集合**整体失效。

## 检查清单（下次同步照做）

1. **取交集**：「本轮手工冲突文件集合」∩「上游本轮新增 include / 新增声明的消费者集合」——交集非空时，逐个手工核对该文件的 include 列表是否完整补齐了上游新增的头文件引用，不能只信任冲突块内已经处理的两行。
2. **整文件三方核对**：解冲突后的自查不能只看冲突块内的几行，必须做**整文件三方核对**——`git merge-file -p ours base theirs` 的结果与最终文件逐字节比对。本轮 `Source/quests.cpp` 的 Critical 缺陷（首次解决时整块取了一侧，丢弃上游 341 行的函数迁移，导致与新文件重复定义符号）正是「只看冲突块」漏掉的典型案例；该文件由任务评审发现后才修好，修复直接 amend 进 merge commit `8e92687a9`。
3. **编译门禁是最后防线**：冲突文件集合里出现的编译错误（尤其「符号未声明」类），优先怀疑「漏带上游在同一 hunk 里新增的 include/声明」，而不是先怀疑自己代码逻辑写错。

## 为什么

Git 的自动三方合并按 hunk 生效：一个文件里，未冲突的 hunk 会自动应用两侧的改动叠加；冲突的 hunk 需要人工二选一或手改。当上游在**同一逻辑改动**（例如一次符号迁移）里既加了新增声明的 hunk（通常自动合并），又恰好触碰了另一个会冲突的 hunk（例如同一文件里旧的 include 列表被两侧都改过），人工解决冲突时的关注范围天然被框定在冲突标记 `<<<<<<<` / `=======` / `>>>>>>>` 之间，容易忽略"冲突块外、但同属这次上游改动"的其他新增行——尤其当它们看起来与冲突内容无关（一行 include vs 冲突块讨论的是别的行）。
