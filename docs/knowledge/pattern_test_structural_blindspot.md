---
name: 测试套件结构性盲区——UT 只测模板隔离不测真实路径
description: Oracle 回归复核用运行时探针抓到 3 个 UT 盲区 bug
type: pattern
created: 2026-08-08
sources: [test/consume_scroll_test.cpp, test/inventory_ui_test.cpp]
---

全量回归审查发现：测试套件对某些功能存在**结构性盲区**——UT 只测函数模板隔离（直接调模板/手工构造状态），不测**真实生产路径**（通过公开 API 触发完整流程）。

具体案例（消耗品经济回归复核）：
- `ConsumeOneOrRemove` 模板测了隔离行为，但 `ConsumeScroll` 的 **fallback 路径**（spellFrom=0 热键施放）没修——整叠消耗 bug 未被套件捕获
- `TryStackInInventory` 测了单件合并，但**多计数合并**（叠 3 放到叠 1）静默丢物品，无测试覆盖
- 网络路径（`sendNetworkMessage=true`）完全无测试

**为什么：** 单元测试倾向隔离验证（构造对象直接调函数），但真实 bug 常在「调用链整合」处（公共 API 触发 → 内部路径选择 → 副作用）。

**何时使用：** 新增功能时，测试应**通过公开 API 触发真实路径**（如 `ConsumeScroll` 设 `executedSpell.spellFrom=0`、`AutoPlaceItemInInventory` 传多计数物品），而非只测内部模板。独立 Oracle 复核用**运行时探针**（临时 fprintf 编译进真实二进制）验证调用真实触发——比纯测试可靠。
