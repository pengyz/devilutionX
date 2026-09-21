# 逐层交战形态（"压力形态"）设计规格

**状态**：草案 v2（2026-09-18，**待作者人工审计**；v1 因"看不懂"重写 ✓）
**一句话**：**通过改变"怪怎么摆"（成群/摆在哪/什么组合），让越往深处走，越必须在"继续冒险"和"撤回准备"之间做选择——而不改任何数值。**
**审计重点**：§4.2（形态是否符合你的直觉）与 §8（需要你决定的问题清单）。

---

## 0. 一分钟速览

| 问题 | 答案 |
|---|---|
| **这是什么** | Diablo 1 的一个改动方案：改变地牢里**怪物的摆放方式**（是否成群、摆在通路还是角落、近战远程怎么搭配） |
| **为什么要改** | 因为**后期地牢不再让人紧张**：你不缺药水、回城几乎不要钱，下去只剩下"捡东西回来卖" |
| **怎么改** | 按四个深度段各规定一种"交战形态"，让深处的战斗**必须多花资源、或必须放弃某条路** |
| **不改什么** | 不改怪物血量/伤害 ✗、不改掉落 ✗、不改物价 ✗、不给玩家加新概念 ✗ |
| **怎么算成功** | ①自动化测试能验证"形态按定义落地"②试玩能验证"玩家确实更需要准备/更需要撤回，且**不觉得在跑腿**" |
| **怎么算失败** | 玩起来只是"怪变多了、更烦"⇒ 撤掉（§6.3 写死了撤销条件） |

---

## 1. 背景：这份文档从哪来

### 1.1 我们在做什么

**Better D1** 是 Diablo 1（下称 D1）的引擎增强版：给老玩家补上现代舒适度，同时**找回 D1 原本的那种紧张感**。D1 的核心体验是"**往下走**"：越深越危险、准备有限、下去是要下决心的事。

### 1.2 我们观察到的现象（问题的起点）

玩家的原话：**"后期补给都不需要了，只是卖装备。"**

也就是说：到了后期，下地牢**不再是为了"能不能活着回来"**，而是为了"捡东西、回城、卖掉"——**准备和恐惧都消失了**（而这两样正是 D1 的味道）。

### 1.3 为什么会这样（四条机理，都用大白话）

| # | 机理 | 说明 |
|---|---|---|
| 1 | **你变强得比地牢变难更快** | 怪物的强度只在"难度档位"（普通/噩梦/地狱）里翻倍，**不随深度变化**；而你的血量、伤害、装备一直在涨 |
| 2 | **你带得下"够用"的补给** | 背包 40 格、腰带 8 格是固定的，但后期一场战斗消耗很少 ⇒ 带一次够用很久 |
| 3 | **回城几乎不要钱** | 压力本来可以靠"多跑一趟"化解，而回城卷轴很便宜 ⇒ 时间成了万能解 |
| 4 | **死了还能读档撤销** | 这是**另一条线**的工作（"存档契约"），本方案**不依赖**它 |

**结论**：D1 的资源循环在结构上是**自洽**的，但在**强度**上会随进度**衰减** ✗ —— 这是本方案要修的那一条。

### 1.4 这件事在整体计划里的位置

我们把"资源循环"拆成四个失败点，各配一条腿（**本方案是 A 腿**）：

| 腿 | 修的失败点 | 本方案是否覆盖 |
|---|---|---|
| **A 需求端** | 你变强 ⇒ 每次交战消耗变少 | ✅ **就是本方案** |
| B 供给端 | 金币后期变成废堆 | ✗ 另一份（商店"购买阶梯"） |
| C 绕行通道 | 压力被"多跑一趟"吸收 | ✗ 另一份（补给成本） |
| D 赌注 | 死亡可以被读档撤销 | ✗ 另一份（存档契约） |

---

## 2. 术语表（读这份文档只需要这些词）

| 词 | 含义 |
|---|---|
| **压力** | 游戏对你的资源**持续索取**的程度：要你花药、要你退、要你放弃某条路。压力**不是**"怪的血量" |
| **需求端 / 供给端** | **需求端**＝游戏要你付出多少（本方案改这里 ✓）；**供给端**＝你手里有多少（本方案**不**改 ✗） |
| **交战形态** | 一层里"**怪怎么摆**"：成群还是零散、摆在通路还是角落、近战远程怎么搭配、有没有领头的 |
| **小队 / 拴系** | 一只"领头怪"带几只随从（D1 已有机制）；玩家把领头引开 4 格以上，随从会**脱队**——这是玩家已有的反制手段 |
| **层段（band）** | 把 24 层分成四段：**教堂 L1-8 / 洞穴 L9-12 / 地狱 L13-16 / Nest-Crypt L17-24** |
| **不可规避** | 玩家**没法**靠"先把药水丢掉、多带一点、绕一下"来躲开这项消耗 |
| **可选择的支出 vs 必须的维护** | **可选择的支出**＝"我要不要花这笔钱/经验换那个东西"（真取舍 ✓）；**必须的维护**＝"东西坏了，必须去修"（跑腿 ✗，我们**不做**这种） |
| **守卫** | 该项目里的**自动化回归测试**：防止既有行为被改坏。本方案要求"既有守卫全量重跑"＝一条都不许退化 |
| **可失败判据** | 一条测试：如果有人把参数改坏，它**必须变红**。做不到这一点的测试等于没有 |
| **预注册试玩** | **先写死**怎么问、问多少人、什么结果算通过，**然后**才去玩；不许玩完再改标准 |

---

## 3. 问题的定义

### 3.1 "压力"在这里的定义

> **一次下潜的压力 ＝ 地牢要你付出的量 ÷ 你带下去的量**

分子（需求）＝这一层的怪有多"费"（要打几场、每场多久、能不能绕）；
分母（存量）＝你带的药水、你的装备强度。

### 3.2 现状为什么压力会衰减

见 §1.3 的四条。用一句话总结：**分子不随深度增长（怪只在难度档位变强），而分母一直在涨（你越来越强、越来越能带）。**

### 3.3 我们**不**采用的解法（明确否定，避免走偏）

| 不采用 | 原因 |
|---|---|
| 加怪物血量/伤害 ✗ | 那是"数值膨胀"，会把 D1 变成另一种游戏（D2 式的数字竞赛） |
| 加装备耐久度 ✗ | 它是"**必须的维护**"（跑腿+付费），不是取舍；可以靠带备用件躲开 |
| 加背包/腰带的容量压力 ✗ | 已有论证：**那是税，不是决策**（玩家会用"多跑一趟"化解） |
| 让玩家"没有补给就不能玩" ✗ | 那是惩罚不是设计 |

---

## 4. 方案

### 4.1 一句话

> **不改数值，只改"怪怎么摆"**：按层段规定四种交战形态，让深处的战斗**更成体系、更费资源、更需要选择**。

### 4.2 四种形态（按层段）

| 层段 | 形态名 | 你会遇到什么 | **你要决定什么** | **你怎么反制** |
|---|---|---|---|---|
| **教堂 L1-8** | **可逃的围压** | 1–2 支小队，摆在通路附近但**总能绕开** | 打掉换经验，还是绕开省药 | 把领头引开 >4 格（随从脱队）/ 先杀领头 |
| **洞穴 L9-12** | **狭窄压制** | 2–3 支小队，**远程怪卡在走廊/门口** | 用掩体慢慢推，还是硬吃伤害冲过去 | 卡门位 / 贴身打远程 / 拉出射程 |
| **地狱 L13-16** | **消耗战** | 3–4 支小队，**近战与远程交替**，逼你连续打 | **手里的药还剩多少 ⇒ 继续下潜还是回城** | 分段清场 / 用回城卷轴认输离场 |
| **Nest-Crypt L17-24** | **精英抉择** | 2–3 支小队（含精英），守在与**通路或奖励**绑定的位置 | 抢这条路/这件奖励，还是绕开保命 | 绕路 / 打时间差 / **放弃这个房间** |

**每个形态都必须能用一句大白话解释**（这是设计约束）：
> 例（地狱段）：「**如果你的药只剩一半，你还继续推进，那么下一场连续交战就会逼你使用回城卷轴。**」

### 4.3 我们具体改什么 / 明确不改什么

| | 内容 |
|---|---|
| **改** | ①**小队数量**（每层几支）②**构成权重**（近战/远程/施法者的比例）③**放置策略**（摆在通路、走廊、奖励旁） |
| **不改** ✗ | 怪物血量/伤害/护甲、掉落表、物价、物品表、玩家属性、深层房间结构（那是 D 腿） |

### 4.4 玩家会看到什么（前后对照）

| 层段 | 现在 | 之后 |
|---|---|---|
| L13-16 | 零散怪，打完就走 | **成队出现、近远交替** ⇒ 你会开始算"药还够不够" |
| L9-12 | 远程怪散在房间里 | **远程怪卡在必经的走廊口** ⇒ 你会开始用掩体/贴身 |
| L17-24 | 精英单独出现 | 精英带小队守在奖励/通路旁 ⇒ 你会开始决定"这个房间值不值得进" |

---

## 5. 为什么这样做是对的（判据速览）

| 判据（来自本项目设计宪章） | 本方案 |
|---|---|
| 不膨胀数值 | ✅ 明确不改任何数值（§4.3） |
| 必须产生**取舍** | ✅ 每种形态都写明"选这个还是那个"（§4.2） |
| 必须有**指名反制** | ✅ 每种形态都写明玩家怎么应对（§4.2 最后一列） |
| 覆盖**全部层段** | ✅ L1-24 四段全覆盖 |
| 近战/远程**分别评估** | ✅ 验收要求逐层×职业的构成表（§6.1） |
| 不新增玩家必记概念 | ✅ 只用了 D1 已有的：小队/引怪/掩体/回城卷轴/药水 |
| 不用开关（默认生效） | ✅ 单一契约，不提供"模式选择" |

---

## 6. 怎么验证

### 6.1 自动化测试（能变红才算数）

| # | 测试 | 改坏了会怎样 |
|---|---|---|
| 1 | 每层段的小队数/构成权重**在参数表区间内** | 参数被改到区间外 ⇒ **红** |
| 2 | **逐层 × 职业**的近战/远程/施法者构成表 | 构成被改坏 ⇒ **红** |
| 3 | **既有守卫全量重跑**（密度/占比/组合数/核心规模/唯一可达） | 任一退化 ⇒ **红** |
| 4 | **"没动数值"的不变量断言**：怪物数值表与掉落表逐字节未变 | 有人偷偷改数值/掉落 ⇒ **红** |
| 5 | **L1-8 必须可绕**（教学段不能被堵死） | 摆到不可绕的位置 ⇒ **红** |

### 6.2 试玩（判"体验"，不进自动化门禁）

| 项 | 内容 |
|---|---|
| 问法（事先写死） | 「**哪一版更需要你下潜前做准备 / 更需要你中途撤回？**」 |
| 反繁琐项 | 「**哪一版更让你觉得在跑腿？**」——如果答案偏向新版本 ⇒ **撤销** |
| 方式 | A/B 盲测、轮流顺序（按已冻结的试玩协议） |

### 6.3 什么时候撤掉它（写死）

1. 深处打起来**消耗没变**（形态没被感知到）；
2. 玩家**完全靠跑/绕**摊平（形态只是障碍，不是压力）；
3. 玩家反馈"**怪更多了、更烦**"（数量 ≠ 压力）；
4. **职业失衡**：近战或远程任一方出现"没法应对"的层。

### 6.4 必须先取的基线（否则无从判断"有没有变"）

1. **各层段现在的**小队数/构成/密度（自动化可得，尚未取 ⚠）；
2. **各层段现在的**补给消耗/回城次数（只能靠试玩取，尚未取 ⚠，且必须**事先**定好口径，不许事后调参 ✗）。

---

## 7. 相关文档（给需要深入的人）

| 文档 | 一句话 |
|---|---|
| `2026-07-27-better-d1-design-charter.md` | 裁决基准：什么改动算哪一类、红线是什么 |
| `2026-09-18-depth-first-principles-design.md` | 上位规格：为什么资源循环会衰减、四条腿的分工（本方案＝A 腿） |
| `2026-09-16-content-density-contract-design.md` | 守卫机器的来源（密度/占比/组合数等自动化测试） |
| `2026-09-18-depth-save-contract.md` | D 腿（存档契约）的计划，与本方案**相互独立** |

---

## 8. 需要你决定 / 我还没弄清的事

### 8.1 请你审计的五个问题

1. **§4.2 的"你要决定什么"是否真的是决策**——尤其地狱段的"何时回城"：回城目前很便宜，所以这条决策**可能现在还不成立** ⚠。若你同意，它应当在 C 腿（补给成本）之后再验收，而不是现在就算达标 ✓；
2. **§4.2 的形态是否符合你的直觉**（例如教堂段是否该有 2 支小队；Nest 段的"精英"上限）；
3. **§6.2 的问法**能不能问出"更需要准备/撤回"，而不把答案引向"更跑腿"；
4. **§6.1 第 4 条**（"没动数值"的不变量断言）是否足够约束我**不越界**；
5. **§9 的四项未核验**里，哪些你要求**先查清再定稿**。

### 8.2 四项未核验（不得当事实使用 ⚠）

1. 各层段**现状基线**（小队/构成/密度）尚未取；
2. **补给消耗与回城次数**基线需试玩，尚未取；
3. D1 现有的主题房放怪频率参数（`monstrnd[...]`）与我们新增参数**如何共存**，未读清；
4. L17-24 的"**精英**"定义口径（依赖名册角色/unique 基座）未核验。

---

## 9. 附录 A：参数草案（待审计 ⚠）

| 层段 | 小队数/层 | 拴系比例 | 远程/施法者权重 | 放置策略 | 精英上限 |
|---|---|---|---|---|---|
| L1-8 | 1–2 | 100% | 低 | 通路可绕 | 0 |
| L9-12 | 2–3 | 100% | 中-高 | 走廊卡位 | 0 |
| L13-16 | 3–4 | 80% | 近远交替 | 分段消耗 | 1 |
| L17-24 | 2–3 | 80% | 高 | 与奖励绑定 | 2 |

## 10. 附录 B：证据锚点（给技术审计用）

| 事实 | 代码/数据位置 |
|---|---|
| 逐层名册与参数 | `assets/txtdata/monsters/level_rosters.tsv`、`level_roster_params.tsv` + `Source/tables/level_roster.{h,cpp}` |
| 小队放置 | `Source/monster.cpp:3054 PlaceGroup`；拴系用法见 `:3528`（`MonsterPack::Leashed`） |
| 放怪时机 | `Source/levels/themes.cpp:349 PlaceThemeMonsts`；`:913 CreateThemeRooms`（**本方案不动**其跳过的 L16/Nest/Crypt） |
| 背包/腰带上限 | `Source/player.h:38-39`：`InventoryGridCells = 40`、`MaxBeltItems = 8` |
| 无自然回复 | `Source/player.cpp` 中 HP/MP 自增均为 Mana Shield/吸血类效果 |
| 商店仅城内 | `Source/stores.cpp:2471/2658/2776` 均有 `leveltype == DTYPE_TOWN` |
| 既有守卫 | `test/sampling_behavior_test.cpp`、`test/level_roster_baseline_test.cpp` |
| 死亡/存档现状（D 腿） | 单机死亡不写档；死亡时 ESC 读旧档 `Source/diablo.cpp:524-532` |

## 11. 附录 C：现状机械基线（2026-09-18 实测，原始输出未加工）

> 来源：`./build/sampling_behavior_test --gtest_filter=SamplingBaselineTest.*`（既有守卫的实测输出 ✓）。
> 用途：§6.4 要求先取基线；本表为**机械侧**基线（试玩侧基线仍待取 ⚠）。

```
[ CORESIZE ] level 1 cores 4
[ CORESIZE ] level 2 cores 5
[ CORESIZE ] level 3 cores 5
[ CORESIZE ] level 4 cores 5
[ CORESIZE ] level 5 cores 4
[ CORESIZE ] level 6 cores 4
[ CORESIZE ] level 7 cores 4
[ CORESIZE ] level 8 cores 5
[ CORESIZE ] level 9 cores 4
[ CORESIZE ] level 10 cores 4
[ CORESIZE ] level 11 cores 4
[ CORESIZE ] level 12 cores 4
[ CORESIZE ] level 13 cores 2
[ CORESIZE ] level 14 cores 2
[ CORESIZE ] level 15 cores 2
[ CORESIZE ] level 16 cores 4
[ CORESIZE ] level 17 cores 2
[ CORESIZE ] level 18 cores 3
[ CORESIZE ] level 19 cores 3
[ CORESIZE ] level 20 cores 2
[ CORESIZE ] level 21 cores 3
[ CORESIZE ] level 22 cores 3
[ CORESIZE ] level 23 cores 3
[ CORESIZE ] level 24 cores 2
```

**说明**：以上为**逐层**现状值（核心规模 / 单局种类 / 占比 / 地板对照 ✓）。**尚未取**的基线：小队数、放置位置、精英构成、补给消耗、回城次数 ⚠（前三项需读放置代码或新增只读统计；后两项需试玩 ✓）。

## 12. 附录 D：需求—反制盘点（Item 1，2026-09-18 实测）

**目的**：回答 C1 的验收问题——**每个层段是否存在 ≥2 种"可准备的反制"** ✓。

### D.1 反制池（词缀表，已核验 ✓）

| 反制族 | 行数 | 样本名 |
|---|---|---|
| `FIRERES`（火抗） | 5 | ⚠ 名称未取（下轮补） |
| `LIGHTRES`（**电**抗；`LIGHT`＝Lightning） | 5 | `Blue` / `Azure` / `Lapis` / `Cobalt` / `Sapphire` ✓（`:54-58`） |
| **`MAGICRES`**（魔抗族）★v2 改名 | 5 | `White` / `Pearl` / `Ivory` / `Crystal` / `Diamond` ✓（`:59-63`） |
| **`ALLRES`**（全抗性）★v2 补漏 | 5 | `Topaz` / `Amber` / `Jade` / `Obsidian` / `Emerald` ✓（`:64-68`） |
| `LIGHT` / `LIGHT_CURSE`（**光照**及其诅咒） | 2 / 2 | `light`(+2) / `radiance`(+4) / `the dark`(−3) / `the night`(−2) ✓ |
| 进攻类（非反制） | `FIRE` 4、`LIGHT_ARROWS` 3、`LIGHTDAM` 1 | — |

⇒ **反制池充足** ✓（**火抗 / 电抗 / 魔抗 / 全抗 / 光照**，共 **5** 族）⇒ "每层段 ≥2 种可准备反制"**在池子层面可满足** ✓。
**★v2 修正（独立复核实测）**：① `MAGIC` 的真名是 **`MAGICRES`**（我写错 ✗）；② **`ALLRES` 5 行我完全漏计** ✗；③ **必须显式排除 `*_CURSE` 族**（`LIGHT_CURSE`＝`the dark`/`the night`、`MANA_CURSE` 等是**负面词缀**，**不是反制** ✗）。

### D.2 各层段的威胁族（名册 × `monstdat.ai`，实测）

| 层段 | 威胁族数 | 近战/其它 | 施法/远程 | 具名施法-远程 |
|---|---|---|---|---|
| **L1-8** | 10 | 9 | **1** | `SkeletonRanged` |
| **L9-12** | 9 | 5 | **4** | `Acid` / `GoatRanged` / `Magma` / `Storm` |
| **L13-16** | **4** | 3 | **1** | `Counselor` |
| **L17-24** | 6 | 3 | 3 | `ArchLich` / `Lich` / `Torchant` |

### D.3 结论（两个真实缺口）

- **G1（L13-16 威胁族最少）** ⚠→**已软化**：地狱段的**威胁族数确实最少（4）** ✓，但**伤害类型多样性并非最弱** ✗ —— `Counselor` 一个怪就**同时携带 Fire 与 Lightning**（`CounselorAi`：`MissileTypes[4] = {Firebolt, ChargedBolt, LightningControl, …}` ✓ `monster.cpp:2719-2721`；`Firebolt`/`Fireball`=`Fire` ✓、`ChargedBolt`/`LightningControl`=`Lightning` ✓ `misdat.tsv:3/8/9/54`）⇒ **§4.2 的"消耗战"表述可保留** ✓，但**论据必须改**（缺口是**族数**，不是**类型多样性**）；另：**近战族本身也是"可准备"的**（护甲/闪避 ✓），不能只数远程/施法 ✗。
- **G2 ★v2 反转（原判被推翻 ✗）**：AI→伤害类型**可以机械导出** ✓✓，路径有**两条**：①通用表 `GetMissileType(MonsterAIID)`（`Source/monster.cpp:1903` ✓，被 `:1958/1987` 调用）；②**各专属 AI 函数覆盖通用表**（如 `CounselorAi` 的导弹表 ✓、`SkeletonBowAi` ⚠）：`SkeletonRanged` 实际发 **`Arrow`＝纯物理**（`misdat.tsv:2` ✓），**不是施法** ✗ ⇒ 对这两个族**通用表不适用** ✗。⇒ C1 的守卫**可自动化**（写一个静态扫描：`GetMissileType` 的 `case` + 各 `XxxAi` 里的 `StartRangedAttack` 调用 ✓），**无需人工维护表** ✓。
- **仍未核验 ⚠**：`unique_monstdat`（unique 的技能/攻击，可能给 L13-16 补火/电，如 `MT_BALROG`）⇒ 未查，不得当结论 ✓。

### D.4 尚未取（下一项）

1. `FIRERES`/`MAGIC` 族的**具体名称**（用于"反制可识别性"的断言）⚠；
2. **AI→伤害类型对照表**（决定 G2 的守卫能否机械化）⚠；
3. 小队数 / 放置位置 / 精英构成（放置代码统计）⚠。

## 13. 附录 E：AI→伤害类型 与 各层段需求覆盖（Item 2，2026-09-18 实测导出）

### E.1 导出的 AI→导弹→伤害类型（静态扫描 `Source/monster.cpp`，**可重复执行** ✓）

**通用表** `GetMissileType(MonsterAIID)`（`Source/monster.cpp:1903`，被 `:1958/1987` 调用）：共导出 **15** 条（**这是"通用表"，不是"全部伤害来源"** ✗ —— v1 的"共15条完整"表述为假 ✗）：

- `Acid` → `Acid` → `Acid`
- `AcidUnique` → `Acid` → `Acid`
- `ArchLich` → `YellowFlare` → `Magic`
- `BoneDemon` → `BlueFlare2` → `Magic`
- `Diablo` → `DiabloApocalypse` → `Physical,Invisible`
- `FireBat` → `Firebolt` → `Fire`
- `GoatRanged` → `Arrow` → `Physical,Arrow`
- `LazarusSuccubus` → `BloodStar` → `Magic`
- `Lich` → `OrangeFlare` → `Magic`
- `Magma` → `MagmaBall` → `Fire`
- `Necromorb` → `RedFlare` → `Magic`
- `Psychorb` → `BlueFlare` → `Magic`
- `Storm` → `ThinLightningControl` → `Lightning,Invisible`
- `Succubus` → `BloodStar` → `Magic`
- `Torchant` → `Fireball` → `Fire`

**专属覆盖（会绕过通用表 ✗）**：

- `SkeletonRanged` → `Arrow` → `Physical,Arrow`（**硬编码覆盖**，`Source/monster.cpp:2136`）
- `Counselor` → `{Firebolt, ChargedBolt, LightningControl, Fireball}`（**硬编码覆盖**，`:2719-2721`）

### E.1b 直接 `AddMissile` 路径（v2 补漏，**经独立复核发现** ✗✓）

`grep AddMissile(` 于 `Source/monster.cpp` 找到 **7 处**，其中 **4 类绕过** `GetMissileType`：

| 导弹 | 站点 | 类型（`misdat.tsv` ✓） | 影响 |
|---|---|---|---|
| `Rhino`（冲撞） | `:2250` / `:2420` / `:2642`（`RhinoAi` / `FallenAi` 的 GLOOM 分支 / `SnakeAi`） | `Physical`（`:22` ✓） | 已有类型，**不改变**集合 ✓ |
| **`Lightning`** | `:2436`（**`BatAi`** 内，`MT_FAMILIAR` 分支 ✓） | **`Lightning`**（`:10` ✓） | **改变 L1-8 集合** ✗（见 E.2 修正） |
| `AcidPuddle` | `:4350`（酸系怪死亡水洼） | `Acid`（`:61` ✓） | 已有类型 ✓ |
| `FlashBottom`/`FlashTop` | `:2737-2738`（`Counselor` 撤退分支） | `Magic`（`:13/14` ✓） | 已有类型 ✓ |

⇒ **口径规则**：C1 守卫若要"穷举伤害来源"，必须**同时扫描** `GetMissileType` 的 `case`、各 `XxxAi` 里的 `StartRangedAttack`、以及**直接 `AddMissile(` 调用** ✓。

### E.2 各层段「需求类型 → 可准备反制」（名册 core × 上表 **+ E.1b**；近战经 `ACP` 计入 ✓）

| 层段 | 威胁族 | 需求类型 | **可准备反制数** | 反制族 |
|---|---|---|---|---|
| **L1-8** | 10 | Physical / **Lightning**（`MT_FAMILIAR` 是 **L8 的 core** ✓，`BatAi` 直接发 `MissileID::Lightning` `:2436` ✓） | **2** | ACP(Fine/Strong…) ; **LIGHTRES**(Blue/Azure/Lapis/Cobalt/Sapphire) |
| **L9-12** | 9 | Acid / Fire / Lightning / Physical | **4** | ACP(Fine/Strong…) ; FIRERES(Red/Crimson/Garnet/Ruby) ; LIGHTRES(Blue/Azure/Lapis/Cobalt/Sapphire) ; —(D1 无酸抗 ⚠) |
| **L13-16** | 4 | Fire / Lightning / Physical | **3** | ACP(Fine/Strong…) ; FIRERES(Red/Crimson/Garnet/Ruby) ; LIGHTRES(Blue/Azure/Lapis/Cobalt/Sapphire) |
| **L17-24** | 6 | Fire / Magic / Physical | **3** | ACP(Fine/Strong…) ; FIRERES(Red/Crimson/Garnet/Ruby) ; MAGICRES(White/Pearl/Ivory/Crystal/Diamond) |

逐怪明细（可审计）：

- **L1-8**：Bat→Physical；Fallen→Physical；Fat→Physical；GoatMelee→Physical；Rhino→Physical；Scavenger→Physical；SkeletonMelee→Physical；SkeletonRanged→Physical；Sneak→Physical；Zombie→Physical
- **L9-12**：Acid→Acid；Fat→Physical；GoatRanged→Physical；Magma→Fire；Mega→Physical；Rhino→Physical；Snake→Physical；Sneak→Physical；Storm→Lightning
- **L13-16**：Counselor→Fire/Lightning；Mega→Physical；SkeletonMelee→Physical；Snake→Physical
- **L17-24**：ArchLich→Magic；FireBat→Fire；Lich→Magic；Scavenger→Physical；SkeletonMelee→Physical；Torchant→Fire

### E.3 C1 守卫的定义（**由实测数据校准** ✓）

1. **L1-8 不再豁免** ✗（v2 修正）：补上 E.1b 后，L1-8 实际有 **2** 种可准备反制（`ACP` + `LIGHTRES`，来自 `MT_FAMILIAR` 的 Lightning 攻击 ✓）⇒ **L1-24 每段 ≥2 —— 现状全部达标** ✓（2 / 4 / 3 / 3）；
2. **守卫必须是"编队构成"断言，而不是"类型集合"断言** ✗（**v2 关键修正，经复核指出**）：若按"每段类型数 ≥2"写静态断言，则**现状即达标** ⇒ 未来改动只可能"不变(PASS)"或"变多(仍 PASS)" ⇒ **它在设计意图下永远不会红 ＝ 伪门禁** ✗（本会话第三类"假守卫"，前两类：HF 确定性断言 ✗、结论已定型断言 ✗）。
   ⇒ **改为**：断言**实际遭遇的编队构成**，例如「**L9-12 的遭遇中，同时出现 ≥2 种需求类型怪物的比例 ≥ 阈值**」，阈值由 §6.4 的**编队基线**实测后写死 ✓；"改坏编队（把两支队伍改成同类型）⇒ 红" ✓。**测试落点**：新建 `test/pressure_shapes_test.cpp`（现有测试目录无同名文件 ✓）+ 注册 `CMake/Tests.cmake` ✓。
   ⇒ 由此 **A 腿的任务仍是"让已有需求咬合"**（构成 / 放置 / 压力 ✓），而不是增加类型 ✓。
3. **只统计"可应答"的需求**：**酸伤无对应抗性**（`Acid` 伤害，D1 无酸抗族 ⚠）⇒ 酸**不计入**，并作为**已知缺口**记录；
4. **显式排除 `*_CURSE` 族** ✗（`LIGHT_CURSE`＝`the dark`/`the night`、`MANA_CURSE`、`ACP_CURSE` ✓）；
5. **近战经 `ACP`（`Fine`/`Strong`…）计入** ✓ —— 已核验机制真实：`GetArmor()` 降低怪物对玩家的近战命中（`Source/player.cpp:719` ✓），近战族不是"没有需求"，而是"经护甲/闪避应答" ✓。

### E.4 unique 的类型（关闭最后一个 ⚠ ✓）

| 层段 | unique 覆盖的类型 |
|---|---|
| **L1-8** | Acid, Fire, Lightning, Magic, Physical（53 个 unique） |
| **L9-12** | Acid, Fire, Lightning, Magic, Physical（22 个 unique） |
| **L13-16** | Fire, Lightning, Magic, Physical（21 个 unique） |
| **L17-24** | Physical（4 个 unique） |

（L13-16 的 unique 明细：Lachdanan(Lachdanan)->Physical；Warlord of Blood(Warlord)->Physical；Fangskin(SkeletonMelee)->Physical；Blackskull(SkeletonMelee)->Physical；Lord of the Pit(SkeletonMelee)->Physical；Rustweaver(SkeletonMelee)->Physical）

⇒ 结论：**unique 未给任何层段补出新的"可应答"需求类型** ✓（类型仍是 Fire/Lightning/Magic/Physical ✓），G1 的强度不变 ✓。

**E.5 未决（下一项）**：编队基线（遭遇同时类型数 / 小队数 / 放置位置 / 精英构成 ⚠）—— 守卫阈值的校准依据 ✓。

## 14. 附录 F：Item 2 复核修正（v3）与**意外发现的引擎缺陷**

### F.1 直接施法清单（完整；`GetMissileType` **不足以**导出，✗ v1/v2 结论）

| 导弹 | 站点 | 类型 | 影响 |
|---|---|---|---|
| `Rhino`（冲撞） | `RhinoAi:2250` / `SnakeAi:2642` | Physical | 已有 ✓ |
| **`Lightning`** | **`BatAi:2436`**（`MT_FAMILIAR`，**L8 的 core** ✓ `level_rosters.tsv`） | **Lightning** | **L1-8 有电** ✗ |
| **`FlashBottom/FlashTop`** | **`CounselorAi:2737-2738`**；`ProcessFlashBottom/Top`（`missiles.cpp:3427/3456`）**含 `CheckMissileCol` ⇒ 实伤** ✓ | **Magic** | **L13-16 有魔抗需求** ✗ |
| **`InfernoControl`→`Inferno`** | **`MegaAi:2817`**（`StartRangedSpecialAttack`） | **Fire**（复核者称 ⚠） | **Mega＝Fire+Physical** ✗ |
| `AcidPuddle` | `MonsterDeath:4350` | Acid | 已有 ✓ |
| `HorkSpawn` | `HorkDemonAi:3006` | （仅生成，无伤） | 无 |
| `MissileID::Null:2736` | — | **不发射**（`MonsterRangedAttack:1229` 判空 ✓） | 是"漏检指示器"，非伤害源 ✓ |

**判定法修正**：`flags` 取值域＝**7 个标记**（5 个 `DamageType`＋`Arrow`/`Invisible` 两个**非伤害**标记）⇒ E.1 把 `Arrow`/`Invisible` 当"伤害类型"列出是**幻影类型** ✗，已删除。

### F.2 修正后的层段覆盖（v3，**取代 E.2**）

| 层段 | 可准备反制数（core） | 含 unique | 说明 |
|---|---|---|---|
| **L1-8** | **2**（`ACP` + `LIGHTRES`） | 4 | 电来自 `BatAi` 直发 ✓ |
| **L9-12** | **3**（`ACP`+`FIRERES`+`LIGHTRES`） | 4 | **E.2 的 4 把"无酸抗"空位也算进去了，与 E.3#3 自相矛盾** ✗ 已修 |
| **L13-16** | **4**（+`MAGICRES`） | 4 | 魔来自 `Counselor` 的 Flash ✓ |
| **L17-24** | **3** ✓ | 3 | 不变 |

### F.3 C1 守卫（v3：**可失败**的形式，取代 E.3#2）

(a) **等式断言**：`types(seg) == 黄金集`（增/删任一类型即红 ✓）；
(b) **闭包断言**：`AIs(名册该段 core) ⊆ AIs(导出映射)`（新 AI 进名册即红 ✓）；
(c) **发射点白名单**：正则扫描 `AddMissile(|StartRangedAttack(|StartRangedSpecialAttack(` 的**字面 `MissileID` 实参**，与白名单相等（**新增硬编码施法即红** ✓）；
(d) **前提断言**：`词缀族 ∩ {*_CURSE} = ∅` 且"**不存在 `ACIDRES` 族**"（一旦有人加酸抗 ⇒ 前提失效须红 ✓）；
(e) **文档表由脚本生成**（禁手抄 ✗）—— 本附录 F.1/F.2 必须由同一脚本产出 ✓。

### F.4 **意外发现：两个引擎缺陷**（与 Depth 设计无关，属缺陷修复 ⚠）

| # | 缺陷 | 证据（已亲自核验 ✓） | 严重度 |
|---|---|---|---|
| **D-1** | **空指针 AI 派发（＝崩溃）**：`AiProc` 中 `/*MonsterAIID::FireMan */ nullptr`（`Source/monster.cpp:3156` ✓），调用点 `:4647` **无判空** ✗；unique **"Warpfire Hellspawn"（`MT_HELLBURN`，ai=FireMan，level 11）存在** ✓ ⇒ **L9-12 可触发** | 实地核验 ✓ | **高（崩溃）** |
| **D-2** | **HF 专属导弹在 base 数据下越界读**：base `misdat.tsv` 仅 **68 行**（末行 `DiabloApocalypse` ✓），`OrangeFlare` **只在 HF 表** ✓；枚举含它（`misdat.h:84` ✓）、断言把它绑在 `LastDiablo+1`（`misdat.cpp:393` ✓） | 前提已核验 ✓；`GetMissileData` 体内越界未逐步复现 ⚠ | 中-高 |

⇒ **建议作为独立 Item（缺陷修复，Base 类）**，本规格只记录，不在此实现 ✓。

## 15. 附录 G：编队基线（Item 3，2026-09-18 实测）

**目的**：为 A 腿（压力形态）的守卫提供**阈值依据**（规格 §6.4 要求"先取基线"✓）。

### G.1 小队参数现状（**全层统一** ✗）

| 层 | squad_chance | squad_size | squad_leashed | tail_draw | class_floors |
|---|---|---|---|---|---|
| L1 | 30 | 2 | 1 | 2 | Melee=2 |
| L2 | 30 | 2 | 1 | 4 | Melee=2 |
| L3 | 30 | 2 | 1 | 4 | Melee=2 |
| L4 | 30 | 2 | 1 | 4 | RangedTurret=1 |
| L5 | 30 | 2 | 1 | 2 | Melee=1 |
| L6 | 30 | 2 | 1 | 2 | Sneak=1 |
| L7 | 30 | 2 | 1 | 2 | Melee=1 |
| L8 | 30 | 2 | 1 | 3 | RangedKite=1 |
| L9 | 30 | 2 | 1 | 3 | RangedKite=1 |
| L10 | 30 | 2 | 1 | 3 | RangedKite=2 |
| L11 | 30 | 2 | 1 | 3 | RangedKite=2 |
| L12 | 30 | 2 | 1 | 3 | RangedKite=2,Melee=2 |
| L13 | 30 | 2 | 1 | 4 | Melee=2 |
| L14 | 30 | 2 | 1 | 4 | Melee=1 |
| L15 | 30 | 2 | 1 | 3 | Melee=2 |
| L17 | 30 | 2 | 1 | 2 | Melee=2 |
| L18 | 30 | 2 | 1 | 1 | Melee=2 |
| L19 | 30 | 2 | 1 | 1 | Melee=2 |
| L20 | 30 | 2 | 1 | 1 | Melee=2 |
| L21 | 30 | 2 | 1 | 1 | Melee=1 |
| L22 | 30 | 2 | 1 | 1 | Melee=2 |
| L23 | 30 | 2 | 1 | 2 | Melee=1 |
| L24 | 30 | 2 | 1 | 2 | Melee=2 |

⇒ **`chance=30 / size=2 / leashed=1` 在所有**已列出的**层完全一致** ✗ —— 小队机制**已参数化，但未按层段塑形** ✓。
**⚠ 例外（v2，经独立复核指出）**：**L16 整行缺失** ✗（不是"同值"）⇒ 该层 `rosterParams == nullptr` ⇒ **名册侧小队机制被完全绕过** ✓。
⇒ **后果（方向更正 v3）**：L16 上 `rolls=0` 且 `max(1, eligibleCoreDraws)=1` ⇒ `rate=0.0` ⇒ 在固定阈值（`0.30-0.05`）下**会报假红（false red）** ✗ ——
v2 写成"恒真＝伪断言"**方向写反** ✗（只有拿 L16 自身基线 0 当阈值才会恒真 ✓）。
⇒ **守卫必须显式排除 L16，并且要断言这件事本身** ✓：
```cpp
// L16 无 squad 参数（名册仍保有该层 core，一旦有人补行，小队会真的开始出现）
EXPECT_EQ(GetLevelRosterParams(16), nullptr) << "L16 的 squad 参数行被补上了，需同步守卫";
```
**为什么必须断言**：`IsCoreRosterMember` **直查名册表**（`monster.cpp:3347-3352` ⚠ 复核者引），而名册中仍保有 L16 的 core（复核者引 `level_rosters.tsv:60-63` ⚠）⇒ 一旦有人给 L16 补 params 行，**该层会真的开始出小队** ✗，而现有测试循环只到 L15（复核者引 `test/level_roster_baseline_test.cpp:1442` ⚠）⇒ **不会红** ✗ ⇒ 显式断言是唯一的防线 ✓。
（另：`monster.cpp:3629-3637` 的注释称"无 params 回退覆盖 L17-24" ⚠ 复核者称已过期——实际 L17-24 均有行，该回退只覆盖 L16 ✓，待核 ⚠。）
**对 A 腿的含义**：§4.2 的"四种形态"＝**调这些既有参数 + 放置策略**（低成本 ✓），而不是新建机制 ✓✓。

### G.2 精英（unique）分布（实测）

| 层段 | unique 数 | 关键事实 |
|---|---|---|
| L1-8 | 47 | **L1 = 0**（首层无精英 ✓，非缺陷）；L2-8 每层 5-8 |
| L9-12 | 22 | L9=7、L10=4、L11=4、L12=7 |
| L13-16 | 21 | **L13 = 9**（最多）；L15/L16 各 3 |
| **L17-24** | **3** | **仅 L19=2、L20=1；L17/L18/L21-L24 全为 0** ✗✗ |

⇒ **深层的"精英抉择"确实不能只靠 `unique_monstdat`** ✓（该层段只有 3 个），**但"只能靠拴系小队"的结论不成立** ✗（v2 修正，经独立复核指出）：
`GetLevelMTypes()`（**定义在 `Source/monster.cpp:3558`** ✓）里**硬编码**了 4 条 `AddMonsterType`（**`:3572`/`:3575`/`:3577`/`:3578`** ✓）——
（**引用更正**：v2 写的 `:463-479` 实为 `PlaceQuestMonsters()` 内部，其定义在 `:458` ✗；已按独立复核实测更正 ✓）

| 层 | 硬编码怪物 | 放置标志 |
|---|---|---|
| **L19** | `MT_HORKDMN`（Hork Demon ✓） | `PLACE_UNIQUE`（**该怪同时也是 L19 的 unique 行** `unique_monstdat.tsv:12`，`monsterPack=None` ✓ ⇒ **不是"额外的"精英** ✗） |
| **L20** | `MT_DEFILER`（The Defiler ✓） | `PLACE_UNIQUE`（**同为 L20 的 unique 行** `:13`，`monsterPack=None` ✓ ⇒ 同上） |
| **L24** | `MT_ARCHLICH`（Arch Lich ✓） | `PLACE_SCATTER` |
| **L24** | `MT_NAKRUL`（Na-Krul ✓） | `PLACE_SPECIAL`；**其"放置"走 `PlaceQuestMonsters` 的 boss 分支（复核者引 `:523-526` ⚠）**，`bosspacksize=0` ⇒ **0 随从** ✓ ⇒ 属任务路径而非小队 ✓ |

| 层 | 怪物 | `unique_monstdat` | `monsterPack` | 随从 |
|---|---|---|---|---|
| **L19** | **Grimspike**（`MT_OBLORD`） | level=19 ✓（`:100`） | **Leashed** ✓ | **≤8**（**unique 侧拴系包** ✗ v2 遗漏） |
| L19 | Hork Demon（`MT_HORKDMN`） | level=19 ✓（`:12`） | `None` ✓ | 0 |
| L20 | The Defiler（`MT_DEFILER`） | level=20 ✓（`:13`） | `None` ✓ | 0 |
| L24 | Na-Krul（`MT_NAKRUL`） | 无（走任务路径 ✓） | — | 0 |

⇒ **修正后的结论（v3，去重后）**：深层精英来源＝
①**`unique_monstdat` 的 3 条**（L19 Grimspike **带 Leashed 包 ≤8** ✗、L19 Hork Demon、L20 The Defiler ✓）；
②**L24 的 Na-Krul 走 `PlaceQuestMonsters` 任务/boss 路径**（0 随从 ✓）；
③**L16 有 `LoadDiabMonsts()` 注入的 `.dun` 嵌入式怪**（`monster.cpp:540`/`:3910` ✓）⇒ **L16 并非"无精英"** ✗（该层的问题只是**无 squad 参数** ✓）；
④**L21-L23 确无精英** ✓（`SetMapMonsters` 其余调用仅 l1-l4 ✓、`MT_GOLEM` 是每层 `PLACE_SPECIAL` 非精英 ✓）。
**另注**：`HORKDMN`/`DEFILER` 与 `GetLevelMTypes()` 的 `AddMonsterType` **指向同一批怪** ⇒ v2 的"unique + 硬编码 boss"在 L19/L20 **重复计数** ✗，已删 ✓。
**§4.2 的 L17-24 形态仍应以小队为主** ✓（因为 L21-L23 没有任何精英来源 ✓），但**不排除**在 L19/L20/L24 复用既有 boss ✓。
（另注：本项此前在附录 D 的"待核验"里就标注过"L17-24 精英口径未核验 ⚠"，我却直接下了结论 ✗ —— 已按此修正 ✓。）
（另有 6 个 `level=0` 的任务 unique 不参与分桶 ✓；还有 1 行 **`level=28`（Doomlock）在 L1-24 之外** ✓——由短确认复核指出，不构成矛盾 ✓。）

### G.3 数量与放置结构

| 事实 | 锚点 | 含义 |
|---|---|---|
| 数量由**布局**推导：`na/30`（`na`＝16..96 区域非实心格数）+ MP ×1.5 + `MaxMonsters-10` 夹取 | `Source/monster.cpp` InitMonsters（`numplacemonsters`）✓ | "小队数/层"是**分布**而非定值 ✗ ⇒ **守卫阈值必须统计化** ✓ |
| 名册侧小队：一个 leader + `squadSize` 个**同层不同 core** 的随从 | `Source/monster.cpp:3960-3995` ✓（含不可达防御分支的完整注释 ✓） | 形态可直接作用于它 ✓ |
| unique 侧小队：`monsterPack != None` ⇒ `PlaceGroup(...Leashed)` | `Source/monster.cpp:3527` ✓ | 深层小队可复用该路径 ✓ |

### G.4 仪器（守卫可用）

- **`GetSquadRollStats()`** ✓（`Source/monster.cpp:3409` 访问器；`SquadRollStats` `:3405` 计数、`:3555` 重置）⇒ 可直接观测 `eligibleCoreDraws` / `rolls` ✓；
- **尚缺** ⚠：真实"**遭遇同时出现 ≥2 种需求类型的比例**"需要一次测试扩展（在采样测试里读 `GetSquadRollStats()` + 每层类型集，输出/断言 ✓）。

### G.4b 可失败阈值示例（采纳复核建议 ✓）

```cpp
// 每层跑 N 次建关，取均值；L16 必须排除（该层无 squad 参数，见 G.1）
const auto &stats = GetSquadRollStats();
const double rate = static_cast<double>(stats.rolls) / std::max<size_t>(1, stats.eligibleCoreDraws);
EXPECT_GE(rate, 0.30 - 0.05) << "该层的 squad_chance 被改坏（现状 30）";
```
**反证**：把某层 `squad_chance` 由 30 改成 5 ⇒ 该断言**变红** ✓。
**仪器字段**（`monster.h:596` ✓，非 static，`test/level_roster_baseline_test.cpp` 已在用 ✓）：
`eligibleCoreDraws / rolls / leaderPlacementFailed / noPartnerAvailable / realised / formed` ✓；
**重置时机**：`InitLevelMonsters()` 内 `:3555` ✓（每次建关调用 ✓）⇒ 可支撑按层断言 ✓。

### G.5 对 A 腿的直接结论

1. **形态＝参数调优**（`squad_chance/size/leashed` 按层段 + 放置策略）✓ ⇒ 实现成本**低于**先前估计 ✓；
   **但两个例外须计入工作量** ✗：**L16 无参数行**（要先补行或显式绕开 ✓）、**深层精英的 4 类硬编码来源**（改动若涉及 L19/L20/L24，需处理 `GetLevelMTypes()` 的硬编码分支 ✓）；
2. **深层精英靠小队**（unique 只剩 3 个）✗ ⇒ §4.2 的 L17-24 形态据此改写 ✓；
3. **守卫阈值统计化**：例如"L9-12 的 `rolls/eligibleCoreDraws` ≥ 现状值（chance=30）"✓ —— 由**现状基线**锁定，改坏参数即红 ✓；
4. 守卫的两条判据（附录 F.3 的 (a)(c)）与本节基线结合后**可写成可失败断言** ✓。
