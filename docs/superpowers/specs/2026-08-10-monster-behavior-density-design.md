# 怪物行为密度（Monster Behavior Density）设计 v3

**日期**：2026-08-10（v1 被 Oracle 一轮评审 REJECT；v2 被二轮评审 APPROVE-WITH-CHANGES；v3 据整体验证修正；v4 据独立复核修正；v5 据独立复核修正）
**状态**：草案（v5——多 agent 独立复核修正：死亡钩子 MP 缺陷 + refcount 对称性）
**分类**：Expansion（宪章决策 31 定位变更后；触及 monstdat 数值字段 → 判定树规则 1 → Expansion）
**取代**：`archive/specs/2026-08-10-monster-behavior-density-design.md`（v1，被 Oracle REJECT）

> **v5 相对 v4 的变更**（据多 agent 独立复核，见框架总览 §8.4——死亡钩子读可变 `monster.enemy` 的 MP 缺陷）：
> - **死亡钩子记录目标玩家 id**：v4 的死亡清理读 `Players[monster.enemy]`——但 `monster.enemy` 由 `UpdateEnemy` 每 tick 维护，熄灯者死亡瞬间目标可能已切换（目标玩家死亡/离层/换目标）→ MP 下可能**清错玩家**（refcount 减到别的玩家头上）甚至 `Players[NoEnemy=-1]` **越界**。v5 修正：熄灯者触发压制时把目标玩家 id **记录在怪物身上**（`monster.var3 = enemy.getId()`，复用 AI 状态变量），死亡清理读**记录的目标**而非可变的 `monster.enemy`——目标固定，refcount 施加/释放对称
> - **refcount 不对称修复**：施加侧（AI 触发）对 `targetPlayer` 递增、释放侧（死亡）对同一目标递减——v4 的释放侧重新读 `monster.enemy` 破坏对称性（若目标已切换，施加与释放不在同一玩家）；v5 两侧都以记录的 `var3` 目标为准
> - **到期路径不变**：到期生命周期仍在 `ProcessPlayers`（玩家侧字段，天然对称——压制活在 Player struct）

> **v4 相对 v3 的变更**（据整体验证 §8.2-3——反制声明修正 + 撤销先例）：
> - **Infravision 反制降级**：v3 声称「Infravision 消耗卷轴预算形成取舍」——但卷轴被 `DarkExpeditionDropOk`（items.cpp:189-190）+ `WitchItemOk`（items.cpp:2038）**双重排除**，唯一通道是 `bookLevel=5` 法术书（36 魔法，施法者专属）。修正：Infravision 降级为「施法者已学法术/暗区已激活卷轴」时的情境反制；**非施法者的通用反制 = 前摇打断 + 时限 + 绕开**——对「环境趣味」定位足够
> - **引用宪章光照压制撤销先例**：宪章 §6 记录「光照压制」曾因「无反制、无预算取舍、职业影响不均」被撤销（「确定不会被原样复活」）。本规格 v4 必须显式论证**非原样复活**：新增前摇打断（通用反制）+ 时限 + MP 正确生命周期（R1-R3）——三个撤销缺陷已修复；职业影响（红线 14）已评估

> **v2 相对 v1 的变更**（据 Oracle 一轮评审 5 个 blocking 问题逐一修复）：
> - **B1**：砍掉 LightShy——其「光照取舍」前提在 D1 不可实现（无开关、无火把、无法主动变暗），保留只会产出幻觉取舍
> - **B2**：行为按键从 `MyPlayer` 改为 `Players[monster.enemy]`（引擎标准访问，`monster.cpp:1267`）；新增多人模式专节
> - **B3**：压制实现为 `CalcPlrLightRadius` 内部修饰符（带 refcount），而非直接 `ChangeLightRadius`——与装备/视野/Dark Expedition/clamp 天然组合
> - **B4**：全层段（1-16 / 17-20 / 21-24）minDunLvl/maxDunLvl 在规格内定义，不留「实施时定」
> - **B5**：先定熄灯者攻击类别与射程，再推导远近程不对称性（结论与 v1 相反，v1 是反的）

> **v3 相对 v2 的变更**（据 Oracle 二轮评审 4 项 required corrections 逐一修复）：
> - **R1**：触发/恢复重算指名 `CalcPlrItemVals(enemy, false)`——v2 的 `CalcPlrLightRadius(enemy, <重算>)` 占位符若填 `_pLightRad` 会二次应用 Dark Expedition 倍率，恢复落在 2 而非 6/8/5（AC4 全挂）
> - **R2**：死亡清理钩 `MonsterDeath`（`monster.cpp:4003`，本地+MP 远程击杀共同汇聚点）而非 `M_StartKill`——后者漏掉被其他玩家击杀的熄灯者，导致 MP 永久压制
> - **R3**：到期生命周期放 `ProcessPlayers`（每 tick 无条件运行）而非熄灯者 AI——AI 拥有的到期在玩家离层后失效，压制跨层永久化；refcount 单次 decrement 防护（到期清空、死亡递减、count>0 守卫）
> - **R4**：17-24 排除理由修正——`min(base,3)` 在任何层段都不会低于 3，「不可行动黑暗」不可能发生；排除改为纯范围决策（Hellfire 专属内容不属于核心 1-16 体验）

---

## 1. 问题陈述

**症状**（可复现）：D1 深度越深，战斗决策越趋同。原因不是「怪物数值高」，而是**行为原语复用过度**——玩家面对的大部分怪物做同一件事（走近→砍），行为差异完全来自数值（移速/射程/伤害），不来自行为本身。「信息稀缺」张力（D1 核心体验）高度依赖「你不知道这个怪物会怎么对付你」——当所有怪物都是同一行为模式，黑暗只剩视觉折扣，不是认知威胁。

**根因核实（2026-08-10）**：`monstdat.tsv` 的 `ai` 列实测复用情况：

| AI 值 | 复用该 AI 的怪物行数 | 行为 |
|---|---|---|
| `SkeletonMelee` | **23 行** | 走近→砍（含换皮/数值变体） |
| `AiRangedAvoidance` 家族 | ~17 行（Storm 8 + Magma 4 + Acid 4 + Diablo） | 边退边射 |
| `AiRanged` 家族 | ~12 行（Succubus/Lich/ArchLich/Psychorb/Necromorb/FireBat/Torchant 等） | 站定→射 |
| `Fallen` 族 | ~10 行 | 召唤+恐惧撤退 |

三个克隆簇（SkeletonMelee 23 / RangedAvoidance 17 / AiRanged 12）占全部 112 类怪物的一半以上。

**本规格要解决的问题**（v4 定位修正，据用户决策 B）：
1. **行为 1（本规格）**：新增「熄灯者」行为——定位为**环境趣味**，修复「黑暗没有主动执行者」的缺失：黑暗层段中黑暗本身不会威胁玩家。熄灯者让「黑暗」变成一个有前摇、可打断、可反制的环境扰动（它弄暗你，但不「强迫你必须先杀它」）。
2. **「优先级目标」（D2 沉沦魔巫师效果）另立规格**（见第 9 节）：熄灯者经评估达不到「强迫必须先击杀」的效果——它的威胁是「削弱玩家」（信息剥夺），沉沦魔巫师的威胁是「增殖敌人」（杀不完）。后者才是「强制优先级」的正解。D1 引擎已有 SkeletonKing 无限召唤先例（LeoricAi），「首领/巫师 + 群落」将作为独立规格实现。
3. **密度诊断的完整修复（另立规格）**：23 行 SkeletonMelee 克隆簇是密度塌缩的主体，区分既有克隆族为独立规格。

## 2. 分类判定

**第一步——改动是否触及数值/掉落/属性/光照/商店/战斗资源？**

- 新增 `MonsterAIID::LightSnuffer` + 新 AI 函数（C++）：**是**（规则 1「TSV ai 列」+ 影响怪物行为）→ **Expansion**
- 新增怪物数据行（等级/伤害/掉落）：**是**（规则 1 TSV 数值）→ Expansion
- 光照交互（压制玩家光源半径）：**是**（规则 4「光照半径」）→ Expansion
- `CalcPlrLightRadius` 增加压制修饰符：**是**（规则 4）→ Expansion

**补充判定**：

| 改动项 | 单独判定 | 结果 |
|---|---|---|
| `MonsterAIID::LightSnuffer` 新枚举值 + AiProc 表新条目 | 规则 1 | Expansion |
| 新 AI 函数 `LightSnufferAi`（C++） | 规则 1 | Expansion |
| `CalcPlrLightRadius` 压制修饰符 | 规则 4 | Expansion |
| 熄灯者怪物数据行 | 规则 1 | Expansion |

**归类结论**：整体判为 **Expansion**，无条件生效（决策 31 定位：无开关）。

## 3. 事实基础

全部数值于 2026-08-10 核实。

### 3.1 光照系统（B3 修复基础）

| 事实 | 值 | 出处 |
|---|---|---|
| 光照计算单一入口 | `CalcPlrLightRadius(Player&, int lrad)` | `Source/items.cpp:2574` |
| 入口内做的事 | Dark Expedition 倍率 → `clamp(2,15)` → `ChangeLightRadius` + `ChangeVisionRadius` + 更新 `_pLightRad`（三件事一起，仅在 `_pLightRad != lrad` 时） | `Source/items.cpp:2574-2585` |
| 压制修饰符插入点 | `clamp` 之后、`if (_pLightRad != lrad)` 之前 | `Source/items.cpp:2578-2579` 之间 |
| 玩家光源 id | `player.lightId` | `Source/player.h:220` |
| `ChangeLightRadius` 对 `NO_LIGHT` | 早退（安全 no-op） | `Source/lighting.cpp:404` |
| `ChangeVisionRadius` 签名 | `(size_t id, int r)` | `Source/lighting.h:74` |
| 玩家基础光照半径 | 10 | `Source/player.cpp:2320` |

### 3.2 怪物行为（B2 修复基础）

| 事实 | 值 | 出处 |
|---|---|---|
| 怪物标准玩家访问 | `Players[monster.enemy]`（非 `MyPlayer`） | `Source/monster.cpp:1267` `MonsterAttackPlayer(monster, Players[monster.enemy], ...)` |
| 敌人目标设置 | `UpdateEnemy` 设 `monster.enemy` + `enemyPosition` | `Source/monster.cpp:679-748` |
| MP 确定性 | `SetRndSeed(monster.aiSeed)` 保证各客户端 AI 决策一致 | `Source/monster.cpp:4272-4275` |
| Special 攻击触发帧 | `currentFrame == animFrameNumSpecial - 1`（16 帧中第 15 帧，约 0.7s 反应窗口） | `Source/monster.cpp:1364` |
| 击杀中断 Special | `StartMonsterDeath` → `MonsterDeath` → mode=Death，回调不再运行 | `Source/monster.cpp:4024-4044` |
| 受击中断 Special | HitRecovery 切换，回调不运行 | `Source/monster.cpp` 受击路径 |
| AI 状态变量 | `var1/var2/var3`、`goalVar1-3` | `Source/monster.h` |
| AI 派发表 | `AiProc[128]`，枚举到 `Custom=55`，55-127 有 72 空槽 | `Source/monster.cpp:3090`、`Source/tables/monstdat.h:24-56` |

### 3.3 怪物承载（Special 帧可用性）

| 事实 | 值 | 出处 |
|---|---|---|
| 有 Special 帧的怪物 | 76/112（Fallen 系 13 帧、Skeleton 系 16 帧、Scavenger 系 11 帧） | `assets/txtdata/monsters/monstdat.tsv` frames[6] |
| 熄灯者承载要求 | 必须有 Special 帧（做熄灯前摇）+ 近战攻击帧 | 同上 |

### 3.4 怪物层段分布（B4 修复基础）

| 层段 | 层数 | 现有怪物池（minDunLvl ≤ 段首） | 出处 |
|---|---|---|---|
| 教堂 | 1-4 | 层1 仅 2 种（Zombie/Fallen），层2-4 各 3 种 | `monstdat.tsv` minDunLvl |
| 地下墓穴 | 5-8 | 层5 起 12 种累积 | 同上 |
| 洞穴 | 9-12 | 层9 起 16 种累积 | 同上 |
| 地狱 | 13-16 | 层13 起 22 种累积 | 同上 |

## 4. 方案

### 4.1 行为：熄灯者（LightSnuffer）——黑暗的环境扰动者

**行为**：熄灯者接近其敌人（`Players[monster.enemy]`）至攻击范围后，播放 Special 动画（16 帧，有前摇可打断），动画在 `animFrameNumSpecial - 1` 帧施放效果：**把敌人的光源半径压制到 SnuffRadius（初始 3）**。压制持续 SnuffDuration 秒（初始 6 秒），到期或熄灯者死亡后恢复。

**对玩家的新决策**（定位修正，据用户决策 B）：熄灯者**不制造「必须先杀它」的强迫**——它是环境扰动：「这个怪物会弄暗你，处理它或接受短暂黑暗」。决策是轻量的（击杀有打断窗口优势、绕开接受 6 秒黑暗），定位为 Dark Expedition 的调味与惊喜，而非「优先级目标」。真正的「优先级目标」（巫师复活群落）见第 9 节。

**反制（DP2）**（v3 据整体验证 §8.2-3 修正，全部指名可执行）：
1. **前摇打断**（已代码验证）：Special 动画期间击杀（mode→Death）或打硬直（HitRecovery）→ 效果不施放。16 帧中前 15 帧可打断（约 0.7s）。**这是对全部玩家唯一可执行的通用反制**——熄灯者是近战怪（贴身才起手），贴身的玩家总能攻击它。
2. **Infravision（修正声明）**：Infravision 生效期间压制无效（`CanTarget` 的 `(infra && IsTileVisible)` 分支不依赖光照半径）。**但 v3 必须诚实声明**：Infravision 卷轴被 `DarkExpeditionDropOk`（items.cpp:189-190）+ `WitchItemOk`（items.cpp:2038）**双重排除**，唯一获取通道是 `bookLevel=5` 的法术书（需 36 魔法，施法者专属）——**非施法者没有任何 Infravision 通道**。因此 Infravision 只是「施法者已学法术 / 暗区已激活卷轴」时的情境反制，**不是通用反制**，也不构成「卷轴预算取舍」（预算通道不存在）。
3. **时限**：压制最长 SnuffDuration（6s），不永久。
4. **装备**：压制作用于最终半径（clamp 后），光系装备加成被压制但可缩短恢复需求。

**通用反制结论（v3）**：对非施法者，实际反制 = 前摇打断（通用）+ 时限（6s 可忍）+ 绕开（近战怪可走位）——对「环境趣味」定位（处理它或接受短暂黑暗）足够；Infravision 降级为施法者专属的情境反制，不再是规格宣称的「真正的反制」。

**与既有系统互动（DP4）**：
- **Dark Expedition**：压制修饰符在 `CalcPlrLightRadius` 内、Dark Expedition 倍率之后——地狱层（半径已 6）压制到 3 时黑暗加深。Infravision 的 CanTarget 通道在暗区对**施法者**有效（学法术者），非施法者靠前摇打断/时限（v3 修正——卷轴预算通道不存在）。
- **装备/诅咒**：压制后的恢复读取的是恢复时刻的 `CalcPlrLightRadius` 计算值（含装备/诅咒/Dark Expedition），不会用陈旧值覆盖合法变化（B3.3 修复）。
- **Level 切换/死亡**：`CalcPlrLightRadius` 在层切换/重算时重新计算，压制修饰符 refcount 为 0 时无效果；死亡时 `lightId = NO_LIGHT`（`ChangeLightRadius` 安全 no-op），重生创建新 lightId，无陈旧压制。

**实现**：

```cpp
// 1. Player 增加压制状态（不持久化——瞬态战斗效果，见「存档/读档」段）
struct Player {
    // ...
    int16_t lightSuppression;      // 当前压制半径（0 = 无压制）
    uint8_t lightSuppressionCount; // 叠加熄灯者数（refcount）
    uint32_t lightSuppressionEnd;  // 压制到期 tick（引擎既有游戏 tick 计数器）
};

// 2. CalcPlrLightRadius 内部插入压制修饰符（B3 修复：单一入口）
void CalcPlrLightRadius(Player &player, int lrad)
{
    lrad = lrad * GetDarkExpeditionLightPercent() / 100;
    lrad = std::clamp(lrad, 2, 15);
    if (player.lightSuppression > 0) {              // ← 压制修饰符插入点
        lrad = std::min(lrad, player.lightSuppression);
    }
    if (player._pLightRad != lrad) {
        if (player.isOnActiveLevel()) {
            ChangeLightRadius(player.lightId, lrad);
            ChangeVisionRadius(player.getId(), lrad);
        }
        player._pLightRad = lrad;
    }
}

// 3. 熄灯者 AI（按键 Players[monster.enemy]，B2 修复；v5 修正：记录目标 id）
void LightSnufferAi(Monster &monster)
{
    // monster.enemy 由 UpdateEnemy 维护（引擎标准，各客户端确定性推导）
    Player &enemy = Players[monster.enemy];
    // 接近敌人至攻击范围 → StartSpecialAttack（复用引擎 Special 路径）
    // Special 动画在 animFrameNumSpecial-1 帧回调 OnSnufferTrigger：
    //   monster.var3 = enemy.getId();            // v5：记录目标玩家 id（固定引用）
    //   enemy.lightSuppression = SnuffRadius;
    //   enemy.lightSuppressionCount++;
    //   enemy.lightSuppressionEnd = <游戏 tick 计数> + SnuffDuration;
    //   CalcPlrItemVals(enemy, false);        // R1：触发重算（raw base 只在此处可得）
    // 到期/死亡清理见「到期生命周期」与「死亡清理」段
}

// 4. 到期生命周期（R3 修复：ProcessPlayers 拥有到期 tick，非 AI）
//    player.cpp ProcessPlayers（player.cpp:2957，每 tick 所有玩家所有层）：
//    if (player.lightSuppressionCount > 0 && player.lightSuppressionEnd != 0
//        && <游戏 tick 计数> >= player.lightSuppressionEnd) {
//        player.lightSuppressionCount = 0;    // 单次 decrement：清空而非递减
//        player.lightSuppression = 0;
//        player.lightSuppressionEnd = 0;
//        CalcPlrItemVals(player, false);      // 恢复（raw base 正确重算）
//    }

// 5. 死亡清理（R2 修复：钩 MonsterDeath，覆盖本地+MP 远程击杀；v5 修正：读记录的固定目标）
//    MonsterDeath（monster.cpp:4003）是 M_StartKill（本地，4054）
//    与 M_SyncStartKill（MP 远程，→ StartMonsterDeath 4071 → MonsterDeath 4003）
//    的共同汇聚点。在此处：
//    if (monster.ai == MonsterAIID::LightSnuffer) {
//        const int targetId = monster.var3;   // v5：固定目标（触发时记录），非可变 monster.enemy
//        if (targetId >= 0 && targetId < MAX_PLRS
//            && Players[targetId].lightSuppressionCount > 0) {
//            Players[targetId].lightSuppressionCount--;
//            if (Players[targetId].lightSuppressionCount == 0) {
//                Players[targetId].lightSuppression = 0;
//                Players[targetId].lightSuppressionEnd = 0;   // 清除过期 timer
//                CalcPlrItemVals(Players[targetId], false);
//            }
//        }
//    }
```

**到期生命周期（R3）**：到期检查在 `ProcessPlayers`（`player.cpp:2957`）每 tick 运行，对全部玩家（含 MP 远程玩家副本）生效——不依赖熄灯者 AI 是否存活或是否在同一层。理由（二轮评审）：压制状态活在 Player struct，跨层/跨重生持久；若到期由熄灯者 AI 拥有，玩家离层后 AI 不再 tick（怪物也未死），压制将永久持续到下一层。`ProcessPlayers` 每 tick 无条件运行，消除该泄漏。游戏 tick 计数用引擎既有计数器（实施时确认 `gnTicks` 或等价符号，规格不虚构符号）。

**死亡清理（R2 + v5 修正）**：钩 `MonsterDeath`（`monster.cpp:4003`）而非 `M_StartKill`——`M_StartKill` 只是本地击杀入口，MP 远程击杀走 `M_SyncStartKill` → `StartMonsterDeath`（`monster.cpp:4071`）→ `MonsterDeath`（`monster.cpp:4003`），两个击杀路径在此汇聚。钩 `M_StartKill` 会漏掉「被其他玩家击杀」的熄灯者（refcount 永不 decrement → 永久压制）。**v5 修正（据独立复核）**：死亡清理**必须读触发时记录的固定目标**（`monster.var3`），不能读可变的 `monster.enemy`——`UpdateEnemy` 每 tick 重设 `enemy`，熄灯者死亡瞬间目标可能已切换（目标玩家死亡/离层/换目标），读 `Players[monster.enemy]` 会**清错玩家**（refcount 减到别的玩家头上）且 `enemy = NoEnemy(-1)` 时 `Players[-1]` **越界**。施加侧（AI 触发时 `var3 = enemy.getId()`）与释放侧（死亡时读 `var3`）都以同一固定目标为准 → refcount 施加/释放对称。

**refcount 单次 decrement 防护（R3 + v5 修正）**：死亡路径与到期路径共享同一 count，必须保证每次施加最多被 decrement 一次：
- 到期路径**清空而非递减**（count=0 直接复位，天然防 double-decrement）
- 死亡路径递减，且 count 归零时清除 `lightSuppressionEnd`（避免陈旧 timer 再次触发）
- 两个路径都以 `count > 0` 为前置守卫
- **v5 对称性**：施加侧只对 `var3` 记录的目标递增；释放侧（死亡）只对同一 `var3` 目标递减——目标固定，MP 下各客户端推导一致（`var3` 是 AI 状态变量，随怪物状态确定性同步，见 MP 确定性段）

**存档/读档**：`_pLightRad` 持久化（`loadsave.cpp:488/1360`），但 `lightSuppression/Count/End` **不持久化**（瞬态战斗效果）。存档时若处于压制中，读档后 `_pLightRad` 为已压制值（如 3），但压制状态为 0——读档路径的 `CalcPlrItemVals` 重算会自动修正为未压制值。自愈，无需特殊处理；本段记录该行为以免实施者误持久化压制字段。

### 4.2 数值口径（B4 修复：全层段规格内定义）

| 怪物 | 层段 | minDunLvl | maxDunLvl | 级别 | 承载动画 |
|---|---|---|---|---|---|
| 熄灯者（Wick Thief） | 教堂 3-8 | 3 | 8 | 参照同层段近战怪中位 | Skeleton 系（Special=16 帧） |
| 熄灯者（深层变体） | 地下墓穴-地狱 | 9 | 16 | 参照同层段近战怪中位 | Skeleton 系或 Scavenger 系（Special 11/16 帧） |

**17-20 Nest / 21-24 Crypt**：熄灯者在 17-24 层段**不放置**（显式决策，B4 修复：不留 default 逃逸口）。**理由（据二轮评审 R4 修正）**：压制为 `min(base, 3)`，在任何层段都不会低于 3（clamp 下限 2）——Crypt 5→3、Hell 6→3、Nest 8→3，压制后半径完全相同，「不可行动黑暗」（半径 < 3）在此机制下不可能发生。因此排除理由不是「避免过深黑暗」，而是**范围决策**：Nest/Crypt 是 Hellfire 专属内容，不属于核心 1-16 体验；本机制先服务经典游戏内容，不在 Hellfire 层段引入。若将来需要，作为独立立项评估（含 Crypt 专属反制设计）。

### 4.3 熄灯者攻击类别与射程（B5 修复：先定再推导）

- **攻击类别**：近战（复用 Skeleton 近战攻击路径 `MonsterAttackPlayer`）
- **射程**：贴身（range 1，Special 起手需要与敌人相邻）
- **远近程不对称性推导**：
  - **近战玩家**：贴身战斗 → 熄灯者起手时玩家就在攻击范围内 → **更容易被打断**（近战天然在前摇窗口内）→ 对近战影响**较小**
  - **远程玩家**：风筝时熄灯者追不上（贴身才能起手）→ **根本不会被压制**（除非被逼入死角）→ 对远程影响**更小**
  - **结论**：按此设计，熄灯者对远近程**都不构成显著不公平**（近战有打断窗口优势，远程有距离优势），红线 14 通过。压制半径固定（无职业差异），效果对远近程一视同仁。
  - **注意**：若压制真的施放（近战被贴身压制到半径 3），近战玩家因目标就在身边、光照 3 仍可见贴身目标，实际影响有限——这是「近战打断窗口」之外的第二个对近战友好因素。规格记录该权衡：熄灯者的实际威胁集中在「黑暗层段中多怪围攻时，被压制后无法看清远处威胁」的场景，此场景对远近程公平。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Expansion |
| 2 | 问题陈述指向具体症状 | 第 1 节：AI 复用实测（SkeletonMelee 23 行 / RangedAvoidance 17 / AiRanged 12）有出处 |
| 3 | 引用的数值标注出处 | 第 3 节全部有出处 |
| 4 | 「已实施」需非测试调用者+验收全过 | 本规格为草案，不标注已实施 |

### 扩充层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 新增内容还是膨胀数值？ | 新增内容 | 新增 1 个行为模式（熄灯者），不触碰既有怪物数值 |
| 10 | 每个新增内容有可执行的反制与明确作用？ | 有且已指名 | 前摇打断（已代码验证，通用）/ 时限（6s 可忍）/ 绕开（近战怪可走位）——v3 修正：Infravision 降级为施法者专属情境反制（卷轴双重排除），不再是规格宣称的反制 |
| 11 | 产生取舍还是负担？ | 取舍 | 熄灯者：击杀（利用打断窗口）vs 绕开（接受 6s 黑暗）——环境趣味定位下的轻量取舍；Infravision 卷轴预算取舍 v3 移除（预算通道不存在） |
| 12 | 在全部相关层段有定义？ | 是 | 1-16 定义放置（3-8 / 9-16 两档）；17-24 **显式不放置**（理由见 4.2，无 default 逃逸口） |
| 13 | 与既有系统产生互动？ | 是 | Dark Expedition（压制在其倍率之后、Infravision 的 CanTarget 通道对施法者有效——v3 修正）+ 装备/诅咒/层切换（通过 CalcPlrLightRadius 单一入口） |
| 14 | 近战/远程影响分别评估？ | 已评估 | 4.3：近战有打断窗口优势、远程有距离优势，两者都不构成显著不公平 |

**红线 14 与 v1 的差异**：v1 断言「熄灯者偏害远程」且给出空泛缓解（压制半径固定）；v2 先定攻击类别（近战/贴身）再推导——结果相反（近战反而有打断窗口优势），修正了 v1 的反向评估（Oracle B5）。

## 6. 验收标准（可执行）

| # | 验证项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 熄灯者施放压制 | unit test（新 harness：跑 `LightSnufferAi` tick + Special 动画推进）：Special 完成 → `enemy.lightSuppression` 被设置、`_pLightRad` 被压到 SnuffRadius | 断言通过 |
| 2 | 前摇打断 | unit test：Special 动画期间 kill（mode→Death）→ `lightSuppression` 未设置 | 断言通过 |
| 3 | 受击打断 | unit test：Special 动画期间受击（HitRecovery）→ `lightSuppression` 未设置 | 断言通过 |
| 4 | 时限恢复 | unit test：推进 SnuffDuration ticks（走 ProcessPlayers 到期路径）→ `lightSuppressionCount` 清空、`lightSuppression` 清零、半径恢复 | 断言通过 |
| 5 | 多熄灯者 refcount | unit test：2 个熄灯者叠加，杀掉 1 个后仍压制，杀光后恢复 | 断言通过 |
| 5b | 到期清空防 double-decrement | unit test：单个熄灯者施放→击杀（死亡路径递减）→ 陈旧 timer 到期不再递减（count 不为 0 时不触发） | 断言通过 |
| 6 | Infravision 抵消（施法者） | unit test：压制生效 + Infravision 激活 → `CanTarget` 对远处 tile 仍 true（infra && IsTileVisible 分支）——v3 限定：验证施法者通道（法术书已学）；不声称非施法者可获 Infravision | 断言通过 |
| 7 | 按键正确 | unit test：`LightSnufferAi` 读取 `Players[monster.enemy]` 而非 `MyPlayer`（双玩家测试：monster.enemy=0，玩家 1 有压制、玩家 2 无 → 只玩家 1 被压） | 断言通过 |
| 8 | MP 确定性 | 双玩家模拟：同一怪物 aiSeed 下，两客户端各自跑 `LightSnufferAi`，决策（施放/不施放）一致 | 断言通过 |
| 8b | MP 远程击杀清理 | unit test：走 `MonsterDeath` 路径（模拟 M_SyncStartKill→StartMonsterDeath→MonsterDeath）击杀熄灯者 → refcount 递减、压制解除 | 断言通过 |
| 8c | 死亡钩子固定目标（v5） | unit test：熄灯者对玩家 1 施放后目标切换（enemy→玩家 2 或 NoEnemy=-1）再死亡 → 死亡清理按 `var3` 记录的玩家 1 递减（玩家 2 不受影响、`Players[-1]` 不越界） | 断言通过 |
| 9 | 装备/诅咒组合 | unit test：压制期间装备光系物品（重算）→ 压制仍生效（refcount 未清零）；解除压制后按新装备值恢复 | 断言通过 |
| 9b | 恢复走 raw base | unit test：Hell（DarkExpedition 60%）压制→到期→恢复为 6（非 2）——验证 `CalcPlrItemVals` 重算而非传 `_pLightRad` | 断言通过 |
| 10 | 全量测试 | 全量 ctest | 无新增失败（基线 = PackTest×2 + Writehero×1 既有失败） |
| 11 | 漂移校验 | `tools/check_drift.py` | 5 项 PASS |
| 12 | harness 量化 | eval case `monster-lightsnuffer`（见第 7 节） | 全过 |

**Harness 前提**：AC1-9b 需要能跑怪物 AI tick 的测试 harness——当前 `dark_expedition_*` 测试不跑 `AiProc`（`ai_registry_test` 只查派发表）。本规格新增一个最小 harness（构造 Monster + Player + 推进 `AiProc[LightSnuffer]` tick + 走 `ProcessPlayers` 到期路径 + 走 `MonsterDeath` 死亡路径），作为该规格的 Infra 前置（I3：它让哪个已立项改动变便宜 → 本规格 + 第 9 节的克隆族区分规格）。

## 7. Harness 量化设计（纳入 eval）

| eval case | 断言 | 量化指标 |
|---|---|---|
| `monster-lightsnuffer` | 施放压制（半径 10→3）、打断（击杀/受击不施放）、时限恢复（ProcessPlayers 路径）、Infravision 抵消（施法者限定）、双玩家按键、MP 远程击杀清理、**死亡钩子固定目标（v5：目标切换/NoEnemy 不越界不清错玩家）**、恢复走 raw base | gtest 断言计数（16/16），数值断言（半径变化、refcount 变化） |

量化口径：沿用现有 eval「gtest 二进制 + 断言计数」模式，不引入主观维度（宪章 §10）。

## 8. 状态

草案（v5：R1-R4 二轮 required corrections + v4 Infravision 反制降级 + v5 死亡钩子固定目标/refcount 对称已全部落地）。评审通过后：实施计划 → 实施 → 全量门禁。

## 9. 相关规格（另立，本规格不实施）

### 9.1 激励者（A2 重构后——优先级目标的载体，用户决策 B）

经评估，熄灯者达不到「必须先击杀」的强迫力（威胁是信息剥夺，可忍）。真正的「优先级目标」由激励者规格承担（`2026-08-10-monster-witch-cluster-design.md`，v3）：**激励者（Aura Buffer）位置半径光环**——半径内所有怪获得增益，先杀激励者解除增益。复用 Fallen 鼓舞术的 dMonster 网格扫描（无 getLeader 依赖——普通散群无 leader，v3 重构依据）。

> 历史：原「巫师群落」规格（A2 v1）曾试图做「首领强化 + 威胁放大器」两机制——经首评 REJECT（群落基础误读：普通散群 `leader=NoLeader`，`setLeader` 强制 `minion.ai=leader.ai`，「跨 AI 混合群落」按字面实现不了）后重构为单机制「激励者」。

### 9.2 克隆族区分——密度塌缩的主体

v1 的教训（Oracle Q7）：23 行 SkeletonMelee 克隆簇是密度塌缩的主体。另立规格（未启动）——**前提已修正**（独立复核 C4）：23 行中仅 12 行可生成（11 行 `availability=Never`），实际目标是 8 个教堂骷髅（已由 A1 教堂骷髅区分规格承担）+ 4 个地狱黑骑士。

- **目标**：区分克隆簇（黑骑士 4 行等）——按行为变体差异化，零新美术（复用既有动画帧）
- **原理**：直接攻击复用表本身，把「换皮+数值」变成「换皮+行为」
- **依赖**：本规格的怪物 AI tick harness——共享同一测试基建

**为什么本规格先做熄灯者而非上述两个**：熄灯者是**单一、完整、可独立验收**的行为单元，验证了「新行为 + 光照压制 + 反制 + MP」的完整管线（含 harness）；激励者与克隆族区分是更高风险/更批量化的规格。先跑通一条完整管线，再批量复制。
