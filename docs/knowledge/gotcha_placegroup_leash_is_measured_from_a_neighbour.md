---
name: PlaceGroup 的 4 格拴系是从 leader 的邻格量起（真实边界 >4，不是 >=4）
description: PlaceGroup 在 leader 分支里先取 leader 位置 + 随机方向作为初始点，再把该点当作 x1/y1 做 |xp-x1|<4 && |yp-y1|<4 的钳制 → 随从与 leader 的距离上限实际大于 4，不能用"距 leader <=4"来断言或推理。
type: gotcha
created: 2026-09-16
sources:
  - Source/monster.cpp（PlaceGroup 的 leader 分支：先 position = leader->position.tile + Direction(offset)，再 x1=xp/y1=yp 作为钳制中心）
  - .superpowers/sdd/2026-09-15-level-rosters-phase-b/task-3-report.md（实现者实测发现）
---

**事实**：`PlaceGroup` 的拴系约束写作 `leashed && (std::abs(xp - x1) >= 4 || std::abs(yp - y1) >= 4) → 放弃该候选`，但 **`x1/y1` 不是 leader 的位置**——leader 分支先算 `position = leader->position.tile + Direction(GenerateRnd(8))` 作为初始点，再用它当钳制中心。也就是说钳制是**相对 leader 的一个邻格**。

**后果**：随从与 **leader** 的距离上限实际 **>4**（邻格偏移 1 + 钳制半径 ~4）。任何"随从应在 leader 4 格内"的断言、图示或平衡推理都会偏差；反过来，写"必须 >4 才算越界"的说法也要小心——准确表述是"钳制相对 leader 的邻格"。

**正确做法**：需要断言邻近性时，直接对**引擎实际使用的中心**（该邻格）比较，或断言一个**上界**（如"与 leader 的距离 ≤5"这类可证的上界），而不是照抄 4 这个数字。

**何时使用**：写拴系/小队邻近性守卫、或解释"为什么随从看起来离 leader 超过 4 格"时。
