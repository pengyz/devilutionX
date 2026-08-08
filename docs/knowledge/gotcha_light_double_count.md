---
name: 光照双重计数 bug
description: 撤销前 10*mult + (_pLightRad-10) 装备加成叠加两次；正确是倍率作用于总量
type: gotcha
created: 2026-08-08
sources: [docs/superpowers/specs/2026-07-27-better-d1-design-charter.md, Source/items.cpp]
---

失败先例 `GetEffectiveLightRadius` 用 `10*mult + (_pLightRad-10)`：装备加成在倍率后叠加。诅咒装备使 `_pLightRad < 10` 时 `_pLightRad-10` 为负，压制被叠加两次。

**为什么：** 倍率只作用于基础 10 而非总量；`_pLightRad` 已含装备加成，再减 10 是重复计数。

**何时使用：** 计算光照半径/任何「基础值 × 倍率 + 修正」公式时。正确写法：`effective = round(_pLightRad * mult)`（倍率作用于含装备的总量），再 clamp。
