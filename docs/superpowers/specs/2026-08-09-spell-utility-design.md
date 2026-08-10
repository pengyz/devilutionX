# 法术实用性（Spell Utility）设计

**日期**：2026-08-09
**状态**：已批准（2026-08-09，两轮 Oracle 独立复核通过）
**分类**：Depth
**取代**：`archive/specs/spell-system.md`（已废弃，含已核实错误）

---

## 1. 问题陈述

宪章待立项 P4：法术实用性（Rage / Etherealize / Golem）。宪章 §2「错误设计」判定与修法：

> Rage / Etherealize / 符文 / 伤害卷轴 | 错误设计 | 本意是给更多选项，实际永远劣于其他选项
> 对错误设计的正确修法是先问：这个东西如果不存在，玩家少了什么取舍？若答案是「没少」，删除它而不是给它加参数。（`宪章:125/:128`）

**本规格范围（经独立复核修订）**：**仅处理 Etherealize 删除**。Rage 经独立复核确认为**野蛮人起始职业技能**（`assets/txtdata/classes/barbarian/starting_loadout.tsv` skill=Rage，玩家可选野蛮人即获得）——不是「没少取舍」的错误设计。移出本规格，回宪章就「可达技能的强度取舍」重新裁决（宪章 :128 对可达机制的正确修法是「修到产生取舍」而非删除）。

**Etherealize 事实调研（2026-08-09，独立复核验证）**：
- `bookLevel=-1`（不可学）、`staffLevel=-1`（不可装杖）
- **无任何获取渠道**：无卷轴（itemdat 0 匹配）、无法杖、无职业 loadout、无 quest/shrine/store 引用、无怪物施放、无 `_iSpell` 充能——上游（origin/master）itemdat 同样 0 匹配
- 在 `spell_book.cpp:50` 可学列表但被 `GetSpellBookLevel` 的 bookLevel=-1 门控实际不可学
- 有完整逃生实现（`SpellFlag::Etherealize` 防箭/防怪，`missiles.cpp:386/:1093`、`monster.cpp:1176`、`player.cpp:717`），但全是死代码路径

**宪章修法应用**：Etherealize 玩家从未能获得，去掉后取舍是「没少」→ **删除是正确处置**（而非给不存在的东西加参数）。

**与 Dark Expedition 规格 §3.7 的关系**：该规格判「Etherealize 保留不动」基于「有完整实现」（当时未核实获取渠道）。本规格重新调研发现其**无获取渠道**——按宪章错误设计修法应删除。本规格更新此裁决。

---

## 2. 分类判定

按宪章第 2 节判定树：

**第一步——改动是否触及数值/数据？**

- 删除 Etherealize 的 `spelldat.tsv` 数据行：**是**（规则 1，TSV 数据文件）→ **Depth**
- 清理相关代码引用：随数据删除，Depth

**归类结论**：判为 **Depth**。这是「错误设计删除」，与 Dark Expedition 的 stub 删除（DoomSerpents/BloodRitual/Invisibility）同性质。

**补充说明**：
- 删除不改变任何玩家可达行为（Etherealize 本就不可达）
- Golem 保留不动（可学 + 卷轴存在，真实可用）
- Rage 不在本规格范围（回宪章重新裁决）

---

## 3. 事实基础

全部数值于 2026-08-09 核实。

### 3.1 法术数据

| 事实 | 值 | 出处 |
|---|---|---|
| Etherealize 数据行 | `bookLevel=-1, staffLevel=-1, minInt 93, manaCost 100` | `assets/txtdata/spells/spelldat.tsv:23` |
| Etherealize 获取渠道 | **无**（itemdat 无卷轴条目，上游也无） | `assets/txtdata/items/itemdat.tsv`（grep 0 匹配）、`git show origin/master` 对照 |
| Etherealize 在可学列表 | `spell_book.cpp:50`（被 bookLevel=-1 门控不可学） | `Source/panels/spell_book.cpp:50` |
| Etherealize 实现 | `SpellFlag::Etherealize` 防箭/防怪 | `missiles.cpp:386/:1093`、`monster.cpp:1176`、`player.cpp:717` |
| Golem 数据行 | `bookLevel=11, staffLevel=9, minInt 81, manaCost 100` | `assets/txtdata/spells/spelldat.tsv:19` |
| Golem 获取渠道 | 可学 + `Scroll of Golem`（itemdat:112） | `assets/txtdata/items/itemdat.tsv:112` |
| Rage 数据行 | `bookLevel=-1` 但为**野蛮人起始技能** | `assets/txtdata/classes/barbarian/starting_loadout.tsv` skill=Rage（上游同款） |

### 3.2 删除影响面（Etherealize）

| 引用点 | 位置 | 处置 |
|---|---|---|
| `missiles.cpp`（弹道处理） | :386/:929/:1093/:1226 | 移除分支 + 状态标志引用 |
| `monster.cpp` | :1176（防怪检查） | 移除 |
| `player.cpp` | :717（防伤检查） | 移除 |
| `player.h`（SpellFlag） | Etherealize 标志位 | 移除 |
| `spell_book.cpp:50` | Etherealize | 移出列表，槽位填 `SpellID::Invalid` |
| `spell_icons.cpp` | :55 | 保留槽位（C 数组，移除错位） |
| `tables/spelldat.cpp` | :112/:221 | 保留解析（防御性） |
| `tables/misdat.cpp` | :104（MissileGraphicID::Etherealize 解析） | 保留（防御性） |
| `spell_tooltip.cpp` | :309 | 保留解析（防御性） |
| `lua/modules/items.cpp` | :254 | 保留枚举映射（防御性） |
| `translation_dummy.cpp` | :672 | 保留翻译（防御性，SPELL_ETHEREALIZE_NAME 无引用） |
| `assets/txtdata/missiles/misdat.tsv` | :36（base+hf） | 保留数据行（弹道 ID 保留，防引用悬空） |
| `assets/txtdata/missiles/missile_sprites.tsv` | :36 | 保留（防御性） |
| `assets/txtdata/spells/spelldesc.tsv` | :131-133（base+hf） | 保留（同 Dark Expedition stub 先例） |
| `SfxID::SpellEtherealize` | `sound_effect_enums.h:194`、`misdat.cpp:327`、`effects.tsv:78`（base+hf） | **保留**——Telekinesis（misdat.tsv:57）与 Warp（hf:80）用作 castSound |
| `misdat.h:70` | MissileGraphicID::Etherealize | 保留（精灵数据索引对齐） |

### 3.3 宪章裁决

| 裁决 | 出处 |
|---|---|
| Etherealize 是错误设计 | `宪章:125` |
| 错误设计修法：去掉后没少取舍则删除，不加参数 | `宪章:128` |
| 删除不改变玩家可达行为（本不可达） | 3.1 事实 |
| Dark Expedition 已删 3 个 stub（先例） | `2026-08-08-dark-expedition-design.md` |
| Rage 回宪章重新裁决（可达职业技能） | 本规格修订，独立复核确认 |

---

## 4. 方案

### 4.1 删除 Etherealize 数据行与代码

1. **数据**：删除 `spelldat.tsv:23`（Etherealize 行）与 HF 版对应行
2. **代码**：移除 `missiles.cpp` 的 Etherealize 分支（:386/:929/:1093/:1226）、`monster.cpp:1176` 与 `player.cpp:717` 的 `SpellFlag::Etherealize` 检查、`player.h` 的 Etherealize 标志位、`spell_book.cpp:50` 可学列表中的 Etherealize（槽位填 `SpellID::Invalid`）
3. **保留**：`spelldat.cpp`/`spell_tooltip.cpp`/`lua`/`translation_dummy`/`misdat.cpp` 的解析与映射、missiles/spelldesc TSV 数据行（防御性，同 Dark Expedition stub 删除先例——枚举值保留，引用不悬空）

### 4.2 保留 Golem 与 Rage

- **Golem**：不动（可学 + 卷轴存在，真实可用）
- **Rage**：不在本规格范围。回宪章重新裁决「野蛮人起始职业技能的强度取舍」（宪章 :128 对可达机制用「修到产生取舍」，非删除）

### 4.3 枚举与持久化策略（关键）

**保留 `SpellID::Etherealize` 与 `MissileID::Etherealize` 枚举值**：
- 与 Dark Expedition stub 删除一致（`spelldat.h` 枚举不重排）
- `pack.cpp`/`loadsave.cpp` 的存档序列化依赖枚举数值稳定
- 数据行删除后，枚举值解析到 `MakeUnloadedSpellData` 默认行（不可学、无弹道）——行为等同删除前（本就不达）

**`SpellFlag::Etherealize` 从枚举移除**：
- `_pSpellFlags` 按 raw uint8 往返持久化（`loadsave.cpp:474` 读、`:1346` 写）——位定义移除后，旧存档该位被剩余代码忽略，无 UB
- 行为变化：旧存档若有 Etherealize 位（实际不可达故极罕见），重载后增益丢失——显式声明

### 4.4 红线 13（开关关闭 = 原版）

删除对任何开关状态无差异——Etherealize 在任何状态下都不可达（原版也无获取渠道）。开关关闭 = 原版自然成立。

---

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Depth |
| 2 | 问题陈述指向具体症状 | 第 1 节：Etherealize 不可达事实 + 宪章错误设计修法 |
| 3 | 每个数值标注出处 | 第 3 节逐条标注 |
| 4 | 「已实施」标注需非测试调用者+验收全过 | 本规格状态为草案 |

### 深度层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 压力来源是限制信息还是膨胀数值？ | 限制信息 | 删除错误设计（纯减负），非数值膨胀 |
| 10 | 有可执行的反制手段？ | 有且已指名 | 删除不引入新限制；Golem 保留可用 |
| 11 | 稀缺产生取舍还是跑腿？ | 取舍 | 删除错误设计不产生稀缺（本不可达），无负担 |
| 12 | 全层段有定义？ | 是 | 数据删除为全局机制（不按层段） |
| 13 | 开关关闭时行为回到原版？ | 是 | Etherealize 任何状态不可达，开关关闭 = 原版自然成立 |
| 14 | 近战/远程分别评估？ | 已评估 | 删除对近战/远程无差异化影响（本不可达）；无信息限制因此无红 14 不对称 |

---

## 6. 验收标准

| # | 验收项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | Etherealize 数据行删除 | grep `^Etherealize` in spelldat.tsv（base+HF） | 无匹配 |
| 2 | SpellFlag::Etherealize 移除 | grep `SpellFlag::Etherealize` in Source/ | 无生产引用 |
| 3 | Golem 保留 | grep `^Golem` in spelldat.tsv | 有匹配（bookLevel=11） |
| 4 | Rage 不受影响 | grep `^Rage` in spelldat.tsv | 有匹配（回宪章裁决） |
| 5 | 存档/网络兼容 | `pack_test` + `writehero_test` + `timedemo` | 全过；SpellID 枚举未重排 |
| 6 | spelldat_test 更新 | `AllLearnableSpellsHaveDescriptions` 移出 Etherealize | 测试通过（Rage 保留在列表） |
| 7 | 全量测试 | `ctest` | 687/687（spelldat_test 调整后） |
| 8 | 漂移校验 | `python3 tools/check_drift.py` | 退出码 0 |
| 9 | 可学列表清理 | `spell_book.cpp:50` | 无 Etherealize，槽位为 `SpellID::Invalid` |

---

## 7. 状态

**草案**。待独立复核修订通过后转「已批准」，再由实施计划落地。

**已知边界**：
- 保留枚举值（SpellID/MissileID）与防御性解析映射——与 Dark Expedition stub 删除一致，避免存档/网络破坏
- `_pSpellFlags` 旧存档未知位忽略语义（3.2/4.3 已声明）
- **Rage 不在本规格**：回宪章重新裁决「野蛮人起始职业技能的强度取舍」——是本 P4 项拆分出的新立项，需宪章决策记录或独立规格
- Golem 的可用性（可学 + 卷轴）已确认，无需改动
- 归档 `spell-system.md` 的其他问题法术（DoomSerpents/BloodRitual/Invisibility）已在 Dark Expedition 删除
