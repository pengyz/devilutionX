# 已实施特性清单

**日期**：2026-07-28
**性质**：事后补记，**不是设计规格**

---

本文档记录本 fork 已落地但没有设计规格的特性。它的唯一目的是让下一个人找到代码并知道边界在哪。

它**不适用**宪章第 4 节的 7 段结构，也**不声称通过红线检查**——这些改动当初就没有过红线，事后追认只会掩盖这个事实。宪章禁令 5 禁止「为了通过检查而填的表」，用今天的标准去追认昨天的决定正是那种表。

有设计规格的特性不在此列，见 `specs/` 目录：浮动信息 UI、法术 tooltip v2、tooltip UI 语言、技能描述 v2、上游集成、漂移校验。

---

## 抽象金币计数器

**做了什么**：金币从 `Item` 变成 `Player::_pGold` 上的纯计数器，不再占用背包格位。

引入于 `0171b2751 feat(gold): abstract gold counter — gold no longer occupies inventory slots`。

**涉及文件**

| 文件 | 改动 |
|---|---|
| `Source/inv.cpp` | `GoldAutoPlace` 与 `AddGoldToInventory` 退化为 `_pGold += value`；`CalculateGold` 已删除 |
| `Source/items.cpp` | `CreatePlrItems` 的起始金币直接加到 `_pGold`，不再创建 `InvList` 条目 |
| `Source/stores.cpp` | `TakeGold` 空实现；消费直接从 `_pGold` 扣 |
| `Source/control/control_gold.cpp` | `RemoveGold` 忽略 `goldIndex`；新增 `OpenGoldDropFromCounter` |
| `Source/qol/stash.cpp` | 新增存金按钮（`StashButtonRect[3]`，标签 `Dep`） |
| `Source/qol/autopickup.cpp` | `HasRoomForGold` 恒返回 `true` |
| `Source/loadsave.cpp` | `LoadPlayer` 把旧存档里的金币 `Item` 迁移到 `_pGold` |
| `Source/pack.cpp` | `CalculateGold` 调用点删除 |

UI 部分：`3fba57b6c` 在扩展背包面板加金币计数条与 Drop Gold 按钮，`81bcda87e` 与 `656a797df` 调整其位置。

**已知边界与问题**

- 旧存档兼容靠 `LoadPlayer` 的一次性迁移。迁移只处理 `InvList` 与 `HoldItem`，**未处理 stash 中的金币** —— 未验证 stash 旧存档是否有金币条目
- `TakeGold` 与 `RemoveGold` 保留了参数但忽略它们（`int /*goldIndex*/`、`bool /*skipMaxPiles*/`），是为调用点稳定而留的空壳
- 曾导致四项测试失败（`InvTest.CalculateGold`、`InvTest.GoldAutoPlace`、`Player.CreatePlayer`、`VisualStoreTest.SmithSell_Success`），已于 2026-07-27 改写测试至新契约
- `HasRoomForGold` 恒真意味着**金币拾取不再有上限检查**。`_pGold` 是 `int`，理论上会溢出，未验证是否有防护

---

## Lua 模块扩展

**做了什么**：为 mod 提供三个 Lua 模块。分类 Infra，通过宪章 Infra 红线 I2 的类型 (c)（有注册点与文档化调用约定的公开 API）。

引入于 `7c689ae71 feat(engine): add runtime-extensible AI, missile, and quest APIs for modding`。

**涉及文件与暴露的 API**

| 模块 | 文件 | 暴露的函数 |
|---|---|---|
| `devilutionx.world` | `Source/lua/modules/world.cpp` | `SpawnMissile(missileName, x, y, direction) -> id`、`DamageTarget(targetId, damage, damageType)`、`GetMonstersInRange(x, y, radius) -> table` |
| `devilutionx.monsters` | `Source/lua/modules/monsters.cpp` | `addMonsterDataFromTsv(path)`、`addUniqueMonsterDataFromTsv(path)`、`CreateMonster(name, x, y) -> id`、`SetAI(monsterId, aiType)` |
| `devilutionx.spells` | `Source/lua/modules/spells.cpp` | 为 tooltip 表达式求值服务，见 `fce91643c` |

注册点：`Source/lua/lua_global.cpp:317` 起。文档字符串由 `LuaSetDocFn` 提供。

**已知边界与问题**

- `SetAI` 只能设置**已存在**的 AI 类型，不能注册新类型。`RegisterAiFunction` 已于宪章决策 16 删除（仅有测试调用者），因此 Lua 侧无法扩展 AI 行为
- 同理 `RegisterMissileAddFn` / `RegisterMissileProcessFn` 已删除，Lua 无法注册新弹道函数
- 这意味着「怪物行为深化」若走 Lua 路线，需要先重新设计注册入口——上游把 `ParseMissileAddFn` 等放在匿名命名空间内，明确表示那不是契约
- `Source/lua/modules/spells.cpp` 的表达式求值曾用 Lua 沙箱（`71157d530`），后改为 source enum + C++ helper（`577937db3`）。当前形态以后者为准

---

## 物品对比（双 tooltip）

**做了什么**：悬停背包物品时，在浮动 tooltip 旁并排显示当前已装备的同类物品数值，供比较。D2 风格。

引入于 `5e32056c2 feat(ui): add D2-style dual tooltip equipment comparison for backpack items`，渲染于 `1413fbaa3 feat(ui): implement dual floating tooltip rendering with ComparisonInfoString`。

**涉及文件**

| 文件 | 改动 |
|---|---|
| `Source/items.cpp` | `PrintItemComparison`：按 `_itype` 找到对应装备位的物品，输出其伤害/护甲/前后缀词条 |
| `Source/control/control_infobox.cpp` | `ComparisonInfoString` 与第二个 tooltip 的布局渲染 |

**已知边界与问题**

- 未鉴定物品也显示对比（`03a7ee0f2` 刻意如此）。这与宪章支柱 B1「未知必须可以被行动解决」的关系未评估——鉴定是可行动的未知，而对比可能泄露了本该靠鉴定获取的信息
- 双手武器与副手的对比逻辑：`ItemType::Shield` 只查 `INVLOC_HAND_RIGHT`，未处理双手武器占用两格的情况
- `8254ff147` 移除了地面物品与怪物的 tooltip，只保留 NPC 与场景物件名称。该决定的理由未记录

---

## 消耗品堆叠

**状态**：原样保留。宪章决策 26 撤销了「改背包堆叠」的决策 10。

原设计规格已归档并带勘误横幅：`archive/specs/2026-07-07-consumable-stacking-design.md`。

**当前实际形态**（与归档规格不同）

| 项 | 现状 |
|---|---|
| 腰带堆叠 | 有：`AutoPlaceItemInBelt` 自动堆叠、`ChangeBeltItem` 拖放堆叠 |
| 背包堆叠 | **无**。`AutoPlaceItemInInventory` 不含任何堆叠逻辑 |
| 上限 | 药水 5、卷轴 3，**不因职业而异**（职业加成已于 `d35f74196` 撤销） |
| 显示 | 腰带 `inv.cpp:1308`、背包 `inv.cpp:1228`（后者是死代码，`InvList` 的堆叠数恒为 1） |
| 持久化 | 存档：`_iMinDex` 后的对齐填充字节；网络：`bId` 位域 |

**已知边界与问题**

- 背包侧的显示分支自加入起就是死代码
- 腰带容量因此为 8 格 × 每格 5 件，是底座层「不改变平衡」承诺的唯一已知例外，见宪章第 9 节
- 持久化编码曾有三处数据损坏，均已修复：`90c5511fc`（`pack.cpp` 的 `bId` 覆写物品品质）、`7c6ebd067`（`msg.cpp` 同类问题致联机物品全部已鉴定）、`db9101903`（存档读写不对称致 `LoadDroppedItems` 越界写）

---

## AI 与弹道派发重构

**状态**：部分保留。上游集成时采用了上游的 `misdat.cpp` 实现，本 fork 的注册表被弃用。

**当前保留的部分**

| 项 | 位置 |
|---|---|
| `AiProc` 数组化 + `static_assert` 尺寸保护 | `Source/monster.cpp`，生产调用点在 AI 派发处 |
| `AiFunction` 类型与 `extern AiProc` 声明 | `Source/tables/monstdat.h`，注释已改为「Exposed for tests; there is no registration entry point」 |

**已删除的部分**

- `RegisterAiFunction`、`FallbackAiImpl`（宪章决策 16：仅有测试调用者，过不了 Infra 红线 I2 与 I3）
- `RegisterMissileAddFn` / `RegisterMissileProcessFn` 与本 fork 的 127 行 `misdat.cpp` 注册表（上游集成时采用上游 if 链）
- `test/missile_registry_test.cpp`（其断言的是上游用匿名命名空间明确表示为「非契约」的内部实现）

---

## 归属核实结果

本清单的特性归属已逐文件核实，本 fork 相对上游的 `Source/` 改动无未归类残余。

`Source/inv.cpp`（332 行）：

| 改动位置 | 归属 |
|---|---|
| `CheckInvHLight`（4 块） | `InfoString` → `FloatingInfoString` 迁移（浮动信息 UI 规格）+ 双 tooltip 对比（物品对比） |
| `ChangeInvItem`（4 块） | 删除金币合堆逻辑（抽象金币计数器） |
| `GoldAutoPlace` / `AddGoldToInventory` / `RoomForGold` / `CreateGoldItemInInventorySlot` | 抽象金币计数器 |
| `AutoPlaceItemInBelt` / `ChangeBeltItem` / `UseInvItem` / `DrawInvBelt` / `DrawInv` | 消耗品堆叠 |
| `DoTelekinesis` / `CheckInvCut` | 金币计数器的调用点适配 |

`Source/qol/stash.cpp`（86 行）：8 块改动全部围绕存金按钮——`StashButtonRect` 增一项、按下与释放处理、`DrawStash` 绘制、`CheckStashHLight` 悬停提示、`DrawGoldWithdraw`。全属抽象金币计数器。

`Source/qol/visual_store.cpp`：纯删除（0 增 12 删），删去的全是 `InfoString` 赋值，属浮动信息 UI 迁移。

`Source/levels/trigs.cpp`、`Source/quests.cpp`、`Source/objects.cpp`、`Source/cursor.cpp`：`InfoString` → `FloatingInfoString` 迁移，属浮动信息 UI 规格范围。

### 上游既有、非本 fork 引入

`VisualStoreNextPage` / `VisualStorePreviousPage` 无生产调用者，对应的 `VisualStoreTest.Pagination_NextAndPrevious` 与 `Pagination_ResetsHighlight` 在 ctest 中长期 Skipped。两者在 merge-base 的 `Source/` 中已存在，且本 fork 对 `visual_store.cpp` 的改动是纯删除、与分页无关——**这是上游的分页功能未接入 UI，不是本 fork 造成的**。已加入漂移校验脚本的检查 E 白名单。
