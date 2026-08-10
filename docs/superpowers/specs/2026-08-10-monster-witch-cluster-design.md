# 激励者（Aura Buffer）设计

**日期**：2026-08-10
**状态**：草案（v4——多 agent 独立复核修正：移除速度机制/修正 rate 引用，增益改用 goal=Attack 纯 AI 攻击性）
**分类**：Expansion（宪章决策 31 定位变更后；触及 monstdat 数值字段 → 判定树规则 1 → Expansion）
**取代**：无（新规格；落实用户决策 B + 系统性扫描落点）

> **定位**：本规格实现「必须先击杀特定怪物」的策略深度（用户对 D2 沉沦魔巫师的诉求），但**不移植 D2 的「复活尸体」机制**——D1 底层（1-5 只/遭遇、无 AoE 清屏）下复活是纯磨而非策略。改为**激励者（Aura Buffer）**——位置半径光环，复用 Fallen 鼓舞术的 dMonster 网格扫描模式。
>
> **v4 相对 v3 的变更**（据多 agent 独立复核，见框架总览 §8.4——速度机制证伪）：
> - **移除速度/攻速机制**：v3 声称「攻速/移速小幅提升」靠改动画速率——但独立复核证实：`rate` 是 **`AnimStruct` 静态字段**（monster.h:170，每个动画 graphic 的静态数据），**`Monster` 结构体无 rate 成员**（v3 引用的 `Monster::rate`（monster.h:170）是错误引用）；`changeAnimationData`（monster.h:301-304）从 `type().getAnimData(graphic)` 读静态 rate。临时改速需 Monster 实例新字段 + 动画系统侵入 = **新机制**（违反「零新机制」不变量）→ **v4 删除**，降级为「不改动画的增益」
> - **增益改用 `goal=Attack`（纯 AI 攻击性）**：激励者复用 Fallen 鼓舞术的完整既有模式——半径内怪获得 `goal=Attack + goalVar1=N` 倒计时（monster.cpp:2347-2356，与 A1 狂乱者 v4 语言一致）。**零新字段、零新机制**；衰减由被增益怪的 own `goalVar1` 倒计时驱动（FallenAi 消费时递减），不依赖激励者 AI 存活
> - **诚实声明生效范围**：`goal=Attack` 仅对**读 goal 的 AI**（FallenAi 等）生效——SkeletonAi 等不读 goal 的 AI 不受激励（no-op）。激励者群落的小怪承载须为读 goal 的 AI（Fallen 系），规格显式声明该约束
>
> **v1 教训**（已废弃）：v1 硬套 D2 沉沦魔巫师「复活尸体」。经系统性扫描确认 D1 战斗规模 1-5 只（`PlaceGroup` na=1~5，`monster.cpp:3741-3743`），「复活」制造的是「同一只怪打两次」的纯磨而非「杀不完」的策略。D2 复活对抗 AoE 清屏，D1 无此底层，手段不成立。
>
> **v2 教训**（已废弃）：v2 分「首领强化（leader buff）+ 威胁放大器」两机制——首领强化依赖 `getLeader()` 群落结构。**首评 REJECT**：普通散群无 leader（`monster.cpp:242` `leader = NoLeader`；`PlaceGroup` 默认 `leader=nullptr, leashed=false`；`setLeader` 仅 leashed 群落调用且强制 `minion.ai = leader->ai`），按字面把鼓舞术改为 `getLeader()==leader` 判定会 buff 零只怪。**v3 重构**：合并为单机制「激励者」——位置半径扫描（无 getLeader 依赖），衰减由 ProcessMonsters 全局驱动。

---

## 1. 问题陈述

**症状**（可复现，扫描诊断）：D1 的战斗缺乏「优先级目标」决策——大部分战斗是「一群同质怪物，挨个砍完」。扫描（`docs/knowledge/analysis_monster_config_landscape.md`）确认三个塌缩点：

| 层段 | 问题 | 数据 |
|---|---|---|
| 洞穴 9-12 | **远程拉扯垄断**——38% 怪物做同一件事 | 14/36 种共享 `AiRangedAvoidance` |
| 地狱 13-16 | **多样性骤降** + 数值悬崖 | 23 种只分 6 类行为；怪级 21-30 vs 玩家 13-19 |
| 教堂 1-4 | 均衡但缺乏压力 | 12 类行为但每类 1-4 种，玩家很快看穿 |

**目标**：通过「激励者」制造「先杀谁」的决策，针对性修复塌缩点（主落点墓穴 5-8 / 地狱 13-16，洞穴 9-12 备选——A3 优先）。

## 2. 分类判定

**第一步——改动是否触及数值/掉落/属性/光照/商店/战斗资源？**

- 新增激励者 AI（`MonsterAIID::AuraBuffer`）：**是**（规则 1）→ **Expansion**
- 新增/修改怪物数据行（monstdat）：**是**（规则 1）→ Expansion

**补充判定**：

| 改动项 | 单独判定 | 结果 |
|---|---|---|
| 新 `MonsterAIID::AuraBuffer` + AiProc 条目 | 规则 1 | Expansion |
| 激励者 AI 函数（位置半径光环） | 规则 1 | Expansion |
| monstdat 新行（激励者怪物） | 规则 1 | Expansion |
| `goal = Attack + goalVar1` 施加（复用既有字段，v4——零新字段） | 规则 1 | Expansion |

**归类结论**：整体判为 **Expansion**，无条件生效（决策 31：无开关）。

## 3. 事实基础

全部数值于 2026-08-10 核实。

### 3.1 群落机制（v3 重构依据——普通散群无 leader）

| 事实 | 值 | 出处 |
|---|---|---|
| 群落生成 | `PlaceGroup(typeIndex, num, leader=nullptr, leashed=false)`——普通散群**无 leader** | `Source/monster.cpp:308` |
| 无 leader 状态 | `leader = NoLeader`（-1） | `monster.cpp:242`、`monster.h:292` |
| setLeader 强制同 AI | `minion.setLeader(leader)` 设 `ai = newLeader->ai`（仅 leashed 群落调用） | `monster.cpp:4893-4907`、`monster.cpp:357` |
| 群组维护 | `packSize`、`getLeader()`、`GroupUnity`——**仅 leashed 群落有** | `monster.cpp:378/1694-1711` |
| 群落规模 | na = 1（层1）/ 2-3（层2-4）/ 3-5（层5+）——**散群，无 leader** | `monster.cpp:3741-3743` |
| 小怪临时状态 | `goal` + `goalVar1` 倒计时（Fallen 鼓舞术模式） | `monster.cpp:2347-2356` |

**v3 结论**：普通散群（`PlaceGroup` 无 leader）的 `getLeader()` 返回 null——「跨 AI 混合群落」在现有引擎不可实现（`setLeader` 强制同 AI）。本规格**不依赖 leader 结构**，用位置半径扫描。

### 3.2 Fallen 鼓舞术（激励者的原型——dMonster 网格扫描）

| 事实 | 值 | 出处 |
|---|---|---|
| 触发 | `StartSpecialStand` + `FlipCoin(4)`（25% 每动画结束） | `monster.cpp:2335-2336` |
| 范围 | 半径 `2×intelligence + 4` | `monster.cpp:2342` |
| 扫描方式 | dMonster 网格双重循环（位置扫描，非 leader 判定） | `monster.cpp:2343-2355` |
| 效果 | 半径内**同 AI（Fallen）**怪获得 `goal=Attack, goalVar1=30×int+105` | `monster.cpp:2347-2356` |
| 限制 | **只影响 `ai == MonsterAIID::Fallen` 的怪**——激励者复用扫描模式但去掉此限制 | `monster.cpp:2351` |
| 自愈 | `hitPoints += 2×int+2` | `monster.cpp:2339` |

### 3.3 怪物速度/攻击（v4 修正——速度机制证伪，增益改用 goal=Attack）

| 事实 | 值 | 出处 |
|---|---|---|
| 攻击动作 | `StartAttack`（近战）/ `MonsterAttackPlayer` | `monster.cpp` |
| 怪物移动 | `MoveEnemy`/`RandomWalk` | `monster.cpp` |
| 动画速率字段 | **`AnimStruct::rate`（静态字段，每个动画 graphic 一份）**——v4 修正：v3 引用的 `Monster::rate`（monster.h:170）是**错误引用**（monster.h:170 实为 `AnimStruct::rate`，`Monster` 结构体无 rate 成员） | `monster.h:170` |
| 动画切换 | `changeAnimationData` 从 `type().getAnimData(graphic)` 读静态 rate（monster.h:301-304）——**临时改速需 Monster 实例新字段 + 动画系统侵入** | `monster.h:301-304` |
| 临时速度修改 | **无现成机制**（v3 自认；v4 结论：**放弃**——改速违反「零新机制」，见 §4.1） | 代码核实 |
| 纯 AI 攻击性增益 | `goal=Attack + goalVar1=N` 倒计时（Fallen 鼓舞术既有模式）——**零新字段** | `monster.cpp:2347-2356` |

## 4. 方案

### 4.1 机制：激励者（Aura Buffer）——位置半径光环（v3 重构，合并原首领强化+放大器）

**v3 重构依据（据 A2 首评 REJECT + 整体验证 §8.2-2）**：原「首领强化」依赖 `getLeader()` 群落结构——但**普通散群无 leader**（`monster.cpp:242` `leader = NoLeader`；`PlaceGroup` 默认 `leader=nullptr, leashed=false`；`setLeader` 仅 leashed 群落调用，且强制 `minion.ai = leader->ai`）。按字面把鼓舞术改为 `getLeader()==leader` 判定会 **buff 零只怪**。重构：**放弃 leader 结构判定，改为位置半径扫描**——复用 Fallen 鼓舞术的 dMonster 网格扫描模式（monster.cpp:2343-2356），把 `ai == MonsterAIID::Fallen` 判定改为「半径内其他怪物」。

**行为（v4 修正——增益为纯 AI 攻击性，非速度）**：激励者（Aura Buffer）在场时，**半径内所有其他怪物**获得增益（`goal=Attack + goalVar1=N` 倒计时——纯 AI 攻击性：近距 StartAttack/远距追击，与 A1 狂乱者 v4 语言一致，**无速度/攻速加成**）。激励者死亡 → 增益自然衰减（被增益怪的 own `goalVar1` 倒计时递减至 0，FallenAi 消费时递减——不依赖激励者 AI 存活）。激励者自身弱、躲在群落后方。

**v4 修正说明（据独立复核）**：v3 的「攻速/移速小幅提升」需改动画速率——但 `rate` 是 `AnimStruct` 静态字段（monster.h:170，每动画 graphic 一份），`Monster` 结构体无 rate 成员（v3 引用错误），临时改速需 Monster 实例新字段 + 动画系统侵入（新机制，违反「零新机制」不变量）。v4 放弃速度机制，改用 Fallen 鼓舞术的 `goal=Attack` 既有模式——零新字段、零新机制。

**生效范围诚实声明（v4）**：`goal=Attack` 仅对**读 goal 的 AI**（FallenAi 等，monster.cpp:2364-2368 消费 goal）生效；SkeletonAi 等不读 goal 的 AI 不受激励（no-op）。激励者群落的小怪承载须为读 goal 的 AI（Fallen 系）——规格显式约束，避免实施期「激励了个寂寞」。

**对玩家的新决策**：「**先杀激励者**（它在幕后让一切更难）」——激励者是隐性的威胁来源，玩家需要识别它并优先处理。这是「威胁放大器」——某个怪在场时其他怪更难对付，而非又一只风筝怪（直击洞穴 9-12 远程垄断的修复）。

**反制（DP2）**（全部指名，可执行）：
1. **优先击杀**：激励者本体弱（低血量、移动慢，参照同层段近战怪下四分位）
2. **范围限制**：光环半径有限，可拉开距离脱离增益
3. **可识别**：激励者有区别于其他怪的外观/行为（Stand 时专注施法姿态，复用 special 帧）
4. **衰减可观察**：激励者死亡后增益递减，玩家可见怪物变弱

**落点（扫描锚定，v3 修正）**：
- **洞穴 9-12**：远程拉扯垄断（38%）——激励者让风筝怪群变成「先杀激励者」的决策，打破纯风筝。**但注意框架 §5.3 A2/A3 竞争**：A3（风筝组合）也落洞穴——两个机制同层段可能过度。**v3 决策**：A3 优先（直接改造风筝怪本身），本规格激励者在洞穴**降级为备选**，主落点移向墓穴 5-8 + 地狱 13-16
- **墓穴 5-8**：多样性峰值但无组织者——引入激励者群落（Fallen 巫师激励者 + 混合小怪）
- **地狱 13-16**：需要打破数值单调——激励者在数值悬崖处提供「先杀激励者」的策略出口

**v3 移除的机制**：原「首领死亡 → 群落瓦解」——依赖 `M_UpdateRelations` 钩子，但普通散群无 leader 关系，瓦解语义不存在。重构后激励者死亡 → 光环自然衰减（倒计时），无需 M_UpdateRelations 钩子。

### 4.3 数值口径（全层段定义）

| 机制 | 层段 | 落点 | 承载 | 数值 |
|---|---|---|---|---|
| 激励者 | 墓穴 5-8 | 5-8 | Fallen 巫师激励者（复用 Fallen 动画，**非 leashed leader——普通散群承载**） | 光环半径、增益幅度参照 Fallen 鼓舞术基线 |
| 激励者 | 地狱 13-16 | 13-16 | Fallen 系激励者（深层变体） | 同上（数值随层段上调） |
| 激励者（备选） | 洞穴 9-12 | 9-12 | 新怪（复用某既有怪物动画） | 同上——**A3 优先，A2 洞穴为备选**（框架 §5.3） |

**17-20 Nest / 21-24 Crypt**：**不放置**（显式决策，同熄灯者规格 4.2：Hellfire 专属内容不属于核心 1-16 体验）。

**增益实现（v4 修正——复用 Fallen 鼓舞术完整模式，零新机制）**：激励者每 tick 给半径内怪物设 `goal = Attack + goalVar1 = N`（monster.cpp:2347-2356 既有字段，无需新字段）——被增益怪由 own AI 消费 goal（FallenAi 近距 StartAttack/远距追击），`goalVar1` 每 tick 递减至 0 后恢复普通行为。**衰减天然独立于激励者存活**：激励者死亡后不再刷新，目标怪的 own `goalVar1` 倒计时走完即恢复（与 Fallen 鼓舞术的天然行为一致，无需 ProcessMonsters 新逻辑）。v3 的「rate 覆盖 + amplifiedTicks」方案**已删除**（rate 是 AnimStruct 静态字段，Monster 无 rate 成员，临时改速是新机制）。

### 4.4 实现要点（v3 重构——位置半径扫描，无 getLeader 依赖；v4 修正——增益用 goal=Attack 零新机制）

```cpp
// 1. 新 AI 枚举值（monstdat.h 空槽位 55+）：
//    MonsterAIID::AuraBuffer（激励者）

// 2. AiProc 表新条目（monster.cpp）：
//    {AuraBuffer} → &AuraBufferAi,

// 3. 激励者 AI——复用 Fallen 鼓舞术的 dMonster 网格半径扫描模式（monster.cpp:2343-2356）：
//    void AuraBufferAi(Monster &monster)
//    {
//        if (monster.mode != MonsterMode::Stand || monster.activeForTicks == 0) return;
//        // 每 tick：扫描 radius 内所有怪物（dMonster 网格，同 Fallen 鼓舞术）
//        for (int y = -rad; y <= rad; y++) {
//            for (int x = -rad; x <= rad; x++) {
//                const int m = dMonster[monster.position.tile.x + x][monster.position.tile.y + y];
//                if (m <= 0) continue;
//                Monster &other = Monsters[m - 1];
//                if (other.mode == MonsterMode::Death || other.isInvalid) continue;
//                // v4：增益 = goal=Attack + goalVar1 倒计时（复用 Fallen 鼓舞术既有字段，零新机制）
//                other.goal = MonsterGoal::Attack;
//                other.goalVar1 = AuraTicks;       // 倒计时由目标怪 own AI 递减（FallenAi 模式）
//            }
//        }
//        // 激励者死亡（MonsterDeath 路径）→ 不再刷新，目标怪 own goalVar1 走完即恢复
//    }

// 4. 生效范围约束（v4 诚实声明）：goal=Attack 仅对读 goal 的 AI（FallenAi 等）生效；
//    激励者群落的小怪承载须为读 goal 的 AI（Fallen 系）——SkeletonAi 等不读 goal，不受激励
```

**临时状态机制（v4 修正——衰减归属天然独立）**：复用 Fallen 鼓舞术的完整既有模式（monster.cpp:2347-2356）：激励者每 tick 给半径内怪物设 `goal = Attack + goalVar1 = N`，被增益怪由 own AI 消费 goal 时递减 `goalVar1`（FallenAi 模式）——**衰减不依赖激励者 AI 运行**（死亡后不再刷新，目标怪倒计时走完即恢复），也**无需 ProcessMonsters 新逻辑或新字段**（v4 删除 v3 的 `amplifiedTicks` 字段 + ProcessMonsters 全局递减——那是为 rate 覆盖配套的，rate 机制已证伪删除）。与 A1 狂乱者 v4（goal=Attack 纯 AI 攻击性）语言一致。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Expansion |
| 2 | 问题陈述指向具体症状 | 第 1 节：扫描诊断（洞穴垄断 38%/地狱 6 类/早期缺压力）有出处 |
| 3 | 引用的数值标注出处 | 第 3 节全部有出处 |
| 4 | 「已实施」需非测试调用者+验收全过 | 本规格为草案，不标注已实施 |

### 扩充层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 新增内容还是膨胀数值？ | 新增内容 | 新增 1 个行为机制（激励者光环），既有怪物数值不动 |
| 10 | 每个新增内容有可执行的反制与明确作用？ | 有且已指名 | 优先击杀（本体弱）/ 范围脱离（光环半径有限）/ 可识别（special 姿态）——全部指名 |
| 11 | 产生取舍还是负担？ | 取舍 | 「先杀激励者 vs 清小怪」是核心取舍；不杀 = 小怪更难（DP3 稀缺压力） |
| 12 | 在全部相关层段有定义？ | 是 | 5-8/13-16 定义放置，洞穴 9-12 备选（A3 优先，框架 §5.3）；17-24 显式不放置（同熄灯者 4.2） |
| 13 | 与既有系统产生互动？ | 是 | 复用 Fallen 鼓舞术的 dMonster 半径扫描模式（v3 重构——不再依赖 getLeader/M_UpdateRelations，普通散群无 leader）；激励者直击洞穴远程垄断（备选） |
| 14 | 近战/远程影响分别评估？ | 已评估 | 见下 |

**红线 14 评估**（v3 重构后单机制）：
- **激励者**：增益影响半径内所有怪，远近程都受影响；激励者本体弱且移动慢，两职业都可优先击杀；增益通过特殊姿态可识别——公平
- **结论**：激励者制造「优先级」决策而非单边职业倾斜，红线 14 通过

## 6. 验收标准（可执行）

| # | 验证项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 激励者光环 | unit test（复用熄灯者 harness）：激励者在场 → radius 内怪物 `goal=Attack + goalVar1>0`（v3：半径扫描，无 getLeader 判定；v4：增益为 goal=Attack，零新字段） | 断言通过 |
| 2 | 激励者范围限制 | unit test：怪物超出光环半径 → `goalVar1` 不刷新、递减归零 | 断言通过 |
| 3 | 激励者死亡衰减 | unit test：激励者死亡（MonsterDeath 路径）→ 不再刷新，目标怪 own `goalVar1` 递减至 0（v4：FallenAi 消费递减，天然独立于激励者存活） | 断言通过 |
| 4 | 衰减独立于激励者 AI | unit test：激励者死亡后模拟多 tick → 增益仍递减至 0（验证衰减由目标怪 own AI 驱动，不依赖激励者 AI 运行） | 断言通过 |
| 4b | 生效范围约束（v4） | unit test：激励者半径内 Fallen 系怪（读 goal）→ 增益生效；SkeletonAi 怪（不读 goal）→ 无效果（no-op，诚实声明验证） | 断言通过 |
| 5 | 激励者本体弱 | 数据断言：血量/伤害 < 同层段近战怪下四分位 | 断言通过 |
| 6 | 可识别 | unit test：激励者有区别于其他怪的状态（special 姿态 / 增益怪物可见变化） | 断言通过 |
| 7 | MP 确定性 | 双玩家模拟：同一 aiSeed 下光环刷新/衰减决策一致（复用熄灯者 AC8 模式） | 断言通过 |
| 8 | 全量测试 | 全量 ctest | 无新增失败（基线 = PackTest×2 + Writehero×1 既有失败） |
| 9 | 漂移校验 | `tools/check_drift.py` | 5 项 PASS |
| 10 | harness 量化 | eval case `monster-aura-buffer`（见第 7 节） | 全过 |

**Harness 前提**：复用熄灯者规格（`2026-08-10-monster-behavior-density-design.md`）的怪物 AI tick harness。

## 7. Harness 量化设计（纳入 eval）

| eval case | 断言 | 量化指标 |
|---|---|---|
| `monster-aura-buffer` | 光环（半径内 `goal=Attack+goalVar1>0`）、范围限制、死亡衰减（目标怪 own 倒计时，独立于激励者 AI）、**生效范围约束（读 goal 的 AI 生效/不读 no-op，v4）**、本体弱、可识别 | gtest 断言计数（16/16），数值断言（光环半径、goalVar1 衰减 tick） |

量化口径：沿用现有 eval「gtest 二进制 + 断言计数」模式，不引入主观维度（宪章 §10）。

## 8. 状态

草案（v4——A2 首评 REJECT 后 v3 重构为「激励者」（移除 getLeader 依赖）；v4 据独立复核移除速度机制（rate 引用错误 + 新机制违规），增益改用 goal=Attack 纯 AI 攻击性（零新字段），衰减由目标怪 own goalVar1 倒计时驱动）。按宪章流程：Oracle 对抗评审 → 实施计划 → 实施 → 全量门禁。

## 9. 相关规格

- **框架总览**（`2026-08-10-density-fix-framework-overview.md`）：本规格的框架定位 + A2/A3 洞穴竞争裁决（§5.3，本规格洞穴降级为备选）
- **熄灯者**（`2026-08-10-monster-behavior-density-design.md`，v5）：环境趣味，本规格的 AI tick harness 前置来源
- **教堂骷髅区分**（`2026-08-10-cathedral-skeleton-differentiation-design.md`，v4）：共享 harness + 行为语言一致性（狂乱者同用 goal=Attack 纯 AI 攻击性，v4 语言一致）
- **怪物配置图谱**（`docs/knowledge/analysis_monster_config_landscape.md`）：本规格落点的扫描依据
