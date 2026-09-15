# 控制者侧交叉核实（与 E1-E6 独立并行，用于校验它们的结论）

> 这些是我（统筹方）在读代码时**亲自验证**的事实，用来防止子 agent 的系统性错误。子 agent 的报告若与此冲突，以此处证据为准或要求其补充证据。

## 1. 法律：**引擎许可本身就禁止商业化**（不只是"素材不可再分发"）

| 事实 | 证据 |
|---|---|
| DevilutionX 采用 **Sustainable Use License v1.0** | `LICENSE.md:1` |
| 只允许"internal business purposes 或 **non-commercial / personal** use" | `LICENSE.md:15` |
| 分发必须**免费且非商业** | `LICENSE.md:16` |
| README 明写"The source code in this repository is for **non-commercial use only**" | `README.md:84` |
| 游戏数据（美术/表格）来自暴雪，仓库不含，需自备原版 CD 或 shareware `spawn.mpq` | `README.md:27` |
| 与暴雪无关联、Diablo 为暴雪商标 | `README.md:86-88` |

**推论（重要）**：任何"上 Steam 卖钱"的方案（E6 的 B/C 路径）**同时被引擎许可与素材版权两道墙挡住**——不是"换掉美术就行"，还要换引擎/获得不同许可。E6 若给出"可商业发行"的结论，属事实错误。

## 2. 内容量口径：**资产并不缺，缺的是"被体验到的内容"**

数据表现状（`assets/txtdata/**` 实测）：

| 表 | 行数 | 说明 |
|---|---|---|
| `monsters/monstdat.tsv` | **112** | 怪物类型总数 |
| `monsters/unique_monstdat.tsv` | **100** | 独特怪物 |
| `items/itemdat.tsv` | 168 行 / 147 唯一名 | 物品基底 |
| `items/unique_itemdat.tsv` | **90** | 独特物品 |
| `items/item_prefixes.tsv` / `item_suffixes.tsv` | 83 / 95 | 前后缀 |
| `missiles/misdat.tsv` | 68 | 弹道 |
| `objects/objdat.tsv` | 109 | 场景物件 |
| `quests/questdat.tsv` | 24 | 任务 |
| `spells/spelldat.tsv` | 32 | 法术 |

对照：**每层段实际进池的怪物只有 ~10-13 类**（`docs/knowledge/analysis_monster_config_landscape.md`）。

**推论**：所谓"内容塌缩"**不是资产短缺**——112 种怪物 / 90 件独特物品已经存在，但玩家体验到的只有其中一小部分，且行为与物品都**不构成 build 分化**。所以"拿 D2 素材做新怪物/新地形"解决的是**另一个问题**（资产上限），而不是当前诊断出的塌陷成因（采样构成 / 行为类别重复 / 编组缺失 / 物品无构筑意义）。这条应与 E5 的结论对照。

## 3. 新增资产的真实技术成本（比"导入美术"高一个量级）

| 项 | 事实 | 证据 |
|---|---|---|
| 怪物动画槽 | **恰好 6 个**：Stand/Walk/Attack/GotHit/Death/Special，`frames[6]`/`rate[6]` 硬性对应 | `Source/monster.h:112-119`、`Source/tables/monstdat.h:111-112` |
| 怪物精灵路径 | `monsters\<spritePath><AnimLetter>` + 扩展名；扩展名由 `DEVILUTIONX_CL2_EXT` 决定（`UNPACKED_MPQS` 下为 `.clx`，否则 `.cl2`） | `Source/monster.cpp:3224`、`Source/engine/load_cl2.hpp:22-27` |
| PNG→精灵工具**存在** | `png2clx <input.png> <output.clx> [num_frames]`；反向有 `cel2png` | `tools/png2clx/main.cpp:15`、`tools/cel2png/` |
| **地形是硬编码枚举 + 硬编码资产路径** | `dungeon_type` 只有 7 个值（TOWN/CATHEDRAL/CATACOMBS/CAVES/HELL/NEST/CRYPT）；地形数据按固定路径加载（`levels\l1data\l1.min` … `nlevels\l6data\l6.min`、`town.min`、以及 `LoadLevelSOLData` 的 `.sol`） | `Source/levels/gendung_defs.hpp:15-25`、`Source/levels/dun_tile_data.cpp:73-103` |
| 可走性来源 | `SOLData[dPiece[x][y]]`（不是 `dungeon`） | `Source/levels/dun_tile_data.hpp:206` |

**推论**：加"新地形"= 新枚举值 + 新加载分支 + 新生成器参数（`Source/levels/drlg_*`）+ 新 `.min`/`.sol`/tileset 美术 + 逐类型的怪物/物件表 + **存档与多人同步的兼容考量**（`leveltype`/`_qlvltype` 会进存档）。这是引擎级工程，不是素材投放。

## 4. 我尚未验证、但影响结论的项（交给子 agent 并需标注）

- 主流程/多人是否有"跳层/回城"等对终局设计的结构性约束（E4）
- `mods/hf` 叠加机制能否用于"发布级"内容包（E5/E6）
- 无上限属性/词缀是否与存档格式冲突（E3/E5，注意 `engine-mod-infra` 原型曾有 `StashVersion`、序列化改动）
- 当代市场对"稀缺向地牢爬行"的规模量级（E1/E6 的外部证据）
---

## 5. 对已回报告的抽样校验（控制者执行）

### E6（Steam 独立游戏维度）—— 2026-09-15

| E6 的断言 | 我的核实 | 结论 |
|---|---|---|
| 引擎许可禁止有偿分发（`LICENSE.md:15-16`、`README.md:84`） | 独立核实一致 | ✅ 成立（且与我 §1 的结论相同） |
| 无 Steamworks/成就/云存档集成 | `grep -rniE "steam_api\|steamworks" Source/ CMakeLists.txt` = **0** | ✅ 成立 |
| `NUMLEVELS 25`、4 套 drlg 算法 | `gendung_defs.hpp:11`；`drlg_l1..l4.cpp` 俱在 | ✅ 成立 |
| Lua mod API、手柄支持存在 | `Source/lua/modules/` 20 个文件；`Source/controls/devices/game_controller.cpp` | ✅ 成立 |
| 25 个 `.po` 翻译文件 | `ls Translations/*.po \| wc -l` = 25 | ✅ 成立 |
| **"`nDifficulty` 硬编码贯穿 81 处引用点"** | `grep -rn "nDifficulty" Source/ \| wc -l` = **52** |  **数字错误**：实际 52 处（不是 81）。结论（难度是硬编码分支、非数据驱动）仍成立，但引用数字须改为 52 |
| 竞品定价 $5-10 带（Halls of Torment/Path of Achra） | 外部数据，未独立复核（E6 已附 Steam 链接） | ⚠️ 采信其带 URL 的引用，但标注"未二次核验" |
| 团队/预算量级（$150k-$600k） | 无一手来源 | ⚠️ E6 已自标"我推断"——汇总时同样标注为推断 |

## 6. 成本锚点（控制者预核实，供 E3/E4/E5 的估算对照）

| 事实 | 证据 | 对方案的约束 |
|---|---|---|
| **难度是持久化的** | `sgGameInitInfo.nDifficulty` 写入 `Source/loadsave.cpp:1505`、读回 `:652`（uint32） | 新增"难度阶梯/热度"式档位 = 动 `_difficulty` 枚举或加持久化字段 → 存档兼容问题 |
| **地形类型按任务持久化** | `quest._qlvltype`（`dungeon_type`）写 `:1788`、读 `:1003`；`DTYPE_LAST = DTYPE_CRYPT` 作为边界 | 新增地形 = 动 `dungeon_type` 枚举 + 校验边界，且要考虑旧存档里的取值 |
| **已有"旧存档→新格式"转换机制** | `MonsterConversionData`/`LevelConversionData`（`loadsave.cpp:258-266`），`LoadMonster` 据其读 level/exp/toHit/toHitSpecial | 存档格式演化**有既有范式可循**（不是"不能改"，但每个字段都要走 Save/Load + 转换数据 + 测试） |
| **加 1 个物品字段的真实成本模式** | `engine-mod-infra:53a7360c8`：加 `_iProcChance` 字段 = 改 `Item` 结构 + `SaveItem`/`LoadItemData` 两处序列化 + 12 条 TSV 映射 + 12 行后缀表 + proc 逻辑 | "机制化装备"每加一层机制都要付这套成本；且另一次为 stash 格式引入 `StashVersion` 迁移（`b748b9fab`） |
| **新机制要过网络 delta 面** | `DeltaImportData`/`DeltaLoad*`/`DeltaSyncObject`/`DeltaPutItem`/`DeltaOpenPortal`/`delta_kill_monster` 等（`Source/msg.cpp:782-2827`） | 任何改层状态/物品/怪物的机制都要考虑多人同步，否则 MP 会不同步 |

### E4（终局维度）—— 2026-09-15 抽样校验

| E4 的断言 | 我的核实 | 结论 |
|---|---|---|
| D1 无终局；`pDiabloKillLevel` 只驱动选角星标 | 写入 `monster.cpp:907,4175`；**唯一消费点** `pfile.cpp:221 → heroinfo->herorank`（选角 UI），另外只做存档持久化（`pack.h:78`、`loadsave.cpp:651/1504`） | ✅ 成立，且比 E4 表述更精确：它只在 `!gbIsMultiplayer` 分支写入（`monster.cpp:906`） |
| `_pSLvlVisited[NUMLEVELS]` 只用了约 10 项 | `player.h:352`（注释即 `// only 10 used`，NUMLEVELS=25）；`_setlevels` 实有 9 个非 NONE 值 | ✅ 成立 |
| Arena 证明"复用瓦片集加载独立 `.dun` 场景"路径已跑通 | `setmaps.cpp:100 LoadArenaMap`；由 `LoadSetMap` 分支调用（`:159/162/165`），**不是** chat 命令直接调用 | ✅ 成立，且**比 E4 更乐观**：门控在 chat 命令（`control_chat_commands.cpp:62` 的 `gbIsMultiplayer`），`LoadArenaMap` 本身与联机无关 → 单机复用这条路径**技术上没有硬障碍**（E4 自标的最大不确定项，此处理应降级为"低风险"） |
| "新增 1-2 个 SL_ 常量不需要改存档字节布局" | `_pSLvlVisited` 是 `bool[25]`，槽位足够 ✅；**但 `SL_LAST`/`SL_FIRST_ARENA` 是 5 处边界**：`msg.cpp:276 MaxMultiplayerLevels = NUMLEVELS + SL_LAST`（**多人层号空间**）、`msg.cpp:2961`、`portals/validation.cpp:31`、`control_chat_commands.cpp:54-55`（arena 列表与参数映射）、`setmaps.h:26`（`index = arenaLevel - SL_FIRST_ARENA` 的数组下标） |  ⚠️ **成本被低估**：在 `SL_LAST` 之后追加会让新层被 `IsArenaLevel` 误判、并扩大 MP 层号空间；插在 `SL_FIRST_ARENA` 之前又会让 arena 下标整体移位。正确做法要在上述 5 处同步处理（或引入独立的枚举空间），成本从"数周"应上调，且**多人层号是协议级兼容点** |
| 好终局骨架 = 复用怪物池 + 加规则层，而非新地图 | 与 D2 Uber / DCSS Ziggurat / StS Ascension 的公开资料一致（E4 附 URL） | ✅ 采信（外部证据未二次核验） |
| 存档"新字段写读不对称"已两次造成数据损坏 | `docs/knowledge/gotcha_save_bid_overwrite.md`、`gotcha_save_stack_append.md` 确实存在 | ✅ 成立 |

### E1（游戏类型/核心体验）—— 2026-09-15 抽样校验（**最重要的前提更正**）

| E1 的断言 | 我的核实 | 结论 |
|---|---|---|
| **D1 不是单命 roguelike**：死亡掉半金币/装备/耳朵，回城复活，存档不删 | `Source/player.cpp:2672 StartPlayerKill`：`dropGold = !gbIsMultiplayer \|\| !(player.isOnLevel(16) \|\| player.isOnArenaLevel())`；`dropItems = dropGold && deathReason == MonsterOrTrap`；`dropEar = dropGold && deathReason == Player`；`DropHalfPlayersGold(player)`；`player._pInvincible = true`。存档函数未在死亡路径中调用 | ✅ **成立，且我的任务书（CONTEXT.md）把它写成"单命 run"是错的**——已在飞行中向 E2/E3/E5 发出更正。含义：真删档/单命在 D1 是**新增**机制（D2 才做成可选 Hardcore），不是"保留体验" |
| 单机已访问层不重新生成；多人每次重掷 | `Source/diablo.cpp:3162`（`LoadGameLevelDungeon`）：`if (firstflag \|\| lvldir == ENTRY_LOAD \|\| !myPlayer._pLvlVisited[currlevel] \|\| gbIsMultiplayer) { …生成… }` | ✅ 成立（"空间记忆"在单机局内是真实机制） |
| 稀缺/高摩擦体验=小众长尾（Darkest Dungeon / Rain World / Don't Starve 数百万份 + 高好评） | 外部数据，E1 附 URL | ⚠️ 采信引用，未二次核验（Rain World 两个来源互相矛盾，E1 已自标） |
| 天花板判断"内容消耗快于机制深度" | 定性归纳，E1 自标无量化模型 | ️ 汇总时标为**假设**而非结论 |

## 7. 内容包分发机制（控制者核实）

| 事实 | 证据 | 含义 |
|---|---|---|
| 引擎支持**目录式 mod 叠加**：`mods/<name>/` 内含 `manifest.ini` + `data/ lua/ nlevels/ txtdata/ ui_art/` 等；另有 Lua 入口 `lua\mods\<name>\init.lua` | `mods/hf/`（manifest.ini 与五个子目录均为实测）；mod 身份识别在 `Source/mods/mod_identity.h`，加载路径见 `Source/engine/assets.cpp:109` | **内容可以以"mod 包"形式发布而不改引擎代码**——这是"内容量"最便宜的交付形态；但**合法性仍由许可与素材版权决定**（见 §1），不因打包方式改变 |

### E5b（美术管道/法律/单位 ROI）—— 2026-09-15 抽样校验

| E5b 的断言 | 我的核实 | 结论 |
|---|---|---|
| `monstdat.tsv` 有 41 列 | `head -1 … \| tr '\t' '\n' \| wc -l` = **41** | ✅ 成立 |
| `dungeon_type` 加载/生成是穷举 switch | `grep -rn "case DTYPE_" Source/levels/*.cpp \| wc -l` = **47**，分布在 `dun_tile_data.cpp`/`gendung.cpp`/`setmaps.cpp`/`trigs.cpp` | ✅ 成立 |
| `themes.cpp` 有 12+ 处 leveltype 判断点 | `grep -c leveltype Source/levels/themes.cpp` = **44**（提及数，判断点为其子集） | ✅ 成立（实际提及 44 次） |
| 「地形美术是否有反向工具链」列为最大不确定项 | 已查清：`tools/segmenter/` 是**翻译分词器**（zh/ja `.po`），`tools/assemble_png.py` 只做 `.raw+.pal → PNG`；**`.til`/`.min`/`.sol` 无任何往返工具链**（精灵侧才有 `png2clx`/`png2cel`/`cel2png`） | ️ **不确定项已解决，且对成本不利**：新增地形的美术成本必须**额外包含"自建元数据生成工具"**（`.min` 瓦片映射/`.sol` 实心表是二进制表，原版只在 MPQ 里），量级是"周"而非"日" |
| 现行许可下唯一合法形态 = 非商业免费分发、且**不含暴雪美术数据** | 与 §1 我独立核实的条款一致 | ✅ 成立 |
| 单位 ROI：新增资产比"重排既有 112 类怪物/90 件独特物品的层段分布"（改 TSV `minDunLvl`/`maxDunLvl`，零新美术）低 1-2 个数量级 | TSV 确有 `minDunLvl`/`maxDunLvl` 列（实测），改这两列不需新美术、不改引擎 | ✅ 成立（量级判断合理） |

### E5（内容塌陷/美术 ROI——原维度，174 行，**质量最高的一份**）—— 2026-09-15 抽样校验

| E5 的断言 | 我的核实 | 结论 |
|---|---|---|
| 瓶颈是 `monstimgtot < 4000`（与 E5a 独立得出同一结论） | `monster.cpp:3516` 实测一致；`MaxLvlMTypes=24` 中后期不起约束 | ✅ 成立（两份独立报告 + 我核实 = 3 方一致） |
| **没有 PNG→CL2 编码器**，且 `UNPACKED_MPQS` 默认 **OFF** | `CMakeLists.txt:168 option(UNPACKED_MPQS … OFF)`；`grep -rniE "clxToCl2\|EncodeCl2\|cl2_encode" Source/ tools/` **零命中** | ✅ **成立**，且**纠正我 §3 的轻描淡写**：我当时只写"两种格式都支持、.clx 在 unpacked 构建下可用"，漏了"unpacked 并非默认"→**新美术路线必须先解决分发形态或自写 CL2 编码器**（E5 估 2-5 人天） |
| 仓库**不携带**任何暴雪怪物精灵 | `find assets -name "*.clx" \| wc -l` = **100**；其中唯一 `monst*` 是 `assets/data/monstertags.clx` | ✅ 成立，且这是"不可再分发素材"的**实现方式**（靠不携带），比许可措辞更硬 |
| 新怪物美术量 ≈ 400-700 帧/只 | `frames[6]` 求和：112 只可生成怪物，**中位 69、最大 98**；×8 方向 ≈ 552 帧 | ✅ 趋势成立（最大值我测 98、E5 测 89，量级一致） |
| 新地形成本 200-500 人时，Crypt 是唯一先例（复用 L1 生成器 + 7 处分支 + 840 行 `crypt.cpp`） | `dungeon_type` 7 值硬编码、`MAXTILES 1379` 上限、生成器内大量裸 tile ID 字面量（E5 统计 drlg_l2 3388 / drlg_l3 2114）——与我 §3/§6 的结论方向一致 | ✅ 采信（E5 给出的字面量统计未逐个数，量级可信） |
| **"从 D2 提取素材"应直接否决** | 逻辑成立：会把项目从"不携带侵权素材的引擎"变成"携带侵权素材的分发物"；DMCA 先例存在（E5 诚实标注其法律依据是 DRM 规避而非单纯素材复用） | ✅ 采信，作为决策材料中的"红线级"结论 |
| 唯一值得现在做的资产类投入 = 物品图标 / 地形 Miniset 装饰（十几到几十帧量级），且有既有 mod 通道 | `mods/hf/data/inv/objcurs2-widths.txt` 证明图标集可通过 mod 叠加扩展（E5 引用） | ✅ 采信 |

### E5 最终校验（含对**在途工作优先级**的影响）

| E5 的断言 | 我的核实 | 结论 |
|---|---|---|
| 改预算会动到**存档语义**：存档里的 `levelType` 是 `LevelMonsterTypes` 的索引 | `Source/loadsave.cpp:681 monster.levelType = file->NextLE<int32_t>();`（写侧 `SaveMonster` 同字段）；该索引由采样顺序决定 | ✅ 成立 → **改采样构成=改存档里怪物的解释**；缓解手段是既有的 `MonsterConversionData`/`LevelConversionData` 转换路径（`loadsave.cpp:258-266`） |
| `image` 列物理单位未证实（≠帧数×常数） | 实测：`MT_XSKELAX` frames=72/image=**553**；`MT_BMAGMA` frames=75/image=**1680**——同为 72-75 帧而 image 差 3× | ✅ 成立（E5 的诚实标注正确）；故"4000→8000 = 内存翻倍"是**推断**，落地前必须用 `monster.cpp:3691` 的 `LogVerbose("Loaded monster graphics: … KiB …")` 实测 |
| `MonsterAIID::Custom = 55` 全仓 0 引用 | `Source/tables/monstdat.h:65 Custom = 55,`；`grep -rn "MonsterAIID::Custom" Source/` = **0** | ✅ 成立（预留槽位未被使用；若将来接 Lua/脚本 AI，可省去改枚举） |

## 8. **对在途工作的优先级影响（控制者判断）**

证据链（E5 + E5a 独立得出、我已经核实三方一致）表明：**密度塌缩的主导成因是 `monstimgtot < 4000` 采样预算**（后期单层实得 2-4 类、distinct AI 仅 2.1-2.2），其次是**编组层已实现却未被普通散布调用**。

而**在途的密度框架 A1/A3 做的是"单个怪物行为变体"**——它解决的是"行为类别重复"这一**次要**成因，且其效果同样受预算截断（每层只有 2-4 类，变体再多也进不来）。

→ **结论**：A1/A3 方向没错但**不是最大杠杆**；决策材料应把"预算 + 采样 + 编组"列为**前置/更高优先级**工作流（零美术、2-18 人日量级，见 E5 §3 与 E5a §3 的估算），并明确它与 A1/A3 的**顺序依赖**（预算不放开，行为变体的收益也被除到接近零）。

### E3（L1/L2/L3 档位）—— 2026-09-15 抽样校验

| E3 的断言 | 我的核实 | 结论 |
|---|---|---|
| **`PlayerPack` 有未使用的保留位 → 玩家级新轴可零格式破坏** | `Source/pack.h:73 uint8_t reserved; :75 reserved2[2]; :81 reserved3[20]`（均注释 `For future use`）；`grep -rn "\.reserved\|->reserved\|wReserved8" Source/`（排除 pack.h）**零命中** | ✅ **成立**（21+ 字节空闲；E3 自标的最大不确定项已解决——这是 L1 档的承重前提） |
| `ItemPack` 是紧凑结构、**无保留位** | `Source/pack.h:19-31` `#pragma pack(push,1)` + 10 个字段，**无 reserved** | ✅ 结论成立；⚠️ **但它给的"16B"不准**：按字段求和是 **19 字节**（4+2+2+1×5+2+4）——数字须更正（结论不变） |
| 每件物品硬性只有 1 前缀 + 1 后缀、无插槽系统 | `Source/items.h:247-248 _iPrePower`/`_iSufPower`（各一个）；`grep -rniE socket Source/*.h Source/*.cpp` = **0** | ✅ 成立 |
| 存档物品是**种子重生成**，改词缀表/谓词会让旧存档物品静默变成别的东西 | 机制为 `RecreateItem`→`SetupAllItems`；仓库已有实证 gotcha `docs/knowledge/gotcha_vendor_predicate_seed_drift.md` | ✅ 成立——**这是对"物品级机制"全路线的硬约束**：任何改词缀/掉落谓词的方案都会静默改变旧存档里的既有物品 |
| `engine-mod-infra` 应重推导而非整体复活（311 提交/227 文件差异、冲突集中在 items.cpp/inv.cpp、原型 0 条 eval） | 差异规模与冲突面与我早前实测（243 文件差、无规格）一致；"0 条 eval"在其分支上成立（该分支早于 eval 体系） | ✅ 采信 |

### E2（坚守体验的强化轴，121 行）—— 2026-09-15 抽样校验

| E2 的断言 | 我的核实 | 结论 |
|---|---|---|
| `THEME_MONSTPIT`/`THEME_BARREL` 不是"不可达"，而是**概率极低** | `ThemeGood[4] = {THEME_GOATSHRINE, THEME_SHRINE, THEME_SKELROOM, THEME_LIBRARY}`（`themes.cpp:844`），`:850`/`:880` 取值后不 fit 则 `GenerateRnd(17)` 全域重摇（`:851-853`/`:876-882`）；两主题均有 `case`（`:923`/`:929`） | ✅ **E2 正确，E5a 表述夸大**（"永不被选中"→"只能靠 fallback 命中"） |
| L16 / Nest / Crypt 整体跳过主题 | 三处同一 guard：`:839`、`:894`、`:906`（`currlevel == 16 \|\| IsAnyOf(leveltype, DTYPE_NEST, DTYPE_CRYPT)` → return） | ✅ 成立 |
| `PlaceGroup` 支持 leader/leashed/HP×2/继承 intelligence/分离重聚，普通散布只传两参 | `monster.cpp:308`（签名）、`:3780`（普通散布 `PlaceGroup(typeIndex, na)`）、`:3411`（unique 传 leader）；分离/重聚状态机 `:1676-1719` | ✅ 成立（与 E5/E5a 一致） |
| 层段配额是 B1 的自然延伸（同一循环 + 同一 harness） | `monster.cpp:3509-3551` 已有 `classCounts` + 上限；`test/sampling_behavior_test.cpp` 直接跑 `GetLevelMTypes()` 统计 | ✅ 成立 |
| Pepin 免费全额治疗是预算漏点 | `stores.cpp:1018-1027`（`_pHitPoints = _pMaxHP`，无花费）——与 E2 引述一致 | ✅ 成立（**但这是五轴中唯一"改变"而非"强化"手感的一条**，E2 自标风险最高、建议最后做） |
| 信息层地基已建好未被消费 | `automap.h:20-28` 五级 `MAP_EXP_*` 只用于着色（`automap.cpp:984-997`）；`OperateShrineSecluded` 全图揭示（`objects.cpp:2732-2740`） | ✅ 采信（E2 给的行号未逐条打开，量级可信） |

**E2 补充的两条（已并入决策文档 §6）**：① 密度框架 §2 只写了"B1 依赖 A1/A3 的类别分布"，**漏了反向的收益依赖**（A1/A3 的变体能否出场取决于采样）；② **编组改动会改变层生成 RNG draw-count** → timedemo 重基线成本未实测（该夹具目前已因上游同步 quarantine，属可接受窗口）。
