---
name: decision_content_density_contract
description: 内容密度契约——四轴地板（vanilla 实测）、非对称 cap、R4 反转（密度优先）、D4 退役、以及实施中踩到的 5 个坑
type: decision
created: 2026-09-18
sources:
  - docs/superpowers/specs/2026-09-16-content-density-contract-design.md
  - docs/superpowers/plans/2026-09-16-content-density-contract.md
  - docs/superpowers/ledgers/2026-09-15-level-rosters-phase-a2/progress.md
---

# 决策：内容密度契约（四轴地板 + 非对称 cap + R4 反转）

**决策（2026-09-16 批准，v4；2026-09-18 实施）**：把"内容密度"立为一等判据，并在它与压力守卫冲突时**以密度为先**。

## 判据四轴

| 轴 | 定义 | 地板 |
|---|---|---|
| D1 单局种类 | 每层每局 realized types | = **vanilla 逐层实测**（超范围单行夹具导出） |
| D2 单局威胁多样性 | distinct AI / distinct 行为类别 | = vanilla |
| D3 跨局总内容量 | 200 seeds 类型并集 | = vanilla |
| D4 曝光率 | 并集 ÷ 候选池 | **已退役**（分母不适定：并集含任务预加/Golem/unique base，实测率 >100%） |

**不变量**：任何层不得确定性（每 seed 组合数 ≥2）；unique 可达性不降；小队形成率不降。

## 手段：非对称 cap

**远程类受压、非远程类放量**——地狱 L13-15（远程 cap 2→1）、L2（远程 cap=1）、caves 的 kite cap（L9-12 ≤2）**退役**（它使 5-6 个 kite 类型永不可抽，L10 并集 12 < vanilla 17）；L16 保持旧约束（只登记、运行时硬编码分支，cap 不在运行时生效）。

配套参数：L13/L14 `tail_draw` 1/2 → 4；L14/L15 去掉 Turret core（否则吃掉 cap1，unique base 不可达）；L17 去掉一把 core 使尾池 ≥3。

## R4 反转（本契约最重要的制度变更）

旧 R4"期望失配只许改数据、不许放宽阈值"在实践中被滥用成**砍内容过守卫**。新规则：**不得以牺牲 D1-D3 任一轴为代价达标**；冲突时只允许 ①扩池 ②**重推守卫** ③若必须砍须明写损失量。`PlacedClassMixWithinBaseline` 的主判据因此从"≤ 基线+5pp（天花板）"改为"**≤ 同种子 vanilla**"，原 ceiling 仅作**报警线**。

## 实测确认的坑

- **历史基线数组 `kRangedShareBaseline` ≠ 同种子 vanilla**（最大差 **14.6pp**，L15 0.4371 vs 0.5835）→ 主判据必须用新实测数组，否则真实回归被掩盖（首跑即抓到 L2/L3/L5/L6/L7 五层越线）。
- **加载器拒绝"仅表头"文件**（`Source/data/file.cpp:53-57` `Error::NoContent`）→ vanilla 地板要用**超范围单行（level 99）**夹具表达。
- **`class_floors` 的补位是确定性的**：L15 的 `RangedTurret=1` floor 曾把每 seed 组合数压成 1（违反不变量①）→ 冗余 floor 必须删（core 后尾池仅 2 条近战、`tail_draw=3` 必然含 ≥1 远程）。
- **core 换成"某 unique 的 base"** 会被校验拒绝（`needs allow_unique_boost`）→ 换 core 只能挑非 unique base 的候选。
- **类型占比 ≠ 数量占比**：抽到的是类型、落地是 3-5 只一组 → 尾抽的远程类型以整组计入 placed 口径，占比放大（spike 实测 0.536 vs 模型估 0.31）。

## 关联

- 规格：`docs/superpowers/specs/2026-09-16-content-density-contract-design.md`（含附录 A 的 A1-A11 外溢清单）
- 计划：`docs/superpowers/plans/2026-09-16-content-density-contract.md`
- 相关：`reference_leader_relation_states_and_consumers.md`、`decision_minion_options_and_formed_counter.md`、`gotcha_tsv_edits_need_mpq_rebuild.md`

**为什么：** 名册功能让"每局遇到的怪"变成被设计决定的名册，但判据只钉了防御性的"远程占比 ≤ 基线+5pp"，裁决 R4 被滥用成**砍内容过守卫**——实测发现地狱并集 15-25→9/16/8、L13 曝光率 60%、L17 每 seed 组合恒 1。密度必须成为一等判据，且冲突时以密度为先。

**何时使用：** 任何要动 `level_roster*.tsv`、`BehaviorClassCapForLevel`、或评估"内容变多/变少"的改动之前：先看四轴地板与不变量，再决定用"扩池/重推守卫/写明损失"哪条路。

## 指名反制（红线 10 收口，2026-09-18）

| 新增内容 | 反制（具体玩家行为） |
|---|---|
| 核心小队（未强化） | ① 把 leader 引开 >4 格 → 随从脱队（拴系断裂）；② 先杀 leader → 拆队（普通 leader 死亡还会清掉悬挂索引） |
| 层身份与构成 | 不新增压力：远程占比主判据 = ≤ 同种子 vanilla（已断言）；既有反制不变（贴身、掩体、拉出射程） |
| L14/L15 core 调整 | 同上；该调整本身是**降低**远程占比以换回 unique 可达（4/6 → 6/6） |
