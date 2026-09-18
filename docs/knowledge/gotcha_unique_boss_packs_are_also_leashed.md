---
name: unique boss pack 也会产生拴系随从（度量"小队"时必须用 isUnique 区分）
description: PlaceUniqueMonsters 在散布循环之前运行，且用默认 MinionOptions → unique boss pack 同样带 leashed 随从。任何"成队率/小队"度量若只看 leaderRelation==Leashed，会把 unique 队误计为小队。
type: gotcha
created: 2026-09-16
sources:
  - Source/monster.cpp（PlaceUniqueMonsters 在 InitMonsters 内先于散布循环；PlaceGroup 默认 MinionOptions 使 unique 随从 HP×2 且继承 AI）
  - docs/superpowers/ledgers/2026-09-15-level-rosters-phase-b/task-3-report.md 与 4807d157 评审（判别器设计）
---

**事实**：`PlaceUniqueMonsters`/`PrepareUniqueMonst` 走的是**默认** `MinionOptions{}`（`tough=true, inheritAi=true, inheritIntelligence=true`），因此 unique boss pack 的随从**也是 `LeaderRelation::Leashed`**。而它运行在散布循环**之前**。

**后果**：任何"小队密度/成队率"的度量或断言，若只筛 `leaderRelation == Leashed`，会把**unique 队**当成"核心小队"计入——在统计上表现为"小队比实际多"，并可能掩盖核心小队根本没形成。

**正确做法**：判别器要加 `leader->isUnique()` 排除（本仓实现即用此），并在测试里**额外断言 unique 随从数 > 0**，防止 A/B 因为"把全部 leashed 随从都归给 unique"而**静默空转**（本会话 Task 3 的 A/B 用例就是这么加固的）。

**何时使用**：写任何与"怪成群"相关的度量、守卫或调试断言时。
