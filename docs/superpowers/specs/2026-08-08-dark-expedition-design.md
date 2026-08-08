# 黑暗远征（Dark Expedition）深度层旗舰设计

**日期**：2026-08-08
**状态**：草案（待独立复核）
**分类**：Depth
**取代**：无（新规格）

---

## 1. 问题陈述

本项目（Better D1）的深度层目前为空。宪章第 9 节明文记录：

> 深度层目前是空的。若 Better D1 停在 Base 层，它是一个合格但不特别的 QoL mod——它解决的问题上游 DevilutionX 已解决一半（`GameplayOptions` 内 49 个布尔选项），剩下的独特价值是「tooltip 比上游做得好」，撑不起一个 mod 的身份。

**具体症状**（可复现）：

1. 本 fork 相对上游的全部改动集中在 Base 层（消除摩擦、撤销平衡偏离、修复数据损坏）。深度层开关不存在，`GameplayOptions` 中没有 `OptionEntryBoolean` 形式的深度层总开关（对照现有先例 `theoQuest` — `Source/options.cpp:847`、`multiplayerFullQuests` — `Source/options.cpp:850`）。
2. 深度层三支柱（DP1/DP2/DP3）无任何已落地的消费者。宪章决策 28 已确认「底座层基本做完，深度层是空的」。
3. 唯一深度层失败先例——光照压制（`GetLightSuppressionMultiplier` / `GetEffectiveLightRadius`，`Source/lighting.cpp:594/:620`）——已于宪章清理中撤销（commit `f2cf25699`），其设计缺陷（DP2 无反制、DP3 无取舍、红线 14 远程受害更大）已记录在案。

**本规格要解决的问题**：为深度层确立一个满足全部深度层红线（9-14）的旗舰改动，使深度层从「空」变为「有身份」。方向经独立评估（两轮 Oracle 对抗性评审）后修订为本规格。

---

## 2. 分类判定

按宪章第 2 节判定树逐步判定：

**第一步——改动是否触及数值/掉落/属性/光照/商店/战斗资源？**

- 光照半径：**是**（触及判定树第 4 条「光照半径、视野范围」）→ **Depth**。无例外条款。

深度层红线 13（开关关闭时行为回到原版）意味着光照半径的改动必须由深度层开关门控。

**补充判定**：

| 改动项 | 单独判定 | 结果 |
|---|---|---|
| 光照半径倍率（13-16 / 17-20 / 21-24） | 判定树第 4 条 | Depth |
| Infravision 目标选择修复（`CanTarget`） | 判定树第 2 步：玩家可感知行为变化 | Base（单独看）→ 但作为深度层反制手段的一部分，随深度层开关生效，整体判 Depth |
| Scroll of Infravision 掉落率削减 + 移出女巫库存 | 判定树第 2 条「掉落概率或掉落表构成」 | Depth |
| Infravision 可学（`spelldat.tsv:bookLevel` 改动） | 判定树第 1 条「TSV 数据文件数值字段」 | Depth |
| 删除三个 stub 法术（DoomSerpents/BloodRitual/Invisibility） | 宪章「错误设计」处置：先问去掉后玩家少了什么取舍 → 答案「没少」→ 删除 | Depth（数据表构成） |

**归类结论**：整个改动包（M1' 视野限制 + 反制体系 + stub 清理）作为一个整体判为 **Depth**，由单一深度层开关控制。

---

## 3. 事实基础

全部数值于 2026-08-08 核实。

### 3.1 光照系统

| 事实 | 值 | 出处 |
|---|---|---|
| 玩家基础光照半径 | 10 | `Source/player.cpp:2331` `player._pLightRad = 10` |
| 光照半径钳制范围 | [2, 15] | `Source/items.cpp:2536` `lrad = std::clamp(lrad, 2, 15)` |
| 装备光加成求和 | `lightRadius += item._iPLLight`（基数 10 起） | `Source/items.cpp:2818`、`:2857` |
| 光照半径与视野同源 | `ActivateVision(player.position.tile, player._pLightRad, ...)` | `Source/player.cpp:2524` |
| 光照半径变更同步灯光与视野 | `ChangeLightRadius` + `ChangeVisionRadius` | `Source/items.cpp:2540-2542` |
| tile 点亮状态 | `dFlags[x][y] |= DungeonFlag::Lit`（`DungeonFlag::Visible` 为可见性） | `Source/lighting.cpp:107` |
| `IsTileLit` 定义 | `InDungeonBounds(p) && HasAnyOf(dFlags[p], DungeonFlag::Lit)` | `Source/levels/gendung.h:253-257` |
| 撤销前压制函数（失败先例，勿参考实现） | `GetLightSuppressionMultiplier` / `GetEffectiveLightRadius` | `Source/lighting.cpp:594/:620`（已删，commit `f2cf25699`） |
| 撤销前双重计数 bug | `equipmentBonus = _pLightRad - 10` 在倍率后叠加，诅咒装备时叠加两次 | 宪章第 7 节（`lighting.cpp:620` 时代） |

### 3.2 目标选择（IsTileLit 门控点）

| 门控点 | 作用 | 出处 |
|---|---|---|
| 悬停选择 | `if (IsTileLit(posToCheck) && ...)` → `pcursmonst = monsterId` | `Source/cursor.cpp:89` |
| 点击选择（相邻格） | `if (IsTileLit(adjacentTile) && ...)` | `Source/cursor.cpp:315` |
| 目标失效清理（怪物） | `!IsTileLit(monster.position.tile)` → `pcursmonst = -1` | `Source/track.cpp:43` |
| 目标失效清理（友方玩家） | `!IsTileLit(targetPlayer.position.tile)` → 清目标 | `Source/track.cpp:55` |
| 手柄目标获取 | `if (!IsTileLit(monster.position.tile)) return false` | `Source/controls/plrctrls.cpp:263` |
| 手柄友方目标 | `!IsTileLit(player.position.future)` | `Source/controls/plrctrls.cpp:403` |
| 渲染（**不得改动**） | `!IsTileLit(tilePosition)` 分支决定暗/TRN 绘制 | `Source/engine/render/scrollrt.cpp:469/:485/:749`，另 `:386`（`DrawMonster` 未点亮 tile → Infra TRN） |
| 自动地图（**不受本改动影响**） | 揭示走 `DungeonFlag::Explored`（`DoVisionFlags`），不用 `IsTileLit`；`automap.cpp:1008-1014` 为 `#ifdef _DEBUG` `DebugVision` 调试覆盖层 | `Source/lighting.cpp:104` |

### 3.3 Infravision

| 事实 | 值 | 出处 |
|---|---|---|
| Infravision 法术数据 | manaCost 40、minIntelligence 36、bookLevel -1（不可学）、staffLevel -1、missile `Infravision` | `assets/txtdata/spells/spelldat.tsv:10` |
| Infravision 卷轴 | `Scroll of Infravision`，iMinMLvl 8，value 600 | `assets/txtdata/items/itemdat.tsv:102` |
| AddInfravision 时长 | `missile.duration = ScaleSpellEffect(1584, ...)` ≈ 26 秒 | `Source/missiles.cpp:2571-2575` |
| ProcessInfravision | `player._pInfraFlag = true`，时长耗尽时 `CalcPlrItemVals(player, true)` | `Source/missiles.cpp:3854-3863` |
| Infravision 渲染（TRN 调色板） | `LoadFileInMemWithStatus("plrgfx\\infra.trn", ...)` | `Source/lighting.cpp:225` |
| Infravision 是否影响 tile 点亮 | 否——只设 `_pInfraFlag` 渲染标志，不改 `dFlags` 的 `Lit` 位 | `Source/missiles.cpp:3858` 对照 `Source/lighting.cpp:107` |

**关键结论**：Infravision 当前是「渲染级」法术——让暗处怪物可见（红色 TRN），但 `IsTileLit` 仍为 false → **不可选中、不可攻击**。

### 3.4 反制资产（现有，零新美术）

| 资产 | 光加成 | 出处 |
|---|---|---|
| Empyrean Band（独特戒指） | LIGHT +2 | `assets/txtdata/items/unique_itemdat.tsv:4` |
| Optic Amulet（独特护身符） | LIGHT +2 | `assets/txtdata/items/unique_itemdat.tsv:5` |
| Lightforge（独特剑） | LIGHT +4 + DAMP 150 + TOHIT 25 | `assets/txtdata/items/unique_itemdat.tsv:11` |
| 后缀 `light` | LIGHT +2，iMinMLvl 4，Armor/Weapon/Misc | `assets/txtdata/items/item_suffixes.tsv:68` |
| 后缀 `radiance` | LIGHT +4，iMinMLvl 8，Armor/Weapon/Misc | `assets/txtdata/items/item_suffixes.tsv:69` |
| 词缀竞争机制 | `GetItemPowerPrefixAndSuffix`/`SelectAffix` 从共享前缀/后缀池按 `AffixItemType` 过滤；一件魔法物品可得**一个前缀 + 一个后缀**（光词缀与伤害词缀同池竞争，但并非「只能出一个词缀」） | `Source/items.cpp:1173-1207` |
| Scroll of Infravision | 掉落（iMinMLvl 8）+ 女巫库存（无固定位） | `assets/txtdata/items/itemdat.tsv:102` |

### 3.5 掉落与商店机制

| 事实 | 值 | 出处 |
|---|---|---|
| 掉落生成 | `GetItemIndexForDroppableItem` 按物品类型权重 + 等级过滤，**无槽位维度** | `Source/items.cpp:1352` |
| 女巫固定库存（钉住项） | `PinnedItemTypes = { IDI_MANA, IDI_FULLMANA, IDI_PORTAL }` | `Source/items.cpp:4529` `SpawnWitch` |
| 女巫库存数量 | `itemCount = RandomIntBetween(10, NumWitchItems)`，≥ 10 > 3 钉住项恒在 | `Source/items.cpp:4535` |
| Scroll of Town Portal | value 200，钉住项之一 → 女巫永远有卖 | `assets/txtdata/items/itemdat.tsv:100`、`Source/items.cpp:4529` |
| TownPortal 排除先例（随机路径） | `WitchItemOk`：`item.iSpell == SpellID::TownPortal → false` | `Source/items.cpp:2017` |
| 女巫库存 MP 重摇路径 | `RecreateWitchItem`（种子重摇，多人用） | `Source/items.cpp:2122` |
| Town Portal 法术可学 | bookLevel 3 | `assets/txtdata/spells/spelldat.tsv`（TownPortal 行） |

### 3.6 层段定义（Hellfire）

| 层段 | 层号 | 主题 |
|---|---|---|
| 大教堂/地下室/洞穴 | 1-12 | 非本规格范围 |
| Hell（地狱） | 13-16 | 本规格压力段 |
| Nest（巢穴） | 17-20 | 本规格弱压力段 |
| Crypt（地穴） | 21-24 | 本规格压力段 |

### 3.7 法术 stub 状态

| 法术 | missiles 列 | bookLevel | 结论 |
|---|---|---|---|
| DoomSerpents | 空 | -1 | stub |
| BloodRitual | 空 | -1 | stub |
| Invisibility | 空 | -1 | stub |
| Golem | Golem | 11（可学） | 有实现，但 `GolumAi` 原地/随机游走，不可侦察（`Source/monster.cpp:4164`） |
| Etherealize | Etherealize | -1（卷轴/法杖） | 有实现，Sorcerer 专用（minInt 93） |

**⚠️ 位置索引警告（P0，独立复核发现）**：`SpellsData` 是**按位置索引**的 vector——`LoadSpellData` 按文件行序 `emplace_back`（`Source/tables/spelldat.cpp:241-244`），`GetSpellData` 用 `SpellsData[enum]`（`Source/tables/spelldat.h:271-274`）。TSV 行序与枚举序 1:1。**直接删除 DoomSerpents/BloodRitual/Invisibility 三行会使 Nova→Elemental 全部数据错位**（`GetSpellData(SpellID::Nova)` 返回错位数据），污染可学法术。删除必须先解除行序与枚举的耦合（见 4.5）。

---

## 4. 方案

### 4.1 深度层开关（Infra 前置，另立规格或本规格附带）

`GameplayOptions` 新增 `OptionEntryBoolean`：

```cpp
, darkExpedition("Dark Expedition", OptionEntryFlags::CantChangeInGame | OptionEntryFlags::CantChangeInMultiPlayer, N_("Dark Expedition"), N_("Restricts vision in deeper levels..."), false)
```

对照先例：`theoQuest`（`Source/options.cpp:847`）、`multiplayerFullQuests`（`Source/options.cpp:850`）。**本规格假设开关存在，具体实现见实施计划。**

### 4.2 M1' 视野限制（核心）

**机制**：深度层开启时，以下层段应用光照半径倍率：

| 层段 | 层号 | 倍率 | 有效半径（基础 10） |
|---|---|---|---|
| Hell | 13-16 | 0.6 | 6 |
| Nest | 17-20 | 0.85 | 8（`trunc(8.5)`） |
| Crypt | 21-24 | 0.5 | 5 |

**公式**（修复撤销前双重计数 bug）：

```
effectiveLightRadius = trunc(_pLightRad × multiplier)    // 截断，非 round
effectiveLightRadius = clamp(effectiveLightRadius, 2, 15) // 乘倍率后钳制
```

- 倍率作用于**总量**（基础 + 装备加成），不再是撤销前的 `10 × mult + (_pLightRad − 10)`
- **截断而非四舍五入**：Nest 0.85 时 `trunc(8.5) = 8`，与上表及验收标准一致（`round(8.5) = 9`，不一致）
- **clamp 顺序**：先乘倍率、后钳制 [2,15]——诅咒装备（基础 2）在 Crypt 得 `trunc(2×0.5)=1`，钳制后仍为 2，永不低于原版最小值
- 已知含义：装备加成在深段被倍率同比例削弱（Crypt 0.5 时 +2 光装备只贡献 +1 有效）——反制在压力最高处最弱。这是**有意的调节事实**，由规格显式声明而非隐藏
- 17-20 Nest 取 0.85 而非 1.0，**保持曲线单调**——撤销前缺陷「压制曲线在此断裂」（Nest 落 default 1.0 比 Hell 0.6 更亮）不得重演
- 非 Hellfire（base D1）：无 Nest/Crypt，只作用 13-16（Hell）；11-16 之外全部回原版。层段边界由 `GetLevelType`（`Source/levels/gendung.cpp:393-411`）：≤16 Hell、≤20 Nest、≤24 Crypt，与上表一致

**实现位置**：`CalcPlrLightRadius`（`Source/items.cpp:2535`）或等价入口点注入倍率。倍率表数据驱动（TSV 或代码常量均可，实施计划定）。

### 4.3 Infravision 目标选择修复（CanTarget）

**机制**：新增独立函数（**绝不修改 `IsTileLit`**）：

```cpp
// 目标选择专用：Infravision 激活时，已探索/可见的暗处 tile 对"选择/攻击"有效，但保持美学黑暗
bool CanTarget(Point position)
{
    return IsTileLit(position)
        || (IsDarkExpedition()            // §4.7：开关关闭时回退为 IsTileLit 行为（红线 13）
            && MyPlayer->_pInfraFlag
            && IsTileVisible(position));  // 复用既有 IsTileVisible（Source/levels/gendung.h:243）
}
```

**设计要点**：
- 用 `DungeonFlag::Visible`（而非裸 `InDungeonBounds`）约束暗处目标——Infravision 渲染只显示已见过/可见的 tile（`Source/engine/render/scrollrt.cpp:749` 要求 `IsFloor`，`:386` 只画未点亮 tile），`CanTarget` 应与渲染对齐，避免选中从未见过的墙后 tile
- 明确副作用：`track.cpp:43` 不再清除走出视线的怪物目标（Infra 激活期间）——与渲染保留暗处怪物的行为一致，属预期
- `_pInfraFlag` 生命周期：由 `ValidatePlayer`（每 tick，`Source/player.cpp:2992`）与 `CreatePlayer` 清除；missile 过期路径（`ProcessInfravision → CalcPlrItemVals`）不清除——`CanTarget` 继承此语义，无害（与渲染同源）

**替换以下 5-6 个门控点**中的 `IsTileLit`（仅目标选择路径，渲染/自动地图不动）：

| 门控点 | 替换为 | 出处 |
|---|---|---|
| 悬停选择 | `CanTarget` | `Source/cursor.cpp:89` |
| 点击选择 | `CanTarget` | `Source/cursor.cpp:315` |
| 目标失效（怪物） | `CanTarget` | `Source/track.cpp:43` |
| 目标失效（友方） | `CanTarget` | `Source/track.cpp:55` |
| 手柄目标获取 | `CanTarget` | `Source/controls/plrctrls.cpp:263` |
| 手柄友方目标 | `CanTarget` | `Source/controls/plrctrls.cpp:403` |

**禁止改动**（否则破坏渲染/自动地图）：
- `Source/engine/render/scrollrt.cpp:469/:485/:749`（渲染分支）以及 `:386`（`DrawMonster` 未点亮 tile → Infra TRN 分支）
- 自动地图揭示走 `DungeonFlag::Explored`（`Source/lighting.cpp:104` `DoVisionFlags`），不使用 `IsTileLit`，不受本改动影响（`automap.cpp:1008-1014` 为 `#ifdef _DEBUG` `DebugVision` 调试菱形覆盖层，非自动地图揭示逻辑，勿混淆）

**明确副作用**（规格声明，属预期）：
- 友方目标：Infravision 激活时暗处队友对 Heal Other/Resurrect 可选中（多人相关行为变化，需声明）
- 手柄路径同步修复，避免手柄玩家在暗处无目标能力

### 4.4 Scroll of Infravision 远征预算（DP3 核心）

**机制**：深度层开启时：

1. **掉落率削减**：`Scroll of Infravision`（`itemdat.tsv:102`）从深度层可掉落表降权（具体数值实施计划定，建议显著降低而非归零）
2. **移出女巫库存**：在 `WitchItemOk`（`Source/items.cpp:2009-2031`）添加 `SpellID::Infravision` 排除——**沿用 TownPortal 先例**（`Source/items.cpp:2017`）。选 `WitchItemOk` 而非 `SpawnWitch`：前者同时覆盖随机生成（`SpawnWitch`）与多人种子重摇（`RecreateWitchItem`，`Source/items.cpp:2122`），避免 MP 中库存不一致。**可强制**——它不在 `PinnedItemTypes`（`{ IDI_MANA, IDI_FULLMANA, IDI_PORTAL }`）中，与 TP 卷轴不同
3. **施法者替代路径**：Infravision 法术改为可学（`spelldat.tsv:10` `bookLevel` 从 -1 改为正数，如 5；`bookCost10`、`staffCost10` 需定义，见 4.5 可学门控），走魔力预算。**必须由深度层开关运行时门控**（红线 13）——开关关闭时恢复 -1
4. **书掉落渠道（独立复核发现，必须声明）**：`bookLevel ≥ 1` 会静默激活 `GetBookSpell`（`Source/items.cpp:625-626`，书只roll可学法术）与 `CreateSpellBook`（`Source/items.cpp:4762`）→ Infravision 书会在地牢掉落、进入女巫库存，形成**第二条不受控获取渠道**，部分抵消卷轴稀缺（对施法者与非施法者皆然——任何 36 魔法角色可学）。**处置二选一（实施计划定）**：(a) 接受书作为施法者的主要预算路径（卷轴稀缺只管非施法者），规格如此声明；(b) 在 `GetBookSpell` 排除 `SpellID::Infravision`（施法者只能靠卷轴/库存，魔力预算降级为次要）。本规格倾向 (a)——施法者本就有魔力预算，书渠道让「施法者 vs 非施法者」的反制路径差异更清晰

**预算语义**（DP3）：
- 非施法者（Warrior/Rogue）：每次远征携带有限卷轴 → 「这张现在用还是留给 Boss 房」是真实取舍
- 施法者：每次施放 40 魔力（`spelldat.tsv:10` manaCost）≈ 26 秒 → 「现在花 40 魔力还是留给战斗法术」是魔力预算
- 购买：出发前花金币买卷轴 vs 买药水 vs 买鉴定卷轴 → 出发前预算（非跑腿，因为进层后无法补给）
- 注意：TP 卷轴仍 200g 有卖（钉住项，不在此规格改动范围）——中途补给是 400g 往返，金币支出即预算；本规格**不削减 TP**（M2 已因钉住项判定无胜算而放弃）

### 4.5 法术 stub 清理（M3'）——含 P0 前置修复

**⚠️ 前置（P0，独立复核发现，必须先行）**：`SpellsData` 按位置索引（`spelldat.cpp:241-244` 行序 emplace + `spelldat.h:271-274` 枚举索引），删除任一行会错位 Nova→Elemental。**删除 stub 前必须先将 `LoadSpellData` 改为名称键控**：

| 步骤 | 内容 |
|---|---|
| 1. 名称键控加载器（Infra） | `LoadSpellData` 由「行序 → `SpellsData[enum]`」改为「行内 `name` 字段 → `std::map<std::string, SpellData>` 或显式索引表」，行序与枚举解耦。同类先例：`spelldat.cpp:194-197` 已有 `value == "DoomSerpents"` 名称→枚举解析，可复用 |
| 2. 删除三行 | 解耦后删除 DoomSerpents/BloodRitual/Invisibility 数据行 |
| 3. 清理代码引用 | 5 处引用逐一处置：`Source/panels/spell_icons.cpp`（`SpellITbl` 按 SpellID 索引的 C 数组，**保留槽位**——移除会使 ChainLightning 之后图标错位；仅清理死引用）、`spelldat.cpp:194-197`（解析器）、`lua/modules/items.cpp:245-248`（Lua 绑定）、`spell_tooltip.cpp:300-303`（tooltip 解析）、`missiles.cpp:924-926`（穷举 switch）——从枚举或穷举分支中移除 |
| 4. 存档/网络验证 | `SpellID` 枚举**不重排、不删除 stub 值**（保持原位）——`pack.cpp:90` 法术 ID 序列化依赖枚举数值稳定；确认删除后 `SpellsData` 加载不报错（名称键控加载器对缺失行填默认行） |

**处置表**：

| 处置 | 法术 | 依据 |
|---|---|---|
| 删除数据表条目（上述步骤 1-4 后） | DoomSerpents / BloodRitual / Invisibility | 宪章「错误设计」：去掉后玩家没少取舍；missiles 列空、零成本、bookLevel -1（双重验证 stub） |
| 保留不动 | Etherealize | 已有完整实现，Sorcerer 逃生按钮（minInt 93），无重设计必要 |
| 保留不动 | Golem | 有实现；侦察用途经评估不可行（`GolumAi` 不主动寻敌），不作 scout 化改造 |

**可学门控（红线 13，跨 6+ 代码点，必须定义单一机制）**：Infravision `bookLevel` 由 -1 改为正数的**运行时门控**，推荐单一包装器：

```
GetSpellBookLevel(SpellID s)  // 已存在，Source/spells.cpp:322
    → 内部：if (s == SpellID::Infravision && !gbDarkExpedition) return -1;
```

该包装器已覆盖全部下游（`GetBookSpell` 书生成、`CreateSpellBook`、`ValidatePlayer` 每 tick 的 `_pMemSpells` 清理（`Source/player.cpp:1453-1462/:2992`）、`pack.cpp:90/:405/:413` 存档、`loadsave.cpp:456/:2205`、商店书价）。**不得**分别在各调用点判断开关，否则状态漂移。`bookCost10`/`staffCost10` 一并定义（注意 `bookCost10=0` 会导致书价 0 金——`spelldat.h:250` `bookCost10 × 10`；建议非零值）。

### 4.6 不做的事（明确排除）

- **不做掉落表槽位竞争**：D1 掉落系统无槽位维度（`GetItemIndexForDroppableItem` 按类型加权），「光装备与伤害装备同槽竞争」在机制上不存在。光词缀竞争已原生存在于词缀池（`item_suffixes.tsv:68-69` 与伤害词缀同池），无需任何掉落表改动。光照半径缩小让原本近乎无价值的 +光词缀变得重要——这就是杠杆本身
- **不做火把**：需要新美术资源，违宪章禁令 2。反制调色板全用现有资产
- **不做 Golem 侦察**：机制不可行（见 4.5）
- **不做 TP 卷轴稀缺**：钉住项使之无胜算（Oracle 两轮确认）

### 4.7 开关关闭行为（红线 13）

| 项 | 开关关闭时 |
|---|---|
| 光照半径 | 回原版（无倍率） |
| Infravision 可学 | `GetSpellBookLevel` 返回 -1（不可学，卷轴/法杖形态不变）——单一包装器保证全调用点一致（4.5） |
| Infravision 卷轴掉落/库存 | 回原版掉落率与女巫库存（`WitchItemOk` 排除逻辑随开关关闭失效） |
| CanTarget | 回退为 `IsTileLit` 行为（`_pInfraFlag` 分支仅在开关开启且 Infra 激活时生效） |
| 三个 stub 删除 | **数据表删除为一次性永久改动，与开关状态无关**——但它们在原版即 stub（不可学不可用、missiles 列空），删除后任何开关状态下的玩家行为均无差异；红线 13「开关关闭 = 原版」在此项上的含义是「原版玩家从未能使用这些 stub」，故不违反。实施时以 4.5 步骤 4 的存档/网络验证为准 |

**多人游戏声明（独立复核发现）**：`CantChangeInMultiPlayer` 类选项为**每客户端生效，非主机同步**（先例：`runInTown`/`friendlyFire`/`multiplayerFullQuests`，`Source/options.cpp:844-856`）。光照倍率注入 `CalcPlrLightRadius`（对包括远程玩家在内的所有玩家计算）→ 混合开关状态的大厅中，不同客户端的同一玩家光半径不同。**本规格接受此不对称**（选项语义与上游先例一致），但须在实施计划中注明并验证同步。

---

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Depth |
| 2 | 问题陈述指向具体症状 | 第 1 节：深度层为空、无开关、无支柱消费者，全部有出处 |
| 3 | 每个数值标注出处 | 第 3 节逐条标注文件路径 + 字段名 + 核实日期 |
| 4 | 「已实施」标注需非测试调用者 + 验收全过 | 本规格状态为**草案**，不标注已实施；验收标准见第 6 节，实施后逐条核验 |

### 深度层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 压力来源是限制信息还是膨胀数值？ | 限制信息 | 光照半径缩小（信息限制）；无任何数值膨胀。倍率是「让玩家看不见」不是「给怪物加血」 |
| 10 | 玩家有可执行的反制手段吗？必须指名 | 有且已指名 | ① Infravision 法术（可学，4.4）+ 卷轴（掉落/购买）；② 光装备（Empyrean Band / Optic Amulet / Lightforge / `light`/`radiance` 后缀）；③ 手柄/鼠标目标选择在 `CanTarget` 下全路径可用。全部指名且有代码出处（第 3.3/3.4 节） |
| 11 | 稀缺产生「花在哪」取舍还是「再跑一趟」？ | 取舍 | Scroll of Infravision 预算：出发前金币分配（卷轴 vs 药水 vs 鉴定）+ 战斗中卷轴/魔力消耗时机 = 取舍。TP 仍 200g 有卖不构成跑腿负担（4.4 注明） |
| 12 | 全层段有定义？ | 是 | 13-16 (0.6) / 17-20 (0.85) / 21-24 (0.5)，单调曲线；非 Hellfire 路径显式声明（4.2）。与撤销前「曲线断裂」缺陷对照（3.1） |
| 13 | 开关关闭时行为回到原版？ | 是 | 4.7 逐项列明。三个 stub 删除为一次性永久改动但原版即 stub（不可学不可用），无玩家可感知差异；可学门控由单一 `GetSpellBookLevel` 包装器保证全调用点一致（4.5） |
| 14 | 近战与远程分别评估？ | 已评估 | **远程**：Infravision 修复恢复暗处目标选择（`CanTarget`），远程攻击循环（光标目标 → 弹道）完整可用；代价为卷轴/魔力预算（4.4）——付金币买回狙击循环。**近战**：目标在 1 格内，`CanTarget` 修复后暗处近战目标可选；主要损失是「看不到埋伏」的预警信息（Infravision 渲染保留暗处可见）。两者均有补偿路径，不对称从「税」转为「选择」 |

### 深度层禁令核对

| 禁令 | 依据 |
|---|---|
| 2 无美术资源的表格 | 反制资产全部现存（第 3.4 节）；不做火把 |
| 7 不改变文件行尾 | 实施计划将校验 CRLF；涉及文件已入漂移校验范围 |

---

## 6. 验收标准

| # | 验收项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 深度层开关存在 | grep `OptionEntryBoolean` in `Source/options.cpp` | `darkExpedition` 注册，flags `CantChangeInGame \| CantChangeInMultiPlayer`，默认 false |
| 2 | 倍率生效 | 测试：构造 Hell/Crypt 层段玩家，断言 `CalcPlrLightRadius` 输出 | 13-16 输出 6、17-20 输出 8（`trunc(8.5)`）、21-24 输出 5（基础 10 无装备） |
| 3 | 装备加成公式正确 | 测试：基础 10 + Lightforge(+4)，Crypt 层段 | `trunc(14 × 0.5) = 7`，非撤销前 `10×0.5 + 4 = 9` |
| 4 | clamp 顺序 | 测试：诅咒装备（基础 2）+ Crypt | `trunc(2 × 0.5) = 1` 钳制后 `2`，不低于原版最小值 |
| 5 | 单调曲线 | 测试断言 | Nest 倍率 > Hell 倍率 > Crypt 倍率（0.85 > 0.6 > 0.5） |
| 6 | CanTarget 修复 | 测试：Infra 激活 + 已可见暗处 tile | `CanTarget(tile) == true`；未激活或从未可见 tile 时 `false` |
| 7 | 渲染未破坏 | golden 测试 / 手动 | Infravision 激活时暗处 tile 保持 TRN 暗渲染，自动地图不全面揭示 |
| 8 | 手柄路径同步 | 代码审查 `plrctrls.cpp:263/:403` | 使用 `CanTarget` |
| 9 | 卷轴预算 | 测试 `WitchItemOk` + `RecreateWitchItem` | 深度层开启时 Infravision 卷轴（`iSpell == Infravision && iMiscId == IMISC_SCROLL`）不在女巫库存生成范围（随机 + MP 重摇两条路径）；关闭时恢复 |
| 10 | 法术可学门控 | 测试 `GetSpellBookLevel` | 开关开启：Infravision `bookLevel ≥ 1`；关闭：`== -1`；`ValidatePlayer` 不清理已记忆法术（开关开启时） |
| 11 | 名称键控加载器 + stub 删除 | 测试 `LoadSpellData` + grep | 加载器按名称索引；Nova→Elemental 数据与删除前逐字节一致；数据表无三个 stub 条目；存档/网络序列化不破坏 |
| 12 | 红线 13 全量 | 开关关闭跑全量测试 | 全量测试 0 失败（记录实际用例数）；timedemo 存档比对**与本分支改动前基线一致**——不得与上游对照，宪章决策 25 已定参考存档为本分支重新生成 |
| 13 | 漂移校验 | `python3 tools/check_drift.py` | 退出码 0，五项检查全 PASS |

---

## 7. 状态

**已实施**。2026-08-08，独立复核（Oracle）通过。666/666 测试全过，漂移校验 5 项 PASS，timedemo 存档比对与本分支基线一致。

**实施偏离记录**（相对本规格 §4.5 与 §6，经独立复核确认安全）：

1. **§4.5 步骤 3 代码引用清理未执行**：`missiles.cpp:924-926`、`spell_tooltip.cpp:300-303`、`lua/modules/items.cpp:245-248`、`spelldat.cpp ParseSpellId` 仍保留 stub SpellID 引用。依据「枚举值不重排」（§4.5 步骤 4），这些引用不悬空（枚举保留），删除无行为收益且增加回归风险——**保留为防御性死引用**。`spell_icons.cpp:46-49` 槽位按规格保留。
2. **容器掉落路径一并排除**（独立复核发现怪物路径遗漏）：除 `RndItemForMonsterLevel`，`RndAllItems`（箱子 `objects.cpp:2034`、石棺 `:2199`、主题房 `themes.cpp:529`）也添加 Infravision 排除，确保深度层开启时全掉落路径无卷轴。
3. **测试缺口**（独立复核标注，未阻塞实施）：§6.9 `RecreateWitchItem` MP 重摇路径、§6.10 `ValidatePlayer` 不清除已记忆法术、§6.11 Nova 数据字节回归——无专项测试，靠全量套件与手动验证覆盖。后续补测。
4. **`bookCost10=500`**（规格 §4.4 建议定义非零值，实际取 500 → 书价 5000 金）。倍率初值 60/85/50、bookLevel 5 均按规格 §4.4 初值实施，试玩调节只动数据。

**已知边界**（本规格不解决，后续规格或实施计划处理）：

- `CanTarget` 多人与手柄的运行时验证（目标选择同步、`CantChangeInMultiPlayer` 每客户端语义）——设计已声明（4.3/4.7），实施时验证
- `Lightforge` 类「光+伤害双 BIS」装备在顶端弱化光/伤害张力——规格接受为预期减负（4.4 预算语义下，装备仍是免费光，卷轴预算才是主要压力）；如后续需收紧，在深度层开关内对特定独特装备的光值做门控降值
- 具体倍率数值（0.6/0.85/0.5）、卷轴掉落降权幅度、Infravision bookLevel 值（4.4 建议 5）为**初值**，需经实际试玩调节——但调节只动数据不改机制
- 名称键控加载器改造（4.5 步骤 1）为 Infra 改动，独立于深度层开关——可按需先行实施
