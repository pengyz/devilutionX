---
name: 存档/网络 bId 位覆写
description: 堆叠数存 bId 位域曾覆写物品品质与鉴定标志
type: gotcha
created: 2026-08-08
sources: [docs/superpowers/specs/2026-07-27-better-d1-design-charter.md, Source/pack.cpp, Source/msg.cpp]
---

两处 P0 数据损坏（2026-07-27 修复）：
- `pack.cpp` 把堆叠数写进 `bId` 位域，覆写物品品质
- `msg.cpp` 的 `TItem` 网络路径同样把堆叠数存 `bId`，致联机物品全部已鉴定

**为什么：** `bId` 位域有既定含义（品质/鉴定标志），堆叠数硬塞进去覆盖了它。网络路径与存档路径是两套编码，只修一处不够。

**何时使用：** 往现有位域/结构体加字段时，先查该字段是否已被使用；存档+网络两套序列化都要检查（改一处不修另一处 = 只治好一半）。修复：堆叠数存 `count-1` 保持与上游存档逐字节兼容。
