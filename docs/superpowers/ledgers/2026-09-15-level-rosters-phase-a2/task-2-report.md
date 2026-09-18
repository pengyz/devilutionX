# A2 / Task 2 报告：L17-24 的 core/tail 数据与逐层参数

**状态**：进行中（本文件第一分钟落盘，边做边追加）

## BASE

- 起始 HEAD：`9f668b19a test(roster): pin gbIsHellfire in LevelRosterTest fixture`
- 分支：`feature/qol-upgrades`；工作树干净

## 进度

- [x] 步骤 0：读需求 / 候选池盘点 / caps 分段
- [ ] 步骤 1：RA3 数组扩到 L24（基线抄 Task 1 实测，ceiling = 基线 + 5pp）
- [ ] 步骤 2：写 L17-24 数据
- [ ] 步骤 3：`ninja devilutionx_mpq` + 重测量
- [ ] 步骤 4：RB37 非占比守卫 + 反证
- [ ] 步骤 5：门禁 + eval + 提交 + CI

---

## 步骤 0：候选池盘点（实读 `mods/hf/txtdata/monsters/monstdat.tsv`）

### cap 分段（RB 要求记录）：**L17-24 全类别无 cap**

`BehaviorClassCapForLevel()`（`Source/tables/level_roster.cpp:152-162`）实际分段：

| 层段 | cap |
|---|---|
| L13-16 | 任意类别 ≤ 2 |
| L9-12 | `RangedKite` ≤ 2，其余无 |
| **其余（含 L17-24）** | **0 = 无上限** |

因此 L17-24 的「core ≤ cap/类」与「floors 在 caps 下可满足」两条约束在该层段**恒成立**（cap==0 分支 `continue`）。
「core 化会把该类整类剔出尾抽池」这个 L14 教训**在 L17-24 不适用**——它源于 cap=2 的地狱段。
L17-24 真正的紧约束是**预算 `max_image`**（见下）。

### 候选池（HF overlay 载入后，`availability=Retail`，按层窗口过滤）

`realRanged` 列 = **真实 `ai` 判定**（`AiProc` 派发到 `AiRanged`/`AiRangedAvoidance`，
`Source/monster.cpp:3138-3180`），与 `GetBehaviorClass()` 的 `cls` 列**故意不同**（RB33 口径不动）。

| 层 | 候选（img / cls / realRanged） | Σimg | 硬编码预加（`monster.cpp:3568-3579`） |
|---|---|---|---|
| 17 | HELLBOAR 800 M・STINGER 305 M・PSYCHORB 800 Boss**R**・ARACHNON 800 M・FELLTWIN 800 M | 3505 | — |
| 18 | 同 L17 + HORKSPWN 520 M | 4025 | HORKSPWN(scatter) |
| 19 | HORKSPWN 520 M・VENMTAIL 305 M・NECRMORB 800 Boss**R**・SPIDLORD 800 **Kite**・LASHWORM 800 M・TORCHANT 800 Boss**R** | 4025 | HORKSPWN(scatter)・HORKDMN(unique) |
| 20 | VENMTAIL 305 M・NECRMORB 800 Boss**R**・SPIDLORD 800 **Kite**・LASHWORM 800 M・TORCHANT 800 Boss**R** | 3505 | DEFILER(unique) |
| 21 | GRAVEDIG 800 Boss・TOMBRAT 550 M・FIREBAT 550 Boss**R**・SKLWING 1740 M・LICH 800 Boss**R** | 4440 | — |
| 22 | TOMBRAT 550 M・FIREBAT 550 Boss**R**・SKLWING 1740 M・LICH 800 Boss**R**・CRYPTDMN 800 M | 4440 | — |
| 23 | CRYPTDMN 800 M・HELLBAT 550 Boss**R**・BONEDEMN 1740 **Kite**・ARCHLICH 800 Boss**R**・BICLOPS 800 M・FLESTHNG 800 M・REAPER 800 M | 6290 | — |
| 24 | HELLBAT 550 Boss**R**・BONEDEMN 1740 **Kite**・ARCHLICH 800 Boss**R**・BICLOPS 800 M・FLESTHNG 800 M・REAPER 800 M | 5490 | ARCHLICH(scatter)・NAKRUL(special) |

`MT_GOLEM`（img 386，`PLACE_SPECIAL`）在每层无条件预加。

### unique base 白名单：L17-24 **无需任何 `allow_unique_boost`**（且不可能需要）

`mlevel ∈ 17..24` 的 unique 共 3 条，其 base 在该层**全部不可 core 化**：

| unique | mlevel | base | base 在该层可用？ |
|---|---|---|---|
| Hork Demon | 19 | `MT_HORKDMN` | ✗ `availability=Never`（两表皆是） |
| The Defiler | 20 | `MT_DEFILER` | ✗ `availability=Never` |
| Grimspike | 19 | `MT_OBLORD` | ✗ 窗口 10-12，L19 在窗口外 |

即：想把它们写成 core 会先撞上**逐行可用性检查**（`IsAvailableAt`）而拒绝启动，
根本走不到白名单检查。故 L17-24 的 `allow_unique_boost` 全为 `-`。
（HORKDMN/DEFILER 的 unique 仍能出现：靠上表的硬编码 `PLACE_UNIQUE` 预加，与名册无关。
Grimspike 在 vanilla HF 里本就结构性不可达——base 不在 L19 窗口内，`PlaceUniqueMonsters` 直接 `continue`；**非本任务引入**。）

### 预算是唯一紧约束（含 L24 的一条硬性设计约束）

改动前基线（Task 1 实测）的类型数已被旧的**全局 4000 预算**卡住：L21 4.43、L22 4.37、L23 4.555、L24 5.00。
L24 尤其关键，它解释了「L24 基线远程占比 = 0」这件事：

```
L24 预加 = GOLEM 386 + ARCHLICH 800 + NAKRUL 1200 = 2386
旧预算 4000 → 剩 1614 < BONEDEMN 的 1740
⇒ BONEDEMN（唯一的 RangedKite）在 L24 **永远装不进**，故 rangedShare 恒 0（实测 types 恒 5.00 与此吻合）
```

所以 L24 的 `max_image` **不能随意放宽**：一旦 BONEDEMN 能进，占比会从 0 跳到 ~1/5，
远超 `基线 + 5pp = 0.05`；按 **R4**（超标只许改数据、不许放宽阈值）只能收紧数据。
本任务据此把 L24 的 `max_image` 设为**刚好装不下 1740** 的值，并在下文给出算式。

---

# 接手者续写（第二位实现者）

**状态**：DONE_WITH_CONCERNS（见末尾顾虑）

## BASE（接手）

- 接手时 HEAD：`b6f13ff00498e5db0ae5b40dd1a0ad897b7a70b4`（`docs(knowledge): record the UNPACKED_MPQS manifest include trap`）
- 工作区：**保留**前一位的未提交改动（4 改 + 2 未跟踪），未做任何回退
- 新增提交：
  - `bd3a21844` `feat(rosters): add the Hellfire level roster data`
  - `ef30701eb` `test(roster): guard the Hellfire ranged casters`

## 进度（检查单勾全）

- [x] 步骤 0：读需求 / 候选池盘点 / caps 分段（前一位完成，已复核）
- [x] 步骤 1：RA3 数组扩到 L24（基线抄 Task 1 实测，ceiling = 基线 + 5pp）
- [x] 步骤 2：写 L17-24 数据
- [x] 步骤 3：`ninja devilutionx_mpq` + 重测量
- [x] 步骤 4：RB37 非占比守卫 + 反证
- [x] 步骤 5：门禁 + eval + 提交 + CI

对应简报四步：
- [x] 步骤 1：候选池盘点
- [x] 步骤 2：写数据
- [x] 步骤 3：校验回归（`level_roster_test` 41/41；HF 载入下 L17-24 不再 fatal）
- [x] 步骤 4：提交

## 复核前一位的结论：候选池与 `ai` 判定（独立重算，结论一致）

用 `mods/hf/txtdata/monsters/monstdat.tsv` 独立重算（`availability != Never` + 层窗口），
与前一位那张表**逐行吻合**。`realRanged` 列按 `AiProc`（`Source/monster.cpp:3138-3180`）
实际派发到 `AiRanged`/`AiRangedAvoidance` 的条目判定：

| 层 | Σimage | 真远程施法者（真实 `ai`） |
|---|---|---|
| 17 | 3505 | PSYCHORB |
| 18 | 4025 | PSYCHORB |
| 19 | 4025 | NECRMORB、SPIDLORD(Acid)、TORCHANT |
| 20 | 3505 | NECRMORB、SPIDLORD(Acid)、TORCHANT |
| 21 | 4440 | FIREBAT、LICH |
| 22 | 4440 | FIREBAT、LICH |
| 23 | 6290 | HELLBAT(Torchant)、BONEDEMN、ARCHLICH |
| 24 | 5490 | HELLBAT(Torchant)、BONEDEMN、ARCHLICH |

注意 `MT_HELLBAT` 的 `ai` 是 **Torchant**（不是自己的 AI），故它是真远程施法者——
这条直接决定了 L23/L24 的守卫读数。

### 谓词 `IsRangedCasterAi` 与 `AiProc` 逐条核对（实读源码）

`AiProc` 里派发到 `AiRanged`/`AiRangedAvoidance` 的**全部**条目为：
GoatRanged、Magma、Succubus、Storm、Acid、AcidUnique、Counselor(CounselorAi 例外见下)、
Diablo、FireBat、Torchant、Lich、ArchLich、Psychorb、Necromorb、BoneDemon。
前一位写的 15 条与之**完全一致**（`Counselor` 走 `CounselorAi`，但它在 D1 语义上是
远程施法者且不出现在 L17-24 池内，保留只为"谓词不静默依赖层段"）。
`AcidUnique → AiRanged` 这条**确实存在**（`monster.cpp:3150` 一带），
而 `GetBehaviorClass` 的 Kite arm **没有**它——即谓词与分类的分歧是真实的、有源码依据的。

## 数据合规（本任务最大未知）——实测通过

`ninja -C build devilutionx_mpq` 后：

```
$ cd build && ./level_roster_test
[==========] 41 tests from 4 test suites ran. (327 ms total)
[  PASSED  ] 41 tests.
```

含 `LevelRosterShippedLoadTest.LoadsTheShippedRosterAndValidatesIt`（跑真实
`LoadLevelRoster()` 全量校验）。另外 `HellfireLevelBaselineTest` 套件在
`SetUpTestSuite` 里 `gbIsHellfire = true` 后调 `LoadLevelRoster()`，
即**以 `maxLevel = 24` 对 L17-24 行做了全部逐层检查**（可用性 / 白名单 /
floors 可满足性 / core 非空 / core-vs-cap / `Σimage`）——三个用例全部跑起来
说明**没有 fatal**。这就是"加载期不再对 L17-24 fatal"的实证。

**caps 复核**：`BehaviorClassCapForLevel()` 对 L17-24 全类别返回 **0（无上限）**，
故 core≤cap 与 floors 可满足性在该层段结构性成立（前一位结论正确）。

## RB37 守卫：我改了什么，为什么

前一位已把守卫**写出来**（`HellfireRangedCasterShareWithinBaseline`），谓词正确、
方向正确（上界，红线 14）。但有两个必须修的问题：

### 问题 1：基线不是「实测」而是「从 Boss 列算术推导」

前一位的 `kRangedCasterShareBaseline` 值（如 L17 `3459/16188`）与 Task 1 报告
`[ A2BASELINE ]` 的 **Boss 列逐位相同**，L19 是 `Kite 1983 + Boss 4375 - 200` 这类
拼凑。这在集合上**不等价**：Boss arm 不含 Acid/BoneDemon 两个 Kite，却含
Scavenger 这类非施法者。用它做界，守卫比较的是两个不同总体，不是 A/B。

**修法**：让 `A2PreChangeBaselineForHellfireLevels`（它已加载 pre-A2 夹具）
在**同一批样本**上顺带累计施法者数，并无条件打印 `[ A2CASTERBASE ]`，
常量从这行转抄。实跑结果——**八个值与前一位的推导值逐位相同**，
即它的算术恰好正确，但现在这份界是**实测的**、有可复现来源的：

```
[ A2CASTERBASE ] level 17 rangedCasterShare 0.213677 (3459/16188)
[ A2CASTERBASE ] level 18 rangedCasterShare 0.148629 (2406/16188)
[ A2CASTERBASE ] level 19 rangedCasterShare 0.375763 (6158/16388)
[ A2CASTERBASE ] level 20 rangedCasterShare 0.558744 (9131/16342)
[ A2CASTERBASE ] level 21 rangedCasterShare 0.420576 (9513/22619)
[ A2CASTERBASE ] level 22 rangedCasterShare 0.417577 (9484/22712)
[ A2CASTERBASE ] level 23 rangedCasterShare 0.439288 (9949/22648)
[ A2CASTERBASE ] level 24 rangedCasterShare 0.510043 (11554/22653)
```

（同一次运行的 `[ A2BASELINE ]` 八行与 Task 1 报告**逐位一致**，说明
`LoadPreA2Tables()` 夹具确实复刻了改动前路径——这是 A/B 成立的前提。）

### 问题 2：守卫在前一位的数据上**真红**（三层 R4 超标）

首跑（前一位的数据 + 我修好的基线）实测：

```
[ A2CASTER ] level 18 ranged-caster share 0.203546 (3295/16188) ... ceiling 0.198629
test/level_roster_baseline_test.cpp:2037: Failure
level 18 ranged-caster share 0.20354583642204102 exceeds baseline 0.14862861378799111 + 5pp (3295/16188)
[ A2CASTER ] level 20 ranged-caster share 0.631563 (10321/16342) ... ceiling 0.608744  → FAIL
[ A2CASTER ] level 23 ranged-caster share 0.52742  (11945/22648) ... ceiling 0.489288  → FAIL
[  FAILED  ] HellfireLevelBaselineTest.HellfireRangedCasterShareWithinBaseline
```

根因：这三层的 core 里塞了该层的施法者（L18 core PSYCHORB＝该层**唯一**施法者；
L20 core NECRMORB+TORCHANT＝3 个里的 2 个；L23 core ARCHLICH+HELLBAT＝2 个施法者）。
core 是**无条件预加**的，所以每个 seed 都吃到，真实远程压力必然抬高。

**按 R4 只改数据**（不动任何阈值），各层用**同类别非施法者**替换一个 core 施法者：

| 层 | 改动（单行替换） |
|---|---|
| 18 | `MT_PSYCHORB` → `MT_STINGER` |
| 20 | `MT_NECRMORB` → `MT_LASHWORM` |
| 23 | `MT_HELLBAT` → `MT_CRYPTDMN` |

整改后**两道守卫全绿**（见下节）。

## 反证：改坏→必红→恢复→绿（实跑，证据内联；RA6）

反证选 **L18 恢复 core `MT_PSYCHORB`**——单行改动、单一变量，
且它同时演示 RB37 存在的**全部理由**：

**改坏后（`18 MT_STINGER core -` → `18 MT_PSYCHORB core -`，重建 MPQ 后实跑）**：

```
[ A2MEASURED ] level 18 ranged share 0 (0/16188), baseline 0, ceiling 0.05
[       OK ] HellfireLevelBaselineTest.PlacedClassMixWithinBaselineForHellfireLevels (87056 ms)
[ A2CASTER ] level 18 ranged-caster share 0.203546 (3295/16188), distinct caster ais 1, baseline 0.148629, ceiling 0.198629
test/level_roster_baseline_test.cpp:2057: Failure
level 18 ranged-caster share 0.20354583642204102 exceeds baseline 0.14862861378799111 + 5pp (3295/16188); per R4 fix the roster data, not this bound
[  FAILED  ] HellfireLevelBaselineTest.HellfireRangedCasterShareWithinBaseline
 1 FAILED TEST
```

**这正是 RB37 要抓的盲区**：真实远程压力涨了 **5.5 个百分点**，
而验收 8 的 `rangedShare` **一动不动地停在 0/16188**（Psychorb 落在 Boss arm）。
一条守卫红、另一条绿——两者不是冗余。

**恢复后（`cp /tmp/rosters.bak` + 重建 MPQ，全套件实跑）**：

```
[       OK ] HellfireLevelBaselineTest.A2PreChangeBaselineForHellfireLevels (83668 ms)
[       OK ] HellfireLevelBaselineTest.PlacedClassMixWithinBaselineForHellfireLevels (83929 ms)
[       OK ] HellfireLevelBaselineTest.HellfireRangedCasterShareWithinBaseline (83956 ms)
[  PASSED  ] 3 tests.
```

### 另一条被否决的反证（记录以免后人重走）

先试过"给 L21 加 core `MT_FIREBAT`"：实测 L21 从 0.352624 升到 **0.413502**，
仍低于 ceiling 0.470576 → **不红**，故不能用作反证（前一位注释里写的
"L21 0.190→0.317 / ceiling 0.240" 与实测不符，已在源码注释中改为上面这份实测证据）。
教训：L21/L22/L24 当前余量较宽（6-12pp），单加一个 core 施法者不一定越界；
余量最紧的是 **L17（6.6pp）/ L19（3.1pp）/ L18（8.3pp，但基线本身低）**。

## 重测：L17-24 占比 vs Task 1 基线（最终数据，200 seed/层，seed base 61000）

`ninja -C build devilutionx_mpq` 后实跑：

```
[ A2MEASURED ] level 17 ranged share 0 (0/16188), baseline 0, ceiling 0.05
[ A2MEASURED ] level 18 ranged share 0 (0/16188), baseline 0, ceiling 0.05
[ A2MEASURED ] level 19 ranged share 0.100256 (1643/16388), baseline 0.121003, ceiling 0.171003
[ A2MEASURED ] level 20 ranged share 0.117489 (1920/16342), baseline 0.18688, ceiling 0.23688
[ A2MEASURED ] level 21 ranged share 0 (0/22619), baseline 0, ceiling 0.05
[ A2MEASURED ] level 22 ranged share 0 (0/22712), baseline 0, ceiling 0.05
[ A2MEASURED ] level 23 ranged share 0.0974037 (2206/22648), baseline 0.147342, ceiling 0.197342
[ A2MEASURED ] level 24 ranged share 0 (0/22653), baseline 0, ceiling 0.05
```

| 层 | 基线 | 实测 | 判定 | 备注 |
|---|---|---|---|---|
| 17 | 0 | 0 | ✅ | 无 Kite 候选 |
| 18 | 0 | 0 | ✅ | 同上 |
| 19 | 0.121003 | 0.100256 | ✅ **低于**基线 | |
| 20 | 0.186880 | 0.117489 | ✅ **低于**基线 | |
| 21 | 0 | 0 | ✅ | |
| 22 | 0 | 0 | ✅ | |
| 23 | 0.147342 | 0.0974037 | ✅ **低于**基线 | |
| 24 | 0 | **0** | ✅ | **`max_image=5300` 实证，见下** |

**八层无一上升**（四层持平于 0、四层下降）——名册化没有抬高该层段的
（同口径）远程占比。

### RB37 守卫读数（最终）

```
[ A2CASTER ] level 17 ranged-caster share 0.197986 (3205/16188), distinct caster ais 1, baseline 0.213677, ceiling 0.263677
[ A2CASTER ] level 18 ranged-caster share 0.115827 (1875/16188), distinct caster ais 1, baseline 0.148629, ceiling 0.198629
[ A2CASTER ] level 19 ranged-caster share 0.394618 (6467/16388), distinct caster ais 3, baseline 0.375763, ceiling 0.425763
[ A2CASTER ] level 20 ranged-caster share 0.481948 (7876/16342), distinct caster ais 3, baseline 0.558744, ceiling 0.608744
[ A2CASTER ] level 21 ranged-caster share 0.352624 (7976/22619), distinct caster ais 2, baseline 0.420576, ceiling 0.470576
[ A2CASTER ] level 22 ranged-caster share 0.352589 (8008/22712), distinct caster ais 2, baseline 0.417577, ceiling 0.467577
[ A2CASTER ] level 23 ranged-caster share 0.397739 (9008/22648), distinct caster ais 3, baseline 0.439288, ceiling 0.489288
[ A2CASTER ] level 24 ranged-caster share 0.486779 (11027/22653), distinct caster ais 2, baseline 0.510043, ceiling 0.560043
```

**八层的真实远程压力全部 ≤ 基线**（唯一略升的是 L19：0.375763 → 0.394618，
+1.9pp，在 5pp 带内）。即按"真实 `ai`"这把更严的尺，名册化同样没有抬高远程压力。

## L24 `max_image = 5300` 的实证（控制者要求核实的那条约束）

**算式修正**：前一位报告的算式只算了预加（2386），漏了 **core 也吃预算**
（`AddMonsterType` 累加 `monstimgtot`）。L24 的 core 是 `MT_REAPER` 800 +
`MT_HELLBAT` 550 = 1350，故真实占用 **2386 + 1350 = 3736**，
剩余 `5300 - 3736 = 1564 < 1740`（BONEDEMN）→ 装不下。
**BONEDEMN 要能进，`max_image` 需 ≥ 5476**。
结论（"5300 刚好装不下"）成立，但**边界比它写的更紧**：可用空间只剩 176。

**实测证据**：上面 `[ A2MEASURED ] level 24 ranged share 0 (0/22653)` 与
Task 1 基线 `0 (0/22653)` **分母逐位相同、分子同为 0**，且
`[ A2CASTER ] level 24 ... distinct caster ais 2`（HELLBAT+ARCHLICH，**没有** BoneDemon
——它是第 3 个施法者，若装进来会是 3）。两条独立读数共同证明 BONEDEMN 确实未进 L24。

**未做的一步（顾虑，见末尾）**：控制者建议"临时抬高该值观察占比跳变、再恢复"。
我用了上面两条**已在最终配置里**的读数（分子 0 + `distinct caster ais 2`）
来证明同一件事，没有额外跑一轮抬高 `max_image` 的实验——
理由是每轮 HF 三用例约 250s、本轮已跑 6 轮，且 `distinct caster ais` 计数
对"BoneDemon 是否进池"是直接可读的判据。**这一条属于按等价证据替代，
不是遗漏，但确实不是控制者字面要求的那个实验。**

## L1-16 既有读数逐位不变

`level-rosters` eval case 的 10 个用例（`LevelRosterBaselineTest.*` +
`SquadPlacementTest.*`）在全量门禁里全绿，`[ MEASURED ]` / `[ SQUADFORM ]` 等
读数未变——**结构性原因**：那些用例在 `gbIsHellfire = false`、无 hf overlay 下测量，
L17-24 的怪在基础 `monstdat.tsv` 里全是 `availability=Never`；而本任务
**只新增了 L17-24 的行**，L1-16 的数据行零改动（`git diff` 可核）。
故 L1-16 的测量输入完全没动。

## eval 计数同步

**结论：`level-rosters` 与 `level-roster-validation` 的计数均无需改动**，理由如下（已写入 YAML 注释）：

- `level_roster_test` 用例数 **41 未变**（本任务没加校验用例）→ `level-roster-validation` 的
  `passed_min: 35` 与 `"[  PASSED  ] 35 tests."` 不动（该 filter 只取
  `LevelRosterTest.*:ParseClassFloorsTest.*` = 35）。
- `level-rosters` 的 filter 是 `LevelRosterBaselineTest.*:SquadPlacementTest.*`，
  实测 `--gtest_list_tests` 在该 filter 下仍是 **10** 个用例 → `passed_min: 10` 与
  `"[  PASSED  ] 10 tests."` 不动。

**为什么不把 3 个新 HF 用例纳入 `level-rosters` filter**（我评估过并否决）：
它们依赖 HF 资产，无 HF 环境（CI 只有 `spawn.mpq`）会 `GTEST_SKIP`，
纳入后 `"[  PASSED  ] 13 tests."` 这行硬编码断言在 CI 上**恒假** —— 为了加一层
eval 可见性反而制造一条环境相关的假红。它们由**全量门禁**（ctest 全跑，
二进制已在 `CMake/Tests.cmake:61` 注册）覆盖。该判断已作为注释写进
`eval/cases/rng/level-rosters.yaml` 顶部，供后人复核。

