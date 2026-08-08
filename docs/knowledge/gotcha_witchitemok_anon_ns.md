---
name: WitchItemOk 在匿名命名空间
description: items.cpp:2009 位于匿名 ns，测试不可直接调；测行为结果而非函数本身
type: gotcha
created: 2026-08-08
sources: [Source/items.cpp]
---

`WitchItemOk`（`items.cpp:2009`）位于匿名命名空间（第一个 ns 块 179-2536 内），`nm` 显示 `GLOBAL__N_1` 前缀。测试无法直接调用它；`RndVendorItem<WitchItemOk>` 模板实例化同样在匿名 ns。

**为什么：** 这是内部实现细节；导出它会扩大 API 面且破坏 `RndVendorItem` 模板一致性。

**何时使用：** 测试女巫库存行为时，改测**公开路径**：调用 `SpawnWitch(lvl)`（`items.h:566`）后检查 `WitchItems`（`stores.h:94`，`DVL_API_FOR_TEST` 导出）向量。注意 `SpawnWitch(1)` 因 `iMinMLvl` 过滤可能不含高等级卷轴——用 lvl=10 让 Infravision 卷轴（iMinMLvl 8）可出现。
