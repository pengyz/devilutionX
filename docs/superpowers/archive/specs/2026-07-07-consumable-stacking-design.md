> **已废弃 — 内容含已裁决撤销的设计，勿作参考。**
>
> - 职业差异化堆叠上限（战士 +3/+1、法师 −2）已撤销：违反深度层红线 11，法师是最依赖蓝瓶的职业却被削减上限，产出的是跑腿而非取舍
> - 腰带堆叠形态已裁决改为背包堆叠：腰带 8 格的红瓶/蓝瓶配比是真决策，堆叠取消了它，违反底座层支柱 B2
> - 第 8 节「网络同步」给出的 `packedItem.bId = item._iStackCount` 是错的，它覆写了物品品质；已于 `90c5511fc` 改为位域合并
>
> 重写见宪章待立项清单优先级 2。

# Consumable Stacking System Design

**日期**: 2026-07-07
**状态**: 设计完成
**作者**: Claude

---

## 问题陈述

D1 的消耗品系统存在以下问题：
1. 每个消耗品占用一个腰带格，导致腰带空间紧张
2. 玩家不愿意携带消耗品，因为占用太多空间
3. 战士缺乏独特的物品管理能力

## 设计目标

1. 实现消耗品堆叠，减少腰带空间压力
2. 为不同职业提供差异化的堆叠能力
3. 实现战士的"物品携带"被动技能
4. 实现法师的"轻装出行"被动技能

## 技术方案

### 1. Item 结构体修改

在 `Source/items.h` 的 `Item` 结构体中添加堆叠计数字段：

```cpp
struct Item {
    // ... existing fields ...
    int8_t _iStackCount = 1;  // 堆叠数量，默认1
};
```

### 2. 堆叠判断函数

```cpp
// 判断物品是否可以堆叠
bool CanStackItem(const Item &item) {
    if (item._itype != ItemType::Misc) return false;
    
    switch (item._iMiscId) {
        case IMISC_HEAL:      // 血瓶
        case IMISC_MANA:      // 蓝瓶
        case IMISC_SCROLL:    // 卷轴
        case IMISC_REJUV:     // 回复药水
        case IMISC_FULLREJUV: // 完全回复药水
            return true;
        default:
            return false;
    }
}
```

### 3. 最大堆叠数计算

```cpp
// 获取最大堆叠数（考虑职业加成）
int GetMaxStackCount(const Item &item, const Player &player) {
    // 根据物品类型确定基础堆叠数
    int base;
    switch (item._iMiscId) {
        case IMISC_HEAL:      // 血瓶
        case IMISC_MANA:      // 蓝瓶
        case IMISC_REJUV:     // 回复药水
        case IMISC_FULLREJUV: // 完全回复药水
            base = 5;
            break;
        case IMISC_SCROLL:    // 卷轴
            base = 3;
            break;
        default:
            return 1;  // 不可堆叠
    }
    
    // 职业加成
    switch (player._pClass) {
        case HeroClass::Warrior:
            if (item._iMiscId == IMISC_HEAL || item._iMiscId == IMISC_MANA)
                base += 3;  // 战士血瓶堆叠+3
            else if (item.isScroll())
                base += 1;  // 战士卷轴堆叠+1
            break;
        case HeroClass::Sorcerer:
            if (item._iMiscId == IMISC_HEAL || item._iMiscId == IMISC_MANA)
                base -= 2;  // 法师药水堆叠-2
            break;
        default:
            break;
    }
    
    return base;
}
```

### 4. 腰带放置逻辑

修改 `Source/inv.cpp` 的 `AutoPlaceItemInBelt` 函数：

```cpp
bool AutoPlaceItemInBelt(Player &player, const Item &item, bool persistItem, bool sendNetworkMessage) {
    if (!CanBePlacedOnBelt(player, item)) return false;
    
    // 尝试堆叠到现有物品
    if (CanStackItem(item)) {
        for (Item &beltItem : player.SpdList) {
            if (!beltItem.isEmpty() && beltItem.IDidx == item.IDidx) {
                int maxStack = GetMaxStackCount(beltItem, player);
                if (beltItem._iStackCount < maxStack) {
                    if (persistItem) {
                        beltItem._iStackCount++;
                    }
                    return true;
                }
            }
        }
    }
    
    // 放置到空槽位
    for (Item &beltItem : player.SpdList) {
        if (beltItem.isEmpty()) {
            if (persistItem) {
                beltItem = item;
                beltItem._iStackCount = 1;
            }
            return true;
        }
    }
    
    return false;
}
```

### 5. 使用物品逻辑

修改物品使用后的处理逻辑：

```cpp
if (speedlist) {
    if (!item->isScroll() && !item->isRune()) {
        if (item->_iStackCount > 1) {
            item->_iStackCount--;  // 减少堆叠数
        } else {
            player.RemoveSpdBarItem(c);  // 移除物品
        }
    }
}
```

### 6. 腰带显示

在 `DrawInvBelt` 函数中添加堆叠数显示：

```cpp
// 显示堆叠数
if (myPlayer.SpdList[i]._iStackCount > 1) {
    DrawString(out, StrCat(myPlayer.SpdList[i]._iStackCount),
        { position + Displacement { 0, 10 }, InventorySlotSizeInPixels },
        { .flags = UiFlags::ColorWhite | UiFlags::AlignRight });
}
```

### 7. 保存/加载逻辑

修改 `Source/loadsave.cpp`：

```cpp
void SaveItem(const Item &item, SaveHelper &file) {
    // ... existing fields ...
    file.WriteLE<int8_t>(item._iStackCount);
}

void LoadItem(Item &item, LoadHelper &file) {
    // ... existing fields ...
    item._iStackCount = file.NextLE<int8_t>();
}
```

### 8. 网络同步

在 `Source/pack.cpp` 的 `PackItem` 和 `UnPackItem` 函数中添加堆叠数同步：

```cpp
// PackItem
packedItem.bId = item._iStackCount;

// UnPackItem
item._iStackCount = std::clamp<int>(packedItem.bId, 1, 127);
```

在 `Source/msg.cpp` 的网络消息中同步堆叠数。

## 职业被动技能

### 战士：物品携带

| 属性 | 值 |
|---|---|
| 名称 | 物品携带 |
| 效果 | 药水堆叠+3，卷轴堆叠+1 |
| 描述 | "战士强壮的体魄让他能携带更多物品。" |

### 法师：轻装出行

| 属性 | 值 |
|---|---|
| 名称 | 轻装出行 |
| 效果 | 药水堆叠-2 |
| 描述 | "法师专注于魔法研究，体质较弱，无法携带太多药水。" |

### 游侠

无特殊被动，使用基础堆叠数。

## 堆叠数汇总

| 物品类型 | 基础 | 战士 | 法师 | 游侠 |
|---|---|---|---|---|
| 血瓶 | 5 | 8 | 3 | 5 |
| 蓝瓶 | 5 | 8 | 3 | 5 |
| 卷轴 | 3 | 4 | 3 | 3 |
| 回复药水 | 5 | 8 | 3 | 5 |

## 测试计划

1. 单元测试：堆叠判断、最大堆叠数计算
2. 集成测试：腰带放置、使用、显示
3. 职业测试：不同职业的堆叠限制
4. 保存/加载测试：堆叠数持久化
5. 网络同步测试：多人游戏中的堆叠同步

## 风险评估

1. **兼容性风险**：添加新字段可能影响存档兼容性
   - 缓解：默认值为1，旧存档自动兼容
   
2. **网络同步风险**：堆叠数同步可能有延迟
   - 缓解：使用现有的网络同步机制

3. **UI 显示风险**：堆叠数显示可能遮挡物品图标
   - 缓解：使用小字体，放在角落位置

## 设计决策

1. **堆叠上限**：基础5，符合D1的简约风格
2. **职业差异**：战士+3，法师-2，体现体质差异
3. **堆叠范围**：所有消耗品，简化管理
4. **UI显示**：堆叠数显示在物品图标右下角
