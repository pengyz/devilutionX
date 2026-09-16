---
name: 抬高逐层精灵预算会曝光 drlg_l2 的既存越界（max_image 与固定地图生成的关系）
description: 把某层的 max_image 从 4000 抬高后，全量日志里出现 1 条 drlg_l2.cpp:2072 的 UBSan 越界（FixTilesPatterns 一带未加边界的 dungeon[i][j+1]/dungeon[i+1][j]）。属既存缺陷被曝光而非引入；抬预算前应先知道这条。
type: gotcha
created: 2026-09-15
sources:
  - Source/levels/drlg_l2.cpp:2060-2075（未加边界的 j+1 / i+1 访问；复核者 UBSan 报点 :2072）
  - docs/superpowers/specs/2026-09-15-level-rosters-design.md（附录 E 第 6 条）
  - .superpowers/sdd/2026-09-15-level-rosters-phase-a/final-review.md（全量日志 1 条 UBSan）
---

**事实**：逐层名册把 L8 的 `max_image` 从 4000 抬到 9000 后，全量门禁日志里出现 **1 条** UBSan 越界：`Source/levels/drlg_l2.cpp:2072`（`FixTilesPatterns` 一带）。该处对 `dungeon[i][j + 1]` 与 `dungeon[i + 1][j]` 的访问**没有边界保护**，在 `j + 1 == DMAXY`（或 `i + 1 == DMAXX`）时越界。最后一次触碰该文件是 `53b91fd7a`，与本功能无关——**是被曝光，不是被引入**。

**为什么**：抬高 `max_image` 会让更多怪物类型进池、进而改变关卡生成期消耗的 RNG 序列与主题/房间选择路径，使原本很少走到的边界分支被走到。因此"抬预算"不只是内存/种类问题，也会**改变生成器被覆盖的分支**。

**何时使用**：任何抬高 `max_image` / 增加层内类型数的改动，在跑全量门禁时若看到 `drlg_*` 的 UBSan/越界告警，先判断是不是**既有缺陷被曝光**（看 `git log -1 -- <file>` 的最后改动是否早于本次），而不是急着回退数据；真正的修法是给该访问加边界（另一个独立工单）。
