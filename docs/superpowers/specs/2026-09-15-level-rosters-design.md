# 逐层名册 + 配额 + 核心小队（Level Rosters, Quotas and Core Squads）

**日期**：2026-09-15
**状态**：草案（待作者复核）
**分类**：Expansion
**取代**：无
**依据**：`docs/superpowers/explore/2026-09-15-content-direction-synthesis.md`（八维度探索汇总）、`docs/superpowers/explore/2026-09-15-p0-sampling-measurement.md`（P0 引擎实测）、`.superpowers/reviews/2026-09-15-level-rosters-design-review.md`（对抗性复核，裁决 needs_revision，发现 9 条，处置见附录 A）

---

## 1. 问题陈述

**症状（可复现）**：进入地狱 13-15 层的任意一局，一层里玩家能遇到的怪物种类是 3 种左右，且每次进同一层遇到的种类不同——层没有可辨识的身份。

**实测数值**（P0 引擎实测，40 seeds 平均；方法见 `docs/superpowers/explore/2026-09-15-p0-sampling-measurement.md`）：

- 单层实际生成的怪物类型数：教堂 L1-4 ≈ 7.0 → 洞穴 L9-12 = 3.3-3.6 → **地狱 L13-15 = 3.2-3.3**
- 单层 distinct AI：L13-15 = **3.0-3.1**；distinct 行为类别：L13-15 = 2.6-2.8
- 单层 Σ`image` = 3523-3889（预算上限 4000）→ 预算已饱和，是绑定约束
- 层的"名册"（同层 200 seeds 的可能类型并集）：后期 **15-25 种**
- 相邻层名册 Jaccard = 0.5-0.7；与 L1 相比 L4-14 ≈ 0.0

**结论**：层的身份**存在但不可见**——单局只露出名册的约 20%，且露出的那一片是随机的。

**为什么这是问题**：D1 的记忆点建立在"某层是什么地方"上（屠夫层、食尸鬼群、洞穴的风筝）。当一层的构成完全随机且只有 3 种时，"这是什么地方"无法被玩家读出，层与层之间只剩数值差异。

## 2. 分类判定

按宪章 §2 判定树逐步判定：

| 步 | 判定 | 结果 |
|---|---|---|
| 第一步 1：是否触及 TSV 数据文件中的数值字段？ | 是——新增 `level_rosters.tsv` / `level_roster_params.tsv`，并读取既有 `monstdat.tsv` 的 `minDunLvl`/`maxDunLvl`/`ai` | **Expansion，无例外条款** |
| （不适用）第二步 | — | 已由第一步判为 Expansion |

**说明**：`monstimgtot` 上限由常量变为逐层数据（`max_monster_image`）属机制修改；核心小队复用既有 `PlaceGroup` 的 leader/leashed 语义，不新增怪物、不新增美术。

## 3. 事实基础

| 事实 | 数值 / 结论 | 出处 |
|---|---|---|
| 采样预算为绑定约束 | `while (nt > 0 && LevelMonsterTypeCount < MaxLvlMTypes && monstimgtot < 4000)` | `Source/monster.cpp:3516` |
| 预算按 `image` 累加 | `monstimgtot += monsterData.image` | `Source/monster.cpp:3305` |
| 层槽上限 | `MaxLvlMTypes = 24` | `Source/monster.h:39` |
| 候选池判据**逐层** | `currlevel >= minDunLvl && currlevel <= maxDunLvl`（shareware 时排除 `Retail`） | `Source/monster.cpp:3161-3170` |
| **覆盖整层段的怪 = 0 种** | 段 1-4 / 5-8 / 9-12 / 17-20 / 21-24 的交集均为 0；13-15 为 3；L16 为 6（单层） | 本轮实算（`assets/txtdata/monsters/monstdat.tsv:minDunLvl,maxDunLvl`） |
| 逐层候选数 | L1:6 L2:20 L3:24 L4:22 … L13:15 L14:15 L15:9 L16:6 | 同上 |
| 逐层候选数（Nest/Crypt，HF overlay） | L17-20 / L21-24 在**基础表**为 0（HF 怪物只在 `mods/hf`） | `assets/txtdata/monsters/monstdat.tsv` 无 17-24 行；`mods/hf/txtdata/monsters/monstdat.tsv`（139 行） |
| `AddMonsterType` 只追加、不拒绝 | `GetMonsterTypeIndex` 线性查找：已存在→返回其下标；否则返回 `LevelMonsterTypeCount`（下一空槽） | `Source/monster.cpp:382-389`、`:3296-3322` |
| 预加**不计入** caps | caps 逻辑只存在于采样循环内（`capKite`/`capSameClass`） | `Source/monster.cpp:3509-3551` |
| caps 计数**包含预加** | `classCounts` 从 `LevelMonsterTypeCount` 起算 | `Source/monster.cpp:3509-3512` |
| 预加**计入**预算 | 预加同样走 `AddMonsterType` → `monstimgtot += image` | `Source/monster.cpp:3305` |
| L16 硬编码分支 | `currlevel == 16` 时预加 `MT_ADVOCATE`/`MT_RBLACK`/`MT_DIABLO` 后**直接 return** | `Source/monster.cpp:3441-3447` |
| **unique 放置被采样门控** | `const size_t minionType = GetMonsterTypeIndex(UniqueMonstersData[u].mtype); if (minionType == LevelMonsterTypeCount) continue;` | `Source/monster.cpp:509-512` |
| unique 的 Leashed 比例 | 70 / 100 行 | `assets/txtdata/monsters/unique_monstdat.tsv` |
| 小队原语已存在 | `void PlaceGroup(size_t typeIndex, size_t num, Monster *leader = nullptr, bool leashed = false)` | `Source/monster.cpp:308` |
| **传 leader 时随从 HP×2** | `minion.maxHitPoints *= 2; minion.hitPoints = minion.maxHitPoints; minion.intelligence = leader->intelligence;` | `Source/monster.cpp:354-356` |
| leashed 约束 | 随从须与 leader 同 `dTransVal` 且坐标差 < 4 | `Source/monster.cpp:346-348` |
| `packSize` 只在 leashed 时设置 | `if (leashed) leader->packSize = placed;` | `Source/monster.cpp:375-377` |
| 普通散布现状 | 每次取**单一类型**并 `PlaceGroup(typeIndex, na)`；`na`：L1 恒 1、L2/Crypt 2-3、其余 3-5 | `Source/monster.cpp:3773-3782` |
| 散布总数与种类数无关 | `totalmonsters = ActiveMonsterCount + numplacemonsters`（由层大小/密度算出） | `Source/monster.cpp:3758-3763` |
| `PlaceGroup` 按 `totalmonsters` 钳制 | `if (num + ActiveMonsterCount > totalmonsters) num = totalmonsters - ActiveMonsterCount;` | `Source/monster.cpp:336-338` |
| 散布候选按**类型**均匀抽取 | `const size_t typeIndex = scattertypes[GenerateRnd(numscattypes)];` | `Source/monster.cpp:3774` |
| 行为类别 8 类 + `default: Boss` | `Melee/RangedTurret/RangedKite/Rally/Charge/Sneak/Summon/Boss` | `Source/tables/monstdat.h:376-386`、`monstdat.cpp:483-515` |
| 层生成 RNG 与后续共用同一流 | `LoadGameLevelDungeon` 不重置种子 | `Source/diablo.cpp:3342-3346` |
| 怪物类型下标过网络 | `DSpawnedMonster.typeIndex` | `Source/msg.cpp:231`、`:2614` |
| 存档记录怪物类型下标 | `monster.levelType = file->NextLE<int32_t>();` | `Source/loadsave.cpp:681` |
| TSV 加载范式 | `DataFile::loadOrDie(filename)` | `Source/tables/monstdat.cpp:410` |
| 采样类 eval 先例 | `sampling-anti-monopoly.yaml` | `eval/cases/rng/sampling-anti-monopoly.yaml` |

**口径声明**：本规格所有候选数与校验以 **retail 数据**（`gbIsSpawn == false`）为准；spawn 数据下 `Retail` 怪被排除（`Source/monster.cpp:3166`），候选数显著减少，故加载期校验在 spawn 下走**宽松模式**（只校验"有名册行的层其 core 非空"）。

## 4. 方案

### 4.1 数据模型（**逐层**，不是层段）

`assets/txtdata/monsters/level_rosters.tsv`（基础表，L1-16）：

```
level   monster_id     role    allow_unique_boost
1       MT_WSKELAX     core    -
1       MT_RFALLSP     core
1       MT_TSKELBW     core
9       MT_MUDMAN      core
9       MT_BMAGMA      core
9       MT_STORML      core
13      MT_GSNAKE      core
...
```

- `level` 允许写单层或区间（`13-15`），但**加载期校验区间内每一层该怪都可用**（`IsMonsterAvailable` 语义），违反即拒绝启动（见 4.4）。
- `mods/hf/txtdata/monsters/level_rosters.tsv`（HF overlay，L17-24）：Nest/Crypt 的怪物只存在于 HF 数据，故其名册**必须**放在 HF overlay；基础表不得引用 HF 怪物 id。

`assets/txtdata/monsters/level_roster_params.tsv`：

```
level   max_image   tail_draw   class_floors          squad_chance   squad_size
1       6000        2           Melee=2,Ranged=1      30             2
9       16000       3           Ranged=1              30             2
13      18000       3           Melee=1,Ranged=1      35             2
```

- `max_image`：该层的**安全上限**（取代全局常量 4000）。内存不是约束（作者已判定），该值只防"点名过多导致失控"。**初值依据**：最重的 6 种 core 合计 13406（L13 实算），洞穴 core 5 种 7220 + 尾池 3 种 → 需 ≈14000 量级；表中 16000/18000 是据此留余量的**提案值**。
- `tail_draw`：尾池抽取数上限（实际取 `min(tail_draw, 可用候选数)`；候选不足时为 0，合法）。
- `class_floors`：行为类别下限（与 B1 caps 合成完整配额）。
- `squad_chance`：core 类型散布时转为小队的概率（%）。**表内为初值**，实施时按验收 5 的观测与体感校准。
- `squad_size`：小队随从数（1-2）。**初值 2**，同上校准。

### 4.2 机制：`GetLevelMTypes()`（`Source/monster.cpp:3439+`）

1. **既有预加不变**：`MT_GOLEM`、任务 unique、SKING 层的随机骷髅。
2. **新增 core 预加**：按 `currlevel` 查名册，对每个 `core` 成员 `AddMonsterType(type, PLACE_SCATTER)`。core 预加**绕过 caps**（caps 只在采样循环内）但**计入预算**。
3. **尾池**：`typelist` = `IsMonsterAvailable` 候选 **减去已加入的 core**。
4. **尾池抽取显式化**：循环最多成功抽取 `tail_draw` 次（现有 while 退化为该次数的外壳）。
5. **配额**：B1 的 caps 保留；新增 floors——在尾池抽取结束后校验，若未满足则在循环内**优先补足**（抽取时优先选能补 floor 的类型）。
6. **预算**：`monstimgtot < max_image(当前层)`。
7. **L16 特例**：保持硬编码分支不变（`Source/monster.cpp:3441-3447` 直接 return）；`level_rosters.tsv` 中 L16 只登记 core 供校验与文档用途，不改变其行为。

### 4.3 机制：核心小队（普通散布，`Source/monster.cpp:3773-3782`）

1. `PlaceGroup` 新增参数 `bool toughMinions = true`（默认值保持既有行为，unique 路径不受影响）：
   `void PlaceGroup(size_t typeIndex, size_t num, Monster *leader = nullptr, bool leashed = false, bool toughMinions = true)`
   `toughMinions == false` 时**不执行** `maxHitPoints *= 2`（`:354-355`），只保留 `intelligence` 继承。
   **理由（DP1）**：HP×2 是数值膨胀；本规格只买"构成"，不买"数值"。
2. 散布循环中，若抽到的类型是 core 成员，以 `squad_chance` 概率转为一支小队：
   - `PlaceGroup(leaderType, 1)` 放置 leader（`leader = &Monsters[ActiveMonsterCount - 1]`）；
   - 从**同一层的其他 core 成员**中抽取一个 partner，`PlaceGroup(partnerType, squad_size, leader, true, false)`（leashed=true 使其成为可辨识的一队，`packSize` 由已有逻辑维护）；
   - 随从**不强化**（HP 不翻倍），但继承 leader 的 intelligence（该行为无法关闭，为 `PlaceGroup` 既有语义，规格显式接受）。
3. 失败与钳制沿用既有语义：同 `dTransVal` 且 <4 格、10 次重试后放弃、按 `totalmonsters` 钳制。

### 4.4 加载期校验（拒绝启动而非静默降级）

1. `level_rosters.tsv` 每行：`monster_id` 必须在**当前加载的数据集**中存在；区间行必须满足"区间内每一层都可用"。
2. `level_roster_params.tsv` 每行：`class_floors` 必须在该层候选池 + B1 caps 约束下**可满足**（否则报错并指出冲突项）。
3. **unique 门控检查**：列出"core 成员的 base 类型属于**同层**某 unique 的 `mtype`"的全部条目并**拒绝启动**，除非该 `(level, monster_id)` 出现在名册表新增的 `allow_unique_boost` 列中（`Source/monster.cpp:509-512` 说明 core 会**提高**该 unique 的出现率；本规格默认保持其稀有性不变）——对应验收 4。
4. spawn 数据（`gbIsSpawn == true`）下走宽松模式：只校验"有名册行的层 core 非空"。

### 4.5 行为类别目标（红线 14 的载体）

散布按**类型**均匀抽取（`:3774`），因此"类型数 3 → 7-9"会重排近战/远程的**怪物配比**。本规格把它作为**显式设计参数**处理：

- 实施**第一步**必须在真实引擎里测出**改动前的 placed class mix**（按行为类别统计已放置的怪物数，而非类型数）作为基线；
- 验收以**相对基线**表述（如"远程类占比不超过基线 +5 个百分点"），避免用未知的绝对值；
- `class_floors` 只用于**补足**（修 kite 垄断与类别缺位），不作为提高远程占比的手段。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 分类 | Expansion（§2 判定树第一步命中：新增/读取 TSV 数值字段） |
| 2 | 问题陈述指向具体症状 | §1：地狱 L13-15 单层实得 3.2-3.3 种、Σ`image` 3523-3889/4000（P0 实测，方法可复现） |
| 3 | 数值均有出处 | §3 每行给出 `文件:行号` 或 TSV 列 |
| 4 | 「已实施」的函数有非测试调用者 + 验收全过 | 状态为**草案**，未声明已实施；新表查询函数由 `GetLevelMTypes`（生产）调用 |

### 扩充层红线

| # | 判定 | 依据 |
|---|---|---|
| 9 | 新增内容/机制修改，非膨胀数值 | 名册只改**构成**；小队随从显式**不强化**（§4.3.1，`toughMinions=false`）；`max_image` 只放宽上限、不改任何怪物数值 |
| 10 | 每个新增内容有反制且指名 | 不新增怪物、不新增攻击手段 → 既有反制（走位/光照/消耗品）不变；名册带来的唯一新压力是**远程配比**，由 §4.5 的基线约束与 `class_floors` 控制 |
| 11 | 产生取舍还是负担 | 不新增规则；玩家侧无新概念（不知道名册也能玩）；取舍体现在"这层会遇到什么"的预期管理 |
| 12 | 全层段定义 | 基础表覆盖 L1-16、HF overlay 覆盖 L17-24、L16 特例显式登记；加载期校验区间有效性（§4.4） |
| 13 | 与既有系统互动 | 复用 B1 的 `classCounts`/caps 与 `PlaceGroup` 的 leader/leashed/`packSize`/分离重聚状态机；与 A1/A3 共用 `BehaviorClass` 分类（A1/A3 计划已登记新 AI 的类别） |
| 14 | 近战/远程分别评估 | §4.5 给出方法（placed class mix 基线 + 相对阈值）与手段（floors 只补足）；**不主张"总数不变所以压力不变"**（该论证已被复核否证） |

### 禁令

1 未实现内容标注状态 ✓（状态=草案）；2 无不可落地表格 ✓（全部字段已定义）；3 数值有出处 ✓；4 不新增玩家必记概念 ✓；5 红线逐条给依据 ✓；6 不用占位测试 ✓（§6 全部为真实断言）；7 不改行尾 ✓（`.md` LF；TSV 新增按 `.editorconfig`）

## 6. 验收标准

| # | 验收 | 方式 |
|---|---|---|
| 1 | 每层 core 成员在 200 seeds 上 **100% 出现**，且每个 core 成员都是该层候选 | `SamplingBaselineTest.RosterCoreAlwaysPresent` |
| 2 | 尾池抽取数 = `min(tail_draw, 可用候选数)`；候选为 0 时合法 | `SamplingBaselineTest.RosterTailDrawBounded` |
| 3 | 每层 `class_floors` 满足且 B1 caps 未被打破 | `SamplingBaselineTest.RosterQuotasSatisfied` |
| 4 | 非法名册行被**拒绝**（怪不存在 / 区间内某层不可用 / floors 不可满足 / core 是某 unique 的 base 且未列白名单） | `SamplingBaselineTest.RosterValidationRejectsInvalidRow`（用合成表驱动校验函数） |
| 5 | core 小队可观测：core 类型的放置中带 leader 关系的比例 ≈ `squad_chance`；**小队随从 HP 不翻倍**；unique 路径随从仍翻倍（回归） | `SamplingBaselineTest.SquadComposition` / `SquadMinionsNotToughened` / `UniqueMinionsStillToughened` |
| 6 | placed class mix（按类别的**怪物数**）相对改动前基线：远程类占比 ≤ 基线 +5 个百分点 | `SamplingBaselineTest.PlacedClassMixWithinBaseline`（基线由实施第一步实测产出，记录在实现计划对应任务与其提交信息中） |
| 7 | 身份守卫：相邻层名册 Jaccard < 0.9（防止名册把层做成一样） | `SamplingBaselineTest.IdentityGuard`（P0-D 度量） |
| 8 | 跨规格一致性：A1/A3 的变体出现在其所属层的 core 中 | `SamplingBaselineTest.A1A3VariantsAreCore` |
| 9 | eval 集成用例通过 | `eval/cases/rng/level-rosters.yaml` + `python3 -m tools.eval.backend --smoke` |
| 10 | 全量门禁 + 漂移 + 夹具重生成 + 台账 | `python3 tools/run_tests.py --json /tmp/ci.json`（100%/0/drift ok）；`timedemo` 等按宪章决策 35 重生成；`docs/knowledge/decision_save_format_policy.md` 台账追加一行 |

## 7. 状态

**草案（待作者复核）** → 复核通过后置为**已批准**，再进入实施计划。

---

## 附录 A：对抗性复核发现的处置（9 条）

| # | severity | 发现 | 处置 |
|---|---|---|---|
| F1 | blocker | 层段 core 与逐层 `minDunLvl/maxDunLvl` 冲突（覆盖整段的怪 = 0 种） | **已采纳**：数据模型改为**逐层**（§4.1），并加区间有效性校验（§4.4.1）；草案原示例数据作废 |
| F2 | blocker | "总数不变→压力不变"不成立（类型数变化重排近战/远程配比） | **已采纳**：删除该论证；§4.5 以 placed class mix 为口径，验收 6 用**相对基线**阈值；红线 14 依据重写 |
| F3 | high | unique 放置被采样门控，core 会**提高** unique 出现率 | **已采纳**：§4.4.3 加载期列出并警告，要求排除 unique 的 base 类型（除非显式接受）；§3 记录门控事实与 70/100 Leashed |
| F4 | medium | 数值矛盾（L13 最重 6 种 = 13406 > 12000；Nest/Crypt 尾池恒空） | **已采纳**：`max_image` 改为**逐层参数**（§4.1，L13=18000 量级）；`tail_draw` 语义改为 `min(...)`，候选不足为 0 合法 |
| F5 | medium | core 预加挤爆 B1 caps 致 L15 无解；`13-16` 应拆 | **已采纳**：`13-16` 拆为 `13-15` + `16`（§4.1/§4.2.7）；§4.4.2 增加"floors 可满足性"校验 |
| F6 | medium | RNG 流共享 → 同 seed 布局/掉落全变；`typeIndex` 过网络属协议级 | **已采纳**：§3 记录两处事实；副作用写入 4.5 与验收 10；多人要求同版本（宪章决策 35） |
| F7 | medium | 8 条验收仅 5 条可落地（2 必然失败、3 可能无解、6 同义反复、5 阈值太松） | **已采纳**：验收重写为 §6 的 10 条，删除同义反复项，候选不足与 floors 不可满足改为显式合法/拒绝语义 |
| F8 | high | A 单独落地交付不出体感改善（L1 散布恒单只） | **已采纳并升级范围**：作者选择方案 ②——把**核心小队**并入本规格（§4.3），使"可辨识的遭遇"随本规格一起交付 |
| F9 | medium | 漏了零引擎改动的"收缩 `minDunLvl/maxDunLvl`"替代方案 | **已采纳**：见附录 B 的替代方案比较与结论（作为**配套数据动作**，非替代） |

## 附录 B：替代方案比较（F9）

| 方案 | 引擎改动 | 能否保证 core 可见 | 对多样性 | 结论 |
|---|---|---|---|---|
| 只收缩 `minDunLvl/maxDunLvl` | **零** | **不能**（仍是随机抽样） | 降低（候选变少） | 作为名册的**配套**：把不合身份的怪移出该层，可与本规格同批作为数据动作 |
| 只做配额（不点名） | 小（扩 caps 为 floors+caps） | 不能 | 不变 | 只修 kite 垄断；身份仍不可见 |
| 只做编组（不做名册） | 小（散布传 leader） | 不适用 | 不变 | 信息量最大的单点，但"谁和谁一队"缺少语义基础（不知谁是招牌） |
| **本规格（名册 + 配额 + 核心小队）** | 中（采样流程 + `PlaceGroup` 参数） | **能** | 提高（可见度从 ~20% 到 core 100% + 尾池） | 三者互补；本规格先落地，后续规格再做完整编组体系（每怪群组范围、精英包） |