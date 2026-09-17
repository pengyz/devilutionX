---
name: 小队为什么不买数值（MinionOptions 全 false）以及形成率为何用 leash 无关的 formed
description: 阶段 B 的两条决策性说明，原本只写在 monster.h/.cpp 的注释里：①核心小队传 MinionOptions{tough=false,inheritAi=false,inheritIntelligence=false}（只买构成不买数值/AI）；②形成率分子必须用 leash 无关的 SquadRollCounters::formed，否则采用规格允许的回退反而会让守卫变红。
type: decision
created: 2026-09-16
sources:
  - Source/monster.h（MinionOptions 与 PlaceGroup 声明、IsBuffedMinion）
  - Source/monster.cpp（PlaceGroup 的三处门控、setLeader 前存 ownAi、SquadRollCounters 的 formed/realised）
  - docs/superpowers/specs/2026-09-15-level-rosters-design.md §4.3.2 与 §6 验收 6/7
---

**决策 1：核心小队**只买构成，不买数值与 AI。
实现：`PlaceGroup(..., MinionOptions{tough=false, inheritAi=false, inheritIntelligence=false})`。三项默认 `true` **保持 unique 包原有行为**（HP×2、继承 leader 的 `ai`、继承 `intelligence`）。
为什么必须全 false：小队若继承 leader 的 AI，"远程随从挂到近战 leader 下变近战"会让**placed class mix 统计与事实不符**，红线 14（近战/远程分别评估）随之失效；若 `tough=true` 则等于**给全层加数值强度**，违反红线 9（非膨胀数值）。
实现细节：`inheritAi=false` 必须在 `setLeader` **之前**存下 `const MonsterAIID ownAi = minion.ai;`（`setLeader` 内部会 `ai = newLeader->ai`），之后恢复；`leashed=false` 时不调 `setLeader`，天然不覆写。

**决策 2：形成率分子用 `formed`，不用 `realised`。**
`realised` 的判定含 `squadLeashed && packSize > 0`，而 `PlaceGroup` **只在 leashed 时**写 `packSize` → 若用 `realised` 做分子，**任何按规格 §4.3.4 采用 `squad_leashed=0` 回退的层会恒读 0%** —— 即"**守卫因为它自己要求的补救被采用而变红**"（自相矛盾守卫）。`formed` 按 roll 计数、与 leash 无关，才是"掷骰是否真的成队"的正确度量。
推论：任何"守卫 + 规格允许的补救"组合，都要检查"补救被采用时该守卫是否仍绿"——这是比"能否变红"更隐蔽的一类缺陷（另见 `pattern_assertions_must_be_failable.md`）。

**决策 3：蓝名＝真的被强化，不是"有 leader"。**
`monhealthbar` 的置色判据改为 `IsBuffedMinion(minion)`（＝ leader 存在且 `leader->isUnique()`），因为阶段 B 之后**小队随从虽有 leader 却未被强化**；旧判据（`leader != NoLeader`）会让 UI 宣称与事实相反。**若将来出现非 unique 的强化路径，该谓词必须重评**（`monster.h` 注释已写明）。

**何时使用**：改动放置/小队/AI 继承/强化/血量条着色、或设计"功能性回退 + 守卫"时。
