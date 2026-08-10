---
name: 怪物等级对战斗压力无贡献（GetMinHit 钳制）
description: 战斗数学证明 D1 怪物等级不驱动实际压力——`hit = 2×(怪级−玩家级) + 30 − AC` 被 `GetMinHit()` 钳制，地狱层任何 AC≥16 的角色悬崖项完全无效。降怪级是错误杠杆（只影响掉落门槛/XP/被钳制的命中）。设计修改怪物难度应从 HP/伤害列或行为入手，而非等级。
type: gotcha
created: 2026-08-10
sources:
  - Source/monster.cpp（MonsterAttackEnemy 命中公式 + GetMinHit）
  - Source/player.cpp（玩家打怪命中公式）
  - docs/superpowers/specs/2026-08-10-hell-recombination-design.md（E1，REJECT 后重构为悬崖评估）
---

# 怪物等级对战斗压力无贡献（GetMinHit 钳制）

**事实**：D1 战斗数学中，怪物 `level` 字段几乎不贡献实际压力：

- **怪物打玩家命中**（`monster.cpp:1191-1195`）：`hit = 2×(怪级−玩家级) + 30 − AC`，然后 `hit = max(hit, GetMinHit())`——地狱层 `GetMinHit()` 是 15/20/25/30（层 13-16，`monster.cpp:1160-1172`）。**任何 AC ≥ 16 的角色都被钳在地板**，悬崖项（2×等级差）完全无效。
- **玩家打怪物命中**（`player.cpp:548`）：`GetMeleeToHit() − CalculateArmorPierce(monster.armorClass)`——**根本不出现怪级**，只出现固定 `armorClass`。
- **怪物 HP/伤害**：固定列（`monster.cpp:208/:1218`），非等级派生。Blood Knight 保持 200 HP / 25-35 伤害，无论 level 是 30 还是 28。

**为什么（根因）**：D1 的难度来自怪物 HP/伤害固定列 + 行为多样性，不是等级。等级只影响：掉落门槛（`RndItemForMonsterLevel`）、XP（`AddPlrMonstExper`）、被钳制的 2-4% 命中。

**何时使用**：
- 设计「降低地狱难度」类改动时——**不要改 `level` 字段**（降 1-2 级只影响被钳制的命中和掉落/XP，且会收缩 25-27 级 unique 掉落）。改 HP/伤害列或行为复杂度。
- 读设计文档声称「数值悬崖 8-11 级」时——先验证玩家等级模型（XP 曲线显示玩家到地狱约 18-24 级，悬崖仅 3-6 级且被钳制）。

**关联**：E1「地狱悬崖评估」（`2026-08-10-hell-recombination-design.md`，可定稿）——Oracle 首评凭此证明「降级是错误杠杆」而 REJECT 原「地狱重组」。
