---
name: 在测试里绕过 LoadGameLevel 必须补齐关卡状态（SOLData / tile 元数据 / trigs）
description: 直接调用 CreateDungeon 建关而不走 LoadGameLevel 时，SOLData、pMegaTiles/.til、trigs 都处于未初始化状态；后果是 InitMonsters 的放置规模恒定撞上限（MaxMonsters-10=190），并触发 lighting.cpp 的 UBSan 越界。含忠实复刻的调用顺序与症状指纹。
type: gotcha
created: 2026-09-15
sources:
  - Source/diablo.cpp:3419-3473（LoadGameLevel 的真实执行顺序）
  - Source/monster.cpp:3751-3763（na / numplacemonsters / MaxMonsters-10 截断）
  - Source/levels/dun_tile_data.hpp:205-208（TileHasAny → SOLData[dPiece]）
  - test/level_roster_baseline_test.cpp（修复后的忠实夹具）
---

**事实**：测试若绕过 `LoadGameLevel` 自行建关（`CreateDungeon(seed, entry)`），必须补齐引擎在该路径上建立的三样状态：

1. **`SOLData`**（tile 固体性）——调 `LoadLevelSOLData()`。否则 `IsTileSolid` 恒 false → `na = 80×80 = 6400` → `numplacemonsters = na/30 = 213` → 被 `MaxMonsters - 10 = 190` **恒定截断**。
2. **tile 元数据**（`pMegaTiles` / `.til`，见 `Source/diablo.cpp:1408-1427` 的路径表）——否则 `dPiece[][]` 退化；此时**只补 SOL 也没用**（放置数会变成 0）。
3. **触发器**（`numtrigs`/`trigs`）——由 `CreateLevel` 按 `leveltype` 分派 `InitL1..L4/Hive/CryptTriggers`（`Source/diablo.cpp:1461-1490`）。否则 `InitMonsters` 里的 `position + {-2,-2}` 从 0 回绕，触发 `Source/lighting.cpp:99` 的 UBSan 越界。

**忠实复刻的顺序**（引擎真实顺序，不是行号顺序）：`LoadLevelSOLData()`（`diablo.cpp:3433`）→ `InitLevelMonsters()`（`:3463`）→ `LoadGameLevelStandardLevel()`（`:3472`，内含 `CreateLevel` → `GetLevelMTypes` → `LoadGameLevelDungeon` → `InitMonsters`）。让 `CreateLevel` 接受任意种子的办法是 `DungeonSeeds[currlevel] = seed`（`Source/levels/dun_tile_data.hpp:140`）。

**为什么**：`LoadGameLevelStandardLevel`（内含 `CreateLevel`）的**定义**在 `diablo.cpp:3336`，看起来早于 `:3433`，但它的**调用**在 `:3472`——**定义位置 ≠ 执行顺序**，只看行号会把顺序判反。

**何时使用**：任何在测试里自行建关的夹具（`drlg_*_test`、采样/放置类测试）。**症状指纹：所有层的放置数完全相同**（例如全部 `placed = seeds × 190`）——真实关卡因可行走面积不同，逐层放置数必然不同。
