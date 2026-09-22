---
title: 改了 txtdata 源表却"没有任何效果"——先重建数据目标
type: gotcha
tags: [build, txtdata, mpq, tests, assets]
date: 2026-09-18
---

# 症状
修改 `assets/txtdata/**` 或 `mods/hf/txtdata/**` 后定向构建某个测试（`cmake --build build --target X_test`）并运行：测试**照旧通过**、打印出来的实测值**逐字不变**，仿佛改动没发生 ✗。

# 根因（三处叠加）
1. 测试经 `LoadLevelRoster()` / `LoadMonsterData()` 等**生产加载路径**读数据 ⇒ 读的是 **`build/` 侧的解包副本 / `*.mpq`**，**不是**受版本控制的 `assets/` 源表 ✗；
2. 定向构建测试目标**不会**刷新数据资产 ⇒ 必须显式 `ninja -C build devilutionx_mpq`（输出形如 `Copying assets/...` + `Building devilutionx.mpq` ✓）；
3. ~~更隐蔽的一层：夹具覆盖件必须放在 `build/test/fixtures/**`~~ **（已于 2026-09-18 修复 ✓）**：`PrefPath`/`OverridePaths` 在 **`TestInitGame()` 内一次性快照**（`Source/engine/assets.cpp:746` ✓）⇒ 之后 `SetPrefPath` 不生效 ✓；**修法**＝把测试里的 `SetPrefPath/SetAssetsPath` 目标由 `BasePath() + "test/fixtures/"`（实为 build 侧 ✗）改为 **`BasePath() + "../test/fixtures/"`**（源树 ✓）—— **12 处**，含共享落点 `test/drlg_test.hpp` ✓。**验证** ✓：只在**源树**放 L9=5 覆盖件 ⇒ 比率守卫红（`measured 0.0446, baseline 0.3466` ✓）。

# 识别信号（今天三次踩坑的共同指纹）
- 改了数据 ⇒ 断言**仍然通过**、实测值与基线**逐位相同** ✗；
- 或 `Unable to load data from file <相对路径>` ✓（路径到了但**不在**加载器搜索的根下 ✓）。

# 正确做法
```bash
# 1) 改源表
# 2) 重建数据目标（关键一步）
ninja -C build devilutionx_mpq
# 3) 再跑测试；若断言钉了实测值，先看它是否变红并打印新值 ✓
```
- 夹具类反证：把覆盖件放进 **`build/test/fixtures/**`**（不是 `test/fixtures/**` ✓），或用 `SetPrefPath(paths::BasePath() + "../test/fixtures/")` 并确保它发生在 **`TestInitGame()` 之前** ✓。

# 关联
- 今天的三次同源踩坑：L16 参数行 ✗、逐字节不变量守卫 ✗、任务 6 调参 ✗（均由此教训收口 ✓）。
