---
name: 整数百分比数学保确定性
description: lrad*pct/100 而非浮点乘（x86 扩展精度 10*0.6→5）；全平台截断一致
type: pattern
created: 2026-08-08
sources: [Source/items.cpp, docs/superpowers/specs/2026-08-08-dark-expedition-design.md]
---

计算「数值 × 比例」时用整数百分比数学 `lrad * pct / 100`，不用浮点乘法。

**为什么：** 32 位 x86 用 80-bit 扩展精度浮点，`10 * 0.6` 截断得 5 而非 6；`std::floor` 也救不了。整数除法 `10*60/100=6`、`10*85/100=8`（trunc 8.5）全平台一致。

**何时使用：** 任何需要确定性截断的比例计算（光照倍率、伤害缩放等）。倍率表存整数百分数（60/85/50），不用 `0.6f`。
