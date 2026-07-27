# Better D1 设计宪章

**日期**：2026-07-27
**状态**：已批准
**分层**：元设计（本文档定义分层规则本身，不参与分层判定）
**取代**：`2026-07-06-better-d1-design-system.md`、`better-d1/design-decision-template.md`、`better-d1/README.md`

本文档为活文档。修订以追加决策记录（第 11 节）的方式进行，不另开新版本文件。

---

## 0. 本文档是什么

宪章是**后续每一份设计规格的评判基准**。它定义 Better D1 是什么、改动分几层、每层允许做什么、文档必须包含什么。

宪章不描述任何具体系统。具体系统各有自己的规格文件，每一份都必须通过本文档第 3 节的红线清单。

### 为什么需要它

前一版设计体系失败了，失败方式值得记录，因为它决定了本宪章的形态：

- **两套五支柱共存三周无人发现。** `2026-07-06-better-d1-design-system.md` 写的是「决策重量 / 信息稀缺 / 世界反应 / 简单但深刻 / 节奏感」；`better-d1/design-decision-template.md` 和 `better-d1/designs/combat-system.md` 写的是「不安感 / 稀缺是美德 / 环境是角色 / 简单但深刻 / 尊重玩家」。只有一条重合。红线清单同样有两份（8 条 vs 9 条）。它们能长期共存，是因为**支柱从未约束过任何一次真实决策**。
- **支柱检查表可以全票通过一篇零内容文档。** `combat-system.md` 通篇是「改进方向」，无一条可落地规格，其支柱自检表五项全 ✅。
- **验证标准不可证伪。** 全部成功标准形如「玩家会犹豫吗？→ 是」，且明确排除了唯一可测的代理指标（死亡次数、药水消耗量）。这套框架没有任何途径被证明是错的。
- **文档与代码事实漂移。** 见第 8 节事实勘误表。

结论：**宪章必须由可判定命题构成。** 需要品味判断的地方只允许有一处——决定深度层要不要某个改动——其余全部靠查代码或查文档回答。

---

## 1. 定位与两层结构

### 一句话定位

> Better D1 是面向 D1 老玩家的 DevilutionX mod。底座层让 D1 更易读、更少摩擦，且不改变任何平衡；深度层用限制信息而非膨胀数值的方式重塑压力曲线，默认关闭。

### 明确废弃的定位框架

以下内容从设计体系中移除，不得在任何设计规格中复现：

| 废弃内容 | 理由 |
|---|---|
| Steam 商店描述 | 本项目基于不可再分发的暴雪素材，LICENSE 禁止商业使用，不会发行 |
| 竞品对标表（D2R / Path of Exile / Hades） | 把「卖点」偷换成决策依据 |
| 「2026 年的独立游戏」框架 | 类别错误。这是 DevilutionX 的 mod |
| 目标玩家画像（「不是画面党」「不是休闲玩家」） | 以排除他人定义自己，不产生任何设计约束 |

评判者是装了这个 mod 的 D1 老玩家，以及作为第一个用户的作者本人。

### 两层定义

| | 底座层 Base | 深度层 Depth |
|---|---|---|
| 默认 | 开启，不可关 | 关闭 |
| 目标 | 消除摩擦 | 重塑压力曲线 |
| 硬约束 | 不改变任何平衡 | 手段限定为限制信息，禁止数值膨胀 |
| 受众 | 全部老玩家，含不想要难度变化的人 | 想要不同体验的人 |

### 深度层开关

`GameplayOptions` 中一个 `OptionEntryBoolean`，flags 为 `CantChangeInGame | CantChangeInMultiPlayer`，默认 `false`。

DevilutionX 已有同类先例，不需要发明机制：

- `theoQuest` — `OptionEntryFlags::CantChangeInGame | OnlyHellfire`，默认 `false`（`Source/options.cpp:847`）
- `multiplayerFullQuests` — `OptionEntryFlags::CantChangeInMultiPlayer`，默认 `false`（`Source/options.cpp:850`）
- flags 枚举定义见 `Source/options.h:102`–`119`

**只有一个开关，不提供逐特性配菜。** 开了就是一整套。

理由：逐特性开关产生 2^n 种未测试的组合；而且「全都做成选项」是逃避设计决策——如果深度层里某一项必须单独关掉才能玩，那说明那一项设计错了，应该修它，不是给它一个开关。

---

## 2. 分层判定规则与支柱

### 分层判定规则

改动是否触及以下任一项？

1. 任何 TSV 数据文件中的数值字段（`itemdat` / `monstdat` / `spelldat` / `attributes.tsv` 等）
2. 掉落概率或掉落表构成
3. 玩家或怪物的属性、伤害、命中、抗性、生命、法力计算
4. 光照半径、视野范围、怪物激活或仇恨判定
5. 商店库存、价格、可购买清单
6. 战斗中可即时使用的资源量（腰带容量、即时可用消耗品数量）

**是 → 深度层。否 → 底座层。无例外条款。**

无例外条款是刻意的。「因为我认为没人用这个东西所以可以改平衡」和「因为我认为要补偿掉落削减所以加 25% 金币」是同一种推理，后者就是从这个口子进来的。规则的全部价值在于它不留后门，包括不给作者本人留。

一个已知的不舒服后果：取消符文和伤害卷轴的掉落是纯减负（这两类物品几乎无人使用），但它改变掉落表构成，因此属于深度层。开关关闭时符文照旧掉落——它本来就无害。

### 底座层支柱

**B1 · 未知必须可以被行动解决**

保留玩家能通过游戏内行动（探索、鉴定、点亮、靠近观察）解决的未知；消除只能靠背诵、查 wiki 或大量重复样本才能解决的未知。后者不是设计，是噪声。

判定问法：玩家能通过游戏内的什么行动知道这件事？如果答案是「查 wiki」或「打一千次记住」，就该显示出来。

| 未知 | 可用行动 | 判定 |
|---|---|---|
| 门后有什么 | 开门 | 保留 |
| 物品好不好 | 鉴定 | 保留 |
| 黑暗里有什么 | 靠近 / 光源装备 / 点亮 | 保留 |
| 怪物剩余生命 | 无 | 消除（可显示） |
| 法术魔法需求 | 背诵 / 查 wiki | 消除（应显示） |
| 伤害公式、掉落率 | 查 wiki / 上千次样本 | 消除（应显示） |

这条同时为光照压制提供了理论依据：黑暗之所以是好的稀缺，正因为它是可行动的未知。由此直接推出——**没有反制手段的黑暗退化为不可行动的未知，也就是噪声**。深度层支柱 DP2 不是额外的仁慈条款，是本条的推论。

**B2 · 消除摩擦，不消除决策**

减少重复操作（背包整理、来回跑腿、背数值），但不代替玩家选择。堆叠背包是消除摩擦；取消腰带红瓶/蓝瓶配比是消除决策。

**B3 · 不改变平衡**

纸面上对战斗结果、难度曲线、战斗中的战术选择零影响。

### 深度层支柱

深度层支柱编号加前缀 `DP`，以免与游戏名 D1 混淆。

**DP1 · 用限制信息制造压力，不用膨胀数值**

想让第 13 层更难，手段是让玩家看不见，不是给怪物加生命。

**DP2 · 每个限制都要有反制**

玩家无法应对的限制不是决策，是税。反制手段必须指名具体装备、消耗品或行为。

**DP3 · 稀缺产生预算，不产生跑腿**

资源紧张必须转化为「花在哪」的取舍，不是「再回城一趟」。

### 废弃的旧支柱

| 旧支柱 | 处置 | 理由 |
|---|---|---|
| 节奏感 | 删除 | 从未被任何决策引用，无可判定内容 |
| 尊重玩家 | 删除 | 同上 |
| 简单但深刻 | 删除 | 所有设计文档都会写的万能句，判定不了任何东西。其可执行部分（不增加玩家要记的概念）移入第 3 节禁令 4 |
| 世界对玩家有反应 / 环境是角色 | 删除 | 对应的三篇设计文档均为无机制的愿望，已裁决删除（第 5 节） |
| 决策重量 / 稀缺是美德 / 不安感 | 吸收 | 拆解为 B2、D1、D3 的可判定形式 |
| 信息是有价值的资源 | 重写 | 原表述推出错误结论（会禁止怪物生命条，而它是上游基础 QoL）。重写为 B1 |

---

## 3. 红线清单与禁令清单

原先「支柱检查表 + 红线清单」是两张表问同一批不可判定的问题，合并为一张。每一条都必须能靠查代码或查文档回答。

### 通用红线（任何设计决策）

| # | 判定 | 必须 |
|---|---|---|
| 1 | 按第 2 节 6 条规则判定分层，输出 Base 或 Depth | 已判定 |
| 2 | 问题陈述是否指向具体症状（可复现的操作序列，或具体数值 + 出处），而不是「感觉不够深」？ | 是 |
| 3 | 文档引用的每个数值是否标注出处（文件路径 + 字段名）？ | 是 |
| 4 | 标注为「已实施」的每个函数是否有非测试调用者？ | 是 |

### 底座层红线

| # | 判定 | 必须 |
|---|---|---|
| 5 | 是否改变战斗结果、难度曲线，或玩家在战斗中的战术选择？ | 否 |
| 6 | 被消除的未知是否属于「无法通过游戏内行动解决」？ | 是 |
| 7 | 是否代替玩家做了任何选择？ | 否 |
| 8 | 是否移除了玩家已有的选项？ | 否 |

### 深度层红线

| # | 判定 | 必须 |
|---|---|---|
| 9 | 压力来源是限制信息还是膨胀数值？ | 限制信息 |
| 10 | 玩家有可执行的反制手段吗？必须指名具体装备/消耗品/行为 | 有且已指名 |
| 11 | 稀缺产生的是「花在哪」的取舍，还是「再跑一趟」？ | 取舍 |
| 12 | 该改动在全部层段（1–16 / 17–20 Nest / 21–24 Crypt）都有定义吗？ | 是 |
| 13 | 开关关闭时行为是否回到原版？ | 是 |

### 禁令清单（无条件，不需要判定）

1. 不写未实现的内容而不标注状态
2. 不写无法落地的表格——需要不存在的美术资源的、有空格的、纯愿望的
3. 不引用未标注出处的数值
4. 不新增玩家必须记住的概念
5. 红线检查不得只填 ✅，必须给出具体依据
6. 不用占位测试（`EXPECT_TRUE(true)`）充当验证

禁令 2 和 6 各有现成违例，见第 8 节。

---

## 4. 文档规范

### 目录结构

```
docs/superpowers/
  specs/
    2026-07-27-better-d1-design-charter.md   ← 本文档
    <YYYY-MM-DD>-<topic>-design.md           ← 系统设计规格，扁平存放
  plans/
    <YYYY-MM-DD>-<topic>.md                  ← 实施计划
  archive/
    specs/  plans/                            ← 被取代或废弃的文档
```

**不设索引文件。** 日期前缀 + 扁平目录，目录列表本身就是索引。索引文件是又一个会漂移的真相来源，前一版的 `better-d1/README.md` 已经漂移（12 个连状态都没有的空条目）。

**归档约定**：归档时保留原文件名，设计规格置于 `archive/specs/`，实施计划置于 `archive/plans/`，并在文件头首行加取代或废弃说明。子目录结构不保留——`better-d1/designs/environment-lighting.md` 归档后为 `archive/specs/environment-lighting.md`。

设计决策模板不单独成文，本节即模板。同内容两份必然漂移，这正是两套五支柱的成因。

### 系统设计规格必备段落

7 段，缺任一段不得进入评审。

| 段 | 要求 |
|---|---|
| 问题陈述 | 具体症状：可复现的操作序列，或具体数值 + 出处。禁止「感觉不够深」 |
| 分层判定 | Base 或 Depth，附第 2 节 6 条规则的逐条判定结果 |
| 事实基础 | 引用的每个数值一行，格式见下 |
| 方案 | — |
| 红线检查 | 通用 4 条 + 所在层红线，每条给依据，不得只填 ✅ |
| 验收标准 | 可执行的验证方式：测试名，或手动复现步骤 |
| 状态 | 见下方枚举 |

**豁免**：UI 语言规范类文档（如 `2026-07-05-spell-tooltip-ui-language.md`）不适用 7 段结构，但仍受禁令清单约束。

### 数值出处格式

```
ManaShield 魔法需求 = 25 — assets/txtdata/spells/spelldat.tsv:minIntelligence（2026-07-27 核实）
玩家基础光照半径 = 10 — Source/player.cpp:2320 _pLightRad（2026-07-27 核实）
```

### 状态枚举

| 状态 | 定义 |
|---|---|
| 草案 | 未评审 |
| 已批准 | 评审通过，未实施 |
| 实施中 | 部分落地 |
| 已实施 | **所声明的每个函数都有非测试调用者，且验收标准全部通过** |
| 已废弃 | 移入 `archive/`，文件头标注被哪份文档取代 |

「已实施」的定义是硬的。仅有测试调用者不算已实施——测试可以在测死代码，这已经发生过四次（第 7 节）。

---

## 5. 文档裁决

### 保留（补分层判定与状态标注，内容不动）

| 文档 | 分层 | 状态 |
|---|---|---|
| `2026-06-29-floating-info-ui-design.md` | Base | 已实施 |
| `2026-07-05-spell-tooltip-system-v2-design.md` | Base | 已实施 |
| `2026-07-05-spell-tooltip-ui-language.md` | 规范类（豁免 7 段结构） | 生效 |
| `2026-07-06-skill-descriptions-v2-design.md` | Base | 已实施 |

`spelldat.tsv:description`（风味文本）与 `spelldesc.tsv`（`section` / `priority` / `formatType` / `source` / `textKey` / `formulaText` 展示配置）是分层互补关系，不是重复。两篇 v2 文档均有效。

### 删除（不归档）

| 文档 | 理由 |
|---|---|
| `better-d1/designs/narrative-system.md` | 50 行纯愿望，零可落地内容 |
| `better-d1/designs/combat-system.md` | 纯愿望 + 全 ✅ 假自检表 |
| `better-d1/designs/living-dungeon.md` | 纯愿望 + 35 格中 15 格为空的对话矩阵 + 依赖已删除的 `world_state` |
| `better-d1/designs/resource-system.md` | 属性上限事实迁入待立项的法术实用性规格；三张职业稀缺曲线表是无机制的愿望 |
| `better-d1/design-decision-template.md` | 折叠进本文档第 4 节 |
| `better-d1/README.md` | 索引由目录列表承担 |
| `plans/2026-07-07-better-d1-implementation.md` | 1261 行，含两个冲突的 `GetMaxStackCount` 签名、两处 `EXPECT_TRUE(true)` 占位测试、以及勾选了「All test code included」「No TBD/TODO in plan」的假自审清单。**不归档**：归档价值是可追溯，但内容是错的，留着会被当参考 |

删除后 `better-d1/` 目录整层消失。

### 归档（`archive/`，文件头标注取代关系）

| 文档 | 理由 |
|---|---|
| `2026-07-05-skill-descriptions-design.md` | 被两篇 v2 取代 |
| `2026-07-06-better-d1-design-system.md` | 被本文档取代 |
| `2026-07-06-wire-up-skeleton-code.md` | 一次性清理任务，大部分已执行；未完成项（怪物激活半径接线）转入待立项清单 |
| `plans/2026-06-29-floating-info-ui.md` | 已执行 |
| `plans/2026-07-05-skill-descriptions.md` | 已执行且被 v2 取代 |
| `plans/2026-07-05-spell-tooltip-system-v2.md` | 已执行 |
| `plans/2026-07-06-skill-descriptions-v2.md` | 已执行 |
| `plans/2026-07-07-consumable-stacking.md` | 已执行 |

### 归档并标注「含已核实错误，勿作参考」

以下三篇的对应机制仍在生效或仍有价值，但文档内容含已核实错误，本次不重写，移入 `archive/` 并在文件头加醒目标注，由待立项清单指向：

| 文档 | 已核实的错误 |
|---|---|
| `better-d1/designs/environment-lighting.md` | 光半径基数写 8 实际 10；「火把半径」列为虚构（静态火把烘焙在地图数据，代码从未压制）；标注「已实现」的怪物激活半径零生产调用者；17–20 层未定义 |
| `better-d1/designs/consumable-system.md` | 「掉落设计」节为回城/识别卷轴掉落提供设计理由，与「取消符文及伤害卷轴掉落」的决策相反 |
| `better-d1/designs/spell-system.md` | 三处数值错误，见第 8 节 |

`2026-07-07-consumable-stacking-design.md` 同样归档并标注：其职业差异化堆叠部分已裁决撤销，且腰带堆叠形态已裁决改为背包堆叠。

---

## 6. 已落地代码裁决

| 改动 | 出处 | 触及规则 | 分层 | 裁决 |
|---|---|---|---|---|
| 金币掉落 +25% | `Source/items.cpp:3178` `rndv = rndv * 5 / 4` | 第 2 条 | Depth | **撤销**。经济重构留待待立项的消耗品经济规格 |
| 职业差异化堆叠上限 | `Source/items.cpp:5146` `GetMaxStackCount` | 第 6 条 | Depth | **撤销加成**，统一药水 5 / 卷轴 3。另违反深度层红线 11 |
| 腰带消耗品堆叠 | `Source/items.h:258` `_iStackCount`；`Source/inv.cpp` 腰带逻辑 | 第 6 条 | Depth | **改为背包堆叠、腰带不堆叠**（待立项）。见下方裁决理由 |
| 光照压制 | `Source/lighting.cpp:594`、`:620` | 第 4 条 | Depth | **移入深度层开关**（待立项）；缺陷一并修复 |
| 抽象金币计数器 | commit `0171b2751`；`Source/inv.cpp:2340` `CalculateGold` | 无 | Base | **保留**，补写设计文档（当前零文档） |
| `floatingInfoBox` 硬编码 `true` | `Source/items.cpp:1632`、`:1637` | 无 | Base | **撤销**，恢复读取选项。违反底座层红线 8 |
| 法术需求文本接线 | `Source/panels/spell_book.cpp` | 无 | Base | **保留**。符合 B1：魔法需求只能靠背或查 wiki |
| tooltip / 物品对比 / 浮动信息面板 | `Source/spell_tooltip.cpp` 等 | 无 | Base | **保留** |

### 腰带堆叠的裁决理由

腰带 8 格的红瓶/蓝瓶配比，是 D1 战斗准备阶段少数真实的决策之一。腰带堆叠把「8 个总量」变成「8 种类型 × N」，取消了这个取舍，违反 B2。

背包堆叠不违反 B2：背包里的消耗品在战斗中不能直接使用，堆叠只消除「背包被 20 个药水占满」的记账摩擦，不影响战斗中的战术选择。

同一推理解释了为什么抽象金币计数器判为 Base：金币占背包格没有对应策略，玩家唯一的应对是回城存钱；而且金币占格不是 1996 年的设计意图，是「金币被实现为 `Item`」的副作用。

### 宪章曾在此处自相矛盾（已修）

初稿的分层判定只有 5 条规则，腰带堆叠不触及任何一条 → 判为 Base；但底座层红线 5 原文为「不改变战斗结果、**资源收支**或难度曲线」，腰带堆叠把可即时使用的药水从 8 个变成 40 个 → Base 不允许它。规则说是 Base，红线说 Base 不能有。

修正：判定规则增加第 6 条「战斗中可即时使用的资源量」；红线 5 由笼统的「资源收支」收紧为「战斗结果、难度曲线，或玩家在战斗中的战术选择」。修正后全部判定一致。

---

## 7. 事实基础

本节数值为本文档所有论断的依据，均于 2026-07-27 核实。

### 职业属性上限

| 职业 | maxMag | 出处 |
|---|---|---|
| Warrior | 50 | `assets/txtdata/classes/warrior/attributes.tsv:maxMag` |
| Rogue | 70 | `assets/txtdata/classes/rogue/attributes.tsv:maxMag` |
| Sorcerer | 250 | `assets/txtdata/classes/sorcerer/attributes.tsv:maxMag` |

### 法术魔法需求（`assets/txtdata/spells/spelldat.tsv:minIntelligence`）

| 法术 | minIntelligence |
|---|---|
| Firebolt | 15 |
| TownPortal | 20 |
| ManaShield | 25 |
| Flash | 33 |
| BoneSpirit | 34 |
| Phasing | 39 |
| Fireball | 48 |
| StoneCurse | 51 |
| Elemental | 68 |
| BloodStar | 70 |

魔法上限 50 的战士可学 22 个法术（共 36 个有 `minIntelligence` 值的条目），其中包含 Fireball。

### 光照

| 事实 | 值 | 出处 |
|---|---|---|
| 玩家基础光照半径 | 10 | `Source/player.cpp:2320` `_pLightRad = 10` |
| 压制倍率函数 | 1–4:1.0 / 5–8:0.9 / 9–12:0.8 / 13–16:0.6 / 21–24:0.5 / 其余 default 1.0 | `Source/lighting.cpp:594` `GetLightSuppressionMultiplier` |
| 有效光半径函数 | `static_cast<int>(10 * multiplier) + (_pLightRad - 10)` | `Source/lighting.cpp:620` `GetEffectiveLightRadius` |
| 有效光半径调用点 | 4 处 | `Source/lighting.cpp:343`、`Source/msg.cpp:2344`、`Source/loadsave.cpp:1949`、`Source/player.cpp:2510` |
| 静态火把/蜡烛 | 烘焙在地图数据，代码从未压制 | `GetLightSuppressionMultiplier` 唯一调用点是 `GetEffectiveLightRadius`，后者只接受 `Player` |

### 仅有测试调用者的函数（按第 4 节定义均非「已实施」）

| 函数 | 定义 | 声明 | 测试 |
|---|---|---|---|
| `GetMonsterActivationRadius` | `Source/monster.cpp:5069` | `Source/monster.h:594` | `test/monster_activation_test.cpp` |
| `DoesAdriaOfferSpellChoice` | `Source/stores.cpp:2892` | `Source/stores.h:134` | `test/quest_reward_test.cpp` |
| `DoesPepinGiveRegenerationPotion` | `Source/stores.cpp:2897` | `Source/stores.h:140` | `test/quest_reward_test.cpp` |
| `DoesGriswoldOfferCustomWeapon` | `Source/stores.cpp:2902` | `Source/stores.h:146` | `test/quest_reward_test.cpp` |

### 上游基础 QoL 参照

| 项 | 事实 | 出处 |
|---|---|---|
| 怪物生命条 | 比例条，无绝对数值；默认 `false`；flags `None` | `Source/options.cpp:858`；`Source/qol/monhealthbar.cpp` |
| 背包格数 | 40，与上游一致，本分支未改 | `Source/player.h:37` `InventoryGridCells` |

---

## 8. 事实勘误表

前一版文档中已核实的错误，删除或归档时不再修正，此处留档以说明宪章第 3 节红线 3 的必要性。

| 文档 | 错误 | 实际 |
|---|---|---|
| `spell-system.md` | ManaShield 需 45 魔法，战士「勉强」 | 25，战士轻松可学 |
| `spell-system.md` | Elemental 需 70 | 68 |
| `spell-system.md` | Fireball 与 Mana Shield 列为法师专属 | 48 / 25，魔法上限 50 的战士均可学 |
| `environment-lighting.md` | 表格以玩家基础光半径 8 计算 | 代码为 10（`player.cpp:2320`），且 `GetEffectiveLightRadius` 硬编码基数 10 |
| `environment-lighting.md` | 「火把半径」列随层压制（5→4→4→3→2） | 静态火把烘焙在地图数据，从未被压制。该列为虚构 |
| `environment-lighting.md` | 怪物激活半径「已实现」 | 零生产调用者 |
| `living-dungeon.md` | 怪物激活半径「✅ 已实现」 | 同上 |
| `living-dungeon.md` | 世界状态跟踪「⚠️ 骨架」 | 已于 commit `ddda610f6` 删除 |
| `environment-lighting.md` | 压制层段覆盖 1–16、21–24 | 17–20 层（Nest）落入 `default` 返回 `1.0f`，比 Hell 的 `0.6f` 更亮，压制曲线在此断裂 |
| `2026-07-07-better-d1-implementation.md` | 自审勾选「All test code included」「No TBD/TODO in plan」 | 含 `gold_drop_test`、`room_decoration_test` 两处 `EXPECT_TRUE(true)` 占位测试 |
| `2026-07-07-better-d1-implementation.md` | Phase 1 与 Phase 2 各自定义 `GetMaxStackCount` | 两个冲突签名。实际代码为 `GetMaxStackCount(const Item&, const Player&)` |
| `living-dungeon.md` | NPC 对话矩阵 7 NPC × 5 状态 | 35 格中 15 格为空；视觉变化表自注「需要新美术资源」 |

另有一处代码缺陷，非文档错误，留给待立项的环境光照规格处理：`GetEffectiveLightRadius` 以 `equipmentBonus = _pLightRad - 10` 计算装备加成，当诅咒装备使 `_pLightRad < 10` 时该值为负，压制被叠加两次。

---

## 9. 本次范围与待立项清单

### 本次范围

- 本文档写入并提交
- 文档重组：按第 5 节执行删除、归档、标注
- 为第 5 节「保留」的 4 篇文档补写分层判定与状态标注（内容不动）
- 代码撤销三项：金币 +25%（`items.cpp:3178`）、职业堆叠加成（`items.cpp:5146`）、`floatingInfoBox` 硬编码（`items.cpp:1632`、`:1637`）
- 删除三个仅有测试调用者的任务奖励函数及 `test/quest_reward_test.cpp`

任务奖励设计（Adria 法术选择、Pepin 再生药剂、Griswold 定制武器）本身有价值，但改变平衡属 Depth，且需要真实的商店 UI 交互设计。删除骨架，概念留待未来独立立项时重新推导。

### 本次不做

以下均为改造而非撤销，各自独立立项：深度层开关实现、光照压制移入开关、腰带堆叠改背包堆叠、三篇 designs 的内容重写。

**已知违规的中间状态**：本次结束后，光照压制与腰带堆叠仍在底座层无条件生效，违反 B3。此处如实标注，不作合规假装——旧文档标注「✅ 已实现」的做法正是其反面。

### 待立项清单

| 待立项 | 分层 | 优先级 | 依赖 | 原文件 |
|---|---|---|---|---|
| 深度层开关实现 | 基础设施 | 1 | — | — |
| 环境光照重写：Nest 断裂、光半径基数、`equipmentBonus` 负值、反制手段、怪物激活半径去留 | Depth | 2 | 深度层开关 | `archive/specs/environment-lighting.md` |
| 腰带堆叠改背包堆叠 | Base | 3 | — | `archive/specs/2026-07-07-consumable-stacking-design.md` |
| 抽象金币计数器补写文档 | Base | 4 | — | 无（commit `0171b2751`） |
| 消耗品经济重构：取消符文及伤害卷轴掉落，重新推导补偿形状 | Depth | 5 | 深度层开关 | `archive/specs/consumable-system.md` |
| 法术实用性：Rage / Etherealize / Golem | Depth | 6 | — | `archive/specs/spell-system.md` |

消耗品经济的补偿形状需重新推导，不得沿用 +25%：取消掉落的痛点集中在前期（玩家贫穷，捡到的回城卷轴是救命的），而按比例放大金币收入在前期绝对值小、后期绝对值大，恰好在不需要补偿处补得最多。

---

## 10. 验证方式

宪章的成功标准关于产物，不关于玩家感受。旧文档「玩家会犹豫吗？→ 是」那套全部废弃。

| 验证项 | 方法 | 通过标准 |
|---|---|---|
| 支柱单一真相来源 | 在 `docs/` 下 grep 支柱名称 | 只出现在本文档一处 |
| 「已实施」标注为真 | 对每个标注已实施的文档，grep 其声明函数的非测试调用者 | 全部有 |
| 数值出处完整 | 抽查设计文档中的数值 | 无未标注出处的数值 |
| 目录整洁 | 本次清理完成时执行 `ls docs/superpowers/specs/` | 只含本文档 + 第 5 节保留的 4 篇；无 `better-d1/` 目录 |
| 撤销生效 | `grep "rndv \* 5 / 4" Source/items.cpp` | 无匹配 |
| 职业堆叠加成撤销 | 读 `Source/items.cpp:5146` `GetMaxStackCount` | 无 `HeroClass` 分支 |
| 选项恢复 | `grep floatingInfoBox Source/items.cpp` | 有匹配 |
| 死代码清零 | grep 第 7 节四个函数名 | 仅 `GetMonsterActivationRadius` 保留并标注待决 |
| 构建与测试 | 项目既有构建与测试流程 | 通过 |

### 不测试的维度

不设「好不好玩」「玩家是否紧张」这类主观维度的验证项。它们无法证伪，前一版文档因此产出了一篇零内容却全票通过支柱检查的设计文档。

---

## 11. 决策记录

| # | 决策 | 状态 |
|---|---|---|
| 1 | 从第一性原理重推导设计，代码裁决写入规格，实际改动交由后续实施计划 | 已定 |
| 2 | 定位为面向 D1 老玩家的 mod，作者本人是第一个用户 | 已定 |
| 3 | 两层结构：QoL 底座 + 单一总开关的深度层 | 已定 |
| 4 | 宪章由可判定命题构成；事实漂移的机械校验脚本延后独立立项 | 已定 |
| 5 | 金币 +25% 撤销 | 已定 |
| 6 | 消耗品掉落取消范围暂定为符文 + 伤害类卷轴，零补偿 | **暂定**，经济重构规格产出后重审 |
| 7 | 本次范围限于宪章 + 文档重组 + 撤销类代码改动 | 已定 |
| 8 | 目录扁平化，删除 `better-d1/` 整层，不设索引文件 | 已定 |
| 9 | `better-d1-implementation.md` 直接删除，不归档 | 已定 |
| 10 | 腰带堆叠改为背包堆叠、腰带不堆叠 | 已定 |
| 11 | 底座层支柱 1 由「规则透明，世界不透明」重写为「未知必须可以被行动解决」 | 已定 |
| 12 | 分层判定规则增加第 6 条；底座层红线 5 收紧 | 已定 |
