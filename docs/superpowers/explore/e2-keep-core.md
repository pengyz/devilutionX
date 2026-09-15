# E2 坚守 D1 核心体验：应该强化什么——报告

## 1. 结论摘要

1. **核心体验的可辩护定义（我推断，但从代码约束反推）**：D1 的体验是"**在信息不足的空间里做不可回滚的资源与路线决策**"。三条引擎级支撑：单机已访问层不重生成（`Source/diablo.cpp:3162` 的 `!_pLvlVisited` 门控）→ 空间是记忆而非刷新场；光照/视野是渲染与选目标的硬门控（`scrollrt.cpp:388/471/487/751`、`gendung.cpp:329`）→ 信息本身是资源；死亡代价是"掉一半金币 + 装备落地可捡"而非删档（`player.cpp:2672`）→ 惩罚是**后勤挫折**，不是终止。**强化 = 加深这三条，不是加数值、不是加单命**。
2. **最大杠杆仍是采样与编组，但"坚守体验"的正确表述不是"多几类怪"，而是"让层的构成可被读出"**。前提已定（`monstimgtot < 4000`，`monster.cpp:3516`；后期实得 2-4 类）。真正伤害体验的不是类型少，而是散布层把每次投放当独立随机事件（`monster.cpp:3773-3782`：随机取一类 + `PlaceGroup(typeIndex, na)` 无 leader/无 leashed），玩家看到的是"同类怪贴在一起"而非"一支队伍"。
3. **编组层是已存在的、被闲置的正确原语**：`PlaceGroup` 已支持 `leader`/`leashed`、minion HP×2、继承 leader `intelligence`、`setLeader` 与 `packSize` 维护（`monster.cpp:308-380`），且已有分离/重聚状态机（`:1676-1719` 的 `LeaderRelation::Leashed/Separated`）。**普通散布只是没传后两个参数**——这是"零新机制、零新美术"的最高 ROI 项。
4. **地形语义的真实缺口比任务书描述的更结构化**：主题系统实际是"随机再摇到装得下的为止"（`themes.cpp:851-853`、`:876-882`：`while (!SpecialThemeFit(...)) j = (theme_id)GenerateRnd(17);`）→ 主题与"层是什么地方"零相关。**同 tileset 的 4 层生成参数确实几乎相同**：`drlg_l1/l2/l3` 各只有 5-6 处 `currlevel` 引用，且全部用于任务/楼梯/种子（`drlg_l2.cpp:2449/2668/2681`、`drlg_l3.cpp:1945/1961/1968`），仅 `drlg_l4` 有 18 处（L15/16 特殊）。
5. **任务书两处描述需修正**（见 §2.1）：`THEME_MONSTPIT`/`THEME_BARREL` **不是不可达**，而是**只能通过 fallback 再摇命中**（首选池 `ThemeGood[4]` 只有 4 个，命不中就 `GenerateRnd(17)` 全域重摇）；L16/Nest/Crypt 整体跳过主题**确认成立**（`themes.cpp:839`、`:894`、`:906` 三处同一 guard）。
6. **资源预算纵深的唯一诚实做法是"移动补给的时间轴"，不是改掉落数值**：Pepin 全额免费治疗（`stores.cpp:1018-1027`：`_pHitPoints = _pMaxHP`，无花费）把"回城"变成零成本重置；这是预算纵深的真正漏点，比药水掉率（`items.cpp:1568`）更结构性。
7. **信息可交易化已有完整落点**：`OperateShrineSecluded` 已在做"一次性全图揭示"（`objects.cpp:2732-2740` 全 `DMAXX×DMAXY` 打 `MAP_EXP_SHRINE`），`AutomapView` 有 5 级来源枚举（`automap.h:20-28`）却只用来选颜色（`automap.cpp:984-997`）。**"信息有来源、来源有可信度"这个层已经建好了地基，但没有任何机制消费它**。
8. **层段压力主题化应实现为"采样约束表"，不是新 AI**：B1 已经证明这条路可行——`monster.cpp:3509-3551` 已有逐层 `BehaviorClass` 配额（洞穴 kite ≤2、地狱同类 ≤2），且有真实引擎 harness（`test/sampling_behavior_test.cpp:36-50` 直接跑 `GetLevelMTypes()` 统计分布）。**把"上限"扩成"配额（下限+上限）"是同一处代码、同一套度量**。
9. **与在途 A1/A3 的关系（明确结论）**：A1/A3 方向正确但**量级预期须下调、顺序须后置**。理由是乘法关系：A1 把教堂 8 骷髅拆 3 变体，但采样只从池里取 2-4 类，**变体进不进池是概率问题**；A3 改 2-3 只洞穴风筝的 `ai` 列，而 B1 已把洞穴 kite 上限压到 2 类——**A3 的承载在最坏情况只有 2 只中的一部分能出场**。建议顺序：编组（A 轴）→ 配额（E 轴）→ A1/A3。
10. **不该做的**：单命/删档、无上限词缀、新地形、新怪物美术、任何"玩家要背的新概念"（详见 §5 不做清单）。

## 2. 证据

### 2.1 仓库内

| 结论 | 证据 |
|---|---|
| 散布投放不传 leader/leashed，编组能力闲置 | `Source/monster.cpp:3773-3782`（`PlaceGroup(typeIndex, na)`，两参数）；能力在 `:308`（`leader=nullptr, leashed=false` 默认值） |
| leader 路径只由 unique 怪走 | `monster.cpp:3411`：`PlaceGroup(minionType, bosspacksize, &monster, ...Leashed)`；数据列 `monsterPack` 在 `unique_monstdat.tsv` 第 17 列 |
| leashed 已带可读性副作用 | `monster.cpp:344`（半径 <4 硬约束）、`:352-355`（minion HP×2、继承 leader `intelligence`）、`:378`（`leader->packSize = placed`） |
| 分离/重聚状态机已实现 | `monster.cpp:1676-1719`（`Leashed ↔ Separated` 双向转换 + `packSize` 增减）；`monster.h:144` `LeaderRelation`、`:289-290` |
| 采样预算与 B1 配额同处 | `monster.cpp:3516`（`monstimgtot < 4000`）；`:3509-3551`（`classCounts` + `capKite`/`capSameClass`） |
| 行为分类法已在引擎内 | `Source/tables/monstdat.h:376` `enum class BehaviorClass`；`monstdat.cpp:483-513` `GetBehaviorClass` 映射 9 类 |
| 真实引擎采样 harness 已存在 | `test/sampling_behavior_test.cpp:36-50`（`InitLevelMonsters()` + `SetRndSeed` + `GetLevelMTypes()`，返回 `BehaviorClass` 向量）；已入门禁（commit `88035420d`） |
| **主题选择与层语义无关**（随机再摇） | `themes.cpp:849-855`（教堂分支）、`:876-882`（其他 tileset）：首选 `ThemeGood[4] = {GOATSHRINE, SHRINE, SKELROOM, LIBRARY}`，不 fit 则 `j = (theme_id)GenerateRnd(17)` 循环 |
| **修正**：MONSTPIT/BARREL 可达但概率极低 | `theme_id` 共 17 值（`Source/tables/objdat.h:18-36`）；`CheckThemeReqs` 对 BARREL/MONSTPIT 走 `default: return true`（`themes.cpp:195-219`）→ fallback 重摇能命中；`CreateThemeRooms` 有其分派（`:923`/`:929`） |
| L16/Nest/Crypt 跳过主题（成立） | 同一 guard 三处：`themes.cpp:839`（`InitThemes`）、`:894`（`HoldThemeRooms`）、`:906`（`CreateThemeRooms`） |
| 主题房怪物投放只取**单一**类型 | `PlaceThemeMonsts`（`themes.cpp`）：`const size_t mtype = scattertypes[GenerateRnd(numscattypes)]`，全房同一 mtype |
| 同 tileset 4 层生成参数几乎相同 | `currlevel` 引用计数：`drlg_l1`=5、`drlg_l2`=5、`drlg_l3`=6、`drlg_l4`=18；l2/l3 的引用全在任务/楼梯/`LevelSeeds`（`drlg_l2.cpp:2449/2668/2681-2685`、`drlg_l3.cpp:1945/1961/1968/1990-1994`） |
| 密度公式与层无关 | `monster.cpp:3757-3763`：`numplacemonsters = na/30`（na=可走格数），仅多人 ×1.5；组规模只按 `currlevel==1`/`==2`/Crypt 分三档（`:3775-3780`） |
| 主题在怪物池之后建立（顺序可用） | `diablo.cpp:3170` `InitMonsters()` → `:3172` `CreateThemeRooms()`；另一路 `:3345 GetLevelMTypes()` → `:3346 InitThemes()` |
| 光照/视野是选目标与渲染的硬门控 | `scrollrt.cpp:388/471/487/751`（`IsTileLit` + `_pInfraFlag`）、`gendung.cpp:329`；半径管道 `items.cpp:2575-2586` → `ChangeLightRadius`/`ChangeVisionRadius`（`lighting.h:68-69`） |
| Dark Expedition 只做了层段固定百分比 | `items.cpp:2559-2571`：HELL 60 / NEST 85 / CRYPT 50 / 其他 100，`std::clamp(lrad, 2, 15)`；无动态/无消耗品交换 |
| 掉落排除是 Dark Expedition 的预算手段 | `items.cpp:184-194` `DarkExpeditionDropOk`（符文 / 伤害卷轴 / Infravision 卷轴），调用点 `:1416`、`:3298` |
| **全图揭示已实现，且揭示来源已分级** | `objects.cpp:2732-2740` `OperateShrineSecluded` 全图 `MAP_EXP_SHRINE`；`automap.h:20-28` 五级 `MAP_EXP_*`；消费点只有着色 `automap.cpp:984-997` 与存档降级 `loadsave.cpp:2102/2658` |
| 回城治疗零成本 | `stores.cpp:1018-1027` `HealPlayer()`：直接 `_pHitPoints = _pMaxHP; _pHPBase = _pMaxHPBase;`，由 `StartHealer()`（`:1032`）无条件调用 |
| 金币 +25% 是仓库已否决的补偿形状 | `docs/superpowers/specs/2026-08-08-consumable-economy-design.md:155-156`、`:172`（引宪章 §9 / `宪章:624`） |
| 声音已有位置化通道（可作 tell 载体） | `effects.cpp:84`/`:217` `PlaySfxLoc` + `CalculateSoundPosition`（音量/声道）；怪物侧 `PlayEffect(monster, MonsterSound::*)` 十余处（`monster.cpp:1101/1276/1342/3715/4010`） |
| TRN 换色（tint）是零美术可读性通道 | `monster.cpp:172-176` `InitMonsterTRN`（`monstdat.tsv` 第 5 列 `trnFile`）；`:3324-3327` unique 侧 `mTrnName`；`:3397` `uniqTrans` |
| 预留 AI 槽未被占用 | 控制者已核实：`monstdat.h:65 Custom = 55`，全仓 0 引用（`controller-factcheck.md` §5） |
| 改采样构成会动存档语义 | 控制者已核实：`loadsave.cpp:681` `monster.levelType` 是 `LevelMonsterTypes` 索引，由采样顺序决定；缓解路径 `MonsterConversionData`（`:258-266`） |
| 任何层状态机制须过 MP delta 面 | 控制者已核实：`msg.cpp:782-2827` `DeltaLoad*`/`DeltaSyncObject`/`delta_kill_monster` |

### 2.2 外部
未使用外部检索：本报告全部断言均为仓库内可核实的机制事实或从中直接推导的设计结论，无需市场/设计史类外部支撑。（诚实标注：§1.1 的"核心体验定义"是**我推断**，其依据是上述三条引擎约束，不是外部文献。）

## 3. 候选方案（五条强化轴）

| 方案 | 机制要点 | 引擎可行性 | 成本量级 | 风险 | ROI | 对核心体验 |
|---|---|---|---|---|---|---|
| **A 采样与编组** | 散布投放改为**小队投放**：每次从 `scattertypes` 取 1 主类 + 1 副类，用第一只做 leader 传入 `PlaceGroup(main, na, leader, /*leashed=*/true)`，副类以 1-2 只混入同一 `dTransVal` 区域。可读性靠**既有** leashed 副作用（半径 <4 聚团 + HP×2 + 继承 leader `intelligence`）+ leader TRN tint | 高。`PlaceGroup` 已支持全部参数（`monster.cpp:308-380`）；`AddMonster(position, dir, typeIndex, inMap)` 可做副类插入（`:3823`）；分离/重聚状态机已在（`:1676-1719`） | **2-5 人日**（依据：改动集中在 `monster.cpp:3773-3782` 单循环 + PlaceGroup 调用面；无新美术、无新存档字段） | **中高**：动层生成 RNG draw-count → timedemo 必重验（`Timedemo.WarriorLevel1to2`）；`levelType` 索引语义随采样顺序变（`loadsave.cpp:681`）；MP delta 面须核 | **最高** | **强化**：把"遭遇"从随机噪声变成可读阵型，直接服务"信息不足下做决策" |
| **B 遭遇与地形语义** | 给每个 (tileset, 层号) 配一张**主题权重表**替换 `GenerateRnd(17)` fallback；`PlaceThemeMonsts` 改为取"该主题声明的行为类"而非随机单类型 | 中高。改点明确：`themes.cpp:849-855`/`:876-882` 的再摇循环、`PlaceThemeMonsts` 的 `mtype` 选取；顺序已保证（`diablo.cpp:3170→3172`）；可用 `BehaviorClass` 作声明语言 | **3-8 人日**（依据：17 主题 × 4 tileset 权重表 + 两处循环改写；L16/Nest/Crypt 需决定是否解除 `:839` guard，解除则成本上界） | 中：同样动生成 RNG；解除 L16/Nest/Crypt guard 会触及 `themeLoc`/`dTransVal` 未验证路径 | 高 | **强化**：让"这是什么地方"可被读出，喂养空间记忆 |
| **E 层段压力主题化** | 把 B1 的"上限"扩成**配额**：每层段声明必到的 1-2 个 `BehaviorClass`（下限）+ 现有上限。例：教堂=Melee+Rally 必到；墓穴=Sneak 必到；洞穴=RangedKite ≤2 且 Melee ≥1；地狱=同类 ≤2 且 ≥3 类 | 高。与 B1 同一处代码（`monster.cpp:3509-3551`），`classCounts` 已在；下限实现 = 在预算耗尽前**优先**抽未达标类别 | **2-4 人日**（依据：B1 已铺好 `classCounts` 与 cap 循环；下限是同循环的一个优先级分支） | 中：**与 4000 预算硬冲突**——下限越多，可容纳的自由抽样越少；须先量化"配额可行域"，否则会挤掉 image 大的后期怪 | 高 | **强化**：让层段有性格，且不要求玩家学新概念 |
| **D 信息作为可交易资源** | 让 `AutomapView` 的 5 级来源**变成可消费语义**：`MAP_EXP_SHRINE` 揭示的区域标记为"二手情报"（显示地形但不显示已清空状态）；`OperateShrineSecluded` 从"全图"改为"当前 `dTransVal` 连通区 + 相邻区"；声音侧用 `PlaySfxLoc` 给未点亮区的怪物投放做**位置化预警**（不给视觉） | 中高。`automap.h:20-28` 分级已在、消费点只有着色（`automap.cpp:984-997`）；`PlaySfxLoc` 已带音量/声道（`effects.cpp:84/217`）；`UpdateAutomapExplorer` 是单一入口（`automap.cpp:1879-1883`） | **3-6 人日**（依据：着色分支 + 一个揭示范围函数 + 声音触发点；`AutomapView` 已在存档里，改**语义**不改布局 → `loadsave.cpp:2102/2658` 的降级逻辑须复核） | 中：`MAP_EXP_OLD → MAP_EXP_SELF` 的存档降级会抹掉新语义（`loadsave.cpp:2102`）；MP 下自动地图同步须核 | 中高 | **强化**：Dark Expedition 只做了"看不见"，这条做"知道多少、从哪知道的" |
| **C 资源预算纵深** | 唯一诚实杠杆是**回城的时间成本**，不是掉率：给 Pepin 免费全额治疗加条件（例：按缺失 HP 比例收金币，或每次远征限一次），使"下一层还是回城"成为真取舍。**不做** +25% 金币式补偿 | 高（代码面极小：`stores.cpp:1018-1027` 单函数 + `StartHealer` 调用点）。但**设计风险最高** | **1-3 人日**代码；**设计验证成本远高于代码成本** | **高**：直接改平衡曲线，属宪章"深度层"而非底座层；老玩家肌肉记忆（回城=满血）被破坏，可能读作"变难"而非"变深"；须开关门控 + 关=原版 | **中（成本低但风险最高，建议最后做）** | **改变**（不只是强化）：这是本报告唯一会动核心手感的一条，须委托方品味裁决 |

### 前三名落地轮廓

**#1 A 采样与编组（2-5 人日）**
1. 在 `monster.cpp:3773` 的 `while (ActiveMonsterCount < totalmonsters)` 内，把"取 1 类"改为"取主类 + 以 1/3 概率取一个不同 `BehaviorClass` 的副类"。
2. 主类第一只用 `AddMonster(pos, dir, mainType, true)` 落地作 leader，其余走 `PlaceGroup(mainType, na-1, leader, true)`。
3. 副类 1-2 只用 `AddMonster` 插在 leader 的同 `dTransVal` 区域（沿用 `PlaceGroup` 的 `dTransVal[xp][yp] != dTransVal[x1][y1]` 判据，`monster.cpp:343`）。
4. 可读性：leader 用 `monstdat.tsv` 第 5 列 `trnFile` 的既有 tint（零新美术）；聚团半径由 `leashed` 的 `<4` 约束天然给出（`:344`）。
5. **度量**：扩 `test/sampling_behavior_test.cpp` 的 harness 到投放层，跑 N=1000 seed × 层 1-16，断言 (a) 有 leader 的组占比 ≥X%（X 由基线定，当前基线=0%，只有 unique 走这条路）；(b) 每组内 distinct `BehaviorClass` 的均值 ≥1.3（当前恒为 1.0，因为 `PlaceGroup` 单类型）；(c) leashed 组的成员两两 Chebyshev 距离 ≤4（`:344` 的合约）；(d) timedemo 与漂移门禁通过。
6. **回滚判据**：若 (b) 提升但 `numplacemonsters` 总量或 RNG draw-count 变化导致 timedemo 无法重基线，先退回"只加 leader、不加副类"的最小版本。

**#2 E 层段压力主题化（2-4 人日）**
1. 在 `monster.cpp:3509` 附近把 `classCounts` 的用途从"只做上限"扩成"上下限"：新增一张 `{层段 → {必到类别集合, 各类上限}}` 常量表。
2. 抽样循环分两阶段：阶段一只从"未达下限的类别"里抽（沿用 `image > 4000 - monstimgtot` 过滤，`:3517`）；阶段二回到现有自由抽样 + cap。
3. **预算可行域必须先测**：用现有 harness 统计每层"满足下限所需的最小 image 总和"，若 > 4000 则该层的下限不可行 → 该层只保留上限。这一步是**前置度量，不是实现**。
4. **度量**：harness 断言每层 distinct `BehaviorClass` 的**均值与 5% 分位数**（当前后期均值 2.1-2.2，见既定前提），以及"必到类别缺席率 = 0%"。全部是分布断言，无主观维度。
5. **顺序依赖**：此项与 4000 预算强耦合——若预算不放开，下限会挤掉 image 大的后期怪（`image` 均值随层段 616→1655）；因此**预算决策必须先于本项落地**。

**#3 B 遭遇与地形语义（3-8 人日）**
1. 替换 `themes.cpp:851-853` 与 `:876-882` 的 `GenerateRnd(17)` fallback 为按 (tileset, 层号) 的权重表抽取（保留 `SpecialThemeFit` 作为硬过滤，不绕过 `TFit_*` 的几何约束）。
2. `PlaceThemeMonsts` 的 `mtype` 从"随机单类型"改为"该主题声明的 `BehaviorClass` 优先，缺失则回退现状"——这条让"骷髅房里是骷髅"成为规则而非巧合。
3. L16/Nest/Crypt 的 guard（`themes.cpp:839/894/906`）**先不动**：解除它会进入未验证的 `themeLoc` 路径，属独立议题。
4. **度量**：drlg harness（复用 `test/drlg_l1_test.cpp` 的固定 seed 模式）断言 (a) 固定 seed 下主题分布匹配权重表（χ² 或直接计数）；(b) MONSTPIT/BARREL 的出现率从"fallback 偶然"变为"表声明值"；(c) 主题房内怪物 `BehaviorClass` 与声明一致率 = 100%；(d) 现有 `drlg_l1..l4_test` 的 `ViewPosition` 断言不回归。

## 4. 需要委托方用品味判断的点

1. **C 轴（回城治疗成本）该不该做**。机器能证明它是预算纵深唯一无膨胀的杠杆（`stores.cpp:1018` 是零成本满血的单点），但**无法判断"破坏老玩家肌肉记忆"与"取舍变真"哪个更重要**——这取决于目标受众是"想重玩 D1 的人"还是"想要更深 D1 的人"，是定位问题不是技术问题。
2. **编组的可读性该用 tint 还是纯位置**。A1 已占用"黑色 tint = 冲锋"（框架 §5.1），若编组也用 tint 会产生语义竞争。机器能列出冲突，但"玩家一眼能记住几条 tint 规则"是感知阈值判断。
3. **4000 预算是否放开、放开到多少**。控制者已把它列为最高优先级，但"多样性提升"与"内存/加载时间"的兑换率需要实测（`monster.cpp:3691` 的 `LogVerbose` KiB 输出），而**可接受的加载时间上限是品味/平台判断**。

## 5. 「不做」清单

| 不做 | 理由 |
|---|---|
| 单命 / 删档 / Hardcore | D1 的死亡是"掉半金币 + 装备可捡回 + 存档不删"（`player.cpp:2672`）→ 单命是**新增**机制，与"坚守核心体验"直接矛盾 |
| +25% 金币或任何数量补偿 | 仓库已认定为错误补偿形状（`2026-08-08-consumable-economy-design.md:155-156`）；补偿数量不改变取舍结构 |
| 新地形 / 新 `dungeon_type` | 控制者已核实：7 值硬编码 + 47 处 `case DTYPE_` + `.min`/`.sol` **无往返工具链** + 存档 `_qlvltype` 兼容 → 引擎级工程，与"强化既有体验"不是同一量级 |
| 新怪物美术 | 控制者已核实：400-700 帧/只、无 PNG→CL2 编码器、`UNPACKED_MPQS` 默认 OFF → 且**与既定前提冲突**：4000 预算下新怪物只会挤掉既有怪物 |
| 无上限属性/词缀、新伤害类型 | 数值膨胀；且每加一个物品字段要付"结构 + 两处序列化 + TSV + MP delta"全套成本（`engine-mod-infra:53a7360c8` 的 `_iProcChance` 先例） |
| 需要玩家背诵的新概念（新资源条、新状态图标、新词汇） | 违反"落在既有心智模型上"的硬要求；编组/主题/配额三轴全部复用玩家已有的"这是一队 / 这是个房间 / 这层有什么"直觉 |
| 解除 L16/Nest/Crypt 的主题 guard（本轮） | `themes.cpp:839/894/906` 三处同一 guard 保护的是未验证的 `themeLoc` 路径；属独立议题，不应捆进 B 轴 |
| 降地狱数值悬崖 | 框架 E1 已裁决为错误杠杆（不变量 4：悬崖是 D1 身份的一部分）——我独立同意：悬崖迫使玩家改路线，正是"不可回滚决策"的一部分 |

## 6. 与在途 A1/A3 的关系（明确结论）

**结论：A1/A3 保留，但顺序后置到编组（A 轴）与配额（E 轴）之后，且量级预期须显式下调。**

依据（乘法关系，非加法）：
- A1 把教堂 8 骷髅拆 3 变体，收益前提是"这些变体能进池"。采样每层只取 2-4 类（既定前提），教堂池 37 种（`analysis_monster_config_landscape.md`）→ **任一具体变体的出场概率是小数**，玩家单局可能完全遇不到 A1 的三种变体差异。
- A3 改 2-3 只洞穴风筝的 `ai` 列，而 **B1 已把洞穴 RangedKite 上限压到 2 类**（`monster.cpp:3531`）→ 承载与约束正面竞争：最坏情况下进池的 2 类 kite 都不是 A3 改过的那 2-3 只。
- 反之，**A 轴（编组）与 E 轴（配额）的收益不依赖具体怪物是谁**：它们改的是"每次遭遇的构成规律"，对任意池内容都生效，因此是 A1/A3 的**乘数**而非并列项。
- 顺序含义：先做 A/E，A1/A3 的每一份行为差异都会被"更可能出场 + 出场时有阵型语境"放大；反序则 A1/A3 的收益被采样与编组两层同时除。
- **对 A1/A3 规格本身无需改动**——它们的机制、tell、验收都成立；需要改的是**框架层的优先序与预期收益表述**（框架 §2 的"依赖关系"目前只写了"B1 依赖 A1/A3 完成后的类别分布"，实际还存在反向的**收益依赖**：A1/A3 的收益依赖采样与编组）。

## 7. 不确定 / 未验证

1. **未编译、未跑测试**（任务硬约束）：所有成本量级来自改动面阅读（文件 + 函数 + 调用点计数），**非实测**；`PlaceGroup` 改造对 RNG draw-count 的具体影响必须实跑 timedemo 才能定。
2. **4000 预算与配额下限的可行域未量化**：E 轴 §3 明确把它列为前置度量。`image` 列的物理单位控制者已核实"未证实为帧数×常数"，故内存影响仍是推断。
3. **MP 同步面未逐点核**：A 轴改投放构成、D 轴改 `AutomapView` 语义，都可能触及 `msg.cpp` 的 delta 路径；我只确认了 delta 面存在（控制者 §6），未逐个函数验证。
4. **D 轴的存档语义风险未完全排除**：`loadsave.cpp:2102/2658` 的 `MAP_EXP_OLD → MAP_EXP_SELF` 降级会抹掉"情报来源"区分，需确认新语义在读档后如何保持——我未读完这两处的完整上下文。
5. **§1.1 的"核心体验定义"是我推断**：它从三条引擎约束反推，逻辑自洽，但不是从玩家研究或外部文献得出的。委托方若不认同这个定义，五条轴的优先序都要重排。
6. **主题权重表的具体数值未给**：B 轴给了机制与度量，但 17 主题 × 4 tileset 的权重需要玩过各层的人定，属品味输入（未列入 §4 是因为它是"填表"而非"决策"）。
