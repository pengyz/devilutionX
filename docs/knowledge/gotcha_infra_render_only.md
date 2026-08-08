---
name: Infravision 是渲染级法术
description: _pInfraFlag 只设渲染标志不改 tile Lit 位；暗处怪物可见但不可选
type: gotcha
created: 2026-08-08
sources: [Source/missiles.cpp, Source/lighting.cpp, docs/superpowers/specs/2026-08-08-dark-expedition-design.md]
---

`ProcessInfravision`（`missiles.cpp:3854`）只设 `player._pInfraFlag = true`，不改 `dFlags` 的 `Lit` 位（`lighting.cpp:107`）。结果：Infravision 让暗处怪物可见（红色 TRN 渲染），但 `IsTileLit` 仍为 false → **不可选中、不可攻击**。对近战是「看到埋伏」（有价值），对远程是「看得见打不着」（最糟）。

**为什么：** `_pInfraFlag` 是渲染标志，`IsTileLit` 检查的是灯光数据（`dFlags & DungeonFlag::Lit`）——两条独立通路。

**何时使用：** 声称「Infravision 已实现可作反制」前先验证能否选目标。修复：新增独立函数 `CanTarget`（`IsTileLit || (InfraFlag && IsTileVisible)`），替换 6 个目标选择门控点（cursor.cpp:89/:315、track.cpp:43/:55、plrctrls.cpp:263/:403），**不改 `IsTileLit`**。
