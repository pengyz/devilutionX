---
name: 多计数堆叠合并会静默丢物品
description: TryStackInInventory 曾 +1 合并，叠 3 放到叠 1 变 2；多计数应开新格
type: gotcha
created: 2026-08-08
sources: [Source/inv.cpp, test/inventory_ui_test.cpp]
---

背包堆叠实现时，`TryStackInInventory` 用 `invItem._iStackCount++` 合并——但传入物品可能是多计数堆叠（叠 3 放到叠 1），`+1` 合并后只剩 2，**静默丢失 2 个物品**。独立回归复核用运行时探针实证。

**为什么：** `AutoPlaceItemInInventory` 处理的是「一个物品」（可能是叠 N 的堆叠）。合并逻辑只加 1，忽略了传入物品自身的 `_iStackCount`。

**何时使用：** 实现堆叠合并时。正确语义：**多计数堆叠不合并**（返回 false 走正常放置开新格，保留全部计数），只有单件（count 1）才合并到现有堆叠。`AutoPlaceItemInInventory` 的调用方（拾取单件/拖拽整叠）都能正确处理。
