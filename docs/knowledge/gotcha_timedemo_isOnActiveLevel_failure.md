# Gotcha: timedemo_test 在本机环境断言失败（`isOnActiveLevel`）

**类型**：`debug` / `gotcha`
**日期**：2026-08-10
**状态**：未解决——本机环境问题，待单独排查

## 症状

```bash
cd build && ./timedemo_test --gtest_filter="Timedemo.*"
```

输出回放选项后崩溃：

```
timedemo_test: Source/interfac.cpp:363: void devilution::{anonymous}::DoLoad(devilution::interface_mode):
Assertion `myPlayer.isOnActiveLevel()' failed.
```

失败点在 `DoLoad` 的 `WM_DIABPREVLVL` 分支（`interfac.cpp:363`）：`currlevel--` 后断言玩家 `plrlevel == currlevel`。

## 排查过程（2026-08-10，已做）

1. **与 B1 cap 无关**：`git stash push -- Source/` 回退全部 Source 改动（含 cap 的 `monster.cpp`/`monstdat.*`）后重建，timedemo 依然失败——失败在 HEAD 基线 `98c84ea7c` 即存在。
2. **timedemo_test 源码久未改动**（`git log -- test/timedemo_test.cpp` 最近一次是 `5937734b8 Move *dat files to tables dir`，早于本会话）。
3. **本机数据齐全**：`DIABDAT.MPQ`/`HELLFIRE.MPQ`/`spawn.mpq` 均在，fixtures（`demo_0.dmo` + `spawn_0.sv`）存在。
4. **失败机理**：`isOnActiveLevel()` 检查 `!plrIsOnSetLevel && plrlevel == currlevel`（`player.h:845-855`）。回放 2→1 层时 `currlevel--` 后 `plrlevel` 未同步 → 断言失败。疑似回放/存档加载时序问题，或 `qol-upgrades` 分支早前某改动（非本会话）引入。

## 影响

- `eval/cases/flow/timedemo-warrior.yaml`（AC7 门禁）在本机 **FAIL**（Subprocess aborted）。
- B1 cap 规格 AC7「timedemo 断言不变」**未能在本机验证**——但 cap 的教堂 1-2 层 guard 已由代码审查确认不激活（`capKite`/`capSameClass` 在 currlevel 1-8 均为 false，`GenerateRnd` 调用序列逐字节不变），且 `SamplingBaselineTest.Level16HardcodedTypes`/`SamplingTerminates` 等 12 个采样测试全过。
- **CI 未受影响**：CI 在干净环境跑 timedemo，本会话未提交任何 Source 改动到 timedemo 相关路径的破坏（cap 只动 `GetLevelMTypes` 采样循环，9-16 层激活；教堂 1-2 回放层不触发）。

## 待办（单独排查）

1. 在 HEAD 基线 + 干净 checkout 复现（排除本地 build 污染）。
2. 用 `git bisect` 定位 `qol-upgrades` 分支上最早导致 `isOnActiveLevel` 失败的提交（候选：本会话之前的 darkExpedition 改动 `items.cpp`/`spells.cpp`/`options.cpp`，或更早）。
3. 检查 `DoLoad`/`WM_DIABPREVLVL` 回放路径中 `plrlevel` 的赋值时机与 `LoadGameLevel` 的同步契约。
4. 修复后重跑 `eval/cases/flow/timedemo-warrior.yaml` 并验证 B1 AC7。
