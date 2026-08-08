---
name: SaveItem 追加/LoadItem 条件读不对称
description: 无条件追加 _iStackCount 而主存档路径从不设标志 → 偏移累积越界写
type: gotcha
created: 2026-08-08
sources: [docs/superpowers/specs/2026-07-27-better-d1-design-charter.md, Source/pack.cpp, Source/loadsave.cpp]
---

`SaveItem` 无条件追加 `_iStackCount` 到存档，`LoadItem` 只在标志为真时读它，而主存档路径从不设置该标志。每次存档多写 1 字节、读档不读 → 偏移累积，最终 `LoadDroppedItems` 越界写。

**为什么：** 写端与读端对「是否存在堆叠字段」的判定不对称（写无条件、读有条件）。

**何时使用：** 任何序列化格式，写读必须对称——同样的条件判定、同样的字段存在性。修复：堆叠数存 `_iMinDex` 后的对齐填充字节，记录尺寸回到上游 368/372，双向兼容且无需版本判别位。
