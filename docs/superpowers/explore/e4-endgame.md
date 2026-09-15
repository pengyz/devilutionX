# E4：终局内容怎么加才合理 —— 报告

## 1. 结论摘要

1. D1 目前无终局：`pDiabloKillLevel` 在杀死 Diablo 时写入难度+1（`Source/monster.cpp:907,4175`），除了在选角界面画几颗星（`Source/DiabloUI/hero/selhero.cpp:109-124`）外**没有任何游玩后果**——这是唯一已存在的"击杀记忆"机制，但它是纯展示，不驱动任何新内容。
2. 引擎已经有一套**未被当作终局利用的资产**：多人 Arena 系统（`SL_ARENA_*`、`Source/levels/setmaps.h:17-27`、`Source/control/control_chat_commands.cpp:59-90`）证明"复用地牢瓦片集渲染一个独立小型 `.dun` 场景、绕开主线关卡进度"这条技术路径**已经跑通并在生产代码里**，只是被限定为 PvP。
3. `_pSLvlVisited[NUMLEVELS]`（25 项，当前只用 9 项，见 `Source/player.h:352`）、`_setlevels` 枚举（`Source/levels/gendung_defs.hpp:47-61`）都还有冗余槽位——**新增 1-2 个 SL_ 常量不需要改存档字节布局**，是三个候选方案里成本最低的落地点。
4. 难度系统是**纯数值乘子**（怪物等级 +15/+30、经验 x2/x4，`Source/monster.h:357-368`），没有"层数以外的规则修饰符"概念；任何"自选惊悚修饰符/Ascension/Heat"式终局都需要新增一套独立于 `nDifficulty` 的状态位，且必须在存档与网络协议（`msg.cpp`）两处都补，历史上这类"新字段塞进旧结构"已经出过两次数据损坏事故（`docs/knowledge/gotcha_save_bid_overwrite.md`、`gotcha_save_stack_append.md`）——这是本维度最大的工程风险点，不是"能不能做"而是"做错了会静默损坏别人的存档"。
5. **横向证据显示"好终局"的共同骨架不是新地图,而是重新排列已有怪物/资源并加一层规则**：D2 Uber 三王只是把已有 boss 换个场景重生（[Pandemonium Event wiki](https://www.purediablo.com/d2wiki/index.php?title=Pandemonium_Event&oldid=25997)），DCSS Ziggurat 是把已有分支怪物池按"楼层递增"重新洗牌（[crawl.chaosforge.org/Ziggurat](http://crawl.chaosforge.org/index.php?title=Ziggurat&oldid=82676)），Slay the Spire Ascension 全部是数值/概率旋钮而非新内容（[slaythespire.wiki.gg/wiki/Ascension](https://slaythespire.wiki.gg/wiki/Ascension?oldid=51298)）。D1 应该抄这个骨架而不是抄"新地图"。
6. **奖励不能只是装备**这条判据在案例里普遍成立的反例恰恰是最容易招致玩家反感的：D2 Uber 的奖励虽是 charm（`Hellfire Torch`），但玩家真正追逐的是它带来的等级压制优势，本质等价于"更好的装备"，是本清单里唯一一个**判据 3 会打低分**的范式，应作反面参照而非直接抄。
7. 多人同步（`Source/msg.cpp`）目前处理的是"关卡编号 + 怪物/物品 delta"，没有"跨局持久化的元进度"概念（keys/currency 类）；`pDiabloKillLevel` 是单机字段（`if (!gbIsMultiplayer)` 条件，见 `Source/monster.cpp:906`），多人处理路径在 `PrepDoEnding`（`Source/monster.cpp:4165`）无条件更新——**任何"跨局元货币"式终局在联机环境下需要额外设计同步/防作弊，比单机实现成本高一个量级**。
8. 三个方案全部可以在**不新增任何美术资产、不改动存档字节布局（仅追加字段/复用空槭位）**的前提下用现有 tile set + 现有怪物池 + 现有 `.dun` 工具链（`arena/*.dun` 的加载路径 `LoadArenaMap`，`Source/levels/setmaps.cpp:100`）实现，成本量级压在"数周"而非"数月"。

## 2. 证据

### 2.1 仓库内（逐条：结论 + 文件路径）

| # | 结论 | 文件路径 |
|---|---|---|
| E1 | 难度三档是纯数值乘子，无规则修饰符维度 | `Source/levels/gendung_defs.hpp:39-44`（`enum _difficulty`只有 NORMAL/NIGHTMARE/HELL）；`Source/monster.h:357-368`（`exp()`按难度乘 1/2/4，`level()`按难度 +0/+15/+30） |
| E2 | 杀 Diablo 唯一的持久后果是选角界面星标，无游玩内容解锁 | `Source/monster.cpp:907`（`pDiabloKillLevel = max(..., nDifficulty+1)`，单机分支）；`Source/monster.cpp:4175`（`PrepDoEnding`同样更新，无 `gbIsMultiplayer`条件）；`Source/pfile.cpp:221`（`herorank = player.pDiabloKillLevel`）；`Source/DiabloUI/hero/selhero.cpp:109-124`（`RenderDifficultyIndicators`只画星星） |
| E3 | 引擎已有"复用瓦片集渲染独立场景，绕开主线进度"的生产级实现——Arena 系统 | `Source/levels/gendung_defs.hpp:47-61`（`_setlevels`含 `SL_ARENA_CHURCH/HELL/CIRCLE_OF_LIFE`，`IsArenaLevel()`）；`Source/levels/setmaps.h:17-27`（`GetArenaLevelType`把 3 个竞技场映射到 `DTYPE_CATHEDRAL`/`DTYPE_HELL`，复用现成瓦片集）；`Source/levels/setmaps.cpp:100-165`（`LoadArenaMap`加载 `arena\church.dun`/`arena\hell.dun`/`arena\circle_of_death.dun`，这三个 `.dun` 文件在 `assets/arena/`里已存在） |
| E4 | 进入 Arena 的判定逻辑（可套用为新终局的入口条件模板） | `Source/control/control_chat_commands.cpp:59-90`（`TextCmdArena`：仅联机可用、需在城镇或另一竞技场、`StartNewLvl(*MyPlayer, WM_DIABSETLVL, arenaLevel)`触发关卡加载） |
| E5 | `_setlevels`枚举与存档数组均有冗余容量，新增终局层不需要动存档字节布局 | `Source/levels/gendung_defs.hpp:47-61`（当前只用到 `SL_ARENA_CIRCLE_OF_LIFE=8`）；`Source/player.h:352`（`_pSLvlVisited[NUMLEVELS]`注释"only 10 used"，NUMLEVELS=25，见 `Source/levels/gendung_defs.hpp:11`） |
| E6 | 存档格式是定长顺序读写，新增字段的"写读不对称"历史上两次造成真实数据损坏 | `docs/knowledge/gotcha_save_bid_overwrite.md`（堆叠数写进 `bId`位域覆写物品品质/鉴定标志，存档+网络两处都要修）；`docs/knowledge/gotcha_save_stack_append.md`（`SaveItem`无条件写、`LoadItem`条件读，偏移累积越界写） |
| E7 | 存档版本判别用布尔标志而非版本号，Hellfire 与非 Hellfire 存档走不同字段集 | `Source/loadsave.cpp:50`（`gbIsHellfireSaveGame`）；`Source/loadsave.cpp:2550`（`if (gbIsHellfireSaveGame != gbIsHellfire)`触发迁移/拒绝逻辑）——新增终局层若要兼容旧存档，必须走类似的"缺省值补齐"路径，不能假设旧存档有新字段 |
| E8 | 任务系统是纯粹的状态机+区域触发，没有"随机词条/难度修饰符"概念 | `Source/quests.h:13-30`（`InitQuests/CheckQuests/CheckQuestKill`）；`Source/tables/questdat.hpp:102-128`（`Quest`结构体只有 `_qlevel/_qactive/_qvar1/_qvar2`，无修饰符字段） |
| E9 | 地图生成的随机性入口是关卡种子（`DungeonSeeds[]`），主题房间数量由 `InitThemes`扫描空间填充，非玩家可控参数 | `Source/diablo.cpp:3114-3122`（`SetRndSeedForDungeonLevel`，竞技场用 `DungeonSeeds[setlvlnum]`，主关卡用 `DungeonSeeds[currlevel]`）；`Source/levels/themes.cpp:826-855`（`InitThemes`按空间填充定理房间，`MAXTHEMES`是硬编码上限） |
| E10 | 多人同步是"关卡编号+delta"模型，没有跨局持久化元进度/货币的先例；杀 Diablo 记录字段是单机专属 | `Source/msg.cpp:2953-2963`（`IsValidLevel`只检查关卡编号范围，`IsValidLevelForMultiplayer`检查 `level <= MaxMultiplayerLevels`）；`Source/monster.cpp:906`（`if (!gbIsMultiplayer)`门控 `pDiabloKillLevel`更新，说明该字段设计时未考虑联机场景的跨局记账） |
| E11 | 怪物池/威胁分布已有系统性图谱：地狱层（13-16）多样性骤降到 6 类行为、23 种怪物，远程拉扯在洞穴层垄断 38% | `docs/knowledge/analysis_monster_config_landscape.md`（第二节表格：地狱层"6 类平分"，洞穴层"远程拉扯 14 种垄断"）——任何终局若复用地狱层怪物池做"高难混战"，会直接继承这个多样性塌陷，需要额外补充怪物变体或接受单调 |
| E12 | `.dun`文件与瓦片集加载工具链已验证可用（现有 Arena 场景），意味着新增"混合怪物池竞技场"式终局不需要新地牢生成算法 | `Source/levels/setmaps.cpp:100`（`LoadArenaMap`函数签名接受任意 `.dun`路径 + 视角/出口坐标）；`tools/`目录下 `png2clx/cel2png/mpqextract/assemble_png.py`（CONTEXT.md 已列出，用于制作新 `.dun`若需要新美术，但复用现有瓦片集则不需要这些工具） |

### 2.2 外部（逐条：结论 + URL）

| # | 结论 | URL |
|---|---|---|
| X1 | D2 Uber 三王（Pandemonium Event）机制骨架：3 把钥匙（各来自特定 Hell 难度 boss 的低概率掉落）→ 3 个红门场景（各含一个"超版本"boss，掉落一个道具）→ 集齐 3 道具合成第 4 把钥匙 → Uber Tristram（3 个超 boss 混战）。奖励是**唯一的**大型护身符（Hellfire Torch），玩家追逐它本质是数值压制而非玩法变化 | [purediablo.com/d2wiki Pandemonium_Event](https://www.purediablo.com/d2wiki/index.php?title=Pandemonium_Event&oldid=25997) |
| X2 | DCSS Ziggurat：27 层递增难度的"竞技场式"分支，每层怪物主题从既有分支池抽取（不新增怪物种类），完整通关后下一次进入的 Ziggurat 更难更富（"Mega-Zig"机制，2016 引入）；一旦进入某层就不能回退，只能前进或从传送门撤退，未完整通关不影响后续 Ziggurat 难度 | [crawl.chaosforge.org/Ziggurat](http://crawl.chaosforge.org/index.php?title=Ziggurat&oldid=82676) |
| X3 | Slay the Spire Ascension：20 级纯"数值/概率旋钮"（精英刷新率、Boss伤害、事件负面化、商店涨价等），**没有一级新增内容**，等级独立于角色进度可自由选择已解锁等级重开 | [slaythespire.wiki.gg/wiki/Ascension](https://slaythespire.wiki.gg/wiki/Ascension?oldid=51298) |
| X4 | Hades Pact of Punishment（"Heat"）：14 种可勾选修饰符（伤害提升/加速/精英强化/减少选择数等），每种有等级和 Heat 消耗；首次在某 Heat 等级通关才拿满奖励（血/钻/仙馐），重复刷相同等级只拿基础货币——**逼玩家提升 Heat 而非无限刷同一难度** | [segmentnext.com/hades-pact-of-punishment](https://segmentnext.com/hades-pact-of-punishment/) |
| X5 | Grim Dawn Bastion of Chaos / Steps of Torment（Roguelike 挑战地牢）：钥匙消耗型、进入后不能存档退出重来（一次性），三难度档独立版本存在，社区共识是"低难度不值得打，留钥匙去 Ultimate 打"——说明**低难度版本奖励不足会导致玩家跳过内容**，是一个反模式警示 | [forums.crateentertainment.com/t/no-bastion-of-chaos](https://forums.crateentertainment.com/t/no-bastion-of-chaos/39555) |
| X6 | PoE 地图系统（Atlas）：击杀本体后进入"地图设备"驱动的开放式内容消费循环，地图本身是消耗品（用后即毁），可用通货预先加难度修饰符换取更高数量/稀有度掉落，是"玩家自主选择风险-收益曲线"的典范，但**核心奖励仍是掉落物品和再生产地图**——不完全满足"不能只靠更好装备"的判据，只能作部分借鉴（"风险-收益旋钮"机制，非奖励形状） | [poewiki.net/wiki/Map](https://www.poewiki.net/wiki/Map) |
| X7 | 反模式案例：D2 后期"Tristram glitch"式速刷——Uber Tristram 里高等级角色带无关角色刷经验，被 wiki 明文记录为已知漏洞式打法，说明**终局内容一旦被高效速刷路径主导，设计初衷（挑战 boss）会被"经验农场"取代** | [purediablo.com/d2wiki Pandemonium_Event（"Tristram glitch"段）](https://www.purediablo.com/d2wiki/index.php?title=Pandemonium_Event&oldid=25997) |

## 3. 判据（供讨论，非最终裁决）

我提出 6 条可判定的"D1 式合理终局"判据，理由随附：

1. **判据 1——不能以数值膨胀延长战斗时长**：怪物血量/伤害线性放大不算内容，是数值通胀（对照 D1 已有的 Nightmare/Hell 难度模型，`Source/monster.h:357-368`已经是这个模式，重复它没有增量价值）。
2. **判据 2——必须复用已有资产（瓦片集/怪物池/`.dun`工具链），不新增大宗美术**：委托方许可"重做定位"，但美术是暴雪版权资产，二次创作的美术资产存在法律风险（见 CONTEXT.md 第 8 行）；复用现成资产（如 Arena 的瓦片集复用）零法律新增风险。
3. **判据 3——奖励形状不能唯一由"更好的装备"构成**：D1 装备数值空间已经被主线 16 层填满，终局装备只会导致装备膨胀，且宪章禁令暗示"底座不改平衡"的精神应延续到终局；奖励应包含至少一种非装备类持久后果（称号/外观/规则解锁/记录）。
4. **判据 4——失败要有代价，但代价不能是永久性丢失投入的时间**（例如死亡清空整个终局进度，而不是清空角色）：这条平衡"有惊有险"与"不劝退"。
5. **判据 5——必须给"重开"提供理由**（种子/词条随机化，或递增难度阶梯），否则打过一次的内容第二次就是纯重复劳动，没有 replay 价值。
6. **判据 6——单人与联机都要有清晰路径**（不能像现有 Arena 一样把整个系统限定为"仅联机"，否则单机玩家——D1 的主要受众——完全无法体验终局）。

## 4. 本引擎可行性

### 4.1 难度系统（`sgGameInitInfo.nDifficulty`）
- **事实**：`_difficulty`只有 3 个档（`Source/levels/gendung_defs.hpp:39-44`），怪物 exp/level/toHit 三个函数（`Source/monster.h:357-420`、`Source/monster.cpp:5045/:5064`）用 `if/else if`硬编码难度加成，不是数据驱动表。
- **可行性**：若要做"自选难度修饰符"（如 Hades Heat/PoE 地图词缀），**不能**简单扩展 `_difficulty`枚举（它同时决定关卡怪物表、`monster.level()`偏移量、掉落表分支，改枚举值会破坏所有引用点）；必须新增一个**独立的位掩码/修饰符结构**，仅在进入终局关卡时读取，不影响主线难度选择。这是新增状态，需要新的存档字段——见 4.4。

### 4.2 地图生成参数与主题（`Source/levels/drlg_l1..l4.cpp`, `themes.cpp`, `setmaps.cpp`）
- **事实**：主关卡走 `CreateDungeon(DungeonSeeds[currlevel], entry)`（`Source/diablo.cpp:1462`），随机种子逐层固定，主题房间由 `InitThemes`按空间填充算法自动放置（`Source/levels/themes.cpp:826-855`），玩家不能控制主题密度或类型。
- **Arena/Setlevel 路径**（`Source/levels/setmaps.cpp:100-165`）走完全不同的加载逻辑：读一个**手绘固定** `.dun`文件，配合 `SetRndSeedForDungeonLevel`用 `DungeonSeeds[setlvlnum]`（`Source/diablo.cpp:3114-3122`）只随机怪物/物品摆放，地形本身是静态的。
- **可行性**：终局场景最现实的路线是**沿用 Arena 路径**——手绘/复用现有 `.dun`（或用 `assemble_png.py`/`png2clx`工具组合现有瓦片拼接新 `.dun`），不是让程序化生成器（`drlg_l*.cpp`）去随机生成"无限深度"地牢。真随机地牢生成需要改 `drlg_l*.cpp`本身的算法参数暴露，工程量显著更大（这几个文件均 1000+ 行,`drlg_l1.cpp`达 1346 行）。

### 4.3 任务系统（`Source/quests.cpp`, `questdat.hpp`）
- **事实**：`Quest`结构体（`Source/tables/questdat.hpp:102-115`）只有 `_qlevel/_qactive/_qvar1/_qvar2`，是纯状态机，没有"随机词条"或"难度修饰符"字段；`MAXQUESTS=24`（`questdat.hpp:14`）已被现有 24 个任务占满（`quest_id`枚举列了 24 项，`Q_ROCK`到 `Q_JERSEY`，`questdat.hpp:74-100`）。
- **可行性**：**不建议**新增终局任务走 `Quest`/`quest_id`路径——数组已满，扩容需要改 `MAXQUESTS`常量和所有遍历循环，风险面大。终局的"进度追踪"更适合用一个独立的新字段（挂在 `Player`结构体或全局状态），绕开任务系统。

### 4.4 存档格式约束（`Source/loadsave.cpp`）——**硬约束，最大风险点**
- **事实**：存档是定长二进制顺序读写（`LoadHelper`/`SaveHelper`，贯穿 `loadsave.cpp`全文件），版本判别靠布尔标志（`gbIsHellfireSaveGame`），不是版本号递增机制。历史教训（`docs/knowledge/gotcha_save_bid_overwrite.md`、`gotcha_save_stack_append.md`）明确：**任何新字段必须写读对称、必须同时处理存档与网络（`msg.cpp`）两条编码路径**，否则会静默损坏数据。
- **可行性**：终局若需要新持久化字段（如"已解锁的终局层数"、"当前 Heat 等级"），最安全的落点是复用现有冗余槭位：
  - `_setlevels`枚举还有空间（当前只到 `SL_ARENA_CIRCLE_OF_LIFE=8`，`_pSLvlVisited[NUMLEVELS=25]`已经预留 25 项数组但注释"only 10 used"，`Source/player.h:352`）——新增 1-2 个 `SL_`常量+复用这个已存在的数组格位，**不需要改存档字节布局**。
  - 若需要额外数值（如 Heat 等级、通关次数计数器），必须新增字段，且**必须**在旧存档加载路径显式补默认值（参照 `gbIsHellfireSaveGame`的迁移写法，`loadsave.cpp:2325-2337`），并在存档（`loadsave.cpp`）与网络（`msg.cpp`）两处同步实现。

### 4.5 多人同步（`Source/msg.cpp`）
- **事实**：`IsValidLevel`（`Source/msg.cpp:2958-2962`）只检查关卡编号是否在 `[0, NUMLEVELS)`或 `[0, SL_LAST]`范围内，`MaxMultiplayerLevels = NUMLEVELS + SL_LAST`（`msg.cpp:276`）。多人同步的怪物/物品走 delta 广播（`DeltaLoadLevel`等），没有"跨局持久元进度"广播通道。
- **可行性**：单人版终局（复用 Arena 式关卡路径）在多人环境下**技术上可行**（`IsValidLevel`已经把 `SL_LAST`纳入合法范围），但若涉及"跨局元货币/成就"，需要新的同步消息类型（`enum _cmd_id`，`Source/msg.h:24`起，已有 60+个 CMD_常量，新增一个不难，但要设计防作弊——联机环境下客户端可能伪造完成状态）。**建议单人优先落地，联机版本作为后续迭代**，避免一次性把同步复杂度全部背上。

## 5. 候选方案

| 方案 | 机制要点 | 引擎可行性 | 成本量级 | 风险 | ROI 判断 | 对核心体验的影响 |
|---|---|---|---|---|---|---|
| **方案 A：黑暗回廊（复用 Arena 路径的"递增试炼"）** | 走完 16 层后，城镇出现一个新场景入口（复用现有瓦片集拼一个新 `.dun`，或直接复用 `arena/hell.dun`）。进入后是"波次竞技场"：怪物池从各层段按当前"层级"随机抽取（复用 `monstdat.tsv`已有的等级过滤逻辑,`Source/items.cpp:1397`同款模式），每通过一波，"层级"+1（类似 Ziggurat 楼层递增,`_setlevels`新增 1 个常量,复用 `_pSLvlVisited`冗余槭位记录"最高纪录层级"）。死亡则本次试炼终止（不清空角色装备/经验），下次从层级 1 重开，但"最高纪录层级"永久保留、显示在选角界面（复用 `RenderDifficultyIndicators`同款星标位置逻辑,`Source/DiabloUI/hero/selhero.cpp:109`）。 | **高**——完全复用 `LoadArenaMap`路径（`setmaps.cpp:100`）+ `_pSLvlVisited`冗余槭位（不改存档布局）+ 现有怪物等级过滤逻辑（`items.cpp:1397`同款代码可套用于随机化怪物池）。单人多人均可用（Arena 路径已验证单人城镇可进入,`control_chat_commands.cpp:81`的判断只挡了"必须在城镇或竞技场",不挡单机）。 | 数周：1 名工程师，主要工作是新 `.dun`场景/复用现有 Arena 场景微调 + 波次生成逻辑（复用怪物随机化代码）+ 1 个新 `_setlevels`常量 + UI 显示最高纪录。**零新美术**（复用现有瓦片集与怪物精灵）。 | **中**：波次生成逻辑若照搬"远程拉扯垄断"的洞穴/地狱层怪物池（`docs/knowledge/analysis_monster_config_landscape.md`已诊断的塌陷），会重复"风筝地狱"体验；需要人工挑选各层段怪物的均衡子集，而非全池随机。 | **高**：成本低、复用度高、有明确的"重开理由"（打破自己纪录），符合判据 1/2/5/6。 | 强化"战斗深度"体验，不改变主线；给硬核玩家一个长期目标（纪录层级），符合项目"内容与机制扩充"定位（决策 31）。 |
| **方案 B：诸圣审判（自选修饰符 + 固定场景，仿 Hades Pact/PoE 地图词缀）** | 城镇 NPC（复用现有对话框架）提供"审判"入口：进入前从若干修饰符里勾选（如"怪物移速+X%"/"你受到的治疗-X%"/"精英怪数量+1"），每个修饰符有"惊悚度"权重,总惊悚度决定进场后的场景难度与奖励丰厚度（复用 Arena `.dun`场景,3 个已有的 arena 地图轮换）。首次在某惊悚度通关才获得"完整奖励"（称号/新法术卷轴的解锁标志/装备染色权限——**非纯数值装备**），重复刷相同惊悚度只给少量金币,逼玩家提升惊悚度而非无限刷（复刻 Hades Heat 逻辑,X4）。 | **中**——需要新增一个"修饰符位掩码"结构（新状态,不能塞进 `_difficulty`,见 4.1）,必须新增存档字段并同步实现读写对称（历史教训 E6/E7）,且必须同时改 `msg.cpp`处理联机模式的修饰符广播。怪物增强修饰符可以通过读取 `monster.hitPoints`/`toHit`在运行时叠加实现（有先例,`Source/monster.cpp:4315-4318`难度加血逻辑),不需要改 TSV。 | 数周到 1-2 月：比方案 A 多一层"修饰符 UI + 存档字段设计 + 双路径（存档/网络)写读对称验证"的工程量,且需要额外测试覆盖新存档字段的旧档兼容性（`timedemo`测试提示这类改动要跑 `Timedemo.WarriorLevel1to2`验证 RNG 确定性)。 | **中高**：新存档字段是本方案案例里唯一"动了存档字节布局"的选项，历史两次数据损坏事故都源于类似改动（E6），需要格外谨慎的写读对称设计与充分测试；若实现不当会波及所有玩家存档。 | **中**：机制上最接近"自选风险-收益"这条判据 5 的最佳范式（Hades Heat 被证明有效延长游玩时长),但工程风险明显高于方案 A。 | 深度改变"游玩节奏"——玩家开始主动配置难度而非被动接受三档,是本清单里对核心体验改变最大的选项,需要委托方判断是否偏离"D1 式"体验太多。 |
| **方案 C：暗影之影（一次性稀有掉落链，仿 D2 Uber 但去数值化）** | 主线 16 层的稀有怪物（如唯一怪）获得极低概率掉落一个新"残章"道具（复用现有物品系统，新增 1 个 `IDI_`常量而非新增大宗物品）。收集 3 种不同残章后可在城镇合成为"通行证"，开启一个一次性场景（复用 Arena 场景资产，仿 D2 三红门→Uber Tristram 骨架）,里面是混战：主线已出现过的 2-3 个唯一怪的"强化重生版"（读取现有 `UniqueMonstersData`,仅运行时叠加 `hitPoints`/`toHit`乘子,不新增怪物数据行)。奖励是**称号+外观特效**（如武器发光特效,复用现有法术特效渲染管线,而非新增词缀数值)。通行证消耗后场景关闭,须重新收集残章才能再来（对照判据 5:重开理由是"再收集一次")。 | **高**：物品新增走已验证路径（`IDI_ARENAPOT`同款"新增一个 misc 道具枚举"先例,`Source/translation_dummy.cpp:368`）；场景复用 Arena 路径；唯一怪强化复用难度加血逻辑（`monster.cpp:4315-4318`同款模式)。称号/外观走现有渲染特效系统,不需要新词缀字段——**回避了方案 B 的存档字段新增风险**。 | 数周：新增 3 个道具枚举 + 1 个新 Arena 式场景 + 唯一怪强化的运行时叠加逻辑 + 称号/特效 UI。**零新美术**（外观特效可用现有法术光效贴图变色实现）。 | **低**：机制与存档改动量都小,主要风险是"稀有掉落链条"若掉率设太低会让玩家觉得纯看脸(D2 的 8%掉率争议历史悠久),需要精心调参并给出保底机制（如"第 N 次杀死同一唯一怪必掉"）。 | **高**：复用度最高,风险最低,且奖励形状（称号+外观)天然满足判据 3。但内容深度不如方案 A/B（一次性,重开理由较弱）。 | 几乎不改变核心体验（战斗节奏、难度曲线不变），是最保守但最安全的选项，适合作为"第一步"验证终局内容的市场反应，再决定是否投入方案 A/B。 |

## 6. 反模式清单

| 反模式 | 案例 | URL |
|---|---|---|
| 奖励本质仍是数值压制装备，玩家追逐它不是为了玩法而是为了继续刷更高难度 | D2 Hellfire Torch（+10-20全属性、全抗性,本质是让角色更快刷更高难度内容的乘子） | [purediablo.com/d2wiki Pandemonium_Event](https://www.purediablo.com/d2wiki/index.php?title=Pandemonium_Event&oldid=25997) |
| 低难度版本奖励不足，玩家跳过内容留着资源去最高难度打，导致"中间难度内容"形同虚设 | Grim Dawn 社区共识"Normal/Elite 的 Bastion of Chaos 不值得打，钥匙留 Ultimate" | [forums.crateentertainment.com/t/no-bastion-of-chaos](https://forums.crateentertainment.com/t/no-bastion-of-chaos/39555) |
| 终局内容被"经验/资源农场"式打法取代设计初衷（挑战 boss变成刷等级效率工具） | D2 "Tristram glitch"——高等级角色带无关角色进 Uber Tristram 分摊经验 | [purediablo.com/d2wiki（同上,"Tristram glitch"段）](https://www.purediablo.com/d2wiki/index.php?title=Pandemonium_Event&oldid=25997) |
| 数值膨胀式"重复同样内容但怪更肿"，玩家体感只是打得更久，没有新决策空间 | 本仓库已有先例——难度三档本身就是纯数值乘子（`Source/monster.h:357-368`），若终局继续用同样手法，只是"第四档难度"，没有新增内容 | 仓库内 `Source/monster.h:357-368`（已验证，非市场传闻） |
| 高稀有度掉落链若无保底机制，玩家体感是"纯看脸"，长期会转化为负面口碑（"设计者故意刁难"） | D2 三把 Uber Key 的低掉率（Countess ~8% on Hell）历史上长期被玩家诟病为"farm 疲劳" | [purediablo.com/d2wiki（同上，"Key of Terror"段）](https://www.purediablo.com/d2wiki/index.php?title=Pandemonium_Event&oldid=25997) |

## 7. 需要委托方用品味判断的点

1. **方案 A vs 方案 B 的"体验改变幅度"取舍**：方案 A（黑暗回廊）几乎不改变游玩节奏，方案 B（自选修饰符）会让玩家开始主动配置难度，更接近"现代 roguelite"的心智模型——这是否符合"D1 式"的怀旧定位，机器无法判断，取决于委托方对"这款游戏应该往哪走"的品味主张（这正是任务书要求探索的问题本身，需要委托方拍板方向）。
2. **称号/外观是否足够"有分量"作为终局唯一奖励**：判据 3 要求不能只靠装备，但称号/外观特效这类"非数值奖励"是否能撑起玩家的长期动机，是纯粹的产品直觉判断，缺乏 D1 这类硬核 ARPG 受众的实证数据（这批玩家历史上对"纯装备驱动"的市场认知根深蒂固，机器无法预测他们对非装备奖励的接受度）。
3. **稀有掉落链的掉率与保底机制的具体数值**：方案 C 的道具掉率设多少既能维持"稀有感"又不至于劝退，属于游戏手感调参，需要实际playtest数据或委托方基于经验直接定数字，机器没有依据推算这个平衡点。

## 8. 不确定 / 未验证

- 未验证 `LoadArenaMap`（`setmaps.cpp:100`）在单人游戏下是否真的能被非 debug 路径触发——`TextCmdArena`命令本身有 `if (!gbIsMultiplayer) return`门控（`control_chat_commands.cpp:61-64`），说明现有 UI 命令层面 Arena 被限定为"仅联机"，但这限制在**命令处理函数**而非 `LoadArenaMap`本身或 `_setlevels`机制层——本报告方案 A/C 假设"新增一个不经过 `TextCmdArena`的独立触发点（如城镇新增一个可交互对象/NPC）可以在单机下复用同一套加载路径"，这个假设**未做端到端验证**（没有实际编译运行测试触发路径,受"只读仓库、不得编译"的工作约束限制)。
- 未验证 `monstdat.tsv`里各层段怪物的"强化重生版"（方案 C 的唯一怪强化）在运行时叠加血量/命中后是否会触发既有的经验计算公式溢出（`AddPlrMonstExper`,`monster.cpp:4044`用 `monster.exp(sgGameInitInfo.nDifficulty)`,若唯一怪基础 exp 已经是 `data().exp * 2`,叠加强化倍率后是否超出 `uint32_t`范围未做数值推演)。
- 未验证联机模式下方案 A 的"波次层级"记录字段若要同步给同队友军观战,`msg.cpp`是否有现成的"玩家自定义状态广播"通道可以复用,还是必须新增 `CMD_`常量——本报告只确认了"关卡编号"层面的同步机制,未深入到"玩家自定义进度值"这类更细粒度的同步需求。
- 三个方案的"数周/数月"成本量级是基于**类比现有代码改动规模**的推断（如 Arena 系统的既有实现体量、`docs/knowledge/`里记录的历史 bugfix 工时线索），非实际估算,委托方若采纳任一方案，建议先做一个最小可行原型（如仅方案 C 的道具枚举+复用现成 arena/hell.dun场景)来验证实际工时,再决定是否投入完整版本。
