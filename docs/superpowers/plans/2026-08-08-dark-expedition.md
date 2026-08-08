# 黑暗远征（Dark Expedition）实施计划

**日期**：2026-08-08
**状态**：草案（随规格 `2026-08-08-dark-expedition-design.md`）
**取代**：无

---

## 0. 文档关系

本计划执行 `specs/2026-08-08-dark-expedition-design.md`。规格为唯一裁决基准；本计划与规格冲突时以规格为准。所有数值出处见规格第 3 节，本计划不重复罗列，只标注「规格 §x.x」。

---

## 1. 执行顺序与并行波次

```
Wave 0（立即，相互独立）:
├── Task 1: 名称键控加载器（Infra，规格 4.5 步骤 1）[deep]
└── Task 2: 深度层开关 darkExpedition（规格 4.1）[quick]

Wave 1（依赖开关）:
├── Task 3: M1' 光照倍率注入 CalcPlrLightRadius（规格 4.2）[deep]
└── Task 4: CanTarget 目标选择修复（规格 4.3，6 门控点）[deep]

Wave 2（依赖 Wave 1）:
├── Task 5: Scroll of Infravision 卷轴预算（规格 4.4，WitchItemOk 排除 + 掉落降权）[deep]
└── Task 6: Infravision 可学门控 GetSpellBookLevel（规格 4.5 可学门控）[deep]

Wave 3（依赖 Task 1 + Task 6）:
└── Task 7: 三个 stub 删除（规格 4.5 步骤 2-4）[quick]

Wave FINAL（门禁）:
├── Task 8: 开关关闭全量测试（红线 13）[deep]
├── Task 9: 开关开启定向验证 + 验收 13 项逐条核对 [unspecified-high]
├── Task 10: 漂移校验 + 行尾检查 [quick]
└── Task 11: 独立复核（Oracle）[oracle]
```

**关键路径**：Task 1 → Task 7 → Task 8 → Task 11
**并行加速**：Wave 0 可并行；Wave 1 可并行；Wave 2 可并行。

---

## 2. 文件改动清单

| 文件 | 改动 | 归属 |
|---|---|---|
| `Source/options.h` / `Source/options.cpp` | 新增 `darkExpedition` 布尔选项 | Task 2 |
| `Source/items.cpp` | `CalcPlrLightRadius`（:2535）注入倍率；`WitchItemOk`（:2009-2031）排除 Infravision 卷轴 | Task 3、5 |
| `Source/levels/gendung.cpp` | 倍率表来源（`GetLevelType` :393-411 对照） | Task 3 |
| `Source/cursor.cpp` | `:89` `:315` 换 `CanTarget` | Task 4 |
| `Source/track.cpp` | `:43` `:55` 换 `CanTarget` | Task 4 |
| `Source/controls/plrctrls.cpp` | `:263` `:403` 换 `CanTarget` | Task 4 |
| `Source/engine/render/scrollrt.cpp` | **不得改动**（`:386/:469/:485/:749`） | — |
| `Source/tables/spelldat.cpp` / `spelldat.h` | 名称键控加载器；stub 行删除；`bookCost10`/`staffCost10` 定义 | Task 1、6、7 |
| `Source/spells.cpp` | `GetSpellBookLevel`（:322）加 Infravision 门控 | Task 6 |
| `Source/spell_icons.cpp` / `Source/lua/modules/items.cpp` / `Source/spell_tooltip.cpp` / `Source/missiles.cpp` | stub SpellID 引用清理 | Task 7 |
| `assets/txtdata/spells/spelldat.tsv` | Infravision `bookLevel` 正数化（开关门控）；stub 三行删除 | Task 6、7 |
| `assets/txtdata/items/itemdat.tsv` | Scroll of Infravision 掉落降权参数（或代码侧降权） | Task 5 |
| `test/` 新测试文件 + `CMake/Tests.cmake` | 每个机制定向测试必须**新建测试文件并注册**（漂移校验检查 A 要求注册 ↔ 源文件匹配；遗漏注册会在 Task 10 漂移校验暴露） | 各 Task |
| 测试文件 | 各机制定向测试（见 Task 各节） | 各 Task |

**行尾约束（禁令 7）**：上述全部文件当前为 CRLF。任何编辑后须校验未引入 LF。漂移校验（Task 10）兜底。

---

## 3. Task 1：名称键控加载器（Infra，P0 前置）

**目标**：解除 `SpellsData` 行序与 `SpellID` 枚举的耦合，使后续删行（Task 7）不产生数据错位。

**现状**（规格 §3.7 警告）：
- `LoadSpellData` 按文件行序 `SpellsData.emplace_back()`，`id` 字段跳过（`spelldat.cpp:241-244`）
- `GetSpellData` 用 `SpellsData[enum]`（`spelldat.h:271-274`）
- TSV 行序 == 枚举序 1:1 → 删行即错位

**改造**：
1. `LoadSpellData` 读取每行 `name` 字段，写入**枚举索引数组**（`SpellsData` 保持 `std::vector<SpellData>`，size = `SpellID::LAST+1`，缺失枚举填默认行）。**不用 `unordered_map`**——`GetSpellData`（`spelldat.h:271-274`）、`GetBookSpell` 的 `s == SpellsData.size()` 越界判断（`items.cpp:639-640`）、`ValidatePlayer` 的 `b < SpellsData.size()` 遍历（`player.cpp:1453`）都假设 `size() == 枚举数`；map 会让 size 收缩并静默截断书掉落/`_pMemSpells` 遍历
2. 加载时校验：按名称查表，所有**在场枚举**（指 TSV 中有行的）必须命中；缺失行 = 默认行（Task 7 删除 stub 后其枚举保持原位但数据为默认行）——**不得**「缺失枚举报错」（会与 Task 7 保留枚举冲突）
3. 复用既有名称→枚举解析（`spelldat.cpp:194-197` 已有 `value == "DoomSerpents"` 模式，可抽公共函数）

**验收**：
- 测试：`LoadSpellData` 后，随机采样 10 个可学法术的 `GetSpellData` 结果与 TSV 行逐字段一致
- 测试：`GetBookSpell` / `ValidatePlayer` 的 size 边界行为不变（`SpellsData.size() == LAST+1`）
- 测试：模拟 Task 7 删除 stub 行后，其余法术数据不漂移、加载不报错
- 全量测试通过（该 Infra 改动不得改变任何玩家可感知行为——红线 I1）

**注意**：这是 Infra 改动，独立于深度层开关，可先行提交。I2 消费者 = 规格 4.5 的 stub 删除（已立项），I3 指向「后续增删法术不再有此坑」。

---

## 4. Task 2：深度层开关（规格 4.1）

**目标**：`GameplayOptions` 新增 `darkExpedition` 布尔选项。

```cpp
, darkExpedition("Dark Expedition", OptionEntryFlags::CantChangeInGame | OptionEntryFlags::CantChangeInMultiPlayer, N_("Dark Expedition"), N_("Restricts vision in deeper levels, limiting information rather than stats."), false)
```

**步骤**：
1. `options.h` 声明 `OptionEntryBoolean darkExpedition;`（对照 `theoQuest` 声明位置）
2. `options.cpp` 构造函数初始化（对照 `theoQuest` :847）
3. `options.cpp` `GetEntries()` 注册
4. 暴露全局读取器（如 `bool IsDarkExpedition()` 或直接 `gbDarkExpedition` 全局），供 Task 3-6 消费

**验收**：
- grep 确认注册、flags、默认值
- 菜单中可见、可切换、游戏中不可改

---

## 5. Task 3：M1' 光照倍率注入（规格 4.2）

**目标**：深度层开启时按层段应用光照半径倍率。

**机制**（规格 §4.2）：
```
effectiveLightRadius = trunc(_pLightRad × multiplier)
effectiveLightRadius = clamp(effectiveLightRadius, 2, 15)
```

**倍率表**（数据驱动，建议代码常量，规格 §4.2 初值）：

| 层段 | 层号 | 倍率 |
|---|---|---|
| Hell | 13-16 | 0.6 |
| Nest | 17-20 | 0.85 |
| Crypt | 21-24 | 0.5 |
| 其他 | — | 1.0 |

**实现位置**：`CalcPlrLightRadius`（`Source/items.cpp:2535`）内，在 `lrad = std::clamp(lrad, 2, 15)` 之前注入：

```cpp
void CalcPlrLightRadius(Player &player, int lrad)
{
    if (IsDarkExpedition()) {
        lrad = lrad * GetDarkExpeditionPercent() / 100; // 整数百分比数学，全平台截断确定性
    }
    lrad = std::clamp(lrad, 2, 15);
    // ... 原逻辑不变
}
```

**注意**：
- **整数百分比数学**（`lrad * pct / 100`）而非浮点乘法——`std::floor` 不解决问题：32 位 x86（CI 矩阵含 x86）上扩展精度 80-bit 浮点 `10 * 0.6` 截断得 5 而非 6。整数除法 `10*60/100 = 6`、`10*85/100 = 8`（trunc(8.5)）、`14*50/100 = 7`，全平台一致。倍率表存整数百分数：Hell 60 / Nest 85 / Crypt 50 / 其他 100
- 层段判定用 `GetLevelType`（`gendung.cpp:393-411`）：≤16 Hell、≤20 Nest、≤24 Crypt。非 Hellfire 时 `currlevel ≤ 16` 只命中 Hell
- `CalcPlrLightRadius` 对远程玩家也执行（多人）——每客户端按本地开关生效，见规格 §4.7 声明

**验收**（规格 §6 验收 2-5）：
- 测试：基础 10 无装备 → Hell 6 / Nest 8 / Crypt 5
- 测试：基础 10 + Lightforge(+4) → Crypt `trunc(14×50/100)=7`，非 `10×0.5+4=9`
- 测试：诅咒装备（基础 2）→ Crypt `trunc(2×50/100)=1` 钳制后 2
- 测试：单调性 85 > 60 > 50

---

## 6. Task 4：CanTarget 目标选择修复（规格 4.3）

**目标**：Infravision 激活时，暗处已可见 tile 对选择/攻击有效，保持美学黑暗。

**新增函数**（**非内联**，置于 `.cpp` 如 `Source/levels/gendung.cpp` 或新文件——`MyPlayer->_pInfraFlag` 需要完整 `Player` 类型，`gendung.h`/`lighting.h` 均不包含 player.h，内联会引入循环包含）：

```cpp
// 目标选择专用：Infravision 激活时，已可见暗处 tile 对选择/攻击有效，但不改变渲染
bool CanTarget(Point position)
{
    return IsTileLit(position)
        || (IsDarkExpedition()            // 规格 §4.7：开关关闭时回退为 IsTileLit 行为
            && MyPlayer->_pInfraFlag
            && IsTileVisible(position));  // 复用既有 IsTileVisible（gendung.h:243，语义 = Visible 位）
}
```

**注**：`IsTileVisible(Point)` 已存在于 `Source/levels/gendung.h:243`，与规格 §4.3 的 `HasAnyOf(dFlags[...], DungeonFlag::Visible)` 条件一致——直接复用，不重复实现。规格 §4.3 的 CanTarget 片段未含 `IsDarkExpedition()` 门控，但 §4.7 要求开关关闭时回退为原版行为，门控为红线 13 所需——以 §4.7 为准，实施时同步修正规格 §4.3 片段。

**6 个门控点替换**（规格 §4.3 表）：

| 门控点 | 文件:行 | 动作 |
|---|---|---|
| 悬停选择 | `cursor.cpp:89` | `IsTileLit` → `CanTarget` |
| 点击选择 | `cursor.cpp:315` | `IsTileLit` → `CanTarget` |
| 目标失效（怪物） | `track.cpp:43` | `!IsTileLit` → `!CanTarget` |
| 目标失效（友方） | `track.cpp:55` | `!IsTileLit` → `!CanTarget` |
| 手柄目标获取 | `plrctrls.cpp:263` | `!IsTileLit` → `!CanTarget` |
| 手柄友方目标 | `plrctrls.cpp:403` | `!IsTileLit` → `!CanTarget` |

**禁止**：改 `Source/engine/render/scrollrt.cpp:386/:469/:485/:749`、`automap` 揭示逻辑、`IsTileLit` 本身（规格 §4.3 禁止段）。

**验收**（规格 §6 验收 6-8）：
- 测试：Infra 激活 + 已可见暗处 tile → `CanTarget == true`
- 测试：Infra 未激活 或 从未可见 tile → `CanTarget == false`
- 代码审查：6 门控点全部替换，渲染/自动地图文件无 diff
- 手动：游戏内 Infravision 激活，暗处怪物可选可攻，画面保持 TRN 暗色

---

## 7. Task 5：Scroll of Infravision 卷轴预算（规格 4.4）

**目标**：深度层开启时削减 Infravision 卷轴供应，形成远征光预算。

**机制**（规格 §4.4）：
1. **女巫库存排除**：`WitchItemOk`（`Source/items.cpp:2009-2031`，参数为 `const ItemData &`）添加：
   ```cpp
   if (item.iMiscId == IMISC_SCROLL && item.iSpell == SpellID::Infravision && IsDarkExpedition())
       return false;
   ```
   **注意字段名**：`WitchItemOk` 接收 `const ItemData &`，字段是 `iMiscId`/`iSpell`（无 `_i` 前缀）。对照 TownPortal 先例（`Source/items.cpp:2017`）。选 `WitchItemOk` 而非 `SpawnWitch` 的原因：同时覆盖 `SpawnWitch` 随机生成（经 `RndVendorItem<WitchItemOk>`）与 `RecreateWitchItem`（`Source/items.cpp:2122`，MP 种子重摇同走 `WitchItemOk`）
2. **掉落降权**：`GetItemIndexForDroppableItem`（`Source/items.cpp:1352`）已接受 `isItemOkay` 过滤回调（`tl::function_ref<bool(const ItemData &)>`）——在调用侧（RndUItem 等）传入 `IsDarkExpedition()` 时排除 Infravision 卷轴的 lambda，复用现有机制，**不改循环本身**。**唯一可行路径是代码侧**——数据侧不可行：`itemdat.tsv:102` 的 `dropRate = 1`，无法「降权」到低于 1 而不归零（归零被规格 §4.4 明确排除）。判定条件用 `item.iSpell == SpellID::Infravision && item.iMiscId == IMISC_SCROLL`（`ItemData` 字段无 `_i` 前缀；**无 `IDI_INFRA` 常量**——卷轴行 `id` 列为空，无枚举索引）

**注意**：规格 §4.4 已声明书掉落渠道（`GetBookSpell`/`CreateSpellBook`）为施法者预算路径（倾向方案 a：接受书渠道）。本任务不做书排除；Task 6 实现可学门控时须与书渠道语义一致。

**验收**（规格 §6 验收 9）：
- 测试：深度层开启时 `WitchItemOk` 拒绝 Infravision 卷轴（`iSpell == Infravision && iMiscId == IMISC_SCROLL`），`RecreateWitchItem` 路径同样拒绝
- 测试：深度层关闭时 `WitchItemOk` 放行 Infravision 卷轴
- 测试：`GetItemIndexForDroppableItem` 开启时不含 Infravision 卷轴（或显著降权）、关闭时含
- 手动：开启后女巫库存与掉落中不再出现 Infravision 卷轴；关闭后恢复

---

## 8. Task 6：Infravision 可学门控（规格 4.5 可学门控）

**目标**：深度层开启时 Infravision 可学（走魔力预算），关闭时不可学（回 -1）。

**单一机制**（规格 §4.5，必须单一包装器，禁止各调用点分别判断）：

```cpp
int GetSpellBookLevel(SpellID s)  // Source/spells.cpp:322 已有
{
    if (s == SpellID::Infravision && !IsDarkExpedition())
        return -1;
    // ... 原逻辑
}
```

**数据改动**（`assets/txtdata/spells/spelldat.tsv:10` Infravision 行）：
- `bookLevel`: -1 → 5（初值，规格 §4.4 建议）
- `bookCost10`: 定义非零值（`bookCost10=0` 会致书价 0 金——`spelldat.h:250` `bookCost10 × 10`）
- `staffCost10` / `staffMin` / `staffMax`: 同步定义
- **`staffLevel` 必须保持 -1**：`GetSpellStaffLevel`（`spells.cpp:339`）**无**门控包装器；staffLevel 为正会生成不受开关门控的 Infravision 法杖，破坏红线 13

**已覆盖的下游**（因单一包装器自动一致，规格 §4.5 列明）：
- `GetBookSpell`（书生成，`items.cpp:625-626`）
- `CreateSpellBook`（`items.cpp:4762`）
- `ValidatePlayer` 每 tick `_pMemSpells` 清理（`player.cpp:1453-1462/:2992`）
- `pack.cpp:90/:405/:413`、`loadsave.cpp:456/:2205`（存档）
- 商店书价

**验收**（规格 §6 验收 10）：
- 测试：开关开启 `GetSpellBookLevel(Infravision) ≥ 1`；关闭 `== -1`
- 测试：开关开启时记忆 Infravision 后，`ValidatePlayer` 不清理
- 手动：开启后可学习、书可掉落/购买；关闭后行为同原版（卷轴/法杖形态）

---

## 9. Task 7：三个 stub 删除（规格 4.5 步骤 2-4）

**前置**：Task 1（名称键控加载器）已完成并合入。

**步骤**：
1. 从 `assets/txtdata/spells/spelldat.tsv` 删除 DoomSerpents（行 17）、BloodRitual（行 18）、Invisibility（行 20）三行
2. 清理代码引用（5 处）：
   - `Source/panels/spell_icons.cpp`（**注意实际路径为 `panels/`，非 `Source/spell_icons.cpp`**）——`SpellITbl` 是按 SpellID 索引的 C 数组（头注释「Maps from SpellID to spelicon.cel frame number」），**保留数组槽位**（移除槽位会使 ChainLightning 之后所有图标错位，与 P0 同类）；仅清理死代码（无生产调用者的引用）
   - `Source/tables/spelldat.cpp:194-197`（名称→枚举解析）
   - `Source/lua/modules/items.cpp:245-248`（Lua 绑定）
   - `Source/spell_tooltip.cpp:300-303`（tooltip 解析）
   - `Source/missiles.cpp:924-926`（穷举 switch）
3. `SpellID` 枚举**不重排、不删除 stub 值**（保持原位）——`pack.cpp:90` 的法术 ID 序列化依赖枚举数值稳定，重排破坏存档/网络。spec §4.5 步骤 4 已定「stub 保持枚举尾部或原位」，本计划承诺：**保留枚举值**，无「一并移除」选项
4. 确认删除后 `SpellsData` 加载不报错——名称键控加载器（Task 1）对缺失行填默认行，stub 枚举数据为默认（零成本、无 missile），行为等同删除前

**验收**（规格 §6 验收 11）：
- 测试：`LoadSpellData` 后 Nova→Elemental 数据与删除前逐字段一致（回归测试）
- 测试：`SpellITbl` 数组长度与 SpellID 枚举数一致（槽位未删）
- grep：数据表无三个 stub 条目
- 全量测试通过；存档加载（含旧存档）不崩溃；`pack.cpp` 序列化往返一致

---

## 10. 门禁（Wave FINAL）

### Task 8：开关关闭全量测试（红线 13）
- 构建 + `ctest` 全量
- 通过标准：全量测试 0 失败（**记录实际用例数**——「651」为规格估值，实际数以 ctest 输出为准）；timedemo 存档比对**与本分支改动前基线一致**（**不得与上游对照**——宪章决策 25：抽象金币计数器改变了存档表示，「与上游对照已不可能」；参考存档已为本分支重新生成）
- 额外：漂移校验 `python3 tools/check_drift.py` 退出码 0

### Task 9：开关开启定向验证 + 验收 13 项逐条核对
按规格 §6 验收表 1-13 逐条执行，每项记录测试名或手动复现步骤与结果。不满足的项不得宣称完成。

额外验证（规格 §4.7 MP 声明「注明并验证同步」）：
- MP 同步测试：两个客户端开关状态不同（一开一关），断言同一玩家在两个客户端的光半径按各自本地开关计算（每客户端不对称，属预期语义，验证行为一致而非报错）
- 开关开启时 CanTarget 在多人下的目标选择行为（远程玩家光半径按本地客户端计算）

### Task 10：行尾检查
- 全部改动文件 grep 校验 CRLF（禁令 7）
- `git diff --numstat` 与 `-w` 版本对比噪声 < 100 行

### Task 11：独立复核（Oracle）
- 复核对象：本计划落地后的 diff，对照规格 §5 红线 9-14 逐条
- 重点：红线 13（开关关闭 = 原版）、红线 12（层段单调）、P0 数据移位是否真正解除

---

## 11. 提交策略

| 提交 | 内容 |
|---|---|
| 1 | Task 1 名称键控加载器（Infra，独立，先行） |
| 2 | Task 2 深度层开关 |
| 3 | Task 3 M1' 光照倍率 |
| 4 | Task 4 CanTarget 修复 |
| 5 | Task 5 卷轴预算 + Task 6 可学门控（数据/库存改动集中） |
| 6 | Task 7 stub 删除 |
| 7 | 门禁修复与文档更新（规格状态 → 已实施） |

每个提交独立可编译、可测试。行尾合规在每个提交内自检。

---

## 12. 状态

**草案**。待规格评审通过后开始执行。Task 1（Infra）可先行启动——它独立于深度层开关，且是 Task 7 的硬前置。

**已知风险**：
- Task 3 浮点截断确定性：**已钉死为整数百分比数学**（`lrad * pct / 100`），全平台确定性——x86 扩展精度浮点 `10*0.6` 会截成 5，整数除法无此问题
- Task 7 枚举处置：**已钉死为保留枚举值、保留 SpellITbl 槽位**（`pack.cpp` 序列化依赖枚举数值稳定）；不删槽位、不重排
- Task 5 掉落降权：**已钉死为代码侧**（`itemdat.tsv` 的 `dropRate=1` 使数据侧降权不可行，降权即归零，被规格排除）
- 书掉落渠道（规格 §4.4）：**已定方案 (a)**——接受书为施法者预算路径，Task 5 不做书排除
- 规格 §4.3 与 §4.7 的 CanTarget 门控张力：**以 §4.7 为准**（开关关闭回退原版），实施时同步修正规格 §4.3 片段

---

## 13. 自审

| 检查项 | 状态 |
|---|---|
| 与规格一致性：全部机制在规格 §4 有对应节 | ✅（Task 1-7 ↔ 规格 4.1-4.5；Task 4 门控以 §4.7 为准并回写规格） |
| 与验收一致性：Task 验收映射规格 §6 | ✅（Task 各节标注验收编号；§6.12「与上游一致」为规格错误，本计划按宪章决策 25 改为本分支基线，规格应出勘误） |
| P0 前置顺序正确 | ✅（名称键控加载器在 stub 删除前；加载器钉死枚举索引数组、缺失行填默认） |
| 无占位测试 | ✅（每 Task 验收为可执行断言；Task 2 的「菜单可见」为手动项已注明） |
| 测试注册 | ✅（每 Task 新建测试文件 + `Tests.cmake` 注册，漂移检查 A 兜底） |
| 行尾合规有门禁 | ✅（Task 10 + 每提交自检；漂移检查 C 为 diff 基，覆盖全部改动文件） |
| 并行波次无依赖冲突 | ✅（Wave 0/1/2 内部独立；Task 3 与 Task 5 同触 items.cpp 但异波次串行） |
| 无幻影引用 | ✅（`panels/spell_icons.cpp` 路径、无 `IDI_INFRA` 常量、`ItemData` 字段名 `iMiscId`/`iSpell` 均已修正） |
| 规格回写项 | ⚠️ 两处规格勘误待实施时回写：§4.3 CanTarget 补门控、§6 验收 12 timedemo 基线 |
