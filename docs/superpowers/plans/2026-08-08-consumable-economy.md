# 消耗品经济重构实施计划

**日期**：2026-08-08
**状态**：草案（随规格 `2026-08-08-consumable-economy-design.md`）
**取代**：无

---

## 0. 文档关系

本计划执行 `specs/2026-08-08-consumable-economy-design.md`。规格为唯一裁决基准；本计划与规格冲突时以规格为准。所有数值出处见规格第 3 节。

---

## 1. 执行顺序与并行波次

```
Wave 0（立即，相互独立）:
├── Task 1: 符文掉落排除（drop-filter 回调，Depth 门控）[quick]
└── Task 2: 伤害卷轴掉落排除（drop-filter 回调，Depth 门控）[quick]

Wave 1（依赖 Wave 0 的模式）:
├── Task 3: 卷轴单张消耗修复（ConsumeScroll _iStackCount--）[quick]
└── Task 4: 腰带不堆叠（AutoPlaceItemInBelt/ChangeBeltItem 去堆叠）[deep]

Wave 2（依赖 Task 4 确定背包放置语义）:
└── Task 5: 背包堆叠（AutoPlaceItemInInventory TryStackInInventory）[deep]

Wave FINAL（门禁）:
├── Task 6: 测试更新（stale 腰带堆叠测试改背包堆叠断言）[deep]
├── Task 7: timedemo 基线重生成（Base 改动影响）[deep]
├── Task 8: 全量门禁 + 漂移 + eval [quick]
└── Task 9: 独立复核（Oracle）[oracle]
```

**关键路径**：Task 4 → Task 5 → Task 7 → Task 8 → Task 9
**并行加速**：Wave 0 可并行；Wave 1 可并行。

---

## 2. 文件改动清单

| 文件 | 改动 | 归属 |
|---|---|---|
| `Source/items.cpp` | 掉落过滤回调加符文/伤害卷轴排除；`ConsumeScroll` 卷轴单张消耗；背包放置堆叠逻辑 | Task 1、2、3、5 |
| `Source/items.h` | （如需）堆叠相关辅助函数声明 | Task 5 |
| `Source/inv.cpp` | `AutoPlaceItemInBelt`/`ChangeBeltItem` 去堆叠；`AutoPlaceItemInInventory` 加 `TryStackInInventory` | Task 4、5 |
| `Source/player.cpp` | `RemoveInvItem`/`RemoveSpdBarItem` 调用点适配（如需要） | Task 3 |
| `test/consumable_stack_test.cpp` | stale 腰带堆叠测试改背包堆叠断言；新增卷轴单张消耗测试 | Task 6 |
| `test/` 新测试文件 | 掉落排除、背包堆叠、腰带 8×1 定向测试 | Task 6 |
| `CMake/Tests.cmake` | 注册新测试 | Task 6 |
| timedemo 参考存档 | 重新生成（`test/fixtures/timedemo/`） | Task 7 |
| `eval/cases/` | 新增/更新 case | Task 8 |

**行尾约束（禁令 7）**：上述全部文件当前为 CRLF。任何编辑后须校验未引入 LF。漂移校验（Task 8）兜底。

---

## 3. Task 1：符文掉落排除（规格 §4.1）

**目标**：深度层开启时掉落池不含符文，关闭时照旧。

**机制**：在 `RndItemForMonsterLevel` 与 `RndAllItems` 的 isItemOkay 回调添加：

```cpp
// 符文（对照规格 §4.1，门控）
if (item.iMiscId > IMISC_RUNEFIRST && item.iMiscId < IMISC_RUNELAST && IsDarkExpedition())
    return false;
```

**注意**：
- **不改 `IsItemAvailable`**（规格 §4.1 已论证：会波及存档/网络校验，静默删除旧存档符文）
- `ItemData::iMiscId` 是正确成员（`itemdat.h:512`）；`IMISC_RUNEFIRST/LAST` 区间（`items.h:402` 同款 idiom）
- 非 HF 上游 `IsItemAvailable` 的符文排除（`items.cpp:2397`）保留不动（上游行为）

**验收**（规格 §6 #1/#2）：
- 测试：开关开时 `RndAllItems`/`RndItemForMonsterLevel` 掉落池不含符文
- 测试：`IsItemAvailable(161..165)` HF 返回 true（不 collateral）；旧存档符文不因开关删除

---

## 4. Task 2：伤害卷轴掉落排除（规格 §4.2）

**目标**：深度层开启时掉落池不含伤害卷轴（SCROLLT），保留 Resurrect，关闭时照旧。

**机制**：同 Task 1 回调添加：

```cpp
// 伤害卷轴（规格 §4.2；保留 Resurrect）
if (item.iMiscId == IMISC_SCROLLT && item.iSpell != SpellID::Resurrect && IsDarkExpedition())
    return false;
```

**验收**（规格 §6 #3/#4/#5）：
- 测试：开关开时掉落池不含 SCROLLT（除 Resurrect）；Resurrect 在池中；关时含伤害卷轴

---

## 5. Task 3：卷轴单张消耗（规格 §4.3 ⚠️）

**目标**：使用叠了 N 的卷轴只消耗 1 张，非整叠移除。

**机制**：`ConsumeScroll`（`inv.cpp:2070-2101`）三个移除路径都改为「先减后删」：

```cpp
// 每个移除路径（InvList 分支 :2084、SpdList 分支 :2091、fallback :2100）：
if (item._iStackCount > 1) {
    item._iStackCount--;
} else {
    // 原移除逻辑
}
```

**注意**：
- **不改 `RemoveInventoryOrBeltItem` 共享 helper 的语义**（`inv.h:348`，其他调用者会受影响）——decrement 逻辑内联在 `ConsumeScroll` 三个路径
- 与 `UseInvItem` 不冲突：它显式排除 scrolls/runes 的 decrement 分支（`inv.cpp:2264-2284`），无双减
- 此修复同时治好既有「腰带 3 张叠卷轴用 1 张全没」bug

**验收**（规格 §6 #10）：
- 测试：叠 3 卷轴用 1 → 堆叠变 2；叠 1 用 → 移除物品

---

## 6. Task 4：腰带不堆叠（规格 §4.4）

**目标**：腰带恢复 8 格 × 每格 1 件（原版红蓝配比）。

**机制**：
1. `AutoPlaceItemInBelt`（`inv.cpp:1339`）：移除堆叠分支（`:1346-1364` 的 `_iStackCount++` 逻辑），每格独立放置
2. `ChangeBeltItem`（`inv.cpp:493`）：移除堆叠分支（`:498-504`），改为正常放置/交换

**验收**（规格 §6 #7/#8）：
- 测试：同类型药水放置到腰带 → 每格独立；8 格填满后第 9 个拒绝

---

## 7. Task 5：背包堆叠（规格 §4.3）

**目标**：背包内同类型消耗品自动堆叠（药水 5/卷轴 3，上限沿用 `GetMaxStackCount`）。

**机制**：`AutoPlaceItemInInventory`（`inv.cpp:1434`）加堆叠入口：

```cpp
bool AutoPlaceItemInInventory(Player &player, const Item &item, bool sendNetworkMessage)
{
    if (item.canStack() && TryStackInInventory(player, item, sendNetworkMessage))
        return true;
    // ... 原有放置逻辑不变
}
```

**`TryStackInInventory`**：遍历 `InvList`，找同 `IDidx` + 未满堆叠（`_iStackCount < GetMaxStackCount`）物品，`_iStackCount++`，发网络消息（`NetSendCmdChInvItem`，msg.cpp:3263 会整体替换该格——无需协议变更），调 `CalcScrolls()`。

**注意**：
- 匹配键用 `IDidx`（既有模式 `inv.cpp:500/:1348`），非 `_iMiscId+_iSpell`
- 堆叠物品占 1 格，merge 不碰 `InvGrid`/`_pNumInv`
- `ReorganizeInventory`（`:1501`）调用 `AutoPlaceItemInInventory` → 整理时自动合并堆叠（期望行为）

**验收**（规格 §6 #6/#9）：
- 测试：同类型药水×N 放置 → 堆叠到同一格，`_iStackCount == min(N,5)`；超 5 开新格

---

## 8. Wave FINAL 门禁

### Task 6：测试更新
- stale 腰带堆叠测试（`BeltStackingLogicIdentifiesMatch`、`BeltStackingRespectsMaxCount`、`BeltStackingDifferentItemsDontStack`、`UsingItemDecrementsStack`、`UsingLastItemRemovesIt`、`FullStackingWorkflow` 等 ~7 个）改为背包堆叠断言
- 新增：掉落排除（符文/伤害卷轴/Resurrect 保留/开关切换）、背包堆叠、腰带 8×1、卷轴单张消耗测试
- 注册到 `CMake/Tests.cmake`（漂移 A 强制）

### Task 7：timedemo 基线重生成
- Base 改动（腰带 8×1 + 背包堆叠）改变 timedemo 流程腰带状态 → 参考存档重新生成（宪章决策 25 先例）
- 重新生成后确定性验证（两次运行一致）

### Task 8：全量门禁
- `python3 tools/run_tests.py --json /tmp/ci.json`：全部测试 + 漂移
- 通过标准：`ctest.passed_pct==100 && failed==0 && drift.drift_ok==true`
- `python3 -m tools.eval.backend --smoke` + 新增 eval cases

### Task 9：独立复核（Oracle）
- 复核对象：落地 diff 对照规格红线逐条
- 重点：红线 13（开关关=现状）、Base 部分无条件、卷轴消耗语义、存档兼容

---

## 9. 提交策略

| 提交 | 内容 |
|---|---|
| 1 | Task 1+2 掉落排除（Depth 门控） |
| 2 | Task 3 卷轴单张消耗（bug 修复，独立可提交） |
| 3 | Task 4 腰带不堆叠 + Task 5 背包堆叠 |
| 4 | Task 6 测试更新 |
| 5 | Task 7 timedemo 基线重生成 |
| 6 | 门禁修复与文档更新（规格状态 → 已实施） |

每个提交独立可编译、可测试。行尾合规在每个提交内自检。

---

## 10. 状态

**草案**。待规格评审通过后开始执行。Task 1-3 可先行（Wave 0/1 独立）。

**已知风险**：
- Task 5 背包堆叠与 `ReorganizeInventory` 的合并行为需验证（整理时堆叠重排是否期望）
- Task 7 timedemo 重生成：Base 改动影响存档，需确认重生成后确定性
- Task 3 卷轴消耗：`RemoveInventoryOrBeltItem` 共享 helper 不动，decrement 内联三路径
- Task 6 stale 测试：20 个 consumable_stack_test 全部手写 SpdList，0 个直接失败（Oracle 核实），但 ~7 个语义过时需改

---

## 11. 自审

| 检查项 | 状态 |
|---|---|
| 与规格一致性：全部机制在规格 §4 有对应节 | ✅（Task 1-5 ↔ 规格 4.1-4.5） |
| 与验收一致性：Task 验收映射规格 §6 | ✅（Task 各节标注验收编号） |
| Base/Depth 门控正确 | ✅（Task 1-2 Depth 门控；Task 3-5 Base 无条件） |
| 存档/网络校验无 collateral | ✅（Task 1 不改 IsItemAvailable） |
| 无占位测试 | ✅（每 Task 验收为可执行断言） |
| 行尾合规有门禁 | ✅（Task 8 + 每提交自检） |
| 并行波次无依赖冲突 | ✅（Wave 0/1 内部独立） |
