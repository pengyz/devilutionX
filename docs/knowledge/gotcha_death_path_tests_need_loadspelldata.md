---
name: 测试走怪物死亡/掉落路径必须先 LoadSpellData — 否则 GetBookSpell 死循环 + 整型溢出
description: SpellsData 为空时 GenerateRnd(0)+1 使 rv==1 永不归零，环绕判据 s == SpellsData.size() 也永不命中，GetBookSpell 无限自增 s 直至 items.cpp:648 signed overflow；夹具须按 diablo.cpp 顺序先 LoadSpellData 再 LoadItemData
type: gotcha
created: 2026-09-17
sources:
  - Source/items.cpp（GetBookSpell，630-660）
  - Source/diablo.cpp:2809（LoadSpellData 在 LoadItemData 之前）
  - test/sampling_behavior_test.cpp（PrepareDeathPathPrerequisites）
---

在测试里驱动怪物**真实死亡路径**（`MonsterDeath` → 掉落）时，只 `LoadItemData()` 是不够的：掉落可以抽到**书**，`GetBookSpell()` 会读 `SpellsData`。若夹具没调 `LoadSpellData()`，`SpellsData` 为空，于是：

- `int rv = GenerateRnd(static_cast<int32_t>(SpellsData.size())) + 1;` → `GenerateRnd(0)` 返回 0 → `rv == 1`；
- 循环体只有 `sLevel != -1 && lvl >= sLevel` 成立时才 `rv--`，空表下永不成立；
- 环绕判据是 `if (static_cast<size_t>(s) == SpellsData.size()) s = 1;`，`size()==0` 而 `s` 从 1 起**单调递增**，永不相等。

结果是 `while (rv > 0)` 无限循环，`s++` 一路涨到 `INT_MAX`，UBSan 报
`signed integer overflow: 2147483647 + 1` at `Source/items.cpp:648`。**表现是测试挂住**（不是干净断言失败），很容易被误判成"这颗种子有问题"而去换种子绕开。

修法：按 `Source/diablo.cpp:2809` 的真实加载顺序，在 `LoadItemData()` **之前**补 `LoadSpellData()`（`#include "tables/spelldat.h"`）。

**为什么：** `SpellsData.size()` 同时充当 `GetBookSpell` 的 RNG 上限**和**环绕边界（见 `gotcha_spelldata_positional_index.md` 对 size 语义的要求），空表让这两个角色同时退化；游戏进程里两张表总是一起加载，所以生产代码从不校验，缺口只在测试夹具里暴露。

**何时使用：** ① 夹具要走死亡/掉落/`CreateItem` 一类路径时，把 spell 表和 item 表当成一组前置一起加载；② 定向测试**挂住**而非失败时，优先怀疑"某张表没加载导致的空表死循环"，别先怀疑种子——换种子只是把缺陷推给下一个人；③ 逐条 `--gtest_filter` 配 `timeout` 可定位是哪条挂住（注意别用 `pkill -f` 匹配自己的命令行，会杀掉调用方 shell，用 `pkill -x <binary>`）。
