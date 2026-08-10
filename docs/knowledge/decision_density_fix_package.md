---
name: 密度塌缩修复框架（Oracle 对抗评审结论与最小连贯包）
description: 怪物行为密度塌缩的 5 层修复设想经 Oracle 对抗评审收敛为最小连贯包：B1 采样约束（第一优先）+ A1/A3 克隆区分 + E1 地狱重组。含三个类别错误纠正（C1 双重计数、D 层非塌缩对策、B1 被低估）与「池≠体验」关键洞察。
type: decision
created: 2026-08-10
sources:
  - docs/superpowers/specs/2026-07-27-better-d1-design-charter.md（决策 31）
  - docs/knowledge/analysis_monster_config_landscape.md（扫描）
  - Source/monster.cpp（GetLevelMTypes 采样）
  - Source/items.cpp（RndUItem 怪物等级门控掉落）
---

> **后续演进注记（2026-08-10 定稿时补充）**：本文档记录的是框架评审阶段的初始结论。经两轮 Oracle 独立复核 + 整体验证（9 项引擎事实核实）后，部分项已演进，**以框架总览 `docs/superpowers/specs/2026-08-10-density-fix-framework-overview.md` 为准**：
> - **E1 地狱重组 → 已 REJECT 并重构为「地狱悬崖评估」**（Oracle 证明降级是错误杠杆——GetMinHit 钳制，怪物等级对战斗压力几乎无贡献；不降数值，可定稿）
> - **A2 巫师群落 → 已 REJECT 并重构为「激励者」**（普通散群无 leader，getLeader 依赖按字面实现不了；改为位置半径光环）
> - **全部 6 份规格已定稿**：A1 v3（冲锋改即时）、A3 v3（即时冲锋+冷却）、B1 v3（Counselor 映射修正，L15 尾 27.8%）、C2 v4（Infravision 反制降级）、A2 v3（激励者）、E1 可定稿
> - **根因教训**：规格引用引擎机制必须核实代码（虚构冲锋前摇/误读群落系统/虚构 Infravision 通道/误分类 Counselor 均为未核实导致）

# 密度塌缩修复框架：Oracle 对抗评审 + 独立复核结论

## 2026-08-10 独立复核补充（APPROVE-WITH-CHANGES，6 项修正）

### 重大发现：4000 图片预算才是每层多样性的真正瓶颈

`GetLevelMTypes` 的实际约束是 `monstimgtot < 4000`（图片预算），**不是** MaxLvlMTypes=24。地狱怪物图片 980-2220，每层物理上只放得下 2-4 种：

| 层段 | 每层实际采样类型数 |
|---|---|
| 教堂 1-4 | 6-8 种 |
| 墓穴 5-8 | 3-8 种 |
| 洞穴 9-12 | 2-4 种 |
| 地狱 13-16 | 2-4 种 |

**「保证地狱行为类覆盖」在现预算下不可能**（最多 2-4 类/层）。B1 的「覆盖保证」不可实现——需提高图片预算（平台内存风险）或**显式接受 2-4 种/层为身份约束**。

### 6 项修正（独立复核）

1. **C1**：B1 地狱腿不可行 → 重spec为**反垄断**（洞穴风筝≤1-2 种/层可行、地狱同类≤2 种/层可行）
2. **C2**：**E1 是三重打击**——`SpawnItem`（items.cpp:3456）用 `monster.level` 门控掉落基础、`monster.data().level` 门控词缀/unique 等级、`AddPlrMonstExper`（monster.cpp:4006）门控 XP。降 21-30→18-24 会使 25-27 级 unique（Stormshield/Demonspike/The Grandfather 等）从地狱怪掉落中消失 → 需掉落补偿或缩小降幅
3. **C3**：「数值悬崖」前提未验证——XP 曲线显示玩家到地狱约 18-24 级非 13-19，悬崖没那么大 → E1 必须先验证玩家等级模型
4. **C4**：**A1 前提膨胀**——23 行 SkeletonMelee 中仅 12 行会生成（11 行 `availability=Never`），目标改为 8 教堂骷髅 + 4 地狱黑骑士
5. **C5**：**A1 三变体与数据冲突**——door-open 死路（可生成行无 CAN_OPEN_DOOR）；charge 在黑骑士上不可能（无 special 帧）→ door-open 砍掉，charge 只用于 8 教堂骷髅（有 16 帧 special）
6. **C6**：B1 教堂「压力档位」无法制造压力（教堂池零风筝怪）→ 教堂加压的杠杆是 A1 不是 B1

### 推荐新优先序

**A1（重spec：8 教堂骷髅）+ A3（洞穴风筝→带 tell 组合）先行 → B1（反垄断）→ E1（重新推导）→ A2**（交换了原包 #1/#2）

### 未覆盖风险

1. 难度曲线拉平未审视（A1 抬地板 + E1 降天花板 + B1 砍风筝 = 扁平化）
2. **Never 怪物数据是陷阱**：24/112 行永不生成，扫描自己也踩了——密度工作必须过滤 availability

---

## 关键洞察：池 ≠ 体验

`GetLevelMTypes`（`Source/monster.cpp:3439`）是**随机采样**：每层从累积可用池（minDunLvl ≤ currlevel ≤ maxDunLvl）随机抽最多 24 种（`MaxLvlMTypes=24`，**实际约束是 4000 图片预算**），16 层硬编码为 Advocate/RBlack/Diablo。

- 扫描诊断的是**池构成**（37/41/36/23），玩家体验的是**采样结果**
- **即使修好池，随机采样仍可能抽出一层全是风筝怪**
- **B1（采样约束）是唯一能在「逐层体验」层面保证行为混合的层**——单函数改动，但地狱腿受图片预算限制

## 最小连贯包（修复诊断的核心）

| 序 | 项 | 内容 | Effort | 风险 |
|---|---|---|---|---|
| 1 | **B1 采样约束** | `GetLevelMTypes` 加逐层行为构成约束：洞穴风筝上限、地狱行为类覆盖保证、教堂压力档位 | Short | timedemo RNG 需重验 |
| 2 | **A1+A3 克隆区分** | SkeletonMelee 拆 3 变体（冲锋/防御/开关门，**每变体带可读 tell + 指名反制 + 逐变体红线 14**）；洞穴 14 只风筝中 2-3 只转带 tell 组合行为（修 38% 垄断，只改 ai 列） | Short-Medium | tell 缺失→噪声；红线 14 需逐变体 |
| 3 | **E1 地狱重组** | 怪级 21-30 → ~18-24（**保留部分悬崖**硬约束），配 1-2 个诡计行为 | Medium | 悬崖不可清零；掉落受影响 |

## 三个类别错误纠正

1. **C1 优先级目标 = A2 的效果**——双重计数，不是独立层
2. **D 层（成长）不是塌缩对策**——是宪章 DP1 内容支柱；但 D2 法术扩容是 E2 的合法形态（玩家力量匹配）
3. **B1 被低估**——池≠体验，采样约束才是第一优先

## 明确排除

- **B2 层专属事件**：重复建设（Butcher/SkeletonKing/16 层硬编码已存在）；保证出现=确定性新颖性衰减
- **C3 掉落-行为反馈**：禁令 4 违规（新概念）+ DP4 平行系统
- **E2 成长曲线调整**：反向数值膨胀，爆炸半径最大；替换为 D2

## 已过审内容层的排期

- **A2 首领强化/威胁放大器**：排在诊断修复（B1+A1/A3+E1）之后
- **C2 熄灯者**：**需重spec**——反制通道过时：Infravision 卷轴已被 `DarkExpeditionDropOk`（items.cpp:189-190）+ `WitchItemOk`（items.cpp:2038）双重排除，唯一通道是 `bookLevel=5` 法术书（36 魔法，施法者专属），非施法者零通道

## 交叉设计规则（所有新行为必须遵守）

1. **可读 tell**：每个新行为变体必须有游戏内可读的视觉提示（冲锋前摇/防御姿态），否则退化为「打一千次记住」的背诵型未知（禁令 4 + B1 支柱）
2. **保留部分悬崖**：D1 难度部分就是数值墙，清零 = 学会后平趟
3. **MP 确定性**：新 AI 按 `aiSeed` 确定性推导
4. **timedemo/漂移硬门禁**：B1 触碰层生成 RNG、E1 触碰 monstdat

## 关联事实

- **怪物等级门控掉落**：`RndUItem(monster)` 用 `itemMaxLevel = monster->level(nDifficulty)` 过滤 `item.iMinMLvl`（items.cpp:1392-1402）——E1 降怪级会限制高 iMinMLvl 物品掉落，需在规格中评估
- 23 行 SkeletonMelee 已有三类生理结构（骨架 8 帧 walk/12-13 attack、虫豸 13 帧 walk/13 attack、骑士 8 帧 walk/16 attack）——A1 分化有真实基础
