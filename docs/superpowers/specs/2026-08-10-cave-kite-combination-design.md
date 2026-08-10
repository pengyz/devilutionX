# 洞穴风筝组合（Cave Kite Combination）设计

**日期**：2026-08-10
**状态**：草案（v4——多 agent 独立复核修正：冲锋伤害投递路径 + 承载选优 + 垄断声明诚实化）
**分类**：Expansion（宪章决策 31 定位变更后；触及 monstdat ai 列 → 判定树规则 1 → Expansion）
**取代**：无（新规格；密度修复框架 A3——洞穴远程垄断修复）

> **定位**：修复扫描诊断的洞穴 9-12「远程拉扯垄断」（38% 怪物共享风筝行为）。方法不是新增风筝怪，而是**把 11 只可生成风筝怪中的 2-3 只转成「带 tell 的组合行为」**——风筝 + 条件性冲锋，打破「全是风筝怪」的单调。扫描文档结论 1 原案 + Oracle 框架评审确认（A3 是修复垄断的直接手段，且零新美术）。

> **v4 相对 v3 的变更**（据多 agent 独立复核，见框架总览 §8.4——冲锋伤害路径修正，与 A1 v4 一致）：
> - **冲锋伤害投递路径修正**：独立复核证实 `MissToMonst → MonsterAttackPlayer` 用 **special 伤害列**（`monster.cpp:4587-4601`）。候选承载中 Magma/Acid 系 special 列全 0（Hell Stone 2-20 普通 / special 0-0）→ 墙角冲锋若直接生效仅 ~1 伤害推搡。v4 修复：给 Hell Stone 设 `minDamageSpecial/maxDamageSpecial = 普通伤害值 2-20`（复用引擎既有路径，与 Rhino 系 `MT_HORNED` special=5-32 一致）；垂死反击者 Storm Lord special 4-16 已有真实伤害，无需改列
> - **承载选优**：垂死反击者 = Storm Lord（special 4-16 已有，保留）；墙角冲锋者 = Hell Stone（保留但需设 special 列）
> - **垄断声明诚实化**：38% 是池级数字（14/36 含 3 Never）；可生成比例实际 11/31 = 35%——「垄断被打破」指行为维度（2-3 只会冲锋），非物种比例大幅下降

> **v3 相对 v2 的变更**（据整体验证 §8.2-1——根因修复，与 A1 v3 一致）：
> - **冲锋改即时**：引擎验证 RhinoAi/SnakeAi 同 tick `AddMissile(Rhino)+mode=Charge`（monster.cpp:2284-2292/2675-2683），**零前摇**——v2 的「冲锋前摇可打断」是虚构（`StartSpecialAttack` 走 `MonsterSpecialAttack` 近战路径，转冲锋需新 MonsterMode 违反零新机制）
> - **反制重写**：从「前摇打断」改为「**走位躲直线 + 冲锋冷却**」——即时冲锋无前摇可打断，反制靠侧移（直线弹道）与冷却（防低血连锁 kill-wall）
> - **新增冲锋冷却字段**：`chargeCooldown`（复用 goalVar 模式），防低血连锁——远程玩家在冷却窗口输出
> - **低血 tell 修正**：从「首次遭遇前摇窗口可打断可存活」改为「首次遭遇因冷却可存活」——学习型反馈成立

> **v2 相对 v1 的变更**（据 Oracle 首轮评审 4 个 blocking 逐一修复）：
> - **B1**：tell 事实修正——special 动画是风筝怪的正常攻击动画（非闲置），「冲锋前摇」与「吐息前摇」视觉相同；tell 改为**情境**（被逼墙角/低血）+ 冲锋启动时刻
> - **B2**：冲锋补距离 + LOS 门控（复用 RhinoAi `distanceToEnemy>=3 && LineClear`）——防隔墙冲锋/贴身必死；距离 <3 走近战
> - **B3**：墙角判定阈值从 <2 提升到 ≤3（走廊+死角触发，开阔地不触发）——修验收 #5 自相矛盾，冲锋更频繁可学习
> - **B4**：低血 tell 修正——HP 条默认关闭，改为「冲锋本身是学习型反馈」；如需强 tell 独立立项强制 HP 条

---

## 1. 问题陈述

**症状**（可复现，扫描诊断）：洞穴 9-12 层，14/36 种怪物（38%）共享 `AiRangedAvoidance`（风筝：边退边射）。玩家面对的战斗从「混合阵型」退化为「全是风筝怪」——差异只有弹道颜色和伤害，行为完全一样。

**根因**：洞穴可生成的风筝怪有 11 只（Acid 3 / Magma 4 / Storm 4），全部共享同一 AI 函数 `AiRangedAvoidance`——「边退边射」是唯一行为模式。玩家无法从行为上区分它们，只能从数值（哪个射得疼）区分。

**对照数据（2026-08-10 核实）**：

| 怪物 | 族 | AI | special 帧 | 层段 |
|---|---|---|---|---|
| Poison Spitter | Acid | AiRangedAvoidance | 12 | 8-10 |
| Pit Beast | Acid | AiRangedAvoidance | 12 | 10-12 |
| Lava Maw | Acid | AiRangedAvoidance | 12 | 12-14 |
| Magma Demon | Magma | AiRangedAvoidance | 18 | 8-9 |
| Blood Stone | Magma | AiRangedAvoidance | 18 | 8-10 |
| Hell Stone | Magma | AiRangedAvoidance | 18 | 9-11 |
| Lava Lord | Magma | AiRangedAvoidance | 18 | 9-11 |
| Red Storm | Storm | AiRangedAvoidance | 14 | 9-11 |
| Storm Rider | Storm | AiRangedAvoidance | 14 | 10-12 |
| Storm Lord | Storm | AiRangedAvoidance | 14 | 11-13 |
| Maelstrom | Storm | AiRangedAvoidance | 14 | 12-14 |

**关键事实**（v2 据 Oracle 评审 B1 修正）：全部 11 只有 special 帧（12/14/18），但 **special 动画是风筝怪的正常远程攻击动画**（`StartRangedSpecialAttack` 播放 `MonsterGraphic::Special`——每次吐息/闪电都用它），**不是闲置的 tell 动画**（与 A1 教堂骷髅不同：骷髅的 special 帧确实闲置）。因此「冲锋前摇」与「吐息前摇」视觉相同——**玩家无法从动画预读「要冲锋」**。真正的 tell 是**情境**（被逼墙角/低血），冲锋启动瞬间（`mode=Charge` 用 `MonsterGraphic::Attack`）是揭示时刻。

## 2. 分类判定

**第一步——改动是否触及数值/掉落/属性/光照/商店/战斗资源？**

- 修改风筝怪 `ai` 列（分配到新组合 AI）：**是**（规则 1）→ **Expansion**
- 新增组合 AI 函数：**是**（规则 1）→ Expansion

**补充判定**：

| 改动项 | 单独判定 | 结果 |
|---|---|---|
| 2-3 只风筝怪 ai 列改派（monstdat） | 规则 1 | Expansion |
| 新组合 AI 函数（基于 AiRangedAvoidance 派生） | 规则 1 | Expansion |
| 零新美术（复用既有 special 帧） | — | 不新增 |

**归类结论**：整体判为 **Expansion**，无条件生效（决策 31：无开关）。

## 3. 事实基础

全部数值于 2026-08-10 核实。

### 3.1 风筝怪数据

| 事实 | 值 | 出处 |
|---|---|---|
| 洞穴 9-12 可生成风筝怪 | 11 只（Acid 3 / Magma 4 / Storm 4） | `monstdat.tsv` |
| 全部共享 AI | `AiRangedAvoidance` | `monstdat.tsv` ai 列 |
| 全部有 special 帧 | Acid 12 / Magma 18 / Storm 14 | `monstdat.tsv` frames[6] |
| 38% 垄断 | 14/36 池（含 Never） | 扫描文档 |

### 3.2 引擎原语

| 事实 | 值 | 出处 |
|---|---|---|
| 风筝 AI | `AiRangedAvoidance`（monster.cpp:2013） | `Source/monster.cpp` |
| 冲锋触发 | `AddMissile(MissileID::Rhino)` + `mode=Charge` | `monster.cpp:2287/2457/2679` |
| 冲锋弹道 | `ProcessRhino` 处理墙碰撞/伤害/打断 | `Source/missiles.cpp` |
| special 动画 | `StartSpecialAttack`（v3 修正：本规格**不使用**它做冲锋前摇——引擎冲锋是即时的） | `monster.cpp` |
| AI 状态变量 | `var1/var2/var3` | `Source/monster.h` |

## 4. 方案

### 4.1 组合行为：风筝 + 条件性冲锋（KiteCharger）

**行为**：风筝怪保留 `AiRangedAvoidance`（风筝），但在特定条件满足时**切换为即时冲锋**（复用引擎 Rhino 冲锋，同 A1 v3——引擎零前摇）：

1. **风筝至墙角/死角**：玩家被逼入墙边时，风筝怪放弃拉扯，转为冲锋（近身压制——玩家无处可退时不再安全）
2. **血量阈值**：风筝怪血量低于 30% 时不再逃跑，转为冲锋（垂死反击——玩家以为风筝怪只会跑，低血突然近身）

**低血 tell 说明**（v3 修正——即时冲锋无前摇窗口）：怪物 HP 条默认关闭（`Source/options.cpp:858`，宪章 §7），玩家在默认安装下看不到 <30% 精确阈值。且**冲锋是即时的**（无前摇窗口可打断）——低血触发是「冲锋启动瞬间」+「冲锋冷却」共同构成学习型反馈（首次遭遇因冷却而可存活，之后玩家学会「低血风筝怪会反击」）。与宪章「错误设计」（隐藏 to-hit roll 零反馈）不同——冲锋本身是可见反馈。若 mod 希望强 tell，可独立立项强制 HP 条可见（Base 决策，本规格不承担）。

**对玩家的新决策**：「**这只风筝怪会在墙角冲锋 / 低血会反击**」——洞穴战斗从「无脑风筝所有怪」变为「注意墙角站位 / 低血时保持距离」。这是对「38% 都是风筝怪」的直接修复：行为密度上升 = 玩家不能对每只怪都用同一套风筝打法。

**反制（DP2）**（v3 据整体验证 §8.2-1 修正——即时冲锋无前摇，全部指名可执行）：
1. **冲锋需距离 + LOS 门控**：冲锋触发必须有 `distanceToEnemy >= 3 && LineClear(...)`（复用 RhinoAi/SnakeAi 的冲锋门控，monster.cpp:2284-2286/2675-2683）——防止「隔墙冲锋」和「贴身冲锋」；距离 <3 走正常近战路径
2. **走位躲直线**：冲锋是直线弹道（`ProcessRhino`），侧移可躲——即时冲锋的反制是走位非打断（v3 修正）
3. **冲锋冷却**：冲锋后进入 `chargeCooldown`（复用 goalVar 模式）——防低血连锁 kill-wall（远程玩家在冷却窗口输出）
4. **墙角可走位**：被逼墙角是玩家可见的空间状态——冲锋门控下玩家有时间走位离开墙角
5. **垂死反击首次可存活**：低血触发是学习型 tell——冷却保证首次遭遇后玩家有输出窗口，不构成必死

**与既有系统互动（DP4）**：
- **走位战斗核心**：组合行为直接强化 D1 的走位决策（墙角站位、距离管理）
- **既有风筝 AI**：保留大部分风筝行为，只加条件性切换——风筝怪的身份不变，只是多了行为维度

### 4.2 承载选择（v4 修正——冲锋伤害投递路径 + 承载选优）

**冲锋伤害投递（v4 依据，据多 agent 独立复核）**：`MissToMonst → MonsterAttackPlayer` 用 **special 伤害列**（`monster.cpp:4587-4601`）。候选承载 special 列现状：

| 承载系 | 候选 | 普通伤害 | special 伤害 | 冲锋伤害可用性 |
|---|---|---|---|---|
| Magma | Hell Stone | 2-20 | **0-0** | ❌ 需设列（否则冲锋 ~1 伤害） |
| Storm | Storm Lord | 12-24 | **4-16** | ✅ 有真实伤害（v3 已选） |
| Acid | Lava Maw | 10-20 | **0-0** | ❌ 需设列 |

**v4 承载决策**：
- **垂死反击者 = Storm Lord（保留）**：special 4-16 已有真实伤害，无需改列
- **墙角冲锋者 = Hell Stone（保留但需设 special 列）**：给 Hell Stone 设 `minDamageSpecial/maxDamageSpecial = 普通伤害值 2-20`——复用引擎既有 `MissToMonst` 路径（与 A1 同方案，与 Rhino 系 `MT_HORNED` special=5-32 一致）。否则墙角惩罚 = 1 伤害推搡（独立复核证实的缺陷）

| 变体 | 承载怪物 | 组合行为 | tell | 反制（DP2） |
|---|---|---|---|---|
| **墙角冲锋者** | Magma 系 1 只（如 Hell Stone，special 18 帧，**需设 special 伤害列 2-20**） | 风筝 + 玩家被逼入墙角时冲锋 | **情境 tell**（被逼墙角可见）+ 冲锋启动时刻（Attack 动画） | 冲锋前可走位离开墙角（距离/LOS 门控下有时间反应）；走位躲直线 + 冲锋冷却 |
| **垂死反击者** | Storm 系 1 只（如 Storm Lord，special 14 帧，special 伤害 4-16 已有） | 风筝 + 血量 <30% 时冲锋 | **情境 tell**（低血 + 冲锋启动时刻） | 走位躲直线 + 冲锋冷却；首次遭遇可存活（冷却窗口）→ 可学习 |
| （备选） | Acid 系 1 只（如 Lava Maw，special 12 帧，**需设 special 伤害列 10-20**） | 风筝 + 冲锋 | 同上 | 12-14 |

**为什么只改 2-3 只**：保留大部分风筝怪（修复垄断不需要消灭风筝，而是打破「全是风筝」的单调）。2-3 只变体 + 原有 8-9 只纯风筝 = 洞穴战斗从「全是风筝」变为「大部分风筝 + 少数会冲锋」——垄断被打破，但洞穴的「远程威胁」身份保留。

**垄断声明诚实化（v4）**：38% 是池级数字（14/36 含 3 Never）；可生成比例实际 11/31 = 35%。「垄断被打破」指行为维度（2-3 只会冲锋），非物种比例大幅下降——诚实声明：物种池仍 ~35% 风筝，但战斗体验因条件冲锋而多样化。

**tint 规则**：本规格的组合行为通过**情境 tell**（墙角/低血 + 冲锋启动时刻）表达，不依赖 tint——与 A1 的 tint 语言无冲突（A1 用 tint+行为，A3 用情境）。

### 4.3 实现要点

```cpp
// 1. 新 AI 枚举值（monstdat.h 空槽位）：
//    MonsterAIID::KiteCharger（或按承载拆分 KiteCornerCharger / KiteLastStand）

// 2. AiProc 表新条目（monster.cpp）

// 3. 组合 AI（派生自 AiRangedAvoidance，v3 即时冲锋修正）：
//    void KiteChargerAi(Monster &monster)
//    {
//        // 条件判定（在 AiRangedAvoidance 之前），全部复用 RhinoAi/SnakeAi 门控：
//        if (monster.distanceToEnemy() >= 3
//            && LineClear(..., monster.position.tile, monster.enemyPosition)
//            && static_cast<MonsterMode>(monster.var1) != MonsterMode::Charge
//            && monster.chargeCooldown == 0) {          // v3 新增：冷却防连锁
//            const bool cornered = (玩家周围可移动方向数 <= 3);   // 墙角/走廊
//            const bool lastStand = (monster.hitPoints < monster.maxHitPoints * 30 / 100);
//            if (cornered || lastStand) {
//                AddMissile(position, enemyPosition, md, MissileID::Rhino, TARGET_PLAYERS, monster, 0, 0);
//                monster.mode = MonsterMode::Charge;   // 同 tick 即时冲锋（引擎事实，零前摇）
//                monster.chargeCooldown = ChargeCooldownTicks;  // v3 新增
//                return;
//            }
//        }
//        if (monster.chargeCooldown > 0) monster.chargeCooldown--;  // 冷却递减
//        AiRangedAvoidance(monster);               // 否则正常风筝
//    }
//
// 门控要点（Oracle 评审 B2 + v3 修正）：distanceToEnemy >= 3 防「贴身冲锋」、
// LineClear 防「隔墙冲锋」；距离 <3 走近战（不冲锋）；冲锋冷却防低血连锁

// 4. monstdat.tsv：2-3 只风筝怪 ai 列改派
```

**冲锋冷却（v3 新增，据整体验证 §8.2-1）**：即时冲锋无前摇 → 若冲锋结束（墙/落空）后立即再冲，形成「低血连锁 kill-wall」——远程玩家在冲锋间隔几乎无输出窗口。冲锋后进入 `chargeCooldown`（如 30 tick ≈ 1.5s），期间走 AiRangedAvoidance 风筝，给玩家输出窗口。这是即时冲锋的必需平衡项（与 A1 v3 一致）。

**「玩家被逼入墙角」判定**（v2 据 Oracle 评审 B3 修正）：检查玩家周围可移动方向数 ≤ 3（即玩家在走廊或死角）——阈值从 <2 提升到 ≤3，使冲锋在**走廊和死角都触发**（更频繁、更可学习），开阔地（≥4 方向）不触发。玩家可通过转弯（冲锋是直线）走位躲避——走廊冲锋可用转角打破视线。实施时验证精确判定。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Expansion |
| 2 | 问题陈述指向具体症状 | 第 1 节：洞穴 38% 垄断 + 11 只风筝怪共享 AI（monstdat 数据）有出处 |
| 3 | 引用的数值标注出处 | 第 3 节全部有出处 |
| 4 | 「已实施」需非测试调用者+验收全过 | 本规格为草案，不标注已实施 |

### 扩充层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 新增内容还是膨胀数值？ | 新增内容 | 新增 1 个组合行为（风筝+条件冲锋），既有怪物数值不动。**v4 修正**：墙角冲锋者需设 special 伤害列（2-20）= 普通伤害值——是行为配套（复用引擎既有 MissToMonst 路径），非膨胀 |
| 10 | 每个新增内容有可执行的反制与明确作用？ | 有且已指名 | 走位躲直线（侧移）+ 冲锋冷却；低血阈值是情境 tell；墙角可走位——全部指名且可执行（v3 修正：即时冲锋无前摇，反制靠走位+冷却） |
| 11 | 产生取舍还是负担？ | 取舍 | 「避免墙角站位 vs 正常风筝」+「低血先处理 vs 保持距离」是决策；情境 tell（墙角/低血）非新负担 |
| 12 | 在全部相关层段有定义？ | 是 | 定义在洞穴 9-12（2-3 只风筝怪层段内）；17-24 显式不放置（同熄灯者 4.2） |
| 13 | 与既有系统产生互动？ | 是 | 直接强化走位战斗核心；保留风筝 AI 主体只加条件切换——零新机制 |
| 14 | 近战/远程影响分别评估？ | 已评估 | 见下 |

**红线 14 评估**（v3 修正——即时冲锋无前摇；v4 修正——无「加速」机制，冲锋是位移非速度）：
- **墙角冲锋**：主要威胁近战（近战玩家更常在墙角近身战）——但近战玩家贴身时冲锋怪走正常近战（距离 <3 不冲锋）；远程玩家保持距离则墙角冲锋很少触发——近战略不利但走位/冷却是公平反制
- **垂死反击**：对远近程都有效——远程风筝时低血怪突然冲锋近身（需保持距离 + 侧移躲直线）、近战贴身时低血怪即时冲锋（位移突进，需优先击杀）——公平（冲锋是位移非「加速」——v4 修正：引擎无速度加成机制）
- **结论**：墙角冲锋略偏害近战（走位反制）、垂死反击公平——无单边倾斜，红线 14 通过

## 6. 验收标准（可执行）

| # | 验证项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 墙角冲锋触发 | unit test（复用熄灯者 harness）：玩家被逼入墙边 → 风筝怪 `AddMissile(Rhino)` + mode=Charge（即时冲锋，v3 修正） | 断言通过 |
| 1b | 冲锋伤害投递 | unit test：冲锋命中 → `MonsterAttackPlayer` 用承载 special 伤害列（Hell Stone 2-20 / Storm Lord 4-16，v4 修正——非 ~1 伤害） | 断言通过 |
| 2 | 垂死冲锋触发 | unit test：血量 <30% → 风筝怪冲锋而非继续风筝 | 断言通过 |
| 3 | 正常情况仍风筝 | unit test：无墙角/血量充足 → 走 `AiRangedAvoidance` 分支 | 断言通过 |
| 4 | 冲锋冷却 | unit test：冲锋后 `chargeCooldown` 生效，冷却期间不冲锋（v3 新增——防低血连锁） | 断言通过 |
| 4b | 冲锋距离门控 | unit test：距离 <3 或视线阻断 → 不冲锋（走近战） | 断言通过 |
| 5 | 墙角判定准确 | unit test：开阔地（≥4 方向）不触发、走廊/死角（≤3 方向）触发（判定函数验证） | 断言通过 |
| 6 | 承载分配正确 | 数据断言：2-3 只风筝怪 ai 列改派、其余 8-9 只不变 | 断言通过 |
| 7 | MP 确定性 | 双玩家 lockstep：组合 AI 与引擎冲锋同 draw-count 纪律 | 断言通过 |
| 8 | 全量测试 | 全量 ctest | 无新增失败（基线 = PackTest×2 + Writehero×1 既有失败） |
| 9 | 漂移校验 | `tools/check_drift.py` | 5 项 PASS |
| 10 | harness 量化 | eval case `monster-kite-combination`（见第 7 节） | 全过 |

**Harness 前提**：复用熄灯者规格（`2026-08-10-monster-behavior-density-design.md`）的怪物 AI tick harness。

## 7. Harness 量化设计（纳入 eval）

| eval case | 断言 | 量化指标 |
|---|---|---|
| `monster-kite-combination` | 墙角冲锋（触发/冷却/开阔不触发）、垂死反击（低血触发）、正常风筝、承载分配、**冲锋伤害投递（special 列 v4）** | gtest 断言计数（16/16），数值断言（冲锋距离、血量阈值、冷却 tick、冲锋伤害） |

量化口径：沿用现有 eval「gtest 二进制 + 断言计数」模式，不引入主观维度（宪章 §10）。

## 8. 状态

草案。按宪章流程：Oracle 对抗评审 → 实施计划 → 实施 → 全量门禁。

## 9. 相关规格

- **框架总览**（`2026-08-10-density-fix-framework-overview.md`）：本规格的框架定位
- **A1 教堂骷髅区分**（`2026-08-10-cathedral-skeleton-differentiation-design.md`）：共享 harness + tint/行为语言一致性
- **B1 采样反垄断**：本规格完成后的风筝类别分布是 B1 洞穴风筝上限的校准依据
