---
title: 逐种子指标依赖全局 RNG 流——必须每次建关显式重置，否则"隔离态绿、全量红"
type: gotcha
tags: [tests, rng, determinism, gtest, ctest]
date: 2026-09-18
---

# 症状（很隐蔽 ✗）
新写的逐种子指标用例：
- **隔离跑**（`--gtest_filter='*X*'`）⇒ 绿 ✓，且打印值与钉值**逐位一致** ✓；
- **同一二进制全量跑**（`./build/X_test`）⇒ **红** ✗，实测值与钉值差出容差（本案 L2 0.54→0.46、L7 0.43→0.51 ✓）。

# 根因
`CreateDungeonForMeasurement(level, seed)` 只设置 `DungeonSeeds[currlevel]`，**并不重置全局 RNG 流** ✗ ⇒
怪物散布/名册采样继续消耗**前序用例剩下的** RNG 位置 ✓ ⇒ 同一种子在不同执行顺序下产出不同结果 ✗✓。

# 为什么本地门禁与 smoke 都没抓到（双重遮蔽 ✗✗）
1. **ctest 把每个 gtest 用例注册成独立的 ctest 测试** ✓ ⇒ 每个用例**各自进程内隔离**运行 ✓ ⇒ 全量门禁绿 ✓；
2. eval **smoke 的用例集合不含该 case** ✗ ⇒ 也不会暴露 ✓；
3. 真正暴露它的是 **eval case 的 filter 运行**（一个进程内跑 12 个用例 ✓）✗✓ —— 复核正是这样抓到的 ✓。

# 正确做法（照既有兄弟用例 ✓）
```cpp
InitLevelMonsters();
SetRndSeed(41000 + seed);        // Source/engine/random.hpp:304
CreateDungeonForMeasurement(level, seed);
// ... GetLevelMTypes() / InitMonsters()
```
本仓既有 `SquadPlacementTest.ShippedSquadChanceRealisesSquadsOnEveryLevel` 就是这么写的（`test/level_roster_baseline_test.cpp:2223-2224` ✓）⇒ **新写逐种子指标时要照抄这一步** ✓。

# 验证方式（必须两态都绿 ✓）
- 隔离：`./build/X_test --gtest_filter='*NewCase*'` ✓；
- **同进程全量**：`./build/X_test` ✓（**这一步不能省** ✗）；
- **eval filter 态**：`python3 tools/run_tests.py --test X_test --filter '<该 case 的 filter>'` ✓。
三态一致才算钉值稳定 ✓。

# 关联
- 素材/数据加载：`gotcha_data_changes_need_asset_rebuild.md` ✓；
- CI 与本地差异：`gotcha_ci_is_the_terminal_authority_check_after_every_push.md` ✓。
