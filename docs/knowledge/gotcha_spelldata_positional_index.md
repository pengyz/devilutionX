---
name: SpellsData 位置索引 — 删行即错位
description: LoadSpellData 按行序 emplace、GetSpellData 按枚举索引；删 TSV 行会错位 Nova→Elemental
type: gotcha
created: 2026-08-08
sources: [docs/superpowers/specs/2026-08-08-dark-expedition-design.md, Source/tables/spelldat.cpp]
---

`SpellsData` 是 `std::vector<SpellData>`，`LoadSpellData`（`spelldat.cpp:233`）按 TSV 行序 `emplace_back`，而 `GetSpellData(SpellID)` = `SpellsData[enum]`（`spelldat.h:271`）。TSV 行序与枚举序 1:1。

直接删除 DoomSerpents/BloodRitual/Invisibility 三行会使 Nova→Elemental 全部数据错位，且枚举尾部越界。独立复核发现此 P0，两个先前的 Oracle 都漏了。

**为什么：** 位置索引假设「行序 == 枚举序」，删行打破该假设；`SpellsData.size()` 还被 `GetBookSpell`/`ValidatePlayer` 用作 RNG 上限与遍历边界。

**何时使用：** 增删 `assets/txtdata/spells/spelldat.tsv`（或 HF 版）的行时。修复：`LoadSpellData` 改为名称键控两阶段加载（先解析 `id` 列 → SpellID，再按枚举索引写入，size = max-enum-present+1，缺失填不可学默认行）。
