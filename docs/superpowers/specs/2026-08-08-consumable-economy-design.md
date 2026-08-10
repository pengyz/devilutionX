# 消耗品经济重构（Consumable Economy）设计

**日期**：2026-08-08
**状态**：已实施（2026-08-08，两轮 Oracle 复核 + 全量门禁通过）
**分类**：Base + Expansion（背包堆叠/腰带 = Base 无条件；掉落排除 = Expansion 无条件——2026-08-10 定位变更前写作「Depth 门控」，变更后开关移除、掉落排除无条件生效，见宪章决策 31）
**取代**：无（新规格；落实宪章 §6 裁决）

> **2026-08-10 定位变更补注（宪章决策 31）**：本文档「掉落排除随 `darkExpedition` 门控」的表述已失效。定位变更后 `darkExpedition` 选项从代码删除，`IsDarkExpedition()` 恒真，掉落排除无条件生效。§4.6 红线 13 相关段落（开关关闭回退）作废。

---

## 1. 问题陈述

宪章已裁决以下消耗品经济问题，本规格落实裁决而非重新推导：

1. **符文掉落**：`Rune of Fire/Lightning/Stone` + 2 个 `Greater Rune`（`AllItemsList` 索引 161-165，`assets/txtdata/items/itemdat.tsv:163-167`）。宪章决策 6 判定「错误设计」：去掉后玩家没少取舍。**非 Hellfire 已在 `IsItemAvailable` 排除（`Source/items.cpp:2397` `(i < 161 || i > 165) // Runes`），但 Hellfire 模式 `IsItemAvailable` 直接 `return true`（`Source/items.cpp:2386`）→ HF 下符文仍在掉落池**。
2. **伤害卷轴掉落**：SCROLLT 类卷轴（Fire Wall/Inferno/Flash/Fireball/Stone Curse/Chain Lightning/Flame Wave/Guardian/Golem，dropRate=1，`itemdat.tsv` 多个条目）。宪章决策 6：几乎无人用，去掉后没少取舍。**当前在掉落池（`GetItemIndexForDroppableItem` 按 dropRate 加权，`Source/items.cpp:1352`）**。
3. **堆叠形态**：宪章 §6 裁决「**改为背包堆叠、腰带不堆叠**」（`2026-07-27-better-d1-design-charter.md:378`）。当前为「腰带 8 格 × 每格 5 件」（`GetMaxStackCount` 药水 5/卷轴 3，`Source/items.cpp:5170`），是 **Base 层唯一已知平衡偏差**（宪章 §9）。背包堆叠使 `_iStackCount` 基础设施从腰带搬到背包，而非删除（决策 26 的担忧不成立）。

**本规格要解决**：落实宪章三项裁决（取消符文+伤害卷轴掉落、背包堆叠/腰带不堆叠、零补偿），使 Base 层「不改变任何平衡」的承诺变为真。

---

## 2. 分类判定

按宪章第 2 节判定树逐步判定：

**第一步——改动是否触及数值/掉落/商店/战斗资源？**

- 掉落表构成（符文/伤害卷轴）：**是**（规则 2）→ **Depth**
- 背包堆叠：**否**（背包内消耗品战斗外，非规则 6「战斗中可即时使用」）→ **Base**（宪章:152 B2 范例「堆叠背包是消除摩擦」；宪章:390 明言背包堆叠不违反 B2）
- 腰带不堆叠：**否**（恢复原版 8 格 = 战斗内即时资源回原版，属 Base 层「不改变平衡」承诺的修复；宪章 §6 先例：金币+25%、光照压制均无条件撤销）

**归类结论**：
- **Base（无条件）**：背包堆叠 + 腰带不堆叠——宪章 §6 已裁决的形态修复，恢复 Base 层「平衡 = 原版」承诺
- **Depth（开关门控）**：符文 + 伤害卷轴掉落排除——宪章决策 6，属规则 2 掉落表构成

**分类依据（独立复核确认）**：
- 红线 13 自洽：Base 无条件 → 开关关闭 = 原版自然成立（原版腰带即 8×1、背包不堆叠）
- 宪章:152 明言「堆叠背包」是 B2（Base 消除摩擦）的范例；宪章:58 Base「开启，不可关」
- 宪章 §6 先例：金币+25%、光照压制（Base 层违规的 Depth 改动）均无条件撤销，非开关门控
- 决策 26 的延迟理由（删 20 测试）经独立复核**不成立**：22 个 consumable_stack_test 全为手写 SpdList 操作，无一个调 `AutoPlaceItemInBelt`/`ChangeBeltItem`，实际零测试受影响

---

## 3. 事实基础

全部数值于 2026-08-08 核实。

### 3.1 掉落池

| 事实 | 值 | 出处 |
|---|---|---|
| 掉落生成 | `GetItemIndexForDroppableItem` 按 dropRate 加权 + iMinMLvl 过滤 | `Source/items.cpp:1352` |
| 符文索引 | 161-165（Rune of Fire/Lightning、Greater ×2、Rune of Stone） | `assets/txtdata/items/itemdat.tsv:163-167` |
| 非 HF 符文排除 | `(i < 161 || i > 165) // Runes` | `Source/items.cpp:2397` |
| HF 模式 | `IsItemAvailable` 直接 `return true` → 符文可掉落 | `Source/items.cpp:2386` |
| 伤害卷轴（SCROLLT） | 12 行；dropRate=1 的 11 个（Fire Wall/Inferno/Flash/Fireball/Stone Curse/Chain Lightning/Flame Wave/Guardian/Golem/Resurrect/**Lightning**） | `assets/txtdata/items/itemdat.tsv` |
| Resurrect 卷轴 | 两条：dropRate=0（iMinMLvl 空）与 dropRate=1 | `itemdat.tsv:36`、`:97` |
| 工具卷轴（SCROLL） | Identify/TP/Infravision/Healing/Search/Phasing/ManaShield/Nova/Teleport/Apocalypse 等（dropRate=1） | `itemdat.tsv:93-103+` |

### 3.2 堆叠现状

| 事实 | 值 | 出处 |
|---|---|---|
| 堆叠上限 | 药水 5 / 卷轴 3，不因职业而异（职业加成已撤销） | `Source/items.cpp:5170 GetMaxStackCount` |
| 堆叠字段 | `Item::_iStackCount`（int8_t，默认 1） | `Source/items.h:258` |
| 背包放置 | `AutoPlaceItemInInventory` **无堆叠逻辑**（直接放新格） | `Source/inv.cpp:1434` |
| 腰带放置 | `AutoPlaceItemInBelt` + `ChangeBeltItem` 有堆叠（`_iStackCount++`） | `Source/inv.cpp:493-511` |
| 腰带容量 | 8 格（`FitsInBeltSlot` 要求 1×1） | `Source/inv.cpp:174` |
| 存档持久化 | `_iStackCount` 存对齐填充字节（**原始值**，`max(1,…)` 读取） | `Source/loadsave.cpp:358/:1261` |
| 网络持久化 | `bId` 位域（`count-1` 编码） | `Source/pack.cpp:141/:350`、`Source/msg.cpp:2685/:2711` |

### 3.3 宪章裁决（本规格落实的依据）

| 裁决 | 出处 |
|---|---|
| 取消符文+伤害卷轴掉落，零补偿 | 决策 6（`宪章:663`） |
| 补偿不得沿用 +25% 金币 | §9（`宪章:624`） |
| 背包堆叠、腰带不堆叠 | §6（`宪章:378`），理由见 `宪章:386-390` |
| 背包堆叠不违反 B2（战斗外，仅记账摩擦） | `宪章:390` |
| 腰带堆叠违反 B2（取消红蓝配比决策） | `宪章:388` |

---

## 4. 方案

### 4.1 符文掉落排除（补 HF 缺口）

**机制**：与 §4.2 相同——在掉落池过滤回调（`RndItemForMonsterLevel` + `RndAllItems` 的 isItemOkay）排除符文，**不改 `IsItemAvailable`**：

```cpp
// 在掉落过滤回调中添加（对照 §4.2 伤害卷轴做法）：
if (item.iMiscId > IMISC_RUNEFIRST && item.iMiscId < IMISC_RUNELAST && IsDarkExpedition())
    return false; // 符文不掉落（开关门控）
```

**为什么不用 `IsItemAvailable`（独立复核确认）**：`IsItemAvailable` 不只管掉落——它还门控 `RemoveInvalidItem`（`loadsave.cpp:2201`，**清空**物品）、`UnPackItem`（`pack.cpp:312`，清空）、网络消息校验（`msg.cpp:1287/:1318/:2150` 等）。若在其中排除符文，开关开启时**旧存档的符文会被静默删除**（违反 §4.7「不删除物品」）且网络中的符文物品被拒。drop-filter 回调只管掉落路径，波及面窄——与 §4.2 伤害卷轴做法一致。

**说明**：
- 非 HF 已有 `IsItemAvailable` 排除（`(i<161||i>165)`，`Source/items.cpp:2397`，**上游行为**，保留不动）——本改动只补掉落路径的 HF 缺口，不触碰既有存档/网络校验
- 开关门控：`IsDarkExpedition()` 为 true 才排除（红线 13：关 = 照旧掉落）

### 4.2 伤害卷轴掉落排除

**机制**：掉落池过滤（`RndItemForMonsterLevel` + `RndAllItems` 的 isItemOkay 回调，对照 Dark Expedition 卷轴预算做法）排除 SCROLLT 类：

```cpp
// 在掉落过滤回调中添加：
if (item.iMiscId == IMISC_SCROLLT && item.iSpell != SpellID::Resurrect)
    return false; // 伤害卷轴不掉落（保留 Resurrect，多人复活必需品）
```

**注意**：
- `IMISC_SCROLLT`（`Source/items.h`）区分伤害卷轴与工具卷轴（`IMISC_SCROLL`）
- **保留 Resurrect**（dropRate=1 那条）——多人复活必需，不属于「伤害卷轴」
- 商店（女巫）是否仍售伤害卷轴：**保留**——玩家需要时仍可购买，只是不再从地牢掉落（这与「取消掉落」而非「删除物品」的裁决一致）

### 4.3 背包堆叠

**机制**：`AutoPlaceItemInInventory`（`Source/inv.cpp:1434`）加堆叠逻辑：

```cpp
bool AutoPlaceItemInInventory(Player &player, const Item &item, bool sendNetworkMessage)
{
    // 新：尝试堆叠到现有同类型物品上
    if (item.canStack() && TryStackInInventory(player, item, sendNetworkMessage))
        return true;

    const Size itemSize = GetInventorySize(item);
    // ... 原有放置逻辑不变
}
```

**`TryStackInInventory`**：遍历 `InvList`，找到同 `IDidx` + 未满堆叠（`_iStackCount < GetMaxStackCount`）的物品，`_iStackCount++`（匹配键用 `IDidx`——既有腰带堆叠的建立模式，`inv.cpp:500/:1348`，比 `_iMiscId+_iSpell` 更强）。

**堆叠上限**：沿用现有 `GetMaxStackCount`（药水 5/卷轴 3）——本规格不改变上限数值，只改变「堆叠发生在背包而非腰带」。

**判定**：背包堆叠不违反 B2（`宪章:390`：战斗外，仅消除「背包被 20 个药水占满」的记账摩擦）。

**⚠️ 卷轴消耗语义（独立复核发现，必须定义）**：现有 `ConsumeScroll`（`inv.cpp:2070-2101`）用 `RemoveInvItem`/`RemoveSpdBarItem`（`player.cpp:1549/:1594`）清除**整件物品**——今天用 1 张叠了 3 的卷轴会消耗全部 3 张（既有 bug）。背包堆叠使卷轴在背包中堆叠后，此 bug 进入背包。**本规格定义**：卷轴使用改为 `_iStackCount--`（若 >1），仅当降到 0 才移除物品。此修复同时治好既有腰带卷轴整叠消耗 bug。

### 4.4 腰带不堆叠

**机制**：
1. `AutoPlaceItemInBelt`：不再尝试堆叠，每格放置独立物品（每格 1 件，恢复原版 8×1）
2. `ChangeBeltItem`（`Source/inv.cpp:493`）：移除 `_iStackCount++` 逻辑，改为正常放置/交换

**效果**：腰带恢复「8 格红蓝配比」决策（`宪章:388`），腰带容量从 8×5=40 回到 8×1=8。

**持久化**：`_iStackCount` 基础设施**保留**（背包堆叠仍需要）——决策 26 的担忧（删基础设施 + 20 测试）不成立，因为背包堆叠复用该字段。

### 4.5 零补偿（决策 6）

**不做**金币 +25% 或其他补偿。理由：
- 符文/伤害卷轴本就在掉落池几乎无人拾取，取消不减少玩家实际获得（`宪章:624` 已分析 +25% 的错误方向）
- 前期痛点（玩家贫穷、回城卷轴救命）与符文/伤害卷轴无关——这两类本就是噪声掉落

### 4.6 红线 13（开关关闭 = 原版）

**Base 部分（背包堆叠 + 腰带不堆叠）无条件生效**——它们恢复原版形态（原版即 8×1、背包不堆叠），开关关闭时自然满足红线 13。Base「开启，不可关」（宪章:58）。

**Depth 部分（掉落排除）随开关门控**：

| 开关 | 符文/伤害卷轴掉落 |
|---|---|
| **开**（Dark Expedition） | 排除（不掉落） |
| **关**（现状） | 照旧掉落（它们本无害，宪章:104） |

### 4.7 不做的事（明确排除）

- **不做** +25% 金币补偿（宪章 §9 否决）
- **不做** 删除 `_iStackCount` 基础设施（背包堆叠复用）
- **不做** 商店库存/价格调整（本规格范围外；前置补给成本留待后续评估）
- **不做** 职业差异化堆叠（宪章已撤销，违反红线 11）

---

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Depth |
| 2 | 问题陈述指向具体症状 | 第 1 节：符文 HF 掉落缺口、伤害卷轴在掉落池、堆叠形态偏差，全部有出处 |
| 3 | 每个数值标注出处 | 第 3 节逐条标注 |
| 4 | 「已实施」标注需非测试调用者+验收全过 | 本规格状态为草案，不标注已实施 |

### 深度层红线（仅 Depth 部分：掉落排除）

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 压力来源是限制信息还是膨胀数值？ | 限制信息 | 取消符文/伤害卷轴掉落是**纯减负**（宪章:104：去掉后玩家没少取舍），非膨胀数值 |
| 10 | 有可执行的反制手段？ | 有且已指名 | 伤害卷轴仍可从女巫商店购买（`WitchItemOk` 无 SCROLLT 排除）；药水/工具卷轴掉落不受影响 |
| 11 | 稀缺产生取舍还是跑腿？ | 取舍 | 取消噪声掉落不产生稀缺（本无害物品，玩家几乎不拾取）；不构成负担 |
| 12 | 全层段有定义？ | 是 | 掉落排除为全局机制（不按层段） |
| 13 | 开关关闭时行为回到原版？ | 是 | 掉落排除随 `darkExpedition` 门控：关 = 符文/伤害卷轴照旧掉落（现状） |
| 14 | 近战/远程分别评估？ | 已评估 | 掉落排除对近战/远程无差异化影响（供给是通用的）；无信息限制因此无红 14 不对称风险 |

### Base 部分（背包堆叠 + 腰带不堆叠）——底座层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 5 | 改变战斗结果/难度曲线/战术选择？ | 否 | 背包堆叠不改变战斗内决策（宪章:390）；腰带 8×1 **恢复**原版红蓝配比决策（宪章:388），战斗结果等同原版 |
| 6 | 被消除的未知是否「无法通过游戏内行动解决」？ | 是 | 无未知消除 |
| 7 | 代替玩家做选择？ | 否 | 堆叠是记账摩擦消除，非代替选择 |
| 8 | 移除玩家已有选项？ | 否 | 腰带 8 格保留（每格 1 件），背包堆叠是增益 |

### 深度层禁令核对

| 禁令 | 依据 |
|---|---|
| 7 不改变文件行尾 | 实施计划将校验 CRLF；涉及文件已入漂移校验范围 |

---

## 6. 验收标准

| # | 验收项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 符文掉落排除（Depth 开） | 测试：`RndAllItems`/`RndItemForMonsterLevel` 开关开时 | 掉落池不含符文（`RUNEFIRST..RUNELAST`） |
| 2 | 符文存档/网络校验不受影响 | 测试：`IsItemAvailable(161..165)`（HF + 非 HF） | 返回 true（非 HF 上游排除除外）；旧存档符文不因开关删除 |
| 3 | 伤害卷轴掉落排除（Depth 开） | 测试：`RndAllItems`/`RndItemForMonsterLevel` 开关开时 | 掉落池不含 SCROLLT（除 Resurrect） |
| 4 | Resurrect 保留 | 测试：掉落池含 Resurrect | true |
| 5 | 开关关闭掉落恢复 | 测试：开关关时掉落池含符文/伤害卷轴 | true |
| 6 | 背包堆叠（Base 无条件） | 测试：放置同类型药水×N | 堆叠到同一格，`_iStackCount == min(N, 5)` |
| 7 | 腰带不堆叠（Base 无条件） | 测试：放置同类型药水到腰带 | 每格独立，无堆叠 |
| 8 | 腰带上限 8×1 | 测试：8 格填满后第 9 个拒绝 | 拒绝 |
| 9 | 背包堆叠上限 | 测试：药水堆到 5 后继续放置 | 开新格，堆叠数不超 5 |
| 10 | 卷轴消耗单张 | 测试：使用叠了 3 的卷轴 | 消耗 1 张，堆叠变 2；非整叠移除 |
| 11 | 持久化兼容 | `pack_test` + `writehero_test` + `timedemo` | 全过；`_iStackCount` 编码不变 |
| 12 | 开关关闭全量 | 开关关跑全量测试 | 670/670，timedemo 与本分支基线一致 |
| 13 | 漂移校验 | `python3 tools/check_drift.py` | 退出码 0，五项 PASS |

---

## 7. 状态

**草案**。待独立复核通过后转「已批准」，再由实施计划落地。

**已知边界**（本规格不解决，实施计划或后续处理）：

- **timedemo 基线**：Base 部分（背包堆叠 + 腰带 8×1）无条件生效，会改变 timedemo 流程中腰带状态 → **参考存档需重新生成**（宪章决策 25 先例）。实施计划须含此步
- **存档迁移**：旧存档腰带 `_iStackCount>1` 的物品加载后——药水自然消耗，卷轴叠放因消耗语义修正（验收 10）逐张消耗，无数据损坏。实施时验证
- **stack-split 语义**：从背包拖堆叠物品到腰带（8×1 不堆叠）——需 split-on-grab 或腰带侧拒绝，实施时定
- **TP 卷轴背包 ×3 容量**：与宪章:121「TP 稀缺是独特性」的张力——背包容量增加但腰带格仍 1×1（即时使用决策保留），接受为记账便利；如后续需收紧，单独评估
- **autoRefillBelt**（`options.cpp:872`）与背包堆叠交互：refill 从背包堆叠消耗，默认关闭，实施时验证
- 商店库存/价格调整（前置补给成本）——需「玩家应带多少补给」模型确定后另行评估
- 符文/伤害卷轴的商店售价保留现状（取消掉落不删物品）
- 背包堆叠的 UI 显示（堆叠数角标）沿用现有 `_iStackCount` 显示逻辑（`inv.cpp:1228` 背包显示分支——当前是死代码，随背包堆叠生效后激活）
