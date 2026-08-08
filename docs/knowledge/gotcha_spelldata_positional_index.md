---
name: SpellsData 位置索引 — 删行即错位（已修复）
description: LoadSpellData 曾按行序 emplace；现已改名称键控，删 TSV 行不再错位
type: gotcha
created: 2026-08-08
sources: [docs/superpowers/specs/2026-08-08-dark-expedition-design.md, Source/tables/spelldat.cpp]
---

`SpellsData` 是 `std::vector<SpellData>`。**历史问题**：`LoadSpellData` 曾按 TSV 行序 `emplace_back`，`GetSpellData(SpellID)` = `SpellsData[enum]`（`spelldat.h:271`），TSV 行序与枚举序 1:1——删除 DoomSerpents/BloodRitual/Invisibility 三行会使 Nova→Elemental 全部错位。独立复核发现此 P0，两个先前的 Oracle 都漏了。

**现状（2026-08-08 已修复）**：`LoadSpellData` 已改为**名称键控两阶段加载**（`spelldat.cpp`）：先解析每行 `id` 列 → `ParseSpellId` → SpellID，再按枚举索引写入，size = max-enum-present+1，缺失填不可学默认行。行序与枚举序已解耦，增删 TSV 行不再错位。

**为什么：** 位置索引假设「行序 == 枚举序」，删行打破该假设；`SpellsData.size()` 还被 `GetBookSpell`/`ValidatePlayer` 用作 RNG 上限与遍历边界（size 必须保持 max-enum-present+1 语义）。

**何时使用：** 增删 `assets/txtdata/spells/spelldat.tsv`（或 HF 版）的行时——无需手动重排（名称键控已处理），但要保持 size 语义（缺失行填 -1 不可学默认行，勿用 AddNullSpell 的 sBookLvl=0 profile）。
