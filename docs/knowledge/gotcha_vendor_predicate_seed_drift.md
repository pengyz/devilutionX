---
name: "供应商谓词改动导致种子物品生成漂移"
description: "改动 RndVendorItem 过滤谓词（如 WitchItemOk）会改变 RNG 重试循环的种子消费路径——同种子生成不同物品，硬编码 pack 测试数据与 golden 存档哈希全部过期"
type: "gotcha"
created: "2026-08-13"
sources:
  - "f474a64c0 (Dark Expedition 开关移除)"
  - "Source/items.cpp:2028 (WitchItemOk), 2056 (RndVendorItem)"
---

改动带 `CF_WITCH`（及其他 vendor 创建标志）物品的生成过滤谓词时，**同一种子会静默生成不同物品**：`RecreateWitchItem` 走 `RndVendorItem<WitchItemOk>`（items.cpp:2056），过滤失败的物品会消费 RNG 重试，谓词每多/少一个排除项，重试次数与 RNG 消费序列就变，最终物品随之改变。

**实例**：宪章决策 31 移除 Dark Expedition 开关后，`WitchItemOk` 无条件排除 Infravision 卷轴（items.cpp:2038）——`War Staff of haste`（idx 150, CF_WITCH）在同一 seed 下重生成变成 `Book of Flame Wave`（idx 111）。连锁破坏：`PackTest.UnPackItem_diablo/hellfire`（硬编码期望数组）、`Writehero.pfile_write_hero`（golden SHA）、`Timedemo.WarriorLevel1to2`（回放存档内物品/光照状态变化）、eval `save-load` 2 个用例。全部是**测试数据过期**而非引擎 bug——修复=重新生成测试数据，不是回滚行为。

**为什么：** vendor 物品生成 = 谓词过滤 + RNG 重试的确定性链；谓词是 RNG 路径的一部分，不是纯布尔门。改谓词=改 RNG 路径=改 seed→item 映射。引擎所有依赖该映射的硬编码期望（测试数组、golden 哈希、demo 回放）都会漂移，且**无编译期信号**。

**何时使用：** 修改任何 `*ItemOk`/`*VendorItem` 过滤谓词、物品可用性（`IsItemAvailable`）或掉落排除后，必须：1) 重跑 `pack_test`/`writehero_test`/`timedemo_test` 确认种子漂移；2) 用 worktree 在提交前后各建一次做二分（构建 3 个测试目标即可定位）；3) 按新确定性输出重新生成测试数据（勿手改数字，跑真实路径抓值）。
