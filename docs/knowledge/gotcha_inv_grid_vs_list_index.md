---
name: 网络物品更新要网格索引非 InvList 索引
description: NetSendCmdChInvItem 期望网格索引；传 InvList 索引会 OOB 读 + 多人损坏
type: gotcha
created: 2026-08-08
sources: [Source/inv.cpp, Source/msg.cpp]
---

背包堆叠实现时，`TryStackInInventory` 曾直接传 InvList 索引给 `NetSendCmdChInvItem(false, invIndex)`——但该函数期望**网格索引**（`msg.cpp:3263` 用 `InvGrid[invGridIndex]` 反查），导致 OOB 读 `InvList[-1]` + 远程库存损坏。

**为什么：** `InvList`（数组）和 `InvGrid`（网格占用图）是两套索引；网格索引 = 位置，InvList 索引 = 物品序号。`NetSendCmdChInvItem` 按网格索引工作。

**何时使用：** 任何需要同步背包物品到网络的代码。用 `NetSyncInvItem(player, invListIndex)`（`msg.cpp:3250`）——它从 InvList 索引反查网格索引再发消息。独立回归复核抓到。
