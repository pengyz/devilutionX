# 教堂骷髅区分（Cathedral Skeleton Differentiation）设计

**日期**：2026-08-10
**状态**：草案（v4——多 agent 独立复核修正：冲锋伤害投递路径 + 狂乱者 no-op/纯攻击性修复）
**分类**：Expansion（宪章决策 31 定位变更后；触及 monstdat ai 列 → 判定树规则 1 → Expansion）
**取代**：无（新规格；密度修复框架第一优先——独立复核重排后 A1 先行）

> **定位**：密度修复框架（`docs/knowledge/decision_density_fix_package.md`）经 Oracle 对抗评审 + 独立复核收敛。独立复核（C4/C5/C6）确认：A1 的目标应从「23 行 SkeletonMelee」收窄为「**8 个教堂骷髅**」（仅 12/23 行可生成，且教堂骷髅是教堂「无压力」诊断的唯一行为杠杆——教堂池零风筝怪，B1 采样无法制造压力，只能靠 A1 区分行为）。

> **v4 相对 v3 的变更**（据多 agent 独立复核，见框架总览 §8.4——**两个行为变体按字面不成立，已修复**）：
> - **冲锋伤害投递路径修正**：独立复核证实 `MissToMonst → MonsterAttackPlayer` 用 **special 伤害列**（`monster.cpp:4587-4601`），而 8 个骷髅 special 列全 0 → 冲锋仅 ~1 伤害。v3 声称的「冲锋伤害走普通伤害」**实际做不到**。v4 改为：给承载设置 special 伤害列（复用引擎既有路径，与 Rhino 系 `MT_HORNED` special=5-32 一致）
> - **狂乱者三重修正**：**(a)** 「攻速/移速提升、无视受击硬直」是造假——`goal=Attack` 是纯 AI 攻击性（近距 StartAttack/远距 RandomWalk 追击，`FallenAi` 2364-2368），无任何速度/硬直机制；**(b)** 原伪代码是 no-op——`SkeletonAi`（2124-2143）不读 `goal`，必须复制 FallenAi pursuit 分支；**(c)** 低血 tell 无视觉变化，靠行为差异（持续追击 vs 走走停停）
> - **验收基线更新**：AC5/AC6（狂乱者「攻速/移速提升」「可观察状态变化」）按 v4 修正为「持续追击/攻击频率提升」

> **v3 相对 v2 的变更**（据整体验证 §8.2-1——根因修复）：
> - **冲锋改即时**：引擎验证 RhinoAi/SnakeAi 同 tick `AddMissile(Rhino)+mode=Charge`（monster.cpp:2284-2292/2675-2683），**零前摇**——v2 的「0.8s 前摇可打断」是虚构（`StartSpecialAttack` 走近战路径，转冲锋需新 MonsterMode 违反零新机制）
> - **反制重写**：从「前摇打断」改为「**走位躲直线 + 冲锋冷却**」——即时冲锋无前摇可打断，反制靠侧移（直线弹道）与冷却（防低血连锁）
> - **新增冲锋冷却字段**：`chargeCooldown`（复用 goalVar 模式），防低血连锁 kill-wall
> - **tell 修正**：从「蓄力姿态」改为「tint=black + 冲锋启动瞬间」（即时冲锋用 Attack 动画，与行走/攻击可区分）

> **v2 相对 v1 的变更**（据 Oracle 对抗评审 5 个 blocking 逐一修复）：
> - **B1**：砍掉「防御者」——30% 减伤 ≈ +43% 有效 HP 是 DP1 明令禁止的数值膨胀；D1 无格挡动画/无背刺，反制不可执行（红线 10 违规）、不可读（无反馈 = 宪章「错误设计」）。替换为「狂乱者」（复用 Fallen 狂暴模式，低血 tell）
> - **B2**：冲锋复用引擎既有 `MonsterMode::Charge + MissileID::Rhino`（RhinoAi/SnakeAi 先例）——原「直线冲锋」无实现机制（SpecialMeleeAttack 是静止的）
> - **B3**：冲锋伤害走 Rhino 弹道碰撞（普通伤害），取消 ×1.5——8 行骷髅 special 数据列全 0，×1.5 无投递路径
> - **B4**：层段声明修正——Horror Captain 4-6 会泄漏到墓穴 5-6，冲锋 AI 加层守卫
> - **B5**：决策声明重述——不夸大「先躲还是先绕」，诚实声明「远程玩家前摇侧移 vs 打断；狂乱者低血决策」

---

## 1. 问题陈述

**症状**（可复现，扫描诊断）：教堂 1-4 层「均衡但缺乏压力」——12 类行为全游戏最均衡，但每类只有 1-4 种怪物，玩家很快看穿「走近砍/慢速近战/弓手」三件套。怪级 1-12 vs 玩家 1-7，玩家有初始装备优势，前几层缺乏压力。

**根因**（独立复核 C4/C5/C6 核实）：8 个教堂骷髅共享 `SkeletonMelee` AI——行为完全一样（走近→砍），差异只有数值（HP/伤害）。玩家在教堂面对的所有骷髅做同一件事。

**对照数据（2026-08-10 核实）**：

| 怪物 | 族 | walk | atk | special | tint | 层段 |
|---|---|---|---|---|---|---|
| Skeleton | skelaxe | 8 | 13 | **16** | white | 1-2 |
| Corpse Axe | skelaxe | 8 | 13 | **16** | skelt | 2-3 |
| Burning Dead | skelaxe | 8 | 13 | **16** | (空) | 2-4 |
| Horror | skelaxe | 8 | 13 | **16** | black | 3-5 |
| Skeleton Captain | skelsd | 8 | 12 | **16** | white | 1-3 |
| Corpse Captain | skelsd | 8 | 12 | **16** | skelt | 2-4 |
| Burning Dead Captain | skelsd | 8 | 12 | **16** | (空) | 3-5 |
| Horror Captain | skelsd | 8 | 12 | **16** | black | 4-6 |

**关键事实**：
- 8 个骷髅全有 **special 帧 16**（charge 前摇的动画基础）
- 两族（skelaxe/skelsd）各有 tint 变体（white/skelt/black）——**tint 是现成的行为 tell**
- 教堂池确认**零风筝怪**（Fallen 8 / SkeletonMelee 8 / Zombie 4 占主导）——教堂加压只能靠行为区分，无法靠采样

## 2. 分类判定

**第一步——改动是否触及数值/掉落/属性/光照/商店/战斗资源？**

- 修改 8 个教堂骷髅的 `ai` 列（分配到新行为变体）：**是**（规则 1）→ **Expansion**
- 新增行为变体（基于 SkeletonMelee 的派生 AI）：**是**（规则 1）→ Expansion

**补充判定**：

| 改动项 | 单独判定 | 结果 |
|---|---|---|
| 8 教堂骷髅 ai 列改派（monstdat） | 规则 1 | Expansion |
| 新 AI 变体函数（基于 SkeletonMelee 派生） | 规则 1 | Expansion |
| 零新美术（复用既有动画帧 + tint） | — | 不新增 |

**归类结论**：整体判为 **Expansion**，无条件生效（决策 31：无开关）。

## 3. 事实基础

全部数值于 2026-08-10 核实。

### 3.1 教堂骷髅数据

| 事实 | 值 | 出处 |
|---|---|---|
| 8 教堂骷髅共享 AI | `SkeletonMelee` | `monstdat.tsv` ai 列 |
| 全部有 special 帧 | 16 帧（skelaxe/skelsd 两族） | `monstdat.tsv` frames[6] |
| tint 变体 | white/skelt/black（两族各有） | `monstdat.tsv` trnFile |
| 教堂池零风筝 | Fallen 8 / SkeletonMelee 8 / Zombie 4 主导 | `monstdat.tsv` ai 分布 |

### 3.2 SkeletonMelee AI 基础

| 事实 | 值 | 出处 |
|---|---|---|
| 基础行为 | 走近→砍（`SkeletonAi`） | `Source/monster.cpp` |
| 攻击动作 | `StartAttack` | `Source/monster.cpp` |
| 移动 | `MoveEnemy`/`RandomWalk` | `Source/monster.cpp` |
| Special 动画 | 16 帧（charge 前摇可复用） | `monstdat.tsv` frames[6] |
| AI 状态变量 | `var1/var2/var3` | `Source/monster.h` |

## 4. 方案

### 4.1 行为变体（基于 SkeletonMelee 派生）

| 变体 | 行为 | 分配（按 tint 区分） | tell | 反制（DP2） |
|---|---|---|---|---|
| **冲锋者**（Charger） | 远距时**即时冲锋**（同 tick `AddMissile(Rhino)+mode=Charge`，复用 RhinoAi/SnakeAi——引擎零前摇）；近距时正常攻击 | skelaxe 族的 Horror（black tint）| **tint=black + 冲锋启动瞬间**（冲锋用 Attack 动画，与普通骷髅行走/攻击可区分） | **走位躲直线**（冲锋是直线弹道，侧移可躲）；**冲锋冷却**（防连锁）；近战贴身不触发 |
| **狂乱者**（Berserker） | 血量低于 50% 时进入**追击狂暴**：复制 FallenAi 的 `goal=Attack` 分支（近距 StartAttack / 远距 RandomWalk 追击，monster.cpp:2364-2368）——v4 修正：这是**纯 AI 攻击性**（无攻速/移速加成、无无视硬直），原「攻速/移速提升」声称被独立复核证伪 | skelaxe 族的 Burning Dead（base tint）| **低血 + 行为变化**（持续追击/更频繁攻击）——v4 修正：无姿态/视觉变化，tell 靠行为差异 | 优先击杀（狂暴是持续追击的威胁）；远程风筝（追击不增射程） |

**v4 变更（据多 agent 独立复核，见框架总览 §8.4）**：
- **狂乱者「攻速/移速提升、无视受击硬直」是造假**：`FallenAi` 的 `goal=Attack` 分支（monster.cpp:2364-2368）只是 `近距 StartAttack / 远距 RandomWalk 追击`——**无任何速度乘数或硬直免疫**（引擎无此类机制，`MFLAG_BERSERK` 只改目标选择不改速度）。狂乱者真实效果 = **纯 AI 攻击性**（相对 SkeletonAi 的 ~40-50% 攻击占空比，狂乱者几乎每 Stand tick 都行动，约 1.5-2× 攻击频率）。
- **原伪代码是 no-op**：v3 的 `goal=Attack; SkeletonAi(monster)` 无效——`SkeletonAi`（monster.cpp:2124-2143）**不读 `monster.goal`**。必须把 FallenAi 的 pursuit 分支**复制进**新 AI（在调 SkeletonAi 前按 goal 分支），并实现 `goalVar1` 倒计时（FallenAi 在 2316-2320 递减）。
- **tell 修正**：`goal=Attack` 无视觉/姿态变化——低血 tell 靠**行为差异**（持续追击 vs 普通骷髅走走停停），玩家可观察但不如 HP 条直观。若需强 tell 独立立项强制 HP 条（Base 决策，本规格不承担）。
| **普通**（Normal） | 保持 SkeletonMelee 现状 | 其余骷髅 | — | — |

**v3 变更（据整体验证 §8.2-1）**：**冲锋改为「即时冲锋」**——引擎验证 RhinoAi/SnakeAi 同 tick `AddMissile(Rhino)+mode=Charge`（monster.cpp:2284-2292/2675-2683），**零前摇**，`isPossibleToHit()` 于 Charge 返回 false（monster.cpp:4969-4977）。v2 的「0.8s 前摇可打断」是虚构（`StartSpecialAttack` 会走 `MonsterSpecialAttack` 的近战路径，且需新 MonsterMode 才能转冲锋——违反「零新机制」）。**反制从「打断前摇」改为「走位躲直线 + 冲锋冷却」**：冲锋是直线弹道（侧移可躲），冲锋后需冷却（防连锁）。

**v2 变更（据 Oracle 评审 B1/B5）**：砍掉原「防御者」——30% 减伤 ≈ +43% 有效 HP 是 DP1 明令禁止的数值膨胀，且 D1 无格挡动画/无背刺机制，其反制不可执行（红色 10 违规）、不可读（无反馈 = 宪章明列的「错误设计」模式）。替换为「狂乱者」——狂暴复用 Fallen 既有 `goal=Attack` 模式（零新机制），低血触发是天然 tell，且有真实决策（低血怪会狂暴 → 优先击杀或拉开）。

**变体分配原则**（独立复核 C7：tell 必须按 tint + 帧数据强制）：
- 每个变体分配到的行，tint 必须与其他变体的行不同（同族内可区分）
- 冲锋变体承载行需要攻击动画可用（冲锋用 Attack 动画；骷髅全有）——v3 修正：不再要求 special 帧（即时冲锋用 Attack 动画，非 special）
- 同一 sprite 家族内，不同变体必须 tint 不同

**具体分配**（初始方案，实施时微调）：

| 怪物 | 族 | tint | 变体 |
|---|---|---|---|
| Skeleton | skelaxe | white | 普通 |
| Corpse Axe | skelaxe | skelt | 普通 |
| **Burning Dead** | skelaxe | (空/默认) | **狂乱者** |
| **Horror** | skelaxe | black | **冲锋者** |
| Skeleton Captain | skelsd | white | 普通 |
| Corpse Captain | skelsd | skelt | 普通 |
| Burning Dead Captain | skelsd | (空/默认) | 普通 |
| Horror Captain | skelsd | black | **冲锋者（Captain 版）** |

**对玩家的新决策**（v2 据 Oracle 评审 B5 重述——声明真实存在的决策，不夸大）：教堂战斗从「8 种骷髅都走近砍」变为：
- **远程玩家面对冲锋者**：识别 black tint 骷髅 → 保持侧向移动（冲锋是直线弹道，侧移可躲）——moment-to-moment 走位决策
- **所有玩家面对狂乱者**：低血怪会狂暴 → 「优先击杀还是拉开距离」——低血时的新决策
- 近战玩家贴身时冲锋者不触发（走正常攻击），冲锋深度主要作用于远程——红线 14 诚实声明
- v3：冲锋是**即时**的（无前摇），反制是「侧移躲直线 + 冲锋冷却」——远程玩家需持续侧向走位，而非「前摇时反应」

**tint 规则声明**（v2 修正，据评审非阻塞 3）：规则是「**黑色 tint 的近战骷髅会冲锋**」——层 4-6 的 `skelbow\black` Horror（远程弓骷髅）不冲锋，靠斧/剑 vs 弓的武器区分。tint 本身是真实 TRN 换色（white/skelt/black；Burning Dead 空 trn = 基础棕），black-vs-white 可读，skelt-vs-base 边缘。

**红线 14 评估**（v4 修正——狂乱者为纯 AI 攻击性）：
- **冲锋者**：主要影响远程（近战贴身不触发）——即时冲锋无前摇反应窗口，但冲锋是**直线弹道**（远程玩家侧移可躲）；1 格走廊冲锋不可侧移（公平性风险，见 4.3 走廊限制 + 冲锋冷却缓解）。对远程略不利，但「侧移躲直线 + 冲锋冷却」是公平反制
- **狂乱者**：持续追击（纯 AI 攻击性，非攻速/移速——v4 修正）对近战（贴身被持续攻击）与远程（被持续追击）都有压力；低血触发是预警——公平
- **结论**：无单边职业倾斜（冲锋偏害远程但有走位反制，狂乱者公平），红线 14 通过

### 4.2 数值口径（v4 修正——冲锋伤害投递路径）

- **冲锋者（Horror）**：复用 Horror 数值（HP 12-20 / dam 4-9）。**冲锋伤害 = 设置承载的 special 伤害列**（`minDamageSpecial`/`maxDamageSpecial` = 普通伤害值 4-9）——v4 修正：独立复核证实 `MissToMonst → MonsterAttackPlayer` 用 special 列（`monster.cpp:4587-4601`），v2 声称的「冲锋伤害走普通伤害」**实际做不到**（special 列全 0 → 冲锋仅 ~1 伤害）。给承载设 special 列是复用引擎既有路径（与 Rhino 系一致：`MT_HORNED` special=5-32）。
- **冲锋者（Horror Captain）**：复用 Horror Captain 数值（HP 35-50 / dam 5-14），special 伤害列 = 普通伤害值 5-14。**不设 ×1.5**（评审 B3 已确认 1.5× 尖刺风险）。
- **狂乱者（Burning Dead）**：复用 Burning Dead 数值（HP 8-12 / dam 3-7），狂暴触发 = 血量 < 50%（见 4.1 狂乱者重写——goal=Attack 是纯 AI 攻击性，非攻速/移速加成，v4 修正）。
- **不新增怪物行**——只改既有 8 行的 ai 列 + special 伤害列 + 2 个派生 AI 函数，**零新美术**（复用 16 帧 special + tint）。

### 4.3 实现要点（v3 修正——即时冲锋，删除虚构前摇）

```cpp
// 1. 新 AI 枚举值（monstdat.h 空槽位 55+）：
//    MonsterAIID::SkeletonCharge, MonsterAIID::SkeletonBerserk

// 2. AiProc 表新条目（monster.cpp）：
//    {SkeletonCharge} → &SkeletonChargeAi,
//    {SkeletonBerserk} → &SkeletonBerserkAi,

// 3. 冲锋者 AI——复用引擎既有即时冲锋（RhinoAi/SnakeAi 先例，零前摇）：
//    void SkeletonChargeAi(Monster &monster)
//    {
//        // 层守卫（B4）：只在本规格定义的层段冲锋（避免 Horror Captain 4-6 泄漏到 5-6）
//        if (currlevel > MaxChargeLevel) { SkeletonAi(monster); return; }
//        // 距离 >= N（RhinoAi 用 5）且视线清晰且非冲锋中且冷却已过 → 即时冲锋
//        if (dist >= N && LineClear(...) && mode != Charge && chargeCooldown == 0) {
//            AddMissile(position, enemyPosition, md, MissileID::Rhino, TARGET_PLAYERS, monster, 0, 0);
//            monster.mode = MonsterMode::Charge;   // 同 tick 触发，零前摇（引擎事实）
//            chargeCooldown = ChargeCooldownTicks; // 防连锁（v3 新增）
//        }
//        // 距离 < N → 退回 SkeletonAi 普通近战
//    }
//
// 引擎已处理（无需新代码）：
//   - 墙碰撞：ProcessRhino MissToMonst snap-back（missiles.cpp:3762-3765）
//   - 伤害：弹道碰撞 → PlayerMHit（missiles.cpp:534）
//   - 冲锋中不可命中：isPossibleToHit() = false（monster.cpp:4974）——即时冲锋无前摇可打断
//   - MP 确定性：Rhino 模式经证明 lockstep 确定
// 新字段（v3）：chargeCooldown（int8_t，冲锋冷却 tick，每 tick 递减）——防低血连锁

// 4. 狂乱者 AI（v4 修正——复制 FallenAi pursuit 分支，非设 goal 后调 SkeletonAi）：
//    void SkeletonBerserkAi(Monster &monster)
//    {
//        // goalVar1 倒计时（FallenAi 在 2316-2320 递减；本 AI 必须自行递减）
//        if (monster.goalVar1 != 0) {
//            monster.goalVar1--;
//            if (monster.goalVar1 == 0)
//                monster.goal = MonsterGoal::Normal;
//        }
//        if (monster.hitPoints < monster.maxHitPoints / 2) {
//            monster.goal = MonsterGoal::Attack;
//            monster.goalVar1 = BerserkTicks;
//        }
//        if (monster.goal == MonsterGoal::Attack) {
//            // 复制 FallenAi pursuit 分支（monster.cpp:2364-2368）——SkeletonAi 不读 goal
//            if (monster.distanceToEnemy() < 2)
//                StartAttack(monster);
//            else
//                RandomWalk(monster, GetMonsterDirection(monster));
//            return;
//        }
//        SkeletonAi(monster);  // 非狂暴时正常骷髅行为
//    }
//    // 注意：goal=Attack 是纯 AI 攻击性（近距攻/远距追），无攻速/移速加成、无无视硬直
//    // （v4 修正——独立复核证伪原「攻速/移速提升」声称）

// 5. monstdat.tsv：8 行的 ai 列 + special 伤害列按 4.1/4.2 分配表改派
```

**冲锋冷却（v3 新增，据整体验证 §8.2-1）**：即时冲锋无前摇 → 若冲锋结束（墙/落空）后立即再冲，形成「低血连锁 kill-wall」——远程玩家在冲锋间隔几乎无输出窗口。冲锋后进入 `chargeCooldown`（如 30 tick ≈ 1.5s），期间走 SkeletonAi 普通近战/移动，给玩家输出窗口。这是即时冲锋的必需平衡项（选项 1 的代价）。

**走廊限制**（评审非阻塞 2 落地方案）：冲锋弹道速度（MissileID::Rhino velocity 18 ≈ 2.2× 玩家步速）在 1 格走廊不可侧移——若不可接受，冲锋距离 N 设上限（如 ≤5），使长走廊冲锋不能单方面剥夺躲避窗口。实施时验证走廊场景并记录。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Expansion |
| 2 | 问题陈述指向具体症状 | 第 1 节：教堂无压力（怪级 1-12 vs 玩家 1-7）+ 8 骷髅共享 AI（monstdat 数据）有出处 |
| 3 | 引用的数值标注出处 | 第 3 节全部有出处 |
| 4 | 「已实施」需非测试调用者+验收全过 | 本规格为草案，不标注已实施 |

### 扩充层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 新增内容还是膨胀数值？ | 新增内容 | 新增 2 个行为变体（冲锋/狂乱）。**v4 修正**：冲锋伤害 = 承载的 special 伤害列（= 普通伤害值，非 ×1.5）——给骷髅设 special 列是新增行为配套，非膨胀 |
| 10 | 每个新增内容有可执行的反制与明确作用？ | 有且已指名 | 冲锋：**走位躲直线（侧移）+ 冲锋冷却**（v3 修正——即时冲锋无前摇，反制从「打断前摇」改为「侧移+冷却」）；狂乱：低血预警（可提前拉开）——全部指名且可执行 |
| 11 | 产生取舍还是负担？ | 取舍 | 远程玩家「持续侧向走位躲冲锋 vs 站位输出」是 moment-to-moment 取舍；狂乱「低血怪优先击杀 vs 拉开」是低血决策——tint 识别是既有视觉语言扩展非新负担 |
| 12 | 在全部相关层段有定义？ | 是 | 定义在教堂 1-4 + Horror Captain 的 4-6 范围（诚实声明——评审 B4：Horror Captain maxDunLvl=6，层 6 属墓穴）。冲锋 AI 含层守卫（仅 ≤4 层冲锋）避免泄漏到 5-6；狂乱者无层段限制。5-16 的其他 SkeletonMelee 行（黑骑士等）不受影响 |
| 13 | 与既有系统产生互动？ | 是 | tint 是既有视觉语言；冲锋复用引擎既有 Rhino 冲锋（RhinoAi/SnakeAi 先例，**即时零前摇**——v3 确认引擎事实）；狂暴复用 Fallen 既有 goal=Attack 模式——全部零新机制（仅新增冲锋冷却倒计时字段，复用 goalVar 模式） |
| 14 | 近战/远程影响分别评估？ | 已评估 | 4.1 v4：冲锋主要影响远程（近战贴身不触发）、狂乱者公平（持续追击对远近程都增加压力，低血触发）——无单边倾斜 |

## 6. 验收标准（可执行）

| # | 验证项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 冲锋者即时冲锋触发 | unit test（复用熄灯者 harness）：距离 ≥ N → 同 tick `AddMissile(Rhino)` + mode=Charge（**无前摇**，v3 修正） | 断言通过 |
| 2 | 冲锋弹道 | unit test：`AddMissile(Rhino)` 生成 → `ProcessRhino` 处理（复用引擎冲锋） | 断言通过 |
| 3 | 冲锋冷却 | unit test：冲锋后 `chargeCooldown` 生效，冷却期间走 SkeletonAi 普通分支不冲锋（v3 新增——防低血连锁） | 断言通过 |
| 4 | 近距退回普通攻击 | unit test：距离 < N → 走 SkeletonAi 普通近战分支 | 断言通过 |
| 5 | 狂乱者狂暴触发 | unit test：血量 < 50% → goal=Attack 追击分支生效（v4 修正：近距 StartAttack / 远距 RandomWalk 追击，非攻速/移速） | 断言通过 |
| 6 | 狂乱者追击行为 | unit test：狂暴期攻击频率/追击持续性 > 普通 SkeletonAi（v4 修正：验证纯 AI 攻击性，非「可观察状态变化」——无视觉变化） | 断言通过 |
| 7 | 变体按 tint 分配 | 数据断言：8 行 ai 列符合 4.1 分配表；同族同 tint 行不跨变体 | 断言通过 |
| 8 | 冲锋层守卫 | 数据断言：层 > 4 时冲锋 AI 不触发（Horror Captain 4-6 不泄漏到 5-6） | 断言通过 |
| 9 | MP 确定性 | 双玩家 lockstep 模拟：冲锋/狂暴决策与 RhinoAi 同 draw-count 纪律（复用评审验证的 per-tick aiSeed 重播种模型） | 断言通过 |
| 10 | 全量测试 | 全量 ctest | 无新增失败（基线 = PackTest×2 + Writehero×1 既有失败） |
| 11 | 漂移校验 | `tools/check_drift.py` | 5 项 PASS |
| 12 | harness 量化 | eval case `monster-skeleton-differentiation`（见第 7 节） | 全过 |

**Harness 前提**：复用熄灯者规格（`2026-08-10-monster-behavior-density-design.md`）的怪物 AI tick harness。

## 7. Harness 量化设计（纳入 eval）

| eval case | 断言 | 量化指标 |
|---|---|---|
| `monster-skeleton-differentiation` | 冲锋（即时触发/弹道/冷却/近距退回/**伤害投递——special 列 v4**）、狂乱（低血追击触发）、tint 分配、冲锋层守卫 | gtest 断言计数（15/15），数值断言（冲锋距离、狂暴阈值、冷却 tick、冲锋伤害） |

量化口径：沿用现有 eval「gtest 二进制 + 断言计数」模式，不引入主观维度（宪章 §10）。

## 8. 状态

草案。按宪章流程：Oracle 对抗评审 → 实施计划 → 实施 → 全量门禁。

## 9. 相关规格

- **密度修复框架**（`docs/knowledge/decision_density_fix_package.md`）：本规格的框架依据（Oracle + 独立复核收敛）
- **熄灯者**（`2026-08-10-monster-behavior-density-design.md`，v4）：环境趣味，harness 前置来源
- **激励者**（`2026-08-10-monster-witch-cluster-design.md`，v3）：位置半径光环（A2 重构后），排期在本规格之后
- **克隆族区分**（未来）：5-16 的 SkeletonMelee 行（黑骑士等）+ 远程拉扯族，独立立项
