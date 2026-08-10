# 怪物行为密度（Monster Behavior Density）设计 v1（已废弃）

> **已废弃（2026-08-10）**：被 `2026-08-10-monster-behavior-density-design.md`（v2）取代。
> Oracle 对抗评审 REJECT：LightShy 的「光照取舍」前提在 D1 不可实现（无开关/无火把/无法主动变暗）；MP 确定性破坏（读 MyPlayer 而非 Players[monster.enemy]）；压制机制与光照不变量冲突（未走 CalcPlrLightRadius 单一入口）；红线 12/14 不满足。v2 砍掉 LightShy，重写为熄灯者-only，修复全部 5 个 blocking 问题。**勿作参考。**

**日期**：2026-08-10
**状态**：已废弃（v1）
**分类**：Expansion（宪章决策 31 定位变更后；触及 monstdat 数值字段 → 判定树规则 1 → Expansion）
**取代**：被 `2026-08-10-monster-behavior-density-design.md`（v2）取代

---

## 1. 问题陈述

**症状**（可复现）：D1 深度越深，战斗决策越趋同。玩家在 16 层内的体验是「同一种近战/远程模式，数值更高」——原版 30 种怪物 AI 的实际可感知行为模式约 10 种（近战冲锋 / 远程站桩 / 远程拉扯 / 徘徊 / 隐身偷袭 / Boss 特殊），其中 ~20 种怪物复用同 4 个 AI 函数（`AiRanged` / `AiRangedAvoidance` / `AiAvoidance` / `SkeletonAi` 系列），行为差异完全来自数值（移速/射程/伤害），不来自行为本身。

**根因**：行为原语太少。D1 的「信息稀缺」张力高度依赖「你不知道这个怪物会怎么对付你」——当所有怪物都是「走近→砍」，黑暗只剩视觉折扣，不是认知威胁。

**2026-08-10 核实出处**：
- AI 派发表 `AiProc[128]`（`Source/monster.cpp:3090-3140`），~20 种怪物复用 4 个函数
- 可感知行为模式约 10 种（见第 2 节分类）
- 光照 API：`ChangeLightRadius(int i, uint8_t radius)`（`Source/lighting.h:67`）、玩家 `lightId`（`Source/player.h:220`）
- 怪物感知玩家：`MyPlayer` 在 AI 函数内可访问（`Source/monster.cpp:906` 等）
- Special 动画帧：76/112 怪物有（`monstdat.tsv` frames 第 6 列 > 0）
- AI ID 槽位：`MonsterAIID` 枚举到 `Custom = 55`（`Source/tables/monstdat.h:54-55`），55–127 有 72 个空槽位

## 2. 分类判定

**第一步——改动是否触及数值/掉落/属性/光照/商店/战斗资源？**

- 新增怪物行为（monstdat.tsv 的 `ai` 列 + 新 `MonsterAIID` 值）：**是**（规则 1「TSV 数据文件数值字段」）→ **Expansion**。无例外条款。
- 新增怪物数值（等级/伤害/血量）：同属规则 1 → Expansion
- 光照交互（熄灯者压制玩家光源）：触及规则 4「光照半径」→ Expansion

**补充判定**：

| 改动项 | 单独判定 | 结果 |
|---|---|---|
| 新增 `MonsterAIID::LightShy` / `LightSnuffer` | 规则 1（TSV ai 列） | Expansion |
| 新 AI 函数（C++，AiProc 表新条目） | 规则 1（影响怪物行为） | Expansion |
| 光照交互（惧光者读取/熄灯者压制） | 规则 4（光照半径） | Expansion |
| 新怪物数据行（等级/掉落） | 规则 1（TSV 数值） | Expansion |

**归类结论**：整体判为 **Expansion**，无条件生效（决策 31 定位：无开关）。

## 3. 事实基础

全部数值于 2026-08-10 核实。

### 3.1 玩家光源结构

| 事实 | 值 | 出处 |
|---|---|---|
| 玩家光源 id | `player.lightId` | `Source/player.h:220` |
| 创建光源 | `AddLight(position, _pLightRad)` | `Source/player.cpp:2517` |
| 改光源半径 | `ChangeLightRadius(lightId, r)` | `Source/lighting.h:67` |
| 玩家基础光照半径 | 10 | `Source/player.cpp:2320` |
| 光源无效标记 | `NO_LIGHT` | `Source/player.cpp:2486` |

### 3.2 怪物感知与状态

| 事实 | 值 | 出处 |
|---|---|---|
| AI 函数内访问玩家 | `MyPlayer` 可用 | `Source/monster.cpp:906` |
| AI 状态变量 | `var1/var2/var3`、`goalVar1-3` | `Source/monster.h` |
| 怪物动画切换 | `NewMonsterAnim(monster, MonsterGraphic::X, dir)` | `Source/monster.cpp:653` |
| Special 动画帧 | 76/112 怪物有（Fallen 系 13、Skeleton 系 16、Scavenger 系 11） | `assets/txtdata/monsters/monstdat.tsv` frames[6] |

### 3.3 AI 注册

| 事实 | 值 | 出处 |
|---|---|---|
| AI 派发表 | `AiProc[128]` 数组 | `Source/monster.cpp:3090` |
| AI ID 枚举 | `MonsterAIID`，到 `Custom = 55` | `Source/tables/monstdat.h:24-56` |
| 空槽位 | 55–127（72 个） | 同上 |

## 4. 方案

### 4.1 行为 1：惧光者（LightShy）——光照是驱散也是暴露

**行为**：怪物感知玩家的当前光照半径。玩家光照范围内 → 怪物退缩避战（MoveEnemy 反向 / AiAvoidance 模式）；玩家光照范围外 → 主动追击。

**对玩家的新决策**：带光源（火把/光系装备）能驱散惧光者，但**光源越大，被它从暗处锁定并引来的范围越大**——「光」从纯保护变成「保护 vs 暴露」的取舍。

**反制（DP2）**：玩家可主动关闭/切换光源（若有开关）或快速冲入其警戒范围打乱其退缩节奏；恐惧怪物退缩时有移动动画可预判。

**与既有系统互动（DP4）**：直接与 Dark Expedition 光照机制互锁——黑暗层段中惧光者成为「光照决策」的活体反馈。

**实现**：
```cpp
void LightShyAi(Monster &monster)
{
    // 每 tick 读取 MyPlayer->_pLightRad + 距离
    // 玩家光照半径内（distance < MyPlayer->_pLightRad 且 IsTileLit(玩家位置)）→ Retreat
    //   （新写退缩逻辑：反向 MoveEnemy 或 RandomWalk 远离光源中心；不复用 AiAvoidance——
    //     后者是「受远程攻击躲避」机制，语义不同）
    // 光照外 → 正常追击（AiMelee 逻辑）
}
```

**数据**：monstdat.tsv 新增行（复用有 Walk 帧的怪物动画，无需 Special 帧），`ai` 列指向新 AI ID。

### 4.2 行为 2：熄灯者（LightSnuffer）——黑暗的主动执行者

**行为**：怪物接近玩家至攻击范围时，播放 Special 动画（有前摇），动画结束时调用 `ChangeLightRadius(MyPlayer->lightId, r)` 把玩家光源半径临时压低（如 10→3）。持续 N 秒后恢复，或怪物被击杀后立即恢复。

**对玩家的新决策**：「优先击杀它，还是绕开等光源恢复」——熄灯者成为黑暗层段中必须优先处理的目标。

**反制（DP2）**：Special 动画有前摇（可打断——在动画期间击杀/击退则熄灯不生效）；熄灯效果有时限；玩家可用火把类装备/Infravision 卷轴在黑暗中维持部分视野。

**与既有系统互动（DP4）**：直接强化 Dark Expedition 黑暗张力——熄灯者是「黑暗」本身变成敌人。

**实现**：
```cpp
void LightSnufferAi(Monster &monster)
{
    // 接近玩家 → NewMonsterAnim(Special, dir)（有前摇，可打断）
    // 动画完成 → ChangeLightRadius(MyPlayer->lightId, SnuffedRadius)
    // var1 = 剩余压制时间；到期或怪物死亡 → ChangeLightRadius(MyPlayer->lightId, 原半径)
}
```

**数据**：monstdat.tsv 新增行（必须选有 Special 帧的怪物：Fallen 系 13 帧 / Skeleton 系 16 帧 / Scavenger 系 11 帧）。

### 4.3 承载怪物选择（数据驱动，零新美术）

| 行为 | 承载怪物建议 | 理由 |
|---|---|---|
| 惧光者 | 新名（如「Night Mote 夜祟」）复用 Scavenger 动画（有 Walk 帧） | 惧光不需要 Special 帧 |
| 熄灯者 | 新名（如「Wick Thief 盗烛者」）复用 Skeleton 系动画（Special=16 帧） | 需要 Special 帧做熄灯前摇 |

具体承载的动画资源复用路径在实施计划中确定，本规格不绑定具体美术（受禁令 2 约束：不写需要不存在美术资源的表格）。

### 4.4 数值口径

两个新怪物的等级/伤害/掉落走 monstdat 标准字段，初始值参照同层段既有怪物的中位值（实施时从 `monstdat.tsv` 同 minDunLvl 区间读取），红线 12 要求全相关层段有定义。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 已判定 | 第 2 节：Expansion |
| 2 | 问题陈述指向具体症状 | 第 1 节：行为模式可感知数量（~10 种）+ 复用证据（AiProc 表） |
| 3 | 引用的数值标注出处 | 第 3 节全部有出处 |
| 4 | 「已实施」需非测试调用者+验收全过 | 本规格为草案，不标注已实施 |

### 扩充层红线

| # | 判定 | 必须 | 依据 |
|---|---|---|---|
| 9 | 新增内容还是膨胀数值？ | 新增内容 | 新增 2 个行为模式（行为多样性），不触碰既有怪物数值 |
| 10 | 每个新增内容有可执行的反制与明确作用？ | 有且已指名 | 惧光者：光源决策/移动预判；熄灯者：Special 前摇可打断/时限/装备反制 |
| 11 | 产生取舍还是负担？ | 取舍 | 惧光者：光=保护 vs 暴露；熄灯者：优先击杀 vs 绕开 |
| 12 | 在全部相关层段有定义？ | 是 | 两个新怪物在 1–16 的 minDunLvl 区间各层段均有放置（具体值实施时定） |
| 13 | 与既有系统产生互动？ | 是 | 与 Dark Expedition 光照机制互锁（DP4 内生） |
| 14 | 近战/远程影响分别评估？ | 已评估 | 见下 |

**红线 14 评估**：
- **惧光者**：近战玩家需贴身（光照内=怪物退缩，近战被拉扯）；远程玩家在光照外更安全但被锁定——**对近战更不利**，需在数值（退缩距离/追击速度）上调平。
- **熄灯者**：压制玩家光源对远程玩家（依赖视野风筝）影响大于近战——**对远程更不利**。缓解：熄灯效果对远近程一视同仁（压制半径固定），且 Special 前摇对远程可更早打断。实施时以数值调平，本规格记录该风险。

## 6. 验收标准（可执行）

| # | 验证项 | 方法 | 通过标准 |
|---|---|---|---|
| 1 | 惧光者退缩行为 | unit test：玩家带光接近 → 怪物退缩（monster.goal 变化）；玩家光照外 → 追击 | 行为断言通过 |
| 2 | 熄灯者压制光源 | unit test：Special 动画完成 → `MyPlayer->lightId` 半径被压低；击杀后恢复 | 半径断言通过 |
| 3 | 熄灯前摇可打断 | unit test：Special 动画期间击杀 → 熄灯不生效 | 半径不变 |
| 4 | 熄灯时限恢复 | unit test：N 秒后半径自动恢复 | 半径恢复 |
| 5 | timedemo 回放一致 | `Timedemo.WarriorLevel1to2` | 存档比对一致（新怪物不进入 timedemo 路径或 RNG 确定性保持） |
| 6 | 全量测试 | 全量 ctest | 无新增失败（基线 = PackTest×2 + Writehero×1 既有失败） |
| 7 | 漂移校验 | `tools/check_drift.py` | 5 项 PASS |
| 8 | harness 量化 | 新增 2 个 eval case（见第 7 节） | 全过 |

## 7. Harness 量化设计（纳入 eval）

每个新行为 1 个 eval case，量化「行为触发」而非主观感受：

| eval case | 断言 | 量化指标 |
|---|---|---|
| `monster-lightshy` | 惧光者：光照内退缩、光照外追击 | 退缩/追击状态切换的 gtest 断言（布尔 → 15/15 计数） |
| `monster-lightsnuffer` | 熄灯者：压制半径→恢复、前摇打断 | 半径变化的 gtest 断言（数值断言 → 15/15 计数） |

量化口径：沿用现有 eval 的「gtest 二进制 + 断言计数」模式（与 dark-expedition case 一致），不引入主观维度（宪章 §10「不测试的维度」）。

## 8. 状态

草案（未评审）。按宪章流程：Oracle 对抗评审 → 实施计划 → 实施 → 全量门禁。
