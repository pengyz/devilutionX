# E5b：新增美术/地形/怪物的技术管道、法律现实与单位资产 ROI：报告

## 1. 结论摘要
1. 新怪物管道**工具链已存在**（`png2clx`），但"能转格式"≠"能进游戏"：还要过 `MonsterData` 41 列 TSV、6 动画槽硬约束、命名路径约定、AI 注册表、掉落/平衡、以及至少 2 类既有回归测试（`ai_registry_test`、`sampling_behavior_test`）。单个朴素怪物估算 **3-6 人日**；带独特 boss/新 AI 行为的估算 **2-4 人周**。
2. 新地形是**引擎级工程，不是素材投放**：`dungeon_type` 只有 7 个硬编码值，`CreateDungeon`/`LoadMinData`/`LoadLevelSOLData` 是穷举 switch，新增一类要新增枚举值 + 4-5 处 switch 分支 + 全新 `drlg_l*` 生成算法（现有 4 套各 1200-2900 行）+ `.min`/`.sol`/`.til`/`.cel` 美术 + 存档兼容 + 4 个 `drlg_l*_test.cpp` 同构测试。估算 **4-8 人周**起，且这是"复用现有生成算法接一套新美术"的下限；写全新生成算法要再加数周。
3. **法律现实明确无歧义**：`LICENSE.md:15-16` 只许"internal business / non-commercial / personal use"且分发必须免费非商业；`README.md:84` 重复"non-commercial use only"。在现行许可下，**唯一合法的内容形态是：非商业、免费分发、面向已拥有正版 D1/Hellfire 资源的玩家的引擎+代码补丁（不含暴雪美术数据）**。任何"卖钱"路径同时被引擎许可（本条）与素材版权（下条）挡死，换引擎许可证本身就是另一个项目。
4. **从 D2 抽素材是法律上更差的选择，不是更好的**：D2 美术版权仍归暴雪所有，且暴雪对 D2 相关衍生工具/mod 有过 GitHub DMCA 执法先例（见 2.2），风险不因"用的是 D2 不是 D1"而降低，反而多背一层"跨作品移植"的可疑性。三条替代路线里，**程序化/纯自绘同风格像素素材**是唯一无版权负债的路线，但质量上限受限于美术投入而非工具。
5. **单位 ROI 上，新增资产的边际收益低于重组已有资产**：控制者已核实"112 种怪物/90 件独特物品已存在但只进池 10-13 类"（factcheck §2）。加 1 个新怪物的开发成本（3-6 人日）能换来的"新鲜感"与"把已有 112 类里没打过的 20-30 类怪物重新编排进当前层段"相比，后者成本几乎是 0（改 TSV `minDunLvl`/`maxDunLvl`/`availability` 列），却能覆盖更大比例的塌陷问题。新资产只在"已有 112 类怪物穷尽仍无法表达某个具体机制"时才值得做。
6. 触发新资产投入的**唯一充分条件**：存在一个已验证的塌陷成因（如"某层段缺乏破护甲/破抗性的机制类"），且已核实**现有 112 类怪物的 AI/属性组合内确实无一能表达它**（需要 E5a 的成因分析交叉验证），否则默认结论是"不做"。

## 2. 证据

### 2.1 仓库内

#### 2.1.1 新怪物管道
- PNG→精灵工具已存在，用法确切：`png2clx <input.png> <output.clx> [num_frames]`；`num_frames` 是**垂直堆叠帧数**（不是每方向独立），内部先用 `SDL_ConvertSurfaceFormat` 转 8 位索引色再走 `SurfaceToClx`（`tools/png2clx/main.cpp:15,27,30,37`）。**约束**：调色板索引 0 固定作透明色（`main.cpp:37` 硬编码 `0`），意味着源 PNG 必须让"透明"落在调色板槽 0，否则整张图会错误挖空；工具本身**不做**8 方向/6 动作的帧序校验，全靠上游手工切帧对齐。
- 引擎侧动画槛严格：6 个固定槛位 `MonsterGraphic::{Stand,Walk,Attack,GotHit,Death,Special}`（`Source/monster.h:112-119`，与 factcheck 一致），`MonsterData` 的 `hasAnim(index)` 决定该槛位是否加载（`LoadMonsterSpritesData`，`Source/monster.cpp:3217-3225` 用 `MultiFileLoader` 按 `hasAnim` 过滤 6 个可能文件）。
- `monstdat.tsv` 需填 **41 列**（`assets/txtdata/monsters/monstdat.tsv:1`）：核心动画/资产列是 `assetsSuffix`/`soundSuffix`/`trnFile`/`width`/`hasSpecial`/`hasSpecialSound`/`frames[6]`/`rate[6]`（第 3,4,5,7,9,10,11,12 列），其余是平衡数值（`minDunLvl`/`maxDunLvl`/`level`/`hitPoints*`/`ai`/`abilityFlags`/`toHit`/`minDamage`/`maxDamage`/`resistance`/`armorClass`/`treasure`/`exp` 等，第 13-41 列）。
- 精灵路径约定：`monsters\<spritePath><AnimLetter>` + 扩展名由 `DEVILUTIONX_CL2_EXT` 决定（`Source/monster.cpp:3224` 的 `FileNameWithCharAffixGenerator`，与 factcheck 一致）；音效路径 `monsters\<soundSuffix><prefix><frame>.wav`（`Source/monster.cpp:3576,3585`）。两套路径独立命名，容易在只改一处时漏改。
- 放进关卡的机制：`GetLevelMTypes()`（`Source/monster.cpp:3439`）先按任务/固定 boss 硬编码 `AddMonsterType`（如 `MT_HORKSPWN`/`MT_DEFILER`/`MT_NAKRUL`，行 3450-3459），再进入随机采样循环：过滤 `IsMonsterAvailable`、按 `image` 字节预算（`monstimgtot < 4000`，`Source/monster.cpp:3516,3518`）与槛数上限 `MaxLvlMTypes = 24`（`Source/monster.h:39`）裁剪候选池，并有 B1 反垄断采样上限（同类行为在洞穴/地狱层段各不超 2 个，`Source/monster.cpp:3505-3536` 的 `classCounts`/`capKite`/`capSameClass`）。新怪物要真正"出现"，必须让其 `minDunLvl`/`maxDunLvl`/`availability` 落入某层段区间，且不与 B1 上限冲突。
- 必须同步的既有测试（改怪物表/AI 会牵连）：`test/ai_registry_test.cpp`（校验 `AiProc` 每个 `MonsterAIID` 都有非空函数指针）、`test/sampling_behavior_test.cpp`（382 行，锁死采样上限行为，新增行为类可能触发它的断言）；无专门"新增怪物"用例目录（未找到 `test/*monst*`）。
- **独特/boss 怪的额外列**：`unique_monstdat.tsv` 20 列（`type/name/trn/level/maxHp/ai/intelligence/minDamage/maxDamage/reduce*/resistance/monsterPack/customToHit/customArmorClass/talkMessage`），比普通怪多"对话/自定义命中/护甲"三项，且独特怪要单独 `AddMonsterType(UniqueMonsterType::X, PLACE_UNIQUE)` 硬编码接入点（`Source/monster.cpp:566-605` 例举 7 处，逐个任务/层段手写）。

**工量估算（假设：1 人日=6 有效工时，已有美术素材、只算工程接入）**：
| 环节 | 工作内容 | 估算 |
|---|---|---|
| 精灵/音效制作+转换 | 6 槛动画帧对齐+调色板对齐+png2clx跑通+.wav 命名 | 1-3 人日（视是否已有像素画师产出） |
| TSV 填表+平衡 | 41 列填写+参照同层段怪物数值校准 | 0.5-1 人日 |
| 接入点+AI（若复用现成 AI） | `GetLevelMTypes` 层段区间校准+B1 上限联调 | 0.5 人日 |
| 新 AI 行为（若需要） | 新 `MonsterAIID`+`AiProc` 实现+`ai_registry_test` 过 | 2-5 人日 |
| 回归测试 | 跑 `ai_registry_test`/`sampling_behavior_test`/相关 eval | 0.5 人日 |
| **合计（复用 AI）** | | **3-6 人日** |
| **合计（新 AI/boss）** | | **2-4 人周** |

**最容易被低估的环节**：调色板索引 0=透明的硬约束（`main.cpp:37`）——业余像素画师用标准 PNG alpha 通道画图,若透明色未落在调色板槛 0，`png2clx` 会转出实心色块而非透明,这类错误在批量转换脚本里不会报错,只在游戏内渲染时才可见,返工成本最高。
**最可能返工的环节**：`sampling_behavior_test.cpp`（382 行的采样锁死测试)——加新怪物改变了某层段候选池组成，即使数值本身没错,也可能撬动该测试里断言的具体采样序列/分布,需要重新核对测试语义而非简单改数字。

#### 2.1.2 新地形管道
- `dungeon_type` 硬编码 7 值 + 边界常量 `DTYPE_LAST = DTYPE_CRYPT`（`Source/levels/gendung_defs.hpp:15-25`，与 factcheck 一致）。
- 关卡数据加载是**穷举 switch**，无数据驱动路径：`LoadMinData`（`Source/levels/dun_tile_data.cpp:69-95`）与 `LoadLevelSOLData`（同文件 `:99-138+`，本报告只读到 `DTYPE_CAVES` 分支即 138 行，`DTYPE_HELL/NEST/CRYPT` 分支在后续行未逐一复核，但结构同构）逐类型 `case`，新增类型必须新增 `case` 并提供对应 `.min`/`.sol` 文件路径。
- 关卡生成算法**四选一分派**，且 `NEST` 复用 `CAVES` 算法、`CRYPT` 复用 `CATHEDRAL`（Hellfire 就是这样省成本的）：`CreateDungeon`（`Source/levels/gendung.cpp:356-380`）把 7 个 `dungeon_type` 映射到 4 个 `CreateL{1,2,3,4}Dungeon` 之一（`DTYPE_CATHEDRAL`/`DTYPE_CRYPT`→`CreateL5Dungeon`(命名沿用旧编号) ；`DTYPE_CAVES`/`DTYPE_NEST`→`CreateL3Dungeon`；`DTYPE_CATACOMBS`→`CreateL2Dungeon`；`DTYPE_HELL`→`CreateL4Dungeon`）。**这意味着 Hellfire 加新地形的真实历史成本是"复用已有生成算法换一套美术"，不是"写新算法"**——但即便如此，仍要新增枚举值+4 处 switch 分支（`LoadMinData`/`LoadLevelSOLData`/`GetLevelTypeFromLevel`-类映射/`CreateDungeon`）。
- 生成算法体量（复用路径的成本上限参照物）：`drlg_l1.cpp` 1346 行、`drlg_l2.cpp` 2859 行、`drlg_l3.cpp` 2217 行、`drlg_l4.cpp` 1255 行（`wc -l`），合计 7677 行——这是"从零写一套新生成算法"要对标的量级，不是要抄，但说明单套算法的复杂度基线。
- `themes.cpp` 里怪物出没/主题/物件是**按 `leveltype` 散落判断**（如 `IsAnyOf(leveltype, DTYPE_CAVES, DTYPE_NEST)`、`leveltype == DTYPE_HELL && themeCount > 0`，`Source/levels/themes.cpp:106,115,124,839,892,978` 等 12 处匹配），新地形类型要在这些判断点里逐一决定归属，否则物件/主题会缺失或错误共享其他类型的规则。
- `objdat.tsv` 有专门 `levelType` 列（第 5 列，`assets/txtdata/objects/objdat.tsv:5`），新地形要给场景物件表新增该类型的行才有可放置物件；否则新层段空场景。
- 存档兼容与多人同步：与 factcheck §1/§3 一致，`quest._qlvltype` 持久化 `dungeon_type`，`DTYPE_LAST` 是存档校验边界，新增枚举值要过 `MonsterConversionData`/`LevelConversionData` 式转换范式（factcheck 已核实）。
- 既有回归测试：`test/drlg_l1_test.cpp`～`drlg_l4_test.cpp` 四个文件逐算法测（未读取具体行数/断言细节，但文件存在本身说明每套生成算法都有专属确定性/结构测试，新地形若复用某算法需通过其对应测试，新算法则需要新建同构测试文件——这是**新增测试基建**成本，非既有测试可覆盖）。

**最小可玩地形改动清单**（复用现有 4 套算法之一，只换美术+枚举，不写新生成器）：
1. `gendung_defs.hpp` 新增 `DTYPE_X`，前移/调整 `DTYPE_LAST`
2. `LoadMinData`/`LoadLevelSOLData` 各加 1 个 `case`，指向新 `.min`/`.sol` 路径
3. `CreateDungeon` 把新类型路由到某个既有 `CreateL{1..4}Dungeon`
4. `themes.cpp` 全部 `IsAnyOf`/`leveltype ==` 判断点逐一决定新类型归属（12+ 处）
5. `objdat.tsv` 新增该 `levelType` 的物件行；`monstdat.tsv` 的 `minDunLvl`/`maxDunLvl` 覆盖新层段
6. 存档：`quest._qlvltype` 边界校验+转换数据同步（走 factcheck 已核实的既有范式）
7. 美术：新 `.min`/`.til`/`.sol`/`.cel` tileset 一套（工具链是否支持逆向不明，见 §5）
8. 新建对应 `drlg_lX_test.cpp` 同构测试

**工量估算**：复用现有算法+新美术：**4-8 人周**（大头是美术 tileset 制作与逐点 themes 判断的联调回归，不是代码量）；写全新生成算法：再加 **4-8 人周**。

### 2.2 外部
- Blizzard 对 Diablo II: Resurrected 相关离线补丁/hack mod 发过 GitHub DMCA 下架通知：[TechNadu 报道](https://www.technadu.com/blizzard-entertainment-against-circulation-diablo-ii-resurrected-offline-patches/282718/#1)、[俄语转述](https://vgtimes.ru/gaming-news/78060-blizzard-zastavila-github-udalit-vse-versii-moda-dlya-vzloma-diablo-2-resurrected.html)（**市场传闻/二次转述，未见到 Blizzard 官方公告原文**，但方向一致：暴雪确有对 D2 衍生工具执法的先例，法律风险不是假设）。Blizzard 官方版权投诉入口存在（[法律页面](https://www.blizzard.com/fr-fr/legal/26b8b902-0480-43dd-81ba-2fc75481f1dc/violation-du-droit-dauteur)），说明其确实运营着主动的版权执法流程。
- Sustainable Use License 官方条款解读（n8n 是该许可证的原始发布者）：["non-commercial" 与 "internal business purposes" 的边界在于是否向第三方收费或作为商业产品的一部分]（[n8n 公告](https://blog.n8n.io/announcing-new-sustainable-use-license/)、[n8n 文档](https://docs.n8n.io/n8n-community-license)）——与 `LICENSE.md:15` 本仓库文本逐字一致，佐证控制者 §1 的解读无歧义。

## 3. 候选方案

| 方案 | 机制要点 | 引擎可行性 | 成本量级 | 风险 | ROI 判断 | 对核心体验的影响 |
|---|---|---|---|---|---|---|
| A. 新增 1 个普通怪物（复用 AI） | 新精灵+TSV 41 列+层段区间调优 | ✅ 管道完整存在（§2.1.1） | 3-6 人日 | 低（透明色/回归测试返工风险） | 低——112 类已有怪物中多数未进池，边际新鲜感 < 重排现有池 | 无关（不改机制，只加内容量） |
| B. 新增 1 个独特/boss 怪 | 独特表 20 列+对话+硬编码接入点+可能新 AI | ✅ 管道存在但接入点分散 | 2-4 人周 | 中（分散在 7+ 处硬编码调用点，遗漏即"幽灵怪") | 中——boss 战确实能提供独特体验，但仅 1 个不解决塌陷面 | 视机制设计而定，可强化"里程碑感" |
| C. 新增 1 套地形 tileset（复用生成算法） | 新枚举+4 处 switch+themes 12+ 判断点+存档兼容 | ⚠️ 引擎级工程，非素材投放（§2.1.2） | 4-8 人周 | 高（存档兼容、多人 delta、themes 判断点遗漏均不可小视） | 低——视觉新鲜感高但不解决"打法趋同"的塌陷根因 | 强——改变视觉/氛围但不改变机制深度 |
| D. 新增地形+全新生成算法 | 同 C + 全新 drlg_l* 实现 | ⚠️ 需对标 1200-2900 行/套的复杂度 | 8-16 人周 | 高（新算法的边界 bug 需要长期打磨，参照现有 4 套的历史 bug 修复量） | 极低——投入产出比最差的选项 | 强，但成本不成比例 |
| E. 从 D2 抠图/移植素材 | 用现成 D2 美术换皮 | ✅ 技术上可行（png2clx 不限制来源） | 表面上更省（省了美术制作） | **极高**——版权/商标双重风险（§2.2），且是主动侵权行为不是灰色地带 | 负——法律风险抵消一切成本节省 | 不适用（不应做） |
| F. 重组已有 112 类怪物/90 件独特物品的层段分布 | 改 TSV 的 `minDunLvl`/`maxDunLvl`/`availability` 列，零新美术 | ✅ 数据驱动改动，零引擎工程 | 数小时到 1-2 人日/层段 | 低 | **高**——与 A-D 同口径对比，成本低 1-2 个数量级，覆盖面更大（这是 E5a 应深入的方向，此处按理解给出对比估算，**标注为推断**） | 直接命中"塌陷"（内容已存在但未被体验到） |

## 4. 需要委托方用品味判断的点
1. **视觉新鲜感 vs 机制深度的取舍权重**：方案 C/D 能提供"看起来不一样"的体验，但 factcheck 已明确塌陷成因不是资产短缺而是采样构成/行为重复。是否仍要为"氛围"投入 4-8 人周，是审美/项目定位判断，非技术判断。
2. **法律风险容忍度**：Sustainable Use License 下"非商业免费分发"是唯一确认安全的形态，若委托方有意愿探索"换引擎许可"或"完全自绘素材走商业发行"，这是需要外部法律意见的商业决策，本报告只能标出风险边界，不能代替法律判断。
3. **"新 boss"是否值得作为里程碑式内容**：方案 B 的 ROI 判断依赖于"boss 战在玩家心智中的权重"，这是设计品味而非可计算的 ROI 数字。

## 5. 不确定 / 未验证
- `LoadLevelSOLData` 的 `DTYPE_HELL`/`DTYPE_NEST`/`DTYPE_CRYPT` 分支未逐字逐行复核（只读到 138 行/219 行），结构假设为同构 `case`，未 100% 确认。
- `.min`/`.sol`/`.til` tileset 的**反向编辑工具链**是否存在（即"美术师能否不写代码肉眼画一套新 tileset"）未核实——只核实了精灵格式的 `png2clx`，未找到/未搜索地形 tileset 的等价工具，这直接影响方案 C/D 的美术成本估算是否准确，是本报告最大的成本类未知项。
- `themes.cpp` 的 12 处判断点是 grep 命中数，未逐一读取上下文确认是否遗漏其他隐式判断（如通过 `currlevel` 而非 `leveltype` 间接分支的情况未穷举)。
- E6 的商业化结论与本报告 §1.3/§2.1 完全一致（法律墙不可绕过），但 E6 报告本身未直接读取，此处对比依赖 factcheck 转述,未独立交叉核对 E6 原文。
- 单位 ROI 表（§3）的"高/中/低"是**数量级定性判断**，未做玩家留存/时长的量化模型（这类数据在仓库内不可得，需要外部产品数据支撑，已标注为推断)。
