# 逐层名册 + 配额 + 核心小队（Level Rosters, Quotas and Core Squads）

**日期**：2026-09-15
**状态**：草案 v3（待作者复核）
**分类**：Expansion
**取代**：无（v1 草案、v2 修订均已被本文覆盖）
**依据**：
- `docs/superpowers/explore/2026-09-15-content-direction-synthesis.md`（八维度探索汇总）
- `docs/superpowers/explore/2026-09-15-p0-sampling-measurement.md`（P0 引擎实测：P0-A/B/C/D）
- `.superpowers/reviews/2026-09-15-level-rosters-design-review.md`（第 1 轮对抗性复核，needs_revision，F1-F9）
- `.superpowers/reviews/2026-09-15-level-rosters-spec-review-r2.md`（第 2 轮对抗性复核，needs_revision，G1-G7）
- 裁决基准：`docs/superpowers/specs/2026-07-27-better-d1-design-charter.md`（§2/§3/§4；决策 33/34/35）

**阶段划分（本规格分两阶段落地，对应两份实现计划）**

| 阶段 | 内容 | 为什么分开 |
|---|---|---|
| **阶段 A** | §4.1 数据模型、§4.2 采样接入、§4.4 校验、§4.5 class mix 口径 | 纯采样与数据，风险低；独立可交付"层身份可读" |
| **阶段 B** | §4.3 核心小队（**前置：修 G1、决 G2**） | "普通怪当 leader"会触发两个既有引擎缺陷；把 A 的门禁绑到无关风险上不可接受（R2 建议） |

---

## 1. 问题陈述

**症状（可复现）**：进入地狱 13-15 层任意一局，一层里玩家能遇到的怪物种类约 3 种，且每次进同一层遇到的种类都不同——层没有可辨识的身份。

**实测数值**（P0 引擎实测，40 seeds 平均）：

- 单层实际生成类型数：教堂 L1-4 ≈ 7.0 → 洞穴 L9-12 = 3.3-3.6 → **地狱 L13-15 = 3.2-3.3**
- 单层 distinct AI：L13-15 = **3.0-3.1**；distinct 行为类别：**2.6-2.8**
- 单层 Σ`image` = 3523-3889（上限 4000）→ **预算饱和，是绑定约束**
- 层的"名册"（同层 200 seeds 并集）：后期 **15-25 种** → **单局只露出约 20%**
- 相邻层名册 Jaccard 0.5-0.7；与 L1 相比 L4-14 ≈ 0.0

**结论**：身份**存在但不可见**，且露出的那一片是随机的。

## 2. 分类判定

| 步 | 判定 | 结果 |
|---|---|---|
| 第一步 1 | 新增/读取 TSV 数值字段（新表 + `monstdat.tsv` 的 `minDunLvl`/`maxDunLvl`/`ai`） | **Expansion（无例外条款）** |

## 3. 事实基础

| 事实 | 数值 / 结论 | 出处 |
|---|---|---|
| 采样预算为绑定约束 | `while (nt > 0 && LevelMonsterTypeCount < MaxLvlMTypes && monstimgtot < 4000)` | `Source/monster.cpp:3516` |
| 预算按 `image` 累加 | `monstimgtot += monsterData.image` | `Source/monster.cpp:3305` |
| 层槽上限 | `MaxLvlMTypes = 24` | `Source/monster.h:39` |
| 候选池判据**逐层** | `currlevel >= minDunLvl && currlevel <= maxDunLvl`（shareware 时排除 `Retail`） | `Source/monster.cpp:3161-3170` |
| **覆盖整层段的怪 = 0 种** | 1-4 / 5-8 / 9-12 / 17-20 / 21-24 交集均为 0；13-15 为 3；L16 为 6 | 本轮实算（`monstdat.tsv:minDunLvl,maxDunLvl`） |
| 逐层候选数（retail） | L1:6 L2:20 L3:24 … L13:15 L14:15 L15:9 L16:6 | 同上 |
| **unique 的 base 占候选的 30-45%** | L13 为 9/20、L4/L5 为 8、L2 为 7（共 98 条 base） | `unique_monstdat.tsv:type,level`（列名即 `type`/`level`） |
| Nest/Crypt 怪物只在 HF overlay | 基础表无 17-24 行；`mods/hf/txtdata/monsters/monstdat.tsv` 为 139 行 | 两表实测 |
| `AddMonsterType` 只追加不拒绝 | `GetMonsterTypeIndex` 线性查找，未命中返回 `LevelMonsterTypeCount` | `Source/monster.cpp:382-389`、`:3296-3322` |
| 预加**不计入** caps | caps 逻辑只在采样循环内 | `Source/monster.cpp:3509-3551` |
| caps 计数**包含预加** | `classCounts` 从 `LevelMonsterTypeCount` 起算 | 同上 |
| 预加**计入**预算 | 预加同样走 `AddMonsterType` | `Source/monster.cpp:3305` |
| L16 硬编码分支 | `currlevel == 16` 预加后**直接 return** | `Source/monster.cpp:3441-3447` |
| **unique 放置被采样门控** | `if (minionType == LevelMonsterTypeCount) continue;` | `Source/monster.cpp:509-512` |
| unique 的 Leashed 比例 | 70 / 100 行 | `assets/txtdata/monsters/unique_monstdat.tsv` |
| 小队原语已存在 | `void PlaceGroup(size_t typeIndex, size_t num, Monster *leader = nullptr, bool leashed = false)` | `Source/monster.cpp:308` |
| 传 leader 时随从被强化 | `maxHitPoints *= 2`、`intelligence = leader->intelligence`、`changeAnimationData`、`mode = Stand`（Gargoyle 除外） | `Source/monster.cpp:354-367` |
| `MFLAG_ALLOW_SPECIAL` 的清除是 no-op | `abilityFlags` 列全表出现 `ALLOW_SPECIAL` **0 次**；`:363` 又被 `:360` 限定在非 Gargoyle 分支 | 复核员实算（R2 §③-2） |
| leashed 约束 | 随从须与 leader 同 `dTransVal` 且坐标差 < 4 | `Source/monster.cpp:346-348` |
| `packSize` 只在 leashed 时设置 | `if (leashed) leader->packSize = placed;` | `Source/monster.cpp:375-377` |
| **`hasLeashedMinions()` 只看 unique** | `return isUnique() && UniqueMonstersData[...].monsterPack == Leashed;` | `Source/monster.h:427-430` |
| **归队门控只对 unique leader 生效** | `DirOK` 里 `if (!monster.hasLeashedMinions()) return true;` 故普通 leader 永不被门控 | `Source/monster.cpp:4401-4427` |
| **G1：非 unique leader 死亡不释放随从** | `M_UpdateRelations` 以 `monster.hasLeashedMinions()` 为入口 → 非 unique 恒 false → 随从保持 `Leashed` + 悬挂 `leader` 索引 | `Source/monster.cpp:4112-4117` + `monster.h:427-430` |
| G1 加剧因素 | 引擎**自己承认**该悬挂引用风险："we should update this->leader to NoLeader ... we leave the reference and hope that no code tries to modify the leader through this instance later" | `Source/monster.cpp:4931-4937` |
| G1 加剧因素 2 | 死亡时槽位 swap、`AddMonster` 复用槽位 → 悬挂索引可能指向一只活着的无关怪 | `Source/monster.cpp:642-643`、`:3826` |
| **G2：`setLeader` 覆写随从 AI** | `this->leader = ...; leaderRelation = Leashed; ai = newLeader->ai;` | `Source/monster.cpp:4934-4944` |
| G6：`packSize` 读档重建**只对 unique+Scavenger** | `SyncPackSize`：`if (!leader.isUnique()) return; if (leader.ai != Scavenger) return;` | `Source/loadsave.cpp:839-853` |
| `intelligence` 继承影响面 | ~20 处 AI 概率/距离计算 | `Source/monster.cpp:1922,1990,2036-2059,2079-2142`；`Source/missiles.cpp:2335,2361`（复核员提供） |
| 普通散布现状 | 每次取**单一类型**并 `PlaceGroup(typeIndex, na)`；`na`：L1 恒 1、L2/Crypt 2-3、其余 3-5 | `Source/monster.cpp:3773-3782` |
| 散布总数与种类数无关 | `totalmonsters = ActiveMonsterCount + numplacemonsters` | `Source/monster.cpp:3758-3763` |
| `PlaceGroup` 按 `totalmonsters` 钳制 | `if (num + ActiveMonsterCount > totalmonsters) num = totalmonsters - ActiveMonsterCount;` | `Source/monster.cpp:336-338` |
| 散布候选按**类型**均匀抽取 | `const size_t typeIndex = scattertypes[GenerateRnd(numscattypes)];` | `Source/monster.cpp:3774` |
| 行为类别 8 类 + `default: Boss` | `Melee/RangedTurret/RangedKite/Rally/Charge/Sneak/Summon/Boss` | `Source/tables/monstdat.h:376-386`、`monstdat.cpp:483-515` |
| 层生成 RNG 与后续共用同一流 | `LoadGameLevelDungeon` 不重置种子 | `Source/diablo.cpp:3342-3346` |
| 怪物类型下标过网络 | `DSpawnedMonster.typeIndex` | `Source/msg.cpp:231`、`:2614` |
| 存档记录怪物类型下标 | `monster.levelType = file->NextLE<int32_t>();` | `Source/loadsave.cpp:681` |
| TSV 加载范式 | `DataFile::loadOrDie(filename)` | `Source/tables/monstdat.cpp:410` |
| `PlaceGroup` 调用点仅 2 处 | `:3410`（unique，带 leader）、`:3781`（普通散布） | `Source/monster.cpp` |
| `Monsters` 为定长数组 | `Monster Monsters[MaxMonsters]` | `Source/monster.h:494`、`Source/monster.cpp:113` |
| 采样类 eval 先例 | `sampling-anti-monopoly.yaml` | `eval/cases/rng/sampling-anti-monopoly.yaml` |

**口径声明**：候选数与校验以 **retail 数据**（`gbIsSpawn == false`）为准；spawn 下 `Retail` 怪被排除（`:3166`），故校验走**宽松模式**（放宽可用性/白名单/floors；类型存在性与 core 非空仍强制，见 §4.4.4）。

## 4. 方案

### 4.1 数据模型（**逐层**）

`assets/txtdata/monsters/level_rosters.tsv`（基础表，L1-16）

```
level   monster_id     role    allow_unique_boost
1       MT_WSKELAX     core    -
1       MT_NZOMBIE     core    -
3       MT_RSKELAX     core    -        # A1 狂乱承载
3       MT_XSKELAX     core    -        # A1 冲锋承载
9       MT_BMAGMA      core    -        # A3 承载
```

`assets/txtdata/monsters/level_roster_params.tsv`

```
level   max_image   tail_draw   class_floors          squad_chance   squad_size
1       6000        2           Melee=2,Ranged=1      30             2
9       16000       3           Ranged=1              30             2
13      18000       3           Melee=1,Ranged=1      35             2
```

`mods/hf/txtdata/monsters/level_rosters.tsv` + `level_roster_params.tsv`（HF overlay，L17-24；基础表不得引用 HF 怪物）。

- `max_image`：逐层安全上限（取代全局 4000）。**初值依据**：L13 最重 6 种 core 合计 13406；洞穴 core 5 种 7220 + 尾池 3 种 → 需 ≈14000 量级。内存不是约束（作者已判定），故留余量取 16000/18000。
- `tail_draw`：上限语义（实际 = `min(tail_draw, 可用候选数)`，为 0 合法）。
- `class_floors`：行为类别下限（与 B1 caps 合成配额）。
- `squad_chance` / `squad_size`：**阶段 B** 才生效；初值待 T1 基线后校准。
- `allow_unique_boost`：白名单列（见 §4.4.3）。

### 4.2 阶段 A：采样接入（`GetLevelMTypes()`，`Source/monster.cpp:3439+`）

1. 既有预加不变（`MT_GOLEM`、任务 unique、SKING 层骷髅）。
2. **core 预加**：按 `currlevel` 查名册，对每个 core 成员 `AddMonsterType(type, PLACE_SCATTER)`；绕过 caps（caps 只在循环内）但**计入预算**。
3. **尾池**：`typelist` = `IsMonsterAvailable` 候选 **减去已加入的 core**。
4. **尾池抽取显式化**：最多成功抽取 `tail_draw` 次。**无名册参数行的层退回旧行为**（预算受限、尾池不设上限）——当前 L17-24（Nest/Crypt）属此类，阶段 A2 补 HF overlay 表后纳入；若退回 `tail_draw = 0`，这些层的采样循环将恒不执行、散布怪物消失（回归）。
5. **配额**：B1 caps 保留；新增 floors，抽取时优先补足未满足的类别。
6. **预算**：`monstimgtot < max_image(当前层)`；无参数行时退回 4000（旧值）。
7. **L16 特例**：保持硬编码分支不变；名册表只登记不改行为。

### 4.3 阶段 B：核心小队（`Source/monster.cpp:3773-3782`）

**前置条件（必须先做，且各自独立验收）**

1. **修 G1（普通 leader 死亡必须释放随从）**：`M_UpdateRelations`（`:4112-4117`）当前以 `hasLeashedMinions()` 为入口，普通 leader 恒 false。改为**按随从侧扫描**判定：存在 `leaderRelation == Leashed && getLeader() == &monster` 的随从即执行释放；对**非 unique** leader 额外**清除 `leader` 索引**（`setLeader(nullptr)` 按设计保留索引以维持血条着色，普通怪槽位会被 `AddMonster` 复用，故必须清）。unique 路径行为**保持不变**。
   验收：`LeaderDeathReleasesMinions`（普通 leader 死亡后其随从 `leaderRelation != Leashed` 且 `leader` 索引已清）、`UniqueLeaderDeathBehaviourUnchanged`（回归）。
2. **决 G2（`setLeader` 覆写 AI）**：`setLeader`（`:4934-4944`）会把随从 `ai` 换成 leader 的 AI。核心小队**必须不覆写 AI**，否则"远程随从挂到近战 leader 下变近战"会使 class mix 统计与事实不符（红线 14 失效）。实现方式：`PlaceGroup` 新增 `MinionOptions` 参数（R2 建议 G7），核心小队传 `{tough=false, inheritAi=false, inheritIntelligence=false}`，默认值保持既有行为（unique 路径不变）。
   验收：`SquadMinionsKeepOwnAi`、`UniqueMinionsInheritLeaderAi`（回归）。

**小队机制（前置完成后）**

3. 散布循环中，若抽到的类型是 core 成员，以 `squad_chance` 概率组队：
   - **前置条件**：剩余槽位 ≥ `1 + 1`，否则降级为单类型组；
   - 记录 `before = ActiveMonsterCount`，执行 `PlaceGroup(leaderType, 1, nullptr, false, opts)`；若 `ActiveMonsterCount == before`（被钳制成 0）则**放弃小队**并降级（否则 `&Monsters[ActiveMonsterCount-1]` 会指向上一只无关怪）；
   - 从**同一层其他 core 成员**中抽一个 partner，`PlaceGroup(partnerType, squad_size, leader, /*leashed=*/true, opts)`；
   - `opts = {tough=false, inheritAi=false, inheritIntelligence=false}` → 小队**只买构成，不买数值/AI**。
4. **回退**（R2 修正）：若某层段实测小队形成率低于阈值，该层段改用 `leashed=false` 但**仍传 leader**（`:322-327` 会围绕 leader 选取初始位置 → 相邻放置有保证），只是不设 `setLeader`/`packSize`、无归队语义。
5. 显式接受的既有语义：leashed 随从受 4 格约束（毁灭后会自然解体再重聚）；leashed 放置失败 10 次后放弃；按 `totalmonsters` 钳制。

### 4.4 加载期校验（拒绝启动而非静默降级）

1. 每行 `monster_id` 必须在**当前加载的数据集**中存在；区间行须满足"区间内每层都可用"。
2. `class_floors` 必须在该层候选池 + B1 caps 下**可满足**，否则报错并指出冲突项。caps 取 L9-12 `RangedKite ≤ 2`、L13-16 任意类别 `≤ 2`（**注**：L16 因硬编码分支提前 return，cap 在 L16 实际不可达，见 §4.2.7）、其余无上限，**抽成单一真相源** `BehaviorClassCapForLevel(level, class)`（`level_roster.h` 导出）；校验器与采样循环（§4.2.5）共用，避免两处漂移。
3. **unique 门控检查**：若 `(level, monster_id)` 的 base 类型等于**同层**某 unique 的 `type`，则必须显式列入 `allow_unique_boost`，否则**拒绝启动**（`:509-512` 会因 core 而提高该 unique 出现率；本规格默认保持稀有性不变）。
4. spawn 数据（`gbIsSpawn == true`）下走宽松模式：只放宽三项（逐行可用性、unique 白名单、floors 可满足性）；**类型存在性检查在两种模式下共用**；并对 `entries ∪ params` 中出现的**每个 `level`** 一律要求至少 1 个 core 成员（比"有名册行"或"有参数行"任一驱动都严格）。

### 4.5 行为类别口径（红线 14 的载体）

- 散布按**类型**均匀抽取（`:3774`），故"类型数 3 → 7-9"会重排近战/远程配比。
- **阶段 A 第一步必须实测改动前的 placed class mix**（按行为类别统计**已放置怪物数**，非类型数）作为基线；验收以**相对基线**表述（"远程类占比 ≤ 基线 +5 个百分点"）。
- **阶段 B 的 `inheritAi=false` 保证"类型 = 实际 AI"**，使该口径成立（这是 G2 必须解决的第二个理由）。
- `class_floors` 只用于**补足**，不作为提高远程占比的手段。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 分类 | Expansion（§2） |
| 2 | 具体症状 | §1（P0 实测：L13-15 单层 3.2-3.3 种、Σ`image` 3523-3889/4000） |
| 3 | 数值有出处 | §3 每行 `文件:行号` / TSV 列 |
| 4 | 「已实施」定义 | 状态为草案，未声明已实施；新表查询函数由 `GetLevelMTypes`（生产）调用 |

### 扩充层红线

| # | 判定 | 依据 |
|---|---|---|
| 9 | 非膨胀数值 | 名册只改构成；阶段 B 的小队传 `MinionOptions` **全部 false**（不 HP×2、不继承 AI、不继承 intelligence）→ 无任何数值或 AI 强化 |
| 10 | 反制且指名 | 不新增怪物/攻击手段；既有反制不变；唯一新压力是远程配比，由 §4.5 基线与 `class_floors` 控制 |
| 11 | 取舍非负担 | 玩家侧无新规则、无新概念 |
| 12 | 全层段定义 | 基础表 L1-16 + HF overlay L17-24 + L16 特例登记；§4.4 校验 |
| 13 | 与既有系统互动 | 复用 B1 `classCounts`/caps、`PlaceGroup`、leashed/分离重聚；与 A1/A3 共用 `BehaviorClass` |
| 14 | 近战/远程分别评估 | §4.5 方法（placed class mix 基线 + 相对阈值）；**不主张"总数不变所以压力不变"**（已被 R1 否证）；阶段 B 靠 `inheritAi=false` 保证统计口径真实 |

### 固定问句（2026-09-16 新增，源自阶段 B 的 S2 漏检）

**本改动是否改变任何"玩家可见的标识 / 提示 / 着色 / 音效"语义？** —— 若改变，必须：①在验收里加一条可核项；②在规格里写明新语义；③给出无头可测的谓词。历史上验收全在量"放置构成与数值"，导致"小队随从被染成蓝名（暗示被强化，实际没有）"这类**语义漂移**漏检。

### 禁令

1 未实现内容标注状态 ✓；2 无不可落地表格 ✓；3 数值有出处 ✓；4 不新增玩家必记概念 ✓；5 红线逐条给依据 ✓；6 不用占位/永真测试 ✓（R2 G3 已删除永真验收）；7 不改行尾 ✓（`.md` LF；TSV 新增按 `.editorconfig`）

## 6. 验收标准

| # | 验收 | 方式 |
|---|---|---|
| 1 | 每层 core 成员 200 seeds 上 **100% 出现**，且均为该层候选 | `SamplingBaselineTest.RosterCoreAlwaysPresent` |
| 2 | 尾池抽取数 = `min(tail_draw, 可用候选数)`；候选为 0 合法 | `RosterTailDrawBounded` |
| 3 | 每层 floors 满足且 B1 caps 未破 | `RosterQuotasSatisfied` |
| 4 | 非法名册行被**拒绝**（怪不存在 / 区间内某层不可用 / floors 不可满足 / core 为 unique base 且未列白名单） | `RosterValidationRejectsInvalidRow`（合成表驱动校验函数） |
| 5 | §4.3 前置 1（G1）：普通 leader 死亡释放随从且清除悬挂索引；unique 路径不变 | `LeaderDeathReleasesMinions`、`UniqueLeaderDeathBehaviourUnchanged` |
| 6 | §4.3 前置 2（G2）：小队随从 `ai` 等于其类型 AI、HP 不翻倍、intelligence 不继承；unique 路径仍继承 AI 且 HP 翻倍 | `SquadMinionsKeepOwnAi`、`SquadMinionsUnbuffed`、`UniqueMinionsBehaviourUnchanged` |
| 7 | **小队形成率**：T1 先实测各层段"尝试成队中至少放下 1 只随从"的比例，**阈值由实测导出**并写入实现计划；低于阈值者改用 §4.3.4 回退 | `SquadFormationRate`（阈值参数化） |
| 8 | placed class mix（按类别的怪物数）相对改动前基线：远程类 ≤ 基线 +5 个百分点 | `PlacedClassMixWithinBaseline`（基线由 T1 产出，记录在计划与提交信息） |
| 9 | 身份守卫：相邻层名册 Jaccard < 0.9 | `IdentityGuard`（P0-D 度量） |
| 9b | **多样性守卫（2026-09-15 最终评审新增，R38）**：L2-15 每层的**每 seed 组合数 ≥ 2**（500 seeds 实测；L1 因候选池仅 6 个允许例外，须在用例内注释说明） | `RosterPerSeedVariety` |
| 9c | **unique 可达性守卫（R38）**：`mlevel ∈ 13-15` 的每个 unique，其 base 类型必须在至少一个 seed 的 realized 集合中出现（地狱段强制；本条是 S1 回归的直接守卫） | `HellUniqueBasesRemainReachable` |
| 10 | A1/A3 的变体出现在其所属层的 core 中 | `A1A3VariantsAreCore` |
| 11 | eval + 全量门禁 + 夹具 + 台账 + 协议声明 | `eval/cases/rng/level-rosters.yaml`、`python3 -m tools.eval.backend --smoke`、`python3 tools/run_tests.py --json /tmp/ci.json`（100%/0/drift ok）、`timedemo` 等按决策 35 重生成、`docs/knowledge/decision_save_format_policy.md` 记账；**明确声明：名册改变 `DSpawnedMonster.typeIndex` 的含义 → 不同版本不可互联（不提供跨版本兼容）** |

## 7. 状态

**实施中**（2026-09-15 更新）。

- **阶段 A（逐层名册）已交付**：计划 `plans/2026-09-15-level-rosters-phase-a.md` 的 4 个任务全部完成，本地门禁 749/0/100% + drift 5/5，远端 CI 绿（`better-d1-ci.yml`）；评审链：任务评审 ×5、范围化复审 ×7、最终全分支评审（裁决"修复后可合并"）。
- **阶段 B（核心小队）实施中**：计划 `plans/2026-09-15-level-rosters-phase-b.md`；前置 G1、G2 已完成（G1 已过 CI；G2 已提交待评审），小队机制与形成率守卫待做。
- **未完成部分（原草案遗留，均为既定范围）**：阶段 A2（HF overlay 的 L17-24 名册表——**红线 12「全层段定义」因此目前只对 L1-16 + L16 特例成立**，L17-24 仍走 R28 legacy）；附录 E 第 1/2 条的阶段 B 待决项（地狱段多样性与 O2 校验语义）。

---

## 附录 A：复核发现的处置

### 第 1 轮（F1-F9，R1）

| # | severity | 发现 | 处置 |
|---|---|---|---|
| F1 | blocker | 层段 core 与逐层可用性冲突（覆盖整段 = 0 种） | 已解决：改为**逐层**（§4.1）+ 区间校验（§4.4.1） |
| F2 | blocker | "总数不变→压力不变"不成立 | 已解决：删除该论证；§4.5 用 placed class mix 相对基线；红线 14 重写 |
| F3 | high | unique 被采样门控，core 会**提高**其出现率 | 已解决：§4.4.3 默认拒绝 + `allow_unique_boost` 白名单 |
| F4 | medium | 数值矛盾（L13 最重 6 种 13406 > 12000；Nest/Crypt 尾池恒空） | 已解决：`max_image` 逐层参数；`tail_draw` 改 `min(...)` |
| F5 | medium | core 预加挤爆 caps；`13-16` 应拆 | 已解决：拆为 `13-15` + L16 特例；§4.4.2 可满足性校验 |
| F6 | medium | RNG 流共享 + `typeIndex` 过网络属协议级 | **R2 判"表面解决"** → 本次补实：§6 验收 11 明确声明不提供跨版本兼容；§3 记录两处事实 |
| F7 | medium | 8 条验收仅 5 条可落地 | **R2 判"残留一条永真"** → 本次删除 `SquadMinionsKeepSpecial`（G3），形成率阈值改为"实测导出"（G4） |
| F8 | high | A 单独交付不出体感改善 | 已解决（作者选择方案 ②）：核心小队并入本规格，作为**阶段 B** |
| F9 | medium | 漏掉"收缩 `minDunLvl/maxDunLvl`"替代方案 | 已解决：附录 B 比较，结论是**配套**非替代 |

### 第 2 轮（G1-G7，R2）

| # | severity | 发现 | 处置 |
|---|---|---|---|
| G1 | **blocker** | 非 unique leader 死亡不释放随从 → 悬挂 `leader` 索引 + 槽位复用 | 已采纳为**阶段 B 前置 1**（§4.3.1），含实现设计与两条验收（5）；unique 路径回归受保护 |
| G2 | **blocker** | `setLeader` 覆写随从 AI → class mix 统计口径失真 | 已采纳为**阶段 B 前置 2**（§4.3.2）：`MinionOptions` 关掉 AI 继承；验收 6 新增 `SquadMinionsKeepOwnAi` |
| G3 | high | `SquadMinionsKeepSpecial` 是永真断言（`ALLOW_SPECIAL` 全表 0 次） | 已解决：删除该验收；§3 记录该 flag 清理为 no-op |
| G4 | high | 50% 形成率无依据、缺基线要求；`leashed=false` 回退不保证相邻 | 已解决：阈值改为**实测导出**（验收 7）；回退语义修正为"仍传 leader、只关 leashed"（§4.3.4，`:322-327` 保证相邻初置） |
| G5 | medium | §3 归队门控表述未限定 unique；解体无验收 | 已解决：§3 修正为"只对 unique leader 生效"；§4.3.5 显式接受"解体再重聚" |
| G6 | medium | `packSize` 读档重建只对 unique+Scavenger | 已解决：§3 更正事实；§4.3 不再声称普通小队 `packSize` 会被重建（阶段 B 需在 T8 处理存档语义） |
| G7 | low | 建议 `PlaceGroup` 第 5 参数改为 `MinionOptions` 标志结构 | 已采纳（§4.3.2） |
| — | — | 建议拆两个实现计划 | 已采纳：本规格分**阶段 A / 阶段 B**（见文档头） |

## 附录 B：替代方案比较（F9）

| 方案 | 引擎改动 | 能否保证 core 可见 | 对多样性 | 结论 |
|---|---|---|---|---|
| 只收缩 `minDunLvl/maxDunLvl` | 零 | **不能**（仍随机抽样） | 降低 | 作为名册**配套**数据动作 |
| 只做配额 | 小 | 不能 | 不变 | 只修 kite 垄断 |
| 只做编组 | 小（但触发 G1/G2） | 不适用 | 不变 | 信息量最大，但缺"谁是招牌"的语义基础 |
| **本规格（阶段 A + B）** | 中 | **能** | 提高 | 先 A 后 B；B 必须先修 G1/G2 |

## 附录 C：core 点名提案（初稿，待作者调整）

| 层 | core 提案 | 逻辑 |
|---|---|---|
| L1-2 | `MT_WSKELAX`/`MT_WSKELSD` + `MT_NZOMBIE` + `MT_RFALLSP` + `MT_TSKELBW` | 教堂开场：贴身 + 群怪 + 远程 |
| L3-4 | **`MT_RSKELAX`（A1 狂乱）** + **`MT_XSKELAX`（A1 冲锋）** + `MT_RSKELBW` + `MT_BFALLSP` | A1 变体进核心 |
| L5-8 | `MT_FAT`→`MT_MUDMAN` + `MT_BGOATMC`/`MT_RGOATBW` + `MT_STALKER`/`MT_UNSEEN` + `MT_NACID` | 墓穴的"重、缠、阴" |
| L9-12 | `MT_MUDMAN`/`MT_TOAD` + `MT_GGOATBW` + **`MT_BMAGMA`（A3）** + **`MT_STORML`（A3）** | 洞穴的压迫 + 拉扯 + 条件冲锋 |
| L13-15 | `MT_GSNAKE` + `MT_BALROG` + `MT_HLSPWN`/`MT_SOLBRNR` + `MT_CABALIST`/`MT_MAGISTR` + **`MT_STORML`（A3@13）** | 地狱的数值压迫 + 远程齐射 |
| L16 | 硬编码（Advocate/RBlack/Diablo）→ 只登记 | 保持终局 |

**约束**：每层 4-5 只为初稿目标；**实测最终值**（2026-09-15，`level_rosters.tsv`）：L1 4、**L2 7**、**L3 6**、L4 5、L5-L7 各 4、**L8 6**、L9-L12 各 4、**L13 2、L14 3、L15 3**、L16 4。L2/L3/L8 超过初稿是为把远程占比压回 R4 带内（见附录 E）；L13-15 低于初稿是"B1 cap ≤2/类 + L13 低基线"挤压的结果（见附录 E 第 1 条）。仍须满足：含该层 A1/A3 承载；不得含 unique 的 base（除非白名单）；`Σimage` 低于逐层 `max_image`。

## 附录 D：玩家可感预测（实现成败判据，来自 R2）

1. 同一层连进三次，都能遇到同一批 core 怪。
2. 地狱 13-15 单局可见种类从 3.2-3.3 升到 ≥6，且玩家能说出怪名。
3. 能看到"一只 + 1-2 只贴身的同/异种怪同行"（小队）。
4. 随从被击杀所需攻击次数与野外同类怪**相同**（无强化）。
5. 把 leader 引开 >4 格，leashed 随从脱队；靠近后重新贴回。

**若实现后 1/2/4 未出现 → 规格失败；3/5 未出现 → 阶段 B 的小队卖点失败（回到阶段 A 的交付水平）。**

## 附录 E：阶段 B / A2 待决问题（本规格记录，不在阶段 A 范围）

1. **地狱段名册曾退化为"零随机性"（最终评审核正后的严重度，2026-09-15）**：为压住远程占比 ceiling，任务 4 的 R4 数据调整把 L13/L14/L15 压成**每 seed 完全相同的固定组合**（500 seeds 实测各 1 种组合；改动前 20-175 种），并因 `PlaceUniqueMonsters` 要求 base 已在池中而使 **12 只 unique 永久不可达**（L13 5/9→0、L14 6/6→0、L15 2/3→0）。根因：L15 四个 core 把两个类别双双顶到 cap 2（尾池被清空）、L13/L14 `tail_draw=1` 且 floors 在 RNG 之前取第一个匹配项。**裁决更正（R37）**：原文"物种数 4-5 低于目标"低估了严重度；R32/R34 据此撤销/调整——采纳 L14 `tail_draw=2`（实测 0.5964 < ceiling 0.60876，余量 1.2pp 值得为恢复随机性支付），并新增 §6 的多样性守卫（9b）与 unique 可达性守卫（9c）。可选出路仍是：① 放宽 cap；② 提高占比上限；③ 扩充 Melee 池；④ 阶段 B 编组层。
2. **O2**：加载期校验目前只看 core 是否超 cap，未把"quest 无条件预加的 base"计入合计；L13 的实际组合仍可能破 cap。属 §4.2.2 预算语义问题，留阶段 B。
3. **阶段 B 前置**：G1（非 unique leader 死亡不释放随从 → 悬挂索引）、G2（`setLeader` 覆写随从 AI）必须先修，见 §4.3。
4. **阶段 A2**：HF overlay 的 L17-24 名册表；L17-24 现走 R28 legacy fallback（保持旧行为，无身份）。
5. **`tail_draw` 存在隐式上界 ≈4（跨任务耦合，2026-09-15 合并前复审记录）**：`test/sampling_behavior_test.cpp` 的 `HellfireNoParamsSamplingTest.NoParamsTailExceedsTheParameterisedCap` 以"无参数层的散布类型数 > 表内最大 `tail_draw`"来量化"不设上限"，因此把某层的 `tail_draw` 提到 ≥5 会让该守卫失败。将来确有需要时，应**重新论证该守卫的表述**（例如改成比较"是否达到候选上限"），而不是放宽它。
7b. **小队形成率实测与阈值（2026-09-16，Task 4）**：出厂表（`squad_chance=30`，L14=10）**500 seed/层**实测形成率 **99.890-100%**（最差 L10 99.890），统一 floor **0.95**（用例内具名数组 `kSquadFormationFloor`，按层索引、L16 为 unconstrained 哨兵；注释列全 15 层实测值与余量说明，并声明"**实测分隔线、非紧界、非规格常量**"）；逐层实测值无条件打印 `[ SQUADFORM ]`。**无任何层需要 §4.3.4 回退**，`level_roster_params.tsv` 零改动。分子用**leash 无关**的 `SquadRollCounters::formed`（若用带 `squadLeashed && packSize>0` 的 `realised`，任何采用回退的层会恒读 0% —— 即守卫因它自己要求的补救而变红）。运行时：该用例 500seed×15层 ≈290s（该二进制 ~475s），eval case `timeout: 900` 余量不宽，**再加长用例须同步上调**。

7. **小队回退邻近性守卫的界与三个参数绑定（2026-09-16，RB18）**：`SquadPlacementTest` 的邻近性上界取 **32**（实测分隔线：出厂路径 max 17 / 去掉 leader 后 max 66；代码可证的 101 不可失败，因散布区 `[16,96)` 内任意两格距离 ≤80）。该界与 `PlaceGroup` 的 `try2` 上限、散布区范围、`squad_size` **绑定**——任一变化都必须重新实测并复核此界（用例无条件打印 `[ SQUADFALLBACK ]` 便于发现漂移）。
6. **既存缺陷（非本规格引入，建议单独开单）**：`Source/levels/drlg_l2.cpp:2072`（`FixTilesPatterns` 一带）存在未加边界的 `dungeon[i][j+1]`/`dungeon[i+1][j]` 访问，在 `j+1 == DMAXY` 时越界；最后一次改动 `53b91fd7a`。合并前复审的全量日志中有 1 条 UBSan 记录，是在**抬高 L8 的 `max_image` 后曝光**（非本功能引入）。
5. **L14 `tail_draw` 1 → 2 的替代解（任务 4 评审实测，裁决 R34 暂不采纳）**：把 L14 的 `tail_draw` 从 1 提到 2，远程占比实测 **0.5964**，仍低于 ceiling **0.60876**，物种数 **4 → 5**，零代码改动、全部测试绿——即阶段 A 的"L14 只有一个数据解"并非绝对，只是"在 ≥5pp 余量标准下唯一"。控制器裁决 R34 **本阶段不采纳**，理由：① 该解余量仅 **1.2pp**（现状 11.7pp），会让 `PlacedClassMixWithinBaseline` 这道守卫变脆，一次无关的采样微调就可能把它推翻；② 与阶段 A 内部一致性冲突——任务 4 曾以"太贴边"为由否掉过同等余量的 `MT_BALROG` 方案，采纳这个会自相矛盾；③ R32 精神是"阶段 A 守卫优先，多样性归阶段 B"。**阶段 B 需要决的取舍**：是否值得用 1.2pp 的守卫余量换 L14 一个物种？若阶段 B 按第 1 条的出路①/②放宽了 cap 或占比上限，本项自然消解（余量重新变宽后 `tail_draw=2` 不再贴边）；若阶段 B 决定守卫余量必须 ≥5pp，则本项应正式关闭为"不采纳"。
