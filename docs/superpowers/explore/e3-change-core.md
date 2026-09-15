# E3 允许改动核心体验：L1 / L2 / L3 三档对比 — 报告

## 1. 结论摘要

1. **ROI 最高是 L1，且优势不是"便宜"而是"引擎有现成承载面"**：D1 物品系统已有 81 个 IPL 效果枚举与完整 prefix/suffix 生成管线（`Source/tables/itemdat.h:520-600`、`Source/items.cpp:1191/1227/1326`），加"一条成长轴"是**在既有轴上加类别**，不是新建子系统。
2. **但 L1 的真实瓶颈被普遍低估**：每件物品**硬性只有 1 前缀 + 1 后缀槽**（`Source/items.h:247-248`），且**无插槽系统**（`grep -i socket Source/*.{cpp,h}` = 0 命中）。所以"机制化词缀"与"数值词缀"**互相排挤同一个槽**——不加槽位就等于用机制词缀挤掉数值词缀，这本身是平衡改动，不是纯增量。
3. **最危险的已验证事实：存档里的物品是"种子重生成"的**，`RecreateItem → SetupAllItems`（`Source/items.cpp:3553,3603`）。任何改动词缀表/生成谓词的档位，都会让**旧存档里已有的物品静默变成别的物品**（已有实证：`docs/knowledge/gotcha_vendor_predicate_seed_drift.md`，同种子 `War Staff of haste` 变 `Book of Flame Wave`）。这条对 L1/L2 同等致命，且**不能靠"加字段时小心"规避**。
4. **存档头部有真实余量，物品结构没有**：`PlayerPack` 有 `reserved`/`reserved2[2]`/`wReserved8`/`reserved3[20]`（`Source/pack.h:73-86`）→ 玩家级新轴（点数/标记/进度）可**零格式破坏**落地；而 `ItemPack` 是 16 字节紧凑结构无保留位（`Source/pack.h:20-31`），物品级新状态必须动格式或榨 `dwBuff` 空闲位（`CF_HELLFIRE`=bit0、`CF_UIDOFFSET`=bit1-4，bit5+ 空闲，`Source/items.h:176-179`）。**L1 应该设计成"玩家级轴"而不是"物品级轴"** —— 这是引擎给出的方向性答案。
5. **L2（装备成主线）在本引擎上的成本不是线性的**：技能持久化只有 `pSplLvl[37] + pSplLvl2[10]` = 47 槽（`Source/pack.h:59,84`），内存端却是 `_pSplLvl[64]`（`Source/player.h:325`）；`_pMemSpells` 是 `uint64_t` 位掩码（`:329`）→ 超过 64 个"技能式构筑单元"直接撞类型上限。D2 式构筑要的宽度与这套定长格式对撞。
6. **L3（换核）在本仓库是死路，但不是因为技术**：许可禁止商业化与有偿分发（`LICENSE.md:15-16`、`README.md:84`，控制者 §1 已核实）；且美术侧地形是 7 值硬编码枚举、`.til/.min/.sol` **无往返工具链**（控制者 §3、§5-E5b）。L3 的成本几乎全部落在"合法可发行的美术与引擎替换"上，而这恰恰是收益最不确定的部分。
7. **`engine-mod-infra` 原型应"选择性摘取，不整体复活"**：Source/ 侧 48 文件 +2602/-122、13 个新文件、`loadsave.cpp` +206 行含 ~84 行序列化改动，但**零 eval 用例**（`git diff --name-only … -- eval/` 为空）、只 +29 行测试。而 merge-base 是 2026-06-27，主线此后已 **311 提交 / Source 227 文件 6265+/3426-**，冲突正好集中在原型改得最重的文件（`items.cpp` 220+/87-、`inv.cpp` 225+/151-、`monster.cpp` 65+/48-）。**重推导比 rebase 便宜**。
8. 原型里**唯一值得直接复活的是"背包 40→60 + pack 40↔60 运行时翻译"**（`ab8c0653c`）——它是 Infra 性质、玩家零平衡感知、且已解决了向后兼容；其余（9 标记 3 槽、4 套装、Holy/Poison/Cold、12 条 proc 词缀）都是**无规格的设计投机**，且 `StashVersion` 已在原型里被抢先 bump 到 1（主线仍为 0，`Source/loadsave.cpp:2428`）→ 直接合并会制造版本号语义冲突。
9. **三个月只够 L1，且交付物应该是"一条轴 + 判据 + 回归网"而非"多个系统"**。L2 三个月只能做出无法评估的半成品（原型已经证明了这一点：4 个系统同时上、无规格、无 eval）。
10. **档位边界建议修正**：把 L1 从"新增一条成长轴"改为"**新增一条玩家级、可关闭、且与既有装备槽不争资源的成长轴**"。理由见第 2/4 条：物品级实现会撞满槽与种子重生成两道墙，档位定义若不写清承载层，L1 会在实施中滑向 L2 成本。

## 2. 证据

### 2.1 仓库内

| 结论 | 证据 |
|---|---|
| 物品效果枚举已有 81 项，扩展是"加枚举 + 加 TSV 行 + 加 `SaveItemPower` 分支" | `Source/tables/itemdat.h:520-600`（81 个 `IPL_`）；`Source/items.cpp:718 SaveItemPower` |
| 每件物品仅 1 前缀 1 后缀，无插槽 | `Source/items.h:247-248`；`grep -in socket Source/*.cpp Source/*.h` = 0 |
| 词缀生成入口集中，改造面小 | `Source/items.cpp:1191 GetItemPowerPrefixAndSuffix`、`:1227 GetItemPower`、`:1326 GetItemBonus` |
| 存档物品按种子重生成 → 词缀表/谓词改动会改写旧存档内容 | `Source/items.cpp:3553 RecreateItem` → `:3603 SetupAllItems`；`Source/pack.cpp:348`；实证 `docs/knowledge/gotcha_vendor_predicate_seed_drift.md` |
| 玩家级新字段有零破坏余量 | `Source/pack.h:73`(`reserved`)、`:76`(`reserved2[2]`)、`:85`(`wReserved8`)、`:88`(`reserved3[20]`) |
| 物品级新字段无余量（16B 紧凑） | `Source/pack.h:20-31`；`dwBuff` 空闲位见 `Source/items.h:176-179` |
| 技能/构筑单元持久化上限 47（内存 64、位掩码 64） | `Source/pack.h:59 pSplLvl[37]`、`:84 pSplLvl2[10]`；`Source/player.h:325 _pSplLvl[64]`、`:329 _pMemSpells` |
| 属性/等级上限是 per-class 数据驱动，改上限不动格式 | `Source/tables/playerdat.hpp:69 maxStr`；`Source/player.cpp:1983 getMaxCharacterLevel`、`:1980` clamp；`Source/pack.cpp:382-388` 读档 clamp |
| 怪物 AI 已有 ~32 个行为类型可复用（L1 的"反制面"不缺素材） | `Source/tables/monstdat.h:24-56 MonsterAIID` |
| Lua 只有 11 个事件挂点，且**没有 on-hit/on-kill 挂点** → 机制化词缀无法纯 Lua 落地，必须动 C++ | `Source/lua/lua_event.cpp`（`CallLuaEvent("…")` 共 11 处；有 `OnMonsterTakeDamage`/`OnPlayerTakeDamage`/`OnPlayerGainExperience`） |
| 原型规模与质量：48 文件、13 新文件、序列化改动 ~84 行、**0 eval 用例** | `git diff --stat feature/qol-upgrades...engine-mod-infra -- Source/` = 48 files 2602+/122-；`-- eval/` 为空；`-- test/` 仅 +29 |
| 原型与主线已严重分叉，冲突集中在原型主战场 | merge-base `2026-06-27`；主线此后 311 提交、Source 227 文件 6265+/3426-；`items.cpp` 220+/87-、`inv.cpp` 225+/151- |
| 原型抢先 bump 了 `StashVersion`（主线仍 0） | 原型 `964fef3e8`；主线 `Source/loadsave.cpp:2428 constexpr uint8_t StashVersion = 0` |
| 原型的背包扩容已做双向兼容（值得摘取） | `ab8c0653c`（`pack.h/cpp` 40↔60 运行时翻译、`heroitems` 版本字节）；主线仍 `InventoryGridCells = 40`（`Source/player.h:38`）、`InventorySizeInSlots {10,4}`（`Source/inv.h:22`） |
| 新持久化字段历史上两次损坏存档（写读不对称） | `docs/knowledge/gotcha_save_stack_append.md`、`gotcha_save_bid_overwrite.md` |
| 引用控制者已核实：死亡不删档（掉半金币+装备可捡回+回城复活） | `Source/player.cpp:2672 StartPlayerKill`（controller-factcheck §5-E1） |

### 2.2 外部（均来自 `web_search`，**未二次 fetch 核验**；机制存在性可信，数字按"市场传闻"处理）

| 结论 | URL |
|---|---|
| D2 符文之语 = "把插槽变成配方空间"，是"装备成主线"的原型机制 | https://www.rockpapershotgun.com/diablo-2-resurrected-runes-runewords |
| Grim Dawn Components/Augments = "消耗品镶嵌到装备"，在**不增加词缀槽**的前提下增加构筑维度 | https://grimdawn-archive.fandom.com/wiki/Augments |
| Grim Dawn Devotion = **玩家级**星座图（与装备槽不争资源），L1 的最佳借鉴形态 | https://grimdawn-archive.fandom.com/wiki/Devotion |
| ToME Prodigies = 高门槛少量"改变玩法规则"的选择，而非大量小加成 | https://te4.org/w/index.php?title=Prodigies |
| Last Epoch 技能专精 = "少量技能深度化"替代"大量技能" | https://www.escapistmagazine.com/how-does-skill-specialization-work-in-last-epoch/ |
| Path of Achra = 极小体量下靠"协同乘算"产生 build 分化（本项目可对标的成本档） | https://steamcommunity.com/app/2128270/discussions/0/4334229775341014889/ |
| Caves of Qud 变异/缺陷 = 用**负面选项**换强度，属"取舍型"成长轴 | https://www.switchbladegaming.com/caves-of-qud/character-creation-guide/ |
| D2R 社区对"护符栏/背包便利"长期激烈反对，理由是**移除机会成本** | https://us.forums.blizzard.com/en/d2r/t/why-personal-loot-item-filters-and-charm-inventory-will-ruin-diablo-2/601/37 |
| Belzebub（D1 HD mod）被明确批评"不适合想要 D1 精神的人" → 老玩家反弹的直接先例 | https://www.gog.com/es/game/diablo_1_hd_mod_belzebub?page=3 |
| Belzebub 仍有可观装机量（"改核"路线有受众，但与原教旨受众分裂） | https://playtracker.net/insight/game/116792 |
| Median XL 属 D2 **总体转换**类别（L3 的现实参照：它靠"另一个游戏"聚众） | https://diablo-archive.fandom.com/wiki/Mods_(Diablo_II) |
| D4 传奇 Aspect 系统被官方称"改变了游戏" → 说明"物品赋予机制"确实能把装备推成主线（L2 的收益上限证据） | https://www.gamespot.com/articles/diablo-4-new-legendary-system-has-transformed-the-game-blizzard-says/1100-6504551/ |
| Halls of Torment 破 50 万销量（小团队、低体量、非商业化对本项目仍不适用，仅作规模参照） | https://www.gamesmarket.global/tiny-teams-festival-halls-of-torment-knackt-500000-sales-d267607887e4dbc5c246ff9b7493e92a/ |
| PoE 洗点成本长期争议 → "不可逆构筑选择"是设计取舍点，不是纯负面 | http://www.pathofexile.com/forum/view-thread/3617490/ |

## 3. 候选方案

| 方案 | 机制要点 | 引擎可行性 | 成本量级 | 风险 | ROI 判断 | 对核心体验的影响 |
|---|---|---|---|---|---|---|
| **L1** 玩家级"取舍型成长轴"（Devotion/Prodigy/Qud 变异混血） | 每 N 级或每任务给 1 点，投入到 ~12-16 个节点；**关键约束：多数节点带负面代价**（如 +伤害/-最大生命、+视野/-抗性），不进装备槽 | 高。玩家级字段落 `PlayerPack::reserved3[20]`（`Source/pack.h:88`）零格式破坏；效果挂点复用 `Source/player.cpp` 伤害/属性计算；UI 复用 `Source/panels/charpanel.cpp`；**不动 `items.cpp` 生成路径 → 规避种子漂移** | ~3-6 人周（含 eval + timedemo 回归） | 中：节点数值膨胀会顶到属性/等级 clamp（`Source/pack.cpp:382-388`）；UI 面板是新美术小件 | **最高** | 强化（新增取舍，不替换空间博弈） |
| **L1'**（不推荐的实现方式）机制化装备词缀 | 12 条 on-hit/on-kill proc（即原型 `ee61e228e` 路线） | 中。要动 `Item` 结构 + 2 处序列化 + TSV + 映射（控制者 §6 成本模式）；**且撞满槽（1 前 1 后）与种子重生成两道墙** | ~4-8 人周 + 存档迁移 | 高：旧存档物品静默改变（§2.1 实证） | 低于 L1 | 改变（装备开始抢注意力，滑向 L2） |
| **L2** 装备/构筑成主线（D2 方向：插槽 + 符文之语 + 套装 + 伤害类型） | 新增插槽字段、配方表、套装集合、Holy/Poison/Cold | 中低。`ItemPack` 无保留位（`Source/pack.h:20-31`）→ 必须版本化物品格式；网络 `TItem`（`Source/msg.h:534-547`）同步扩展；构筑宽度撞 47 槽 / `uint64_t` 掩码上限 | ~6-12 人月 | 高×3：存档迁移、MP delta 面（控制者 §6）、平衡重做（掉落表 `Source/items.cpp:1907/3297`） | 中（收益真实但成本非线性） | **替换**核心循环（空间博弈降为背景） |
| **L3** 换核（D1 仅作引擎/素材） | 新地形、新怪物族、新循环 | 低。7 值硬编码地形枚举 + `.til/.min/.sol` 无往返工具链（控制者 §3/§5）；许可禁商业化（`LICENSE.md:15-16`） | ~1-3 人年 | 极高：法律天花板 + 美术管道自建 + 受众重建 | 最低 | 无关（不再是同一游戏） |

### L1：身份剩余度 / 决策 / MVP

- **D1 身份剩余度：高**。保留不可牺牲项：地牢内不能跑、单键攻击、死亡跑尸、退出重进重掷（宪章 `…design-charter.md:52`）。牺牲项：**"角色成长完全由属性点决定"** 这一条不属宪章独特性表列项，可动。
- **三个关键决策**：① 点数来源=**升级**（与既有节奏耦合、无新内容）还是**层段首杀/任务**（推动探索、但要新持久化位）→ 后者更贴 D1 空间博弈，成本 +1 位标记。② 节点是否**不可逆**（PoE 式：产生真取舍、但劝退试错）还是可洗点（降低压力、削弱取舍）→ 建议不可逆 + 新档试错。③ 是否**默认开启**（=改核心体验，需宪章豁免）还是单一开关默认关（=Depth 类，社区风险最低）→ 建议后者。
- **MVP（≤2 人周）**：4 个节点（各带负面代价）+ 升级发点 + 复用字符面板显示 + 2 条 eval 用例 + timedemo 通过。
- **失败判据（任一出现即放弃）**：① 4 节点里出现"无脑必选"（即负面代价不成立，取舍退化为纯加成）；② 需要动 `items.cpp` 生成路径才能表达节点效果（说明轴选错了承载层，成本滑向 L2）；③ timedemo/存档回归无法在不重新生成 golden 数据的前提下通过（说明已污染种子链）。

### L2：身份剩余度 / 决策 / MVP

- **D1 身份剩余度：中低**。"装备是构筑核心"会让"我该带哪几瓶药水下几层"退居次要；D2R 社区史显示移除机会成本会引发强反弹（见 §2.2）。
- **三个关键决策**：① 插槽是**新增字段**（干净，但物品格式版本化 + MP `TItem` 扩展）还是**复用 `dwBuff` 空闲位 bit5+**（零格式破坏，但位宽极窄、语义污染）→ 前者正确、后者是快速原型。② 深度来自**配方**（符文之语：可发现、有信息层）还是**词缀数量**（撞 1 前 1 后上限）→ 必须配方。③ 是否**同时重做掉落表**（不做则新构筑单元拿不到 → 系统空转）→ 必须做，这是 L2 成本非线性的主因。
- **MVP（≤6 人周）**：1 类可插槽基底 + 3 个插入物 + 1 个 2 件配方 + 掉落接入 + 存档迁移测试。
- **失败判据**：① MVP 后玩家最优策略仍是"攒属性点 + 找高数值前缀"（说明装备没成为主线，只是加了噪声）；② 存档迁移在 MVP 阶段就需要第二次版本 bump（说明格式设计不收敛）；③ MP delta 出现不同步（`Source/msg.cpp` 面未覆盖）。

### L3：身份剩余度 / 决策 / MVP

- **D1 身份剩余度：低（有意为之）**。可牺牲全部标志机制；唯一保留的是引擎与手感。
- **三个关键决策**：① 是否**脱离本仓库/换引擎**（否则许可墙锁死一切发行形态）。② 美术是**自建 `.min/.sol` 工具链**（周级工程，控制者 §5 已核实无现成工具）还是**只用精灵不加地形**（`tools/png2clx` 可用，控制者 §3）→ 后者是唯一现实解。③ 目标受众是 D1 老玩家还是新受众（决定是否还叫 Better D1）。
- **MVP（≤4 人周）**：**不写代码**。做一份"合法可发行形态 + 美术来源 + 目标受众"三页论证；论证不成立即整档否决。
- **失败判据**：MVP 论证无法给出**不含暴雪素材且不受 Sustainable Use License 限制**的发行路径 → 立即放弃（当前证据倾向于此）。

## 3.9 必答三问

- **哪档 ROI 最高**：**L1**。比较口径 = (可评估的体验增量) ÷ (人周 × 存档/MP 风险系数)。L1 分子中等、分母最小（玩家级字段有保留位、零 `items.cpp` 触碰、可开关）；L2 分子最大但分母平方级增长（格式 + MP + 平衡三重面）；L3 分母含法律不可解项。**依赖假设**：`reserved3[20]` 真为未用（已读到声明，未运行时验证）；以及"取舍型节点"能靠数值设计做出非必选（这是品味问题，见 §4）。
- **只投 3 个月选哪档、交付到什么程度**：**L1**，交付 = 单开关、12-16 节点、点数来源接入、字符面板显示、**≥4 条 eval 用例 + timedemo 绿**、一份 7 段规格。明确**不交付**：任何物品结构改动、任何掉落表重排。
- **`engine-mod-infra` 的 L2 味原型：复活还是重推导** → **重推导为主，选择性摘取为辅**。依据三条，均已验证：① **代码资产已过期**：merge-base 2026-06-27，主线此后 311 提交、Source 227 文件 6265+/3426-，且冲突集中在原型改得最重的 `items.cpp`/`inv.cpp`/`monster.cpp`——rebase 成本接近重写，且 rebase 出的代码**仍无规格**。② **质量门禁不达标**：原型 0 条 eval、+29 行测试，却含 ~84 行序列化改动；本仓库已有两起"新持久化字段损坏存档"事故（`gotcha_save_*.md`）——把这类改动整体并入是在已知高危面上重复历史错误。③ **它同时改了 4 个方向**（套装/proc 词缀/伤害类型/9 标记 3 槽）+ 铁人模式，无法分档评估，恰好违反 L1 的核心约束（"不主导注意力"）。**建议直接摘取**：`ab8c0653c` 的背包 40→60 + `pack` 40↔60 双向翻译（Infra 性质、已解决兼容、玩家零平衡感知；但注意 D2R 社区史证明"扩容=移除机会成本"本身会引发反弹，见 §2.2）；**建议摘取结构但重设计数值**：9 标记 3 槽的"每槽 A/B 分叉"框架（见 §5 末条）；**必须丢弃**：`964fef3e8` 的 `StashVersion` bump（与主线 0 冲突，应由主线自己按需推进）。原型的价值是**设计草稿与踩坑记录**，不是可合并代码。

## 4. 需要委托方用品味判断的点

1. **L1 节点的"负面代价"要多痛**。机器能验证"是否必选"只能靠玩后感；代价太轻 → 退化为纯加成（=数值膨胀，撞宪章红线 9）；太重 → 玩家一律不点，轴等于不存在。这是手感阈值，无客观判据。
2. **是否愿意用"默认开启"换体验增量**。默认关 = 社区风险几乎为零但绝大多数玩家不会体验到；默认开 = 才有真实体验改动，但直接踩宪章"独特性"叙事。这是项目定位取舍，不是工程判断。
3. **L2 是否值得为"3-5 年后的内容承载力"预付成本**。若长期目标是持续出内容包（controller-factcheck §7：mod 目录叠加可发布），L2 的插槽/配方是**内容乘数**；若目标是做完一版就收，L2 是净亏。取决于委托方对项目寿命的意愿，无法从代码推断。

## 5. 不确定 / 未验证

- `PlayerPack::reserved3[20]` 等保留位**是否真无隐式使用**：我只读到声明（`Source/pack.h:73-88`）与注释 `For future use`，**未** grep 全量写入点，也未运行验证。落地前必须核实（禁止编译/跑测试，本次无法验证）。
- **L1 节点效果的实际挂点数量**：我确认了 Lua 只有 11 个事件且无 on-hit/on-kill（`Source/lua/lua_event.cpp`），但**未逐一定位** C++ 侧伤害/属性计算的插入点数量，所以 "3-6 人周" 是数量级推断而非分解估算。
- **多人模式对玩家级新轴的同步需求**：控制者 §6 指出任何机制都要过 delta 面（`Source/msg.cpp`），但玩家级被动加成是否必须同步（还是可各客户端自算）我**未验证** `PlayerNetPack`（`Source/pack.h:90`）的语义边界。这可能使 L1 成本上浮。
- 全部 §2.2 外部条目**只经 `web_search` 摘要，未 fetch 原文**：机制存在性可信，销量/装机量数字按"市场传闻"处理。
- 原型 9 标记 3 槽**结构上正是玩家级轴**（`git show engine-mod-infra:Source/mastermark_warrior.cpp` 头部注释：每标记 3 槽、每槽 A/B 二选一，如 Crimson Brand `Slot2: A=HP<30% 效果×1.5 / B=HP<10% ×3`）——即"分叉式选择"而非纯加成，与我推荐的 L1 承载层**一致**。但抽样所见 A/B 选项**几乎全是纯增益方向的二选一，未见负面代价**，所以它满足"玩家级"但不满足 §4-1 的"取舍型"。含义：重推导的工作量可能小于我的估计（可复用其结构与 UI），但**节点数值必须全部重设计**。我只读了 warrior 一档的注释，rogue/sorcerer 两档未读。
