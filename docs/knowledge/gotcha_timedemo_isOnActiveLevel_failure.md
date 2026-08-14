# Gotcha: timedemo_test 在本机环境断言失败（`isOnActiveLevel`）

**类型**：`debug` / `gotcha`
**日期**：2026-08-10
**状态**：已定位元凶（2026-08-13）——`f474a64c0`（Dark Expedition 开关移除），测试数据需重新生成，修复待立项

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

## 定位（2026-08-13，worktree 二分）

用 worktree 在三个提交分别构建 `pack_test`/`writehero_test`/`timedemo_test`：

| 提交 | 内容 | timedemo / PackTest / Writehero |
|---|---|---|
| `dc10d9d1b` | Etherealize 删除前 | ✅ 全过 |
| `a9fc8d4b4` | Etherealize 删除 | ✅ 全过 |
| `f474a64c0` | **开关移除（决策 31）** | ❌ 全挂 |
| `341451f13`（B1）+ HEAD | B1 cap + docs | ❌ 全挂（同 f474a64c0） |

**结论**：4 个失败（PackTest ×2 / Writehero / Timedemo）全部由 `f474a64c0` 引入，**与 B1 无关**（B1 父提交即失败，B1 未新增任何回归）。此前「stash 回退」只回退了 B1 cap 的 Source 改动，未回退 f474a64c0，故误判为「HEAD 基线即存在」——实际 98c84ea7c 是 f474a64c0 的祖先，当时必过。

**机理**：开关移除使 `WitchItemOk` 无条件排除 Infravision 卷轴（`items.cpp:2038`）→ `RecreateWitchItem` 的 `RndVendorItem<WitchItemOk>` 过滤+RNG 重试循环的种子消费路径改变 → 同种子生成不同物品（War Staff→Book of Flame Wave）→ 硬编码测试数据与 golden SHA 全部过期。这是决策 31 的**预期后果**，非引擎 bug；修复 = 重新生成测试数据/参考，不是改引擎代码。

## Timedemo 独立机理（2026-08-13，实验验证）

Timedemo 的失败**不是**物品形变（回退 `WitchItemOk` 仍失败），而是 **`DarkExpeditionDropOk` 掉落排除的 RNG 消费偏移**（实验：把 `DarkExpeditionDropOk` 改为恒 true 后 `Timedemo.WarriorLevel1to2` PASS）。掉落排除的过滤重试循环改变 RNG 流 → 回放（纯输入驱动、无 RNG 检查点）游戏状态分叉 → 层级转换时机错位 → `DoLoad` 断言。spawn 数据下 `IMISC_SCROLLT` 伤害卷轴即触发（排除重试）。修复 = **重录 demo 夹具**（真人游玩，上游同样操作：`git log -- test/fixtures/timedemo/` 见 "Record a new demo"），非代码改动。

## 影响

- `eval/cases/flow/timedemo-warrior.yaml`（AC7 门禁）在本机 **FAIL**（Subprocess aborted）。
- B1 cap 规格 AC7「timedemo 断言不变」**未能在本机验证**——但 cap 的教堂 1-2 层 guard 已由代码审查确认不激活（`capKite`/`capSameClass` 在 currlevel 1-8 均为 false，`GenerateRnd` 调用序列逐字节不变），且 `SamplingBaselineTest.Level16HardcodedTypes`/`SamplingTerminates` 等 12 个采样测试全过。
- **CI 未受影响**：CI 在干净环境跑 timedemo，本会话未提交任何 Source 改动到 timedemo 相关路径的破坏（cap 只动 `GetLevelMTypes` 采样循环，9-16 层激活；教堂 1-2 回放层不触发）。

## 待办（修复立项）

1. **PackTest**：✅ 已修（2026-08-13，3 条 CF_WITCH 条目按新确定性输出重生成——War Staff→Book of Flame Wave、White Staff→Scroll of Teleport、Plentiful Staff→Scroll of Phasing）。
2. **Writehero**：✅ 已修（golden SHA 更新为 `59bc3968...`；字节级对比确认唯一差异 = witch 法杖槽位 `Soldier's Staff of Apocalypse`→`Potion of Full Rejuvenation`，同 seed 706028607）。
3. **Timedemo**：⛔ **待真人重录 demo 夹具**（`DarkExpeditionDropOk` RNG 偏移使旧输入对不上新布局）。重录流程参照上游：游玩记录 → `--record` + `--create-reference` 产出 `demo_0.dmo`/`demo_0_reference_spawn_0.sv`/`spawn_0.sv`。
4. **StoreTransaction**：✅ 已修（测试 `OpenVendor(WitchBuy)` 绕过顶层菜单导致 `CurrentItemIndex==0` → `StartWitchBuy` 被跳过 → `PreviousScrollPos=0` → idx 偏移选中错误物品；本地全量数据侥幸买得起掩盖了 bug，CI spawn 数据暴露。修复：OpenVendor 走真实 Talk→Buy 流程）。
5. 修复后重跑全量门禁 + `eval/cases/flow/timedemo-warrior.yaml` + save-load eval 并验证 B1 AC7。
6. 修复提交后把本 gotcha 状态改为已解决。
