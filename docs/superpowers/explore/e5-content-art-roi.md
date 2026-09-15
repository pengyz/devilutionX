# E5 内容塌陷与「新增资产」可行性 / ROI：报告

> 探索员立场：本报告不受设计宪章约束。所有断言标注 **【已验证】**（附文件路径:行号 / URL）、**【我推断】**、**【市场传闻】**。
> 只读作业：未修改任何被跟踪文件，未编译，未跑测试。所有代码结论来自阅读源码 + 对 TSV 数据的离线统计（Python 读文件，不写仓库）。

---

## 1. 结论摘要

1. **塌陷的主因不是"池子小"，而是引擎的 4000 精灵预算把每层实际可见怪物种类压到 2-4 种。**【已验证】`Source/monster.cpp:3516` 的采样循环条件是 `monstimgtot < 4000`；`MaxLvlMTypes=24`（`Source/monster.h:39`）在中后期完全不起约束作用。我按 `monstdat.tsv` 的 `image` 列忠实模拟采样：L2 realized ≈5.9 种，L8 ≈3.3，L10 ≈2.5，L14 ≈2.2。**这一点与仓库既有结论一致**（`docs/knowledge/decision_density_fix_package.md:23-34`），不是我的新发现，但它是回答 E5 的决定性前提。

2. **因此"加新怪物"在当前预算下几乎不改善逐层体验，是本报告最重要的负面结论。** 新增 1 个怪物只是让 typelist 多 1 个候选（分母 15→16），而玩家每层仍只看到 2-4 种。**新增资产的边际收益被预算除到接近零。**【已验证】推理链全部落在 `monster.cpp:3491-3560`（typelist 构建 + 采样循环）与 `monster.cpp:3305`（`monstimgtot += monsterData.image`）。

3. **真正便宜且高杠杆的动作是"改预算/改采样"，而不是"加资产"。** 把 4000 提到 8000 是**一个常量**的改动，能把地狱层的可见种类从 ~2.2 提到 ~4.4——**用零美术拿到"翻倍内容量"的观感**。代价是精灵内存翻倍与低内存平台风险（仓库确实构建 3DS/Vita/AmigaOS：`CMakeLists.txt:375,533,277`）。这是我给出的**第一优先建议**，也是所有"新增资产"提案的**前置条件**：预算不放开，加资产等于把钱倒进封住口的瓶子。

4. **新怪物的技术管道能走通，但有一个当前仓库无法闭合的环节：没有 PNG→CL2 编码器。**【已验证】`tools/png2clx/main.cpp` 只输出 `.clx`；仓库中搜不到任何 `ClxToCl2`/`EncodeCl2` 实现；而 `DEVILUTIONX_CL2_EXT` 仅在 `UNPACKED_MPQS` 下为 `.clx`（`Source/engine/load_cl2.hpp:22-28`），该选项默认 **OFF**（`CMakeLists.txt:168`）。**结论：走新美术路线必须先绑定 `UNPACKED_MPQS=ON` 的分发形态，或先写一个 CL2 编码器（我估 2-5 人天）。** 这是所有"加新怪物/新地形"提案的隐藏前置成本，此前的密度修复框架文档未覆盖此项。

5. **新怪物的美术工量是 400-700 帧/只，这是"不做"结论的算术基础。**【已验证】`monstdat.tsv` 的 `frames[6]` 求和：可生成的 88 只怪物中位数 69 帧、最大 89 帧；动画槽恰好 6 个（`Source/monster.h:112-119` ↔ `monstdat.h:111-112`），且行走/攻击等按 8 方向存为 sprite sheet（`Source/monster.h:162-167` `spritesForDirection`）→ **中位 69×8 = 552 帧/只**。对比外部像素画报价量级（8 方向单怪 ~$114 起，[freelancer](https://www.freelancer.com/projects/2d-game-art/direction-gothic-banshee-sprite)），一只**风格达标**的 D1 怪物是 40-120 人时 / 数百到数千美元量级。

6. **新地形是本维度成本最高、ROI 最差的一项，因为地形不是"资产"而是"资产 + 一套生成器"。**【已验证】`dungeon_type` 是 7 个硬编码枚举（`gendung_defs.hpp:15-25`），资产路径硬编码在 4 处 switch（`dun_tile_data.cpp:73-95` min、`:101-159` sol、`diablo.cpp:1392-1438` cel/til/special、`gendung.cpp:360-380` 生成器派发），且 `MAXTILES 1379` 是硬上限（`dun_tile_data.hpp:29`）。生成器内部是**几千个裸 tile ID 字面量**（我统计 drlg_l2.cpp 3388 个、drlg_l3.cpp 2114 个数字字面量），Miniset 表逐个写死（`drlg_l1.cpp:35-93`）。**Crypt 是仓库内唯一的"新地形"先例，它复用 L1 生成器 + 7 处 `leveltype == DTYPE_CRYPT` 分支 + 840 行 `crypt.cpp`**——即"最省的做法"仍是 800+ 行代码。我估一套新地形 200-500 人时（美术另计）。

7. **法律现实（准确表述，非印象）**：仓库自身代码用 **Sustainable Use License**，明文"非商业 / 不得收费分发"（`LICENSE.md` Limitations 段；`README.md:82-84`）；**暴雪美术与游戏数据不在仓库内**——引擎要求玩家自备 `DIABDAT.MPQ`（`README.md:27-32`），仓库仅捆绑自制/授权的 UI 与字体（`assets/**` 只有 105 个 `.clx` + 数据表，无怪物精灵：`find assets -name '*.clx' | grep -i monst` 只命中 `monstertags.clx`）。`README.md:86-88` 声明 Diablo® © 1996 Blizzard、项目与暴雪无关联。**所以"不可再分发暴雪素材"在本仓库是通过"不携带素材"实现的，而不是靠许可豁免。**

8. **"从 D2 提取素材"应当直接否决。** 它会把项目从"不携带侵权素材的引擎"变成"携带侵权素材的分发物"，从而**摧毁当前唯一的合法立足点**。风险不是理论：暴雪对 D2 相关 GitHub 项目发过 DMCA 并导致全部 fork 被删（[KitGuru](https://www.kitguru.net/tech-news/matthew-wilson/unofficial-diablo-2-resurrected-offline-patch-removed-from-github/) / [TorrentFreak](https://torrentfreak.com/blizzard-dmca-notice-wipes-diablo-ii-resurrected-offline-patches-from-github-210610/)）。**诚实标注**：那一次的法律依据是 DRM 规避（CRC32 绕过）而非单纯素材复用，所以它**不是**素材侵权的直接判例——但它证明了权利人对 D2 生态的执法意愿与 GitHub 的配合速度。另外 D2 与 D1 调色板/比例/画风不同，借来的素材还会**破坏 D1 气质**（见 §6）。

9. **ROI 判据（触发条件）**：新增资产**只在四个条件同时成立时**才值得做——(a) 精灵预算已放开（否则收益被除到零）；(b) 零美术手段（采样约束、行为分化、编组、机制交叉）已实证穷尽；(c) 已锁定 `UNPACKED_MPQS` 分发形态或已有 CL2 编码器；(d) 有稳定的、能匹配 D1 调色板的美术产能。**当前四条一条都不成立，所以本维度的诚实结论是「现在不做新增资产」。**

10. **唯一我推荐现在就做的"资产类"投入是最便宜的那一档：物品图标与地形 Miniset 装饰（十几到几十帧的量级），不是怪物、不是地形。**【已验证】物品掉落图与 `objcurs` 图标是单方向静态帧（`items.cpp:2444-2452` `InitItemGFX`、`cursor.cpp:443-449`），且 `mods/hf/data/inv/objcurs2-widths.txt` 证明存在**通过 mod 叠加扩展图标集**的既有通道。这是"每帧收益"最高的一档。

---

## 2. 证据

### 2.1 仓库内（结论 + 文件路径[:行号]）

#### A. 塌陷机制：五种成因逐一验证

| 成因 | 是否成立 | 证据 |
|---|---|---|
| **(1) 采样构成 —— 预算截断** | **成立，且是主因** | `Source/monster.cpp:3516` `while (nt > 0 && LevelMonsterTypeCount < MaxLvlMTypes && monstimgtot < 4000)`；`:3518` 预算过滤；`:3305` `monstimgtot += monsterData.image`。`Source/monster.h:39` `MaxLvlMTypes = 24`（不起约束）。我的模拟（含 Golem 预加 386，`monster.cpp:3441`）：L1 6.0 / L2 5.9 / L4 5.9 / L6 4.5 / L8 3.3 / L10 2.5 / L12 2.4 / L14 2.2 / L15 2.2 种。 |
| **(2) 行为类别重复** | 成立，但**已被 B1 部分处理** | `Source/tables/monstdat.cpp:490-517` `GetBehaviorClass` 把 40 个 AIID 压成 8 类；`monster.cpp:3504-3560` 已实施 cap（洞穴 RangedKite ≤2、地狱同类 ≤2）。既有规格自评为"尾部保险"，量级 0.7-4.1%（`docs/superpowers/specs/2026-08-10-sampling-anti-monopoly-design.md`）。 |
| **(3) 编组缺失** | **成立，且被低估** | `monster.cpp:3772-3782`：散怪按 `PlaceGroup(typeIndex, na)` 成组，但**每组只有一个 type**（`na` = 1 / 2-3 / 3-5），组间无阵型关系；`PlaceGroup`（`:308-348`）只有 leader 分支（仅 boss pack 用）才有 leash。**普通层不存在"近战墙 + 后排远程"的混合编组**——这解释了"池够大仍雷同"：玩家遇到的是**同类小群的重复串联**。 |
| **(4) 地形同质** | **成立，且是零美术方向里最被忽视的一条** | 7 个 `dungeon_type`（`gendung_defs.hpp:15-25`）覆盖 16 层 → 每 4 层共用一套 tileset 与一个生成器；`gendung.cpp:360-380` 显示 Crypt 复用 L1 生成器、Nest 复用 L3 生成器，即**7 种地形实际只有 4 个生成算法**。`themes.cpp` 有 44 处 `case THEME_` 主题房——这是现成但未被内容设计使用的变化源。 |
| **(5) 玩家应对单一** | 成立（不在本维度深挖） | 与 §3 的"机制交叉"对应；`monstdat.tsv` `resistance`/`resistanceHell` 列在地狱层大量 IMMUNE_* 使可用手段收窄。 |

**关键判断**：成因 (1) 与 (3)(4) 都是**零美术可动**的，且 (1) 的杠杆远大于"加资产"。这直接决定了 §3 的排序。

#### B. 新怪物管道（逐环节，含工量级）

| 步 | 需要什么 | 证据（文件:行） | 工量级 |
|---|---|---|---|
| 1 | 美术：6 个动画 × 8 方向 | 槽位恰好 6：`Source/monster.h:112-119`（Stand/Walk/Attack/GotHit/Death/Special）↔ `Source/tables/monstdat.h:111-112` `frames[6]`/`rate[6]`；8 方向由 sprite sheet 承载：`monster.h:162-167` `spritesForDirection` 按 `Direction` 索引 sheet | **中位 552 帧**（69×8，`frames[6]` 统计）。40-120 人时 |
| 2 | PNG → 精灵 | `tools/png2clx/main.cpp:37` `SurfaceToClx(surface, frames, 0)`（纵向堆叠帧，索引 0 透明）；**只产 `.clx`** | 分钟级/文件 |
| 3 | **`.clx` 能否被引擎读？** | `Source/engine/load_cl2.hpp:22-28`：`UNPACKED_MPQS` → `.clx`，否则 `.cl2`；`CMakeLists.txt:168` 该选项默认 OFF；加载器 `monster.cpp:3222-3224` 用 `DEVILUTIONX_CL2_EXT` 拼名 | **⚠️ 阻塞环节**。要么锁 `UNPACKED_MPQS=ON`，要么写 CL2 编码器（**2-5 人天**） |
| 4 | 数据行：`monstdat.tsv` 39 列 | 列清单 `Source/tables/monstdat.cpp:351-404`；关键列 `assetsSuffix`（→ 精灵路径 `monsters\\<suffix><letter>.cl2`，字母表 `monster.cpp:152` `"nwahds"`）、`frames[6]`/`rate[6]`、`hasSpecial`（决定 5 or 6 个动画：`monster.cpp:154-157`）、`image`（预算权重）、`ai`、`trnFile`、`treasure`、`availability`/`minDunLvl`/`maxDunLvl` | 1-2 人时/只 |
| 5 | **新怪物 ID 无需改 C++** | `monstdat.cpp:336-349`：ID 不在 `_monster_id` 枚举时写入 `AdditionalMonsterIdStringsToIndices` 并 `emplace_back` 追加槽位。上限 `NUM_MAX_MTYPES = 200`（`monstdat.h:303`，注释说明与存档兼容绑定）。`mods/hf/txtdata/monsters/monstdat.tsv` 138 行 vs base 112 行，**26 个 ID 通过 mod 叠加新增**（MT_LICH/MT_HORKDMN/… 实测 comm 差集） | 0（数据驱动） |
| 6 | **新 AI 必须改 C++** | `monstdat.cpp:246-253` `ParseAiId` 只接受 `MonsterAIID` 枚举（`monstdat.h:23-64`）；Lua 只能**改已有 AI 编号**（`Source/lua/modules/monsters.cpp:47-52` `SetMonsterAILua`、`:66-70` `ai` 属性），**不能定义新行为循环**。有 `MonsterAIID::Custom = 55` 但仓库内 **0 处引用**（grep 无命中）→ 是预留占位，不是可用扩展点 | 新 AI：**3-10 人天**（含 MP 确定性 + 测试） |
| 7 | 音效 | `monster.cpp:3563-3589` `InitMonsterSND`（由 `AddMonsterType` 调用，`:3317`）：固定 4 类前缀 `a/h/d/s` × 2 变体 = **必须 8 个 `.wav`**（`monsters\\<soundSuffix><prefix><1|2>.wav`）；`s` 仅当 `hasSpecialSound` | 8 文件；4-16 人时（自制音效） |
| 8 | 掉落 | `treasure` 列解析 **不完整**：`monstdat.cpp:299-308` 只认 `""`/`None`/`Uniq(SKCROWN)`/`Uniq(CLEAVER)`，其它值返回 `"Invalid value. NOTE: Parser is incomplete"`。掉落主要由 `level` 列经 items.cpp 门控（既有结论 `decision_density_fix_package.md:39,98`） | 0-1 人时（但想指定掉落需先补 parser） |
| 9 | 进关卡 | `monster.cpp:3491-3499` typelist 只按 `IsMonsterAvailable`（`:3161-3170`：availability ≠ Never 且 `minDunLvl ≤ currlevel ≤ maxDunLvl`）→ **填对 3 列即自动进池**；随后受 §A(1) 预算截断 | 0 |
| 10 | TRN 调色变体（**最便宜的"新怪"**） | `monster.cpp:172-193` `InitMonsterTRN` 读 `monsters\\<trnFile>.trn` 做 256 色重映射。`monstdat.tsv` 已大量使用（如 Ghoul = Zombie 精灵 + `zombie\bluered`）。**注意**：同 `assetsSuffix` 共享精灵数据（`monster.cpp:3221-3232` `MonsterSpritePaths` 去重 + `InitAllMonsterGFX` 按 spriteId 分组），所以 TRN 变体**几乎不额外吃 `image` 预算的独立拷贝**……**但 `image` 列仍按行累加**（`:3305`），即预算上仍算一份 | **0.5-2 人时/只** |

**已知的坑**：
- **存档**：`NUM_MAX_MTYPES = 200` 的注释明确"same as MaxMonsters, for the sake of save game compability"（`monstdat.h:303`）；存档存的是 `monster.levelType`（`loadsave.cpp:681`）= `LevelMonsterTypes` 的**索引**，而该表由 `GetLevelMTypes` 每次重建（`diablo.cpp:3288,3345`）。**改动采样顺序/池构成会改变索引语义** → 老存档的怪物可能变成别的种类。这是"加怪物"最容易被忽略的破坏面。
- **多人同步**：新 AI 必须按 `aiSeed` 确定性推导（既有交叉规则 `decision_density_fix_package.md:93`）。
- **timedemo/漂移门禁**：改 `monstdat` 或采样会触碰 RNG 消费序列（`CLAUDE.md` 测试门禁 + `Timedemo.WarriorLevel1to2`）。
- **翻译**：怪物名进 pot（`tools/extract_translation_data.py:10-11,62` 直接读 `monstdat.tsv`）→ 新怪物名会进入全部 ~30 个 `.po`，产生翻译债。
- **多平台**：3DS / Vita / AmigaOS 在构建矩阵内（`CMakeLists.txt:375,533,277`）→ 精灵内存是真实约束。

#### C. 新地形 tileset 管道

**需要的资产文件（已核实，非猜测）**：`.cel`（微块像素）+ `.til`（MegaTile：4×uint16 微块索引，`dun_tile_data.hpp:56-61`）+ `.min`（每 tile 10/12/16 块，`dun_tile_data.cpp:166-180`）+ `.sol`（TileProperties 表，`:99-163`）+ special cel（`l1s`/`l2s`/`l5s`，`diablo.cpp:1383-1387`）+ 可选 `.pal`（`setmaps.cpp:124` `l1_2.pal`）+ 可选 `.dun` 预制房间（`assets/levels/l1data/sklkngt.dun` 等）。**注意 `.cl2` 不用于地形**——地形是 CEL/自定义 TileType 编码（`dun_tile.hpp:20-76` 六种 TileType）。

**与 `dungeon_type` 的耦合点（最小改动面 = 5 处 switch + 1 个生成器）**：
1. `gendung_defs.hpp:15-25` 加枚举值（+ `DTYPE_LAST`）
2. `parse_dungeon_type.cpp:7-17` 加解析分支
3. `dun_tile_data.cpp:71-95` `LoadMinData` 加 `.min` 路径
4. `dun_tile_data.cpp:101-162` `LoadLevelSOLData` 加 `.sol` 路径（**注意**：既有各档都带一串手工 SOL 修补，如 L1 的 20 行 `SOLData[n] |= BlockLight`——新地形要么美术做对，要么也要一串修补）
5. `diablo.cpp:1391-1438` `LoadLvlGFX` 加 cel/til/special 路径；`diablo.cpp:1464-1487` `CreateLevel` 加 triggers 分支
6. `gendung.cpp:360-380` `CreateDungeon` 派发生成器
7. `SetDungeonMicros`（`dun_tile_data.cpp:166-177`）的 `microTileLen`/`blocks`：Town 16、Hell 12/16、其余 10 —— 新地形要选一档
8. 硬上限 `MAXTILES 1379`（`dun_tile_data.hpp:29`）

**Crypt 先例的真实成本（这是最可靠的工量锚点）**：Crypt **复用 L1 生成器**（`gendung.cpp:364-367` `DTYPE_CATHEDRAL` 与 `DTYPE_CRYPT` 同走 `CreateL5Dungeon`），代价是 `drlg_l1.cpp` 内 **7 处 `leveltype == DTYPE_CRYPT` 分支**（:1019,1159,1213,1220,1243,1309,1337）**加上独立的 840 行 `crypt.cpp`**（内含 13 张 Miniset 表 + 专属 shadow pattern + 专属 stairs/substitution/lights）。**结论：即使"抄近路复用生成器"，一套新地形也是 800-1500 行代码 + 完整一套 tile 资产。我估 200-500 人时（代码）+ 美术另计（一套 tileset 数百个微块）。**

#### D. 物品 / boss / 图标（便宜档）

- **物品掉落图**：`items.cpp:2444-2452` `InitItemGFX` 循环 `ItemDropNames[]`（`items.cpp:247+`，`ITEMTYPES 43`，`items.h:26`）→ **新增物品"类别"要改 C++ 数组**，但复用已有类别只需 TSV。掉落图是单方向动画（`ItemAnimWidth`）。
- **背包图标**：`cursor.cpp:443-449` 载 `objcurs.clx` + **可选** `objcurs2.clx`（`LoadOptionalClx`）→ `mods/hf/data/inv/objcurs2-widths.txt` 证明**mod 可扩展图标集而不改主资产**。单帧静态。**这是全仓库最便宜的新美术通道。**
- **新 boss**：`unique_monstdat.tsv` 22 列（`monstdat.cpp:428-455`）——**boss 复用已有 base type 的精灵 + TRN 换色 + 自定义 HP/AI/抗性/`monsterPack`**（`:449` `ParseUniqueMonsterPack`，`monster.cpp:3410` `PlaceGroup(minionType, bosspacksize, ...)` 带 leash）。**即"新 boss"可以做到零新美术**，是本报告里 ROI 最好的"新内容"形态。但独有 AI（如 Diablo/Lazarus 级）要改 C++。

### 2.2 外部（结论 + URL）

1. 【已验证事实，判例性质需谨慎】暴雪曾对 D2:R 相关 GitHub 项目发 DMCA，导致原库与多个 fork/mirror 被删；其**依据是 DRM/完整性校验规避**，非单纯素材复用 —— [KitGuru 报道](https://www.kitguru.net/tech-news/matthew-wilson/unofficial-diablo-2-resurrected-offline-patch-removed-from-github/)（引 [TorrentFreak](https://torrentfreak.com/blizzard-dmca-notice-wipes-diablo-ii-resurrected-offline-patches-from-github-210610/)）。**我据此推断**：权利人对 D2 生态执法活跃 + GitHub 快速配合 ⇒ 携带 D2 素材的公开仓库属高风险，但不能声称"已有素材侵权判例"。
2. 【市场量级参考】8 方向单怪像素精灵的外包报价均值约 **$114 USD**（56 家投标均值）—— [freelancer 项目页](https://www.freelancer.com/projects/2d-game-art/direction-gothic-banshee-sprite)。**注意**：该报价对应的帧数远少于 D1 怪物的 ~552 帧，故应视为**下界**。
3. 【市场量级参考】独立像素画师公开价目表（角色/图块/立绘分档）—— [Pixel Joint 报价帖](https://pixeljoint.com/forum/forum_posts.asp?TID=27648)。
4. 【生态参照】OpenRA 生态存在长期的自由素材替换努力（OpenRA/ArtSrc、OpenHV 等独立自由素材项目）—— [OpenRA/ArtSrc](https://relatedrepos.com/gh/OpenRA/ArtSrc)、[OpenHV](https://libraries.io/github/OpenHV/OpenHV)。**我推断**：完全自由素材替换是**年级别**的社区工程，不是单人 mod 的可选项。
5. 【生态参照】D1 社区确有"总体改造 + 新美术"的 mod（The Hell 2 自述为 HD total overhaul）—— [ModDB: Diablo The Hell 2](https://www.moddb.com/mods/diablo-the-hell-2)。**未验证**其素材来源与授权状态，不作为可复制路线引用。

---

## 3. 候选方案

| 方案 | 机制要点 | 引擎可行性 | 成本量级 | 风险 | ROI 判断 | 对核心体验的影响 |
|---|---|---|---|---|---|---|
| **P0 放开精灵预算** 4000→6000/8000（可按平台/设置分档） | 改 `monster.cpp:3516,3518` 的常量；可做成 options 项按平台降档 | **已验证可行**：单常量；`InitAllMonsterGFX` 已按 spriteId 去重共享（`monster.cpp:3676-3695`），实际内存增长低于线性 | **2-8 人时** + 平台内存实测 | 3DS/Vita/Amiga 内存（`CMakeLists.txt:375,533,277`）；**存档 `levelType` 索引语义变化**（`loadsave.cpp:681`）；timedemo RNG | **最高。零美术拿到"每层种类翻倍"** | **强化**。D1 的"下层遇到陌生组合"体验本就被预算削平，这是修复而非改变 |
| **P1 编组阵型（零美术）** 让一次 `PlaceGroup` 混入 2 个 type（近战前排 + 远程后排） | 改 `monster.cpp:3772-3782` 的散怪循环，让组内可含两个 scattertype | **已验证可行**：`PlaceGroup` 已支持 leader/leash（`:308-348`） | **1-3 人天** | 平衡（混合组比同类组更危险）；MP 确定性；RNG 序列 | **高**。直击成因(3)，玩家立刻感到"阵型不同" | **强化**。把"同类小群串联"变成"要判断先打谁"，正是 D1 战术味 |
| **P2 地形变化（零美术）** 用现成 44 个 `THEME_*` + Miniset 密度做层内差异 | `themes.cpp`（44 处 `case THEME_`）、`PlaceMiniSetRandom`（`drlg_l1.cpp:1251`） | **已验证可行**，纯参数/规则层 | **1-4 人天** | 生成器 RNG 与 quest 房冲突 | **中高**。直击成因(4)，零资产 | **强化**（不改战斗，改"这一层长得不一样"） |
| **P3 TRN 变体 + unique boss（近零美术）** 复用精灵，换色 + 自定 HP/AI/抗性/随从包 | `monstdat.tsv` `trnFile`（`monster.cpp:172-193`）+ `unique_monstdat.tsv` 22 列（`monstdat.cpp:428-455`） | **已验证可行**，纯数据（新 AI 除外） | **0.5-2 人时/只**（boss 含随从包 4-16 人时） | 换色怪被识破为"换皮"（正是既有 A1 诊断的病）；`image` 预算仍逐行累加 | **中**。P0 之后才有意义（否则挤占预算） | **弱化风险**：滥用会加重"换皮"感——这是 D1 原版已被批评的点 |
| **P4 新物品图标/掉落图（最便宜的真新美术）** | `objcurs2` mod 叠加通道（`cursor.cpp:447-449` + `mods/hf/data/inv/objcurs2-widths.txt`）；掉落图需改 `ItemDropNames[]`（`items.cpp:247`） | **已验证可行**（图标）；掉落类别需改 C++ | 图标 **0.5-2 人时/个**；新掉落类别 +1-2 人天 | 无美术风格约束（单帧静态最易达标） | **中高（单位成本最低的新美术）** | **无关/轻度强化**。不改战斗 |
| **P5 一个全新怪物（新美术 + 新 AI）** | 6 动画 × 8 方向 + 39 列 TSV + 8 音效 + 新 AI | 可行但**需先解 `.cl2` 编码或锁 `UNPACKED_MPQS`**（`load_cl2.hpp:22-28`、`CMakeLists.txt:168`） | **美术 40-120 人时 + AI 3-10 人天 + 音效/数据/测试 2-5 人天 ⇒ 总 80-250 人时/只** | 风格不匹配即成"违和物"；预算未放开则**几乎不可见**；翻译债；存档索引 | **低（当前条件下为负）** | **改变**。新怪物是"内容扩充型 mod"的标志物，会把项目从"D1 精修"推向"D1+" |
| **P6 一套新地形** | cel/til/min/sol/special + 5 处 switch + 生成器 | 可行（Crypt 先例）但代价固化 | **代码 200-500 人时 + 一套 tileset 美术（数百微块，量级 100-400 人时）⇒ 300-900 人时** | `MAXTILES 1379` 上限；SOL 手工修补债；quest/trigger 耦合 | **最低。明确建议"不做"** | **改变**。等于宣布做资料片 |
| **P7 借用 D2 素材** | 从 D2 提取精灵转 CL2 | 技术上可行 | 低技术成本 | **摧毁项目的合法立足点**（仓库现在"不携带素材"，`README.md:27-32`）；DMCA/下架/无法发布/永久无法商业化；风格断裂 | **否决** | **破坏**。D2 的比例与调色板会让画面变成拼贴 |

### 3.1 ROI 数量级表：单位资产成本 vs 内容量增益

估算依据：成本取 §2.1B/C 的逐环节工量之和；「内容量增益」按**玩家实际可感知的量**计算，即**必须过 4000 预算这一关**（`monster.cpp:3516`）。"预算放开前/后"两列是本表的核心——同一资产的 ROI 差一个数量级。

| 资产单位 | 人时（美术+代码+数据+测试） | 内容量增益（**预算未放开**） | 内容量增益（**预算放开后**） | 每人时收益 | 判断 |
|---|---|---|---|---|---|
| **1 个新怪物**（新美术 + 新 AI） | **80-250** | 池 +1（15→16）；**每层可见仍 2-4 种 ⇒ 单层出现概率 ~1/16，玩家可能整轮不遇到** | 每层可见 4-5 种，出现概率 ~1/4，且带来新行为类 | **极低 → 中** | 预算未放开则**不做** |
| **1 个新怪物**（复用精灵 + TRN 换色 + 已有 AI） | **0.5-2** | 同上的概率问题，且**是换皮**（不增行为多样性） | 同上 | 中（但增益质量差） | 只作填充，别当内容 |
| **1 套新地形** | **300-900** | 4 层的视觉底色变化（若替换现有层段）或 +N 层（若扩容） | 同（地形不受精灵预算约束） | **最低** | **不做** |
| **1 个新 boss**（复用精灵 + unique_monstdat + 随从包） | **4-16** | +1 个记忆点事件，**不受预算逐层随机采样影响**（quest/unique 怪在采样循环**之前**用 `PLACE_UNIQUE`/`PLACE_SPECIAL` 预加，`monster.cpp:3463-3479`；但仍计入 `monstimgtot`，故会**挤占**普通怪预算） | 同左 | **最高（新内容类）** | **推荐** |
| **1 个新 boss**（含专属 AI + 新美术） | **100-300** | 同上但质量更高 | 同左 | 低 | 触发条件成立后再议 |
| **1 个新物品类别**（图标 + 掉落图 + `ItemDropNames[]`） | **8-30** | 全程可见（物品不受怪物预算约束），影响每一轮 | 同左 | **高** | **推荐** |
| **1 个新物品图标**（`objcurs2` mod 叠加） | **0.5-2** | 单件物品的辨识度 | 同左 | **最高（单位成本最低）** | **推荐** |
| 对照：**放开预算**（零资产） | **2-8** | — | **每层可见种类 ×2（2.2→4.4）** | **压倒性最高** | **先做这个** |

**触发条件（新增资产何时才值得做）**——四条**同时**成立：
- (a) 精灵预算已放开且平台内存实测通过（否则 §1.2 的"收益被除到零"成立）；
- (b) 零美术手段已穷尽：P0 预算 + P1 编组 + P2 地形主题 + B1 采样 cap 均已落地且实测仍雷同；
- (c) 分发形态已锁 `UNPACKED_MPQS=ON`，或已有 PNG→CL2 编码器（`load_cl2.hpp:22-28` 的阻塞已解）；
- (d) 有能匹配 `diablo.pal` 256 色 + 8 方向一致性的稳定美术产能。

**当前四条全不成立 ⇒ 本维度结论：现在不做新增怪物/新地形。** 例外是**不受预算约束**的三类：新 boss（复用精灵）、新物品图标、新物品类别。

### 3.2 若走新增资产路线，核心体验会变成什么

**我的判断：会变得更接近 D2，而且这不是可以靠"克制"避免的副作用，是路线的内在方向。** 依据三条：

1. **预算约束本身塑造了 D1 的气质。** 每层 2-4 种怪（§2.1A(1)）导致玩家对"这一层是什么"形成强记忆——洞穴就是风筝地狱，地狱就是数值墙。D2 的体验相反：怪物种类多、组合杂、单种存在感低，玩家记住的是**词缀与掉落**而非"这一层的怪"。**放开预算 + 加怪物 = 主动向 D2 的密度美学移动。**【我推断，依据是 §2.1A 的机制事实 + 两作的公认差异】
2. **新增资产的自然重心会滑向"装备与构筑"。** ROI 表显示最便宜的新美术是**物品图标**（0.5-2 人时）而最贵的是怪物（80-250）。任何理性的资源分配都会先做物品——而"物品驱动的内容扩张"正是 D2 的核心循环。仓库内已有先例佐证这个引力：`engine-mod-infra` 原型分支（CONTEXT 记载）做的正是**套装 / 行为化词缀 / 伤害类型 / 3 槽系统**——全是 D2 语汇。
3. **反过来说，最能保住 D1 气质的新内容是"新 boss + 新地形"而非"新怪物 + 新物品"** ——因为 D1 的记忆点是**具名的、一次性的、地点绑定的**（Butcher / Skeleton King / Lazarus）。但新地形是 ROI 最差的一项（300-900 人时），新 boss 又恰好是最便宜的（4-16 人时，复用精灵）。**这是本报告最有用的一条设计指引：想做新内容又想保住 D1 气质，就做 unique boss，不要做怪物池扩容。**

**诚实标注**：以上是设计判断而非可验证事实。可验证的部分只有成本与机制（§2.1）；"哪种更像 D1"属于 §4.1 的品味命题。

### 三条替代路线（对 D2 素材的替代）及其代价与质量上限

| 路线 | 代价 | 质量上限 | 我的判断 |
|---|---|---|---|
| **自绘同风格** | 40-120 人时/怪；需掌握 D1 的 256 色调色板（`diablo.pal`/`diablo_d1.pal` 在仓库根，可作为约束）+ 8 方向一致性 | **上限最高**（可做到与原作难分），但取决于画师 | **唯一能达标的路线**；触发条件成立后走这条 |
| **许可宽松素材库**（CC0/OpenGameArt 类） | 单价极低甚至零 | **上限低**：几乎没有"D1 的 8 方向 + 该调色板 + 该比例"的现成资产；改造成本常高于自绘 | 只适合**图标/UI/装饰**（即 P4），不适合怪物 |
| **程序化生成**（用现有精灵合成/变形/换色） | 低（TRN 已是这条路的原生形态） | **上限 = "换皮"**，而"换皮"正是既有诊断认定的病因之一（`analysis_monster_config_landscape.md` 第三节"全是同族换皮"） | 作为 P3 的延伸可用；**不能解决"新内容"诉求** |

---

## 4. 需要委托方用品味判断的点

1. **是否接受"每层 2-4 种怪"作为 D1 的身份约束，还是把它当 bug 修掉。** 机器只能算出放开预算后种类翻倍、内存翻倍；**但"D1 的层应该显得贫瘠还是丰富"是审美命题**。原版的稀疏感可能正是压抑气质的来源之一——放开预算可能让游戏"更好玩但更不像 D1"。我给不出这个取舍的答案。
2. **换皮的容忍度。** P3（TRN 变体/unique boss）是成本最低的"新内容"，但它生产的正是既有诊断批评的"同族换皮"。**同一手法在"节省成本"框架下是优点、在"内容雷同"框架下是缺点**，取决于委托方认为玩家在意的是"视觉新奇"还是"行为新奇"。机器无法测量玩家的失望阈值。
3. **项目定位的自我认知：精修 D1，还是做资料片。** P5/P6 一旦启动就不可逆地把项目推向后者（外部可见的新怪物/新地形是"资料片"的社会信号）。这不是技术判断，是**作者想被怎样看待**的判断。

---

## 5. 不确定 / 未验证

1. **`image` 列的确切物理单位未能证实。** 我验证了它是预算权重（`monster.cpp:3305,3516-3518`）与 4000 的比较对象，但它**不等于** `frames` 之和×常数（Zombie: 69 帧 → image 799；Fallen: 77 帧 → 543）。**我推断**它是原版遗留的近似 KiB 估值（`monster.cpp:3691` 的日志确实以 KiB 计实际精灵字节数，但那是运行时实测值，与 `image` 列不同源）。**因此"放开到 8000 = 内存翻倍"是我的推断，不是验证结论**——P0 落地前必须用 `LogVerbose` 的实测 KiB 做一次真实测量。**这是本报告最需要先验证的一项。**
2. **`MonsterAIID::Custom = 55` 的意图未知。** 仓库内 0 引用（grep 无命中）。可能是上游预留的 Lua/脚本 AI 接口占位。若上游已有对应机制，我关于"新 AI 必须改 C++"的结论需要修正。
3. **未编译、未运行**（作业约束）。所有采样数字来自我对 `monstdat.tsv` 的 Python 离线模拟，与仓库既有 gtest（`test/sampling_behavior_test.cpp`，据规格称 13/13 PASS）**方法同源但未交叉验证**；我的模拟未建模 quest 怪预加的随机性，故 L13/L14 数字应视为"无任务"条件值。
4. **`.clx` 是否真的无法在默认构建下加载，我只验证了宏定义与缺失编码器两侧，未实测。** 存在我未找到的转换路径的可能（如上游 devilutionx-mpq-tools 在仓库外）。README 提到的 `devilutionx-mpq-tools`（`CMakeLists.txt:168` 注释）可能正是缺失的那一环——**未验证其能否 PNG→CL2**。
5. **美术单价的外部引用是弱证据。** freelancer 单条项目均值（$114）对应帧数远低于 D1 的 552 帧/怪，我用它做的是**下界**而非估值；未找到"D1 规格 8 方向全动画怪物"的可靠公开报价。
6. **未评估 `NUM_MAX_MTYPES = 200` 的实际余量对存档兼容的具体破坏形式**（注释声明与 `MaxMonsters` 绑定，但我未追 `loadsave.cpp` 的完整兼容分支）。
7. **前提更正采纳**：控制者已核实 D1 死亡为"掉半金币 + 装备落地可捡 + 回城复活 + 存档不删"（`Source/player.cpp:2672` `StartPlayerKill` / `DropHalfPlayersGold`），**非"单命 run"**。本报告未依赖"单命"前提，§3/§6 的核心体验判断不受影响。
