---
name: IsTileLit 三用途耦合
description: 选择(cursor/track)、渲染(scrollrt)、自动地图(Explored) 共用；改它破坏三者
type: gotcha
created: 2026-08-08
sources: [Source/levels/gendung.h, Source/engine/render/scrollrt.cpp, Source/automap.cpp]
---

`IsTileLit`（`gendung.h:253`）被三处独立使用：
- **目标选择**：cursor.cpp:89/:315、track.cpp:43/:55、plrctrls.cpp:263/:403（悬停/点击/失效/手柄）
- **渲染**：scrollrt.cpp:469/:485/:749（暗/TRN 绘制分支）
- **自动地图**：经 `DungeonFlag::Explored`（lighting.cpp:104），不直接用 IsTileLit

若让 Infravision 时 `IsTileLit` 返回 true，则渲染全亮（TRN 美学破坏）+ 自动地图全揭示。

**为什么：** 一个「tile 是否点亮」的谓词被选择、渲染、地图三个子系统共享，改动影响面是三者交集。

**何时使用：** 任何「让暗处可选」的需求。修复必须新增独立函数（`CanTarget`），只替换选择路径的 6 个门控点，渲染/自动地图路径零改动。
