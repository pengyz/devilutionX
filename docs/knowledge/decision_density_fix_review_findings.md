---
name: 密度修复框架独立复核结果（4 agent，发现系统性引擎事实错误）
description: 4 个独立 Oracle agent 对密度修复框架全部规格的最终复核。发现先前所有评审（含整体验证 §8）遗漏的系统性引擎事实错误：冲锋伤害路径断裂（special 列=0 → 1 伤害）、B1 27.8% 尾夸大 7×（Golem 预算遗漏）、E1 GetMinHit 证明错误（漏 base toHit）、狂乱者三重造假。框架是「reviewed to death」——文档自洽但数字未过忠实模拟。
type: decision
created: 2026-08-10
sources:
  - docs/superpowers/specs/2026-08-10-density-fix-framework-overview.md
  - docs/superpowers/specs/2026-08-10-cathedral-skeleton-differentiation-design.md（A1 v3）
  - docs/superpowers/specs/2026-08-10-cave-kite-combination-design.md（A3 v3）
  - docs/superpowers/specs/2026-08-10-sampling-anti-monopoly-design.md（B1 v3）
  - docs/superpowers/specs/2026-08-10-monster-behavior-density-design.md（C2 v4）
  - docs/superpowers/specs/2026-08-10-monster-witch-cluster-design.md（A2 v3）
  - docs/superpowers/specs/2026-08-10-hell-recombination-design.md（E1）
---

# 密度修复框架独立复核结果（2026-08-10 第二轮多 agent 复核）

## 复核设置

4 个独立 Oracle agent 并行复核全部规格 + 框架总览，每个都**忠实重实现引擎逻辑**（不信任规格自述），发现先前所有评审（含整体验证 §8「9 项引擎事实核实」）遗漏的错误。

## 复核裁决

| 规格 | 裁决 | 关键发现 |
|---|---|---|
| A1 教堂骷髅 | NEEDS-FIX | 冲锋伤害路径断裂（special 列=0 → ~1 伤害）；狂乱者 no-op + 攻速/移速/无视硬直声称造假 + tell 不可见 |
| A3 洞穴风筝 | NEEDS-FIX | 同一伤害路径缺陷（Hell Stone Magma special 0-0 → 墙角惩罚=1 伤害推搡）；「38% 垄断被打破」夸大 |
| B1 采样反垄断 | NEEDS-FIX | **27.8% L15 尾被夸大 ~7×**（忠实模拟 ~4.1%）；洞穴 cap 定义漏洞 |
| C2 熄灯者 | NEEDS-FIX | 死亡钩子读可变 `monster.enemy`（MP 清错玩家 + 可越界）；refcount 不对称；Infravision 反制对引擎为假 |
| A2 激励者 | NEEDS-FIX | 速度机制违反「零新机制」+ 代码引用错误；增益幅度未承诺 |
| 框架总览 | NEEDS-WORK | **E1 GetMinHit 证明错误 + B1 27.8% 夸大 7×**——「9 项验证」表自身含 2 处错误 |

## 最严重的系统性错误（先前评审全部遗漏）

### 1. 冲锋伤害路径断裂（A1+A3 共享）
`MissToMonst → MonsterAttackPlayer` 用 `minDamageSpecial/maxDamageSpecial`——8 个骷髅 + Magma 系承载的 special 列全是 0 → **冲锋造成 ~1 伤害**，反制失去意义（DP2/红线 10 违规）。
整体验证 §8.2#1 核实了「冲锋即时」但**从未核实伤害路径**——这是遗漏的根源。

### 2. B1 27.8% 是假数字
忠实重实现 `GetLevelMTypes`（含 `AddMonsterType(MT_GOLEM)` 预加消耗 386 图片预算）后，L15 同类尾实际 **~4.1%**（L13 0%、L14 0.7%）。27.8% 只在**省略 Golem 预算**时复现。规格「主战场」声明夸大 7×。

### 3. E1 GetMinHit 证明错误
公式 `hit = 2×(mlvl−plvl) + 30 − AC` **漏 base toHit**（地狱 13-16 怪 toHit 列 90-130——Counselor 90 / Magistrate 100 / Balrog 130，`monster.cpp:1274` `MonsterAttackEnemy(monster, monster.toHit(...), ...)`）。真实公式 `baseToHit + 2×diff + 30 − AC` 被 GetMinHit **下限**（抬高中招率）非消除等级项。Blood Knight 130+2×10+30−90 ≈ 100+ → 常驻命中，下限从不生效。等级项系数 2（±2%/级）——微小但非零（非 v1 声称的「完全无效」）。结论（别降级）靠「HP/伤害是固定列 + 等级项系数小」仍成立，但旗舰证明是假的（§8.2-9 打了 ✅，v5 已修正）。

### 4. 狂乱者三重造假（A1）
- `goal=Attack` 是纯 AI 攻击性（`FallenAi` 2364-2368：近距 StartAttack / 远距 RandomWalk），**非攻速/移速/无视硬直**——代码路径无任何速度或硬直免疫
- `SkeletonAi`（2124-2143）**不读 goal** → 规格伪代码 `goal=Attack; SkeletonAi()` 是 **no-op**
- 低血 tell 不可见（HP 条默认关，`options.cpp:857`）→ B1/禁令 4 违规

### 5. C2 死亡钩子 MP 缺陷
- `monster.enemy` 被 `M_StartHit`（3992）在硬击时重定向 → MP 下「杀熄灯者恢复光源」可能清错玩家
- `monster.enemy` 可指向**怪物索引**（Golem 相邻时 `MFLAG_TARGETS_MONSTER`）→ `Players[monster.enemy]` 越界
- refcount 单位不对称：施放次数递增 vs 死亡递减（施放 2 次 → 击杀后仍压制）

### 6. A2 速度机制违规
- `monster.h:170 rate` 是 `AnimStruct`（每动画数据）字段，**非 `Monster::rate`**——代码引用错误
- 无逐怪物 rate 覆盖机制 → 速度加成是**新原语**，违反框架不变量「零新机制」（A1/A3 已修正为零新机制，A2 未修）

## 核心结论（框架复核 Q7）

> **框架是「reviewed to death」**——文档互相交叉评审（自洽），但 5 次修订后仍带出错的数字。**首次忠实模拟验证（B1 采样）正是它崩的地方**。跨文档自洽 ≠ 正确；只有忠实引擎模拟能抓到这类错误。建议先实现 B1 采样模拟 harness（纯测试，不改游戏代码）重新推导真实尾部数字——最便宜地验证或否定框架旗舰声明。

## 已验证正确的引擎事实（复核确认）

- 冲锋即时（RhinoAi 2284-2292 / SnakeAi 2675-2683 同 tick `AddMissile(Rhino)+mode=Charge`；`isPossibleToHit()==false` 于 Charge 4974）
- 散群无 leader（`PlaceGroup` 默认 leader=nullptr；`setLeader` 仅 leashed 且强制 `ai=newLeader->ai`）
- Counselor 是远程施法者（`CounselorAi` 2724 `StartRangedAttack` Firebolt/ChargedBolt/LightningControl/Fireball）
- Infravision 卷轴双重排除（`DarkExpeditionDropOk` 189 + `WitchItemOk` 2038）
- 洞穴池 36 / 风筝 14 = 39%（含 3 Never；可生成 11/31 = 35%）
- 冲锋弹道 velocity 18 ≈ 2.2× 玩家步速（A3 的 ≥3 门控下 ~11 tick 飞行不可反应躲避，只有预走位）

## 修订方向（待用户决策）

1. **A1/A3 冲锋伤害**：重新设计伤害投递（普通伤害列 / special 列 / 新路径）——设计决策
2. **B1 27.8% → ~4%**：cap 价值大幅缩水，是否仍独立规格？
3. **E1 GetMinHit 证明**：修正事实基础（结论「别降级」保留）
4. **A2 速度机制**：改 aggression buff（零新机制）还是保持新机制（修不变量）？
