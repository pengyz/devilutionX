---
name: 存档格式策略与格式变更台账（初版功能阶段不做兼容）
description: 本项目在初版功能阶段不保证存档（含格式）兼容，兼容层留到初版功能完成后单独立项；本文档同时是存档格式变更台账——每次改动格式记一行，作为将来兼容层的地图。
type: decision
created: 2026-09-15
sources:
  - docs/superpowers/specs/2026-07-27-better-d1-design-charter.md（决策 21、24、25、34、35）
  - Source/loadsave.cpp（SaveMonster/LoadMonster、SaveItem/LoadItem）
  - docs/knowledge/gotcha_save_bid_overwrite.md
  - docs/knowledge/gotcha_save_stack_append.md
---

**策略（宪章决策 34 + 35）**：初版功能阶段**完全不做存档兼容**——不设版本迁移、不为旧存档做映射、不为字段增删写兼容路径；**兼容层在初版功能完成后单独立项**。现阶段只要求：改动格式后重新生成硬编码夹具（`pack_test` 期望数组、`writehero` golden SHA、`timedemo` 参考存档、采样用例），并在此台账记一行。

**为什么：** 开发期没有外部玩家，旧存档可以直接丢弃；把兼容成本推迟到格式稳定之后，比为一批"很快还会再变"的格式反复做迁移便宜得多。

**何时使用：** 任何要动 `ItemPack` / `PlayerPack` / `Monster` 序列化、存档读写顺序、或改变已持久化字段语义的改动——动之前查本台账（避免与在途的格式改动撞车），动之后追加一行。

## 格式变更台账

> 从本文档建立（2026-09-15）时起完整记录；此前的变更可从决策 21/24/25 追溯。

| 日期 | 变更 | 触碰 | 兼容处置 |
|---|---|---|---|
| 2026-07（决策 21） | `ItemPack` 增加堆叠计数字段，存 `count-1` | `Source/pack.h`、`Source/items.cpp`、`Source/loadsave.cpp` | **已做兼容**：`count-1` 使旧档读出 `count=1`，与上游存档逐字节兼容 |
| 2026-07（决策 24） | `SaveItem` 曾无条件追加、`LoadItem` 条件读取 → 写读不对称 | `Source/loadsave.cpp` | **已修**（P0 数据损坏），教训见 `gotcha_save_stack_append.md` |
| 2026-07（决策 25） | 抽象金币计数器改变英雄状态表示 | 玩家状态/存档 | 已接受：与上游对照基准改为"与本分支一致"（timedemo 参考存档重生成） |
| 2026-09-15（计划中） | `Monster::chargeCooldown`（`int8_t`）新增并序列化 | `Source/monster.h`、`Source/loadsave.cpp` | **不做兼容**（决策 35）；需重生成夹具。用于 A1/A3 冲锋冷却 |
| 2026-09-15（计划中） | 采样/名册/配额改造 → `monster.levelType`（`LevelMonsterTypes` 索引）语义变化 | `Source/monster.cpp`、`Source/loadsave.cpp` | **不做兼容**（决策 35）；旧档怪物种类会被重新解释 |
| 2026-09-16（**已落地**，Phase A） | 逐层名册改变采样顺序 → `monster.levelType`（`LevelMonsterTypes` 索引）语义变化：core 预加先入表，尾部抽取受 B1 cap 与 `class_floors` 约束，同层同 seed 的索引与旧档不再对应 | `Source/monster.cpp`、`assets/txtdata/monsters/level_rosters.tsv`、`assets/txtdata/monsters/level_roster_params.tsv` | **不做兼容**（宪章决策 35）；旧档 `levelType` 会被重新解释成另一种怪，需重开新档 |
| 2026-09-16（**已落地**，Phase B） | 核心小队接入散布循环：**普通怪**（此前只有 unique 才可能带非默认 `leaderRelation`/`packSize`）现在也会以 `leaderRelation=Leashed`（或按层回退为 `None`）、`packSize>0` 的组合出现在存档里；`leaderRelation`/`packSize` 字段**本身未新增**（`Source/loadsave.cpp:776-777` 的序列化布局早已存在），变化的只是**普通怪走到这些字段非默认值的路径**——即存档**内容模式**变化，非**文件格式**变化 | `Source/monster.cpp`（`PlaceGroup` 给普通 core leader 及其随从传 `leader`/`leashed`），无 `Source/loadsave.cpp` 改动 | **不做兼容**（宪章决策 35）；旧档里普通怪 `leaderRelation`/`packSize` 的字节位置和编码不变，读档端 `M_UpdateRelations`（G1 修复后不再按 `isUnique()` 门控）已按普通怪同样处理释放逻辑，不会误读——**若未来给存档加版本化兼容层，本行需纳入"内容语义变化"清单**（区别于纯粹的字段增删/顺序变化），评估是否要为老档的普通怪强制清空这两个字段 |
| 2026-06-29（**未合入主线**） | `_iProcFlags`/`_iProcChance` 字段与序列化、`StashVersion` bump | `engine-mod-infra` 分支 | 仅作参考：该分支未合入，主线 `StashVersion` 仍为 0，**不要直接套用其版本号** |

## 将来做兼容层时需要什么

1. 本台账的完整列表（哪些字段/顺序/语义变过）。
2. 一个**显式版本标记**（主线目前**没有**存档版本字段——`StashVersion` 为 0 且仅用于仓库；这是兼容层要加的第一件东西）。
3. 迁移方向：只支持"旧 → 新"单向迁移即可（决策 34 已放弃双向兼容）。
4. 失败语义：版本不匹配时**明确拒绝或明确忽略**，**不得静默误读**——本仓库两起数据损坏事故（`gotcha_save_bid_overwrite.md`、`gotcha_save_stack_append.md`）的共同点正是"静默误读"而非崩溃。

## 附记（2026-09-18，内容密度契约）

名册的类别上限语义已变更：**地狱 L13-15 远程类 cap=1、非远程免 cap**；**L2 远程类 cap=1**；**caves 的 kite cap（L9-12 ≤2）退役**；L16 保持旧约束。这只改变**存档内容**（同层怪种类/构成），**存档格式未变**，按决策 35 不做兼容处理。
