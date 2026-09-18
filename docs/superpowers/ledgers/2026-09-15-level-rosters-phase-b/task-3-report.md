# Task 3 报告：核心小队接入（squad）

**BASE HEAD**：`369438ebe fix(eval): gate retail/HF-dependent cases on their real data dependency (U1)`
**分支**：`feature/qol-upgrades`
**开始时间**：见 git 提交时间

## 进度日志（增量追加）

- [x] 落盘报告骨架，确认 HEAD = `369438ebe`，工作树干净
- [x] 读需求（brief / plan Task 3 / dispatch / spec §4.3）
- [x] 读代码现状（monster.cpp 散布循环 `:3846-3861`、`PlaceGroup :3030-3110`、TSV 解析 `level_roster.cpp:119-134`）

### 读码结论（影响实现的关键事实）

1. `RecordReader` 是**位置式**读列：给 `level_roster_params.tsv` 加三列后，
   `LoadLevelRosterParamsFromFile` 若无条件多读三列，则**夹具表**
   `test/fixtures/txtdata/monsters/level_roster_params_interleaved.tsv`（只有 4 列）
   会撞 `DataFile::Error::NotEnoughColumns` 致命错。→ 夹具表必须同步加列，或用 optional 读。
   决定：**夹具表同步加三列**（显式表达"这三列是表格式的一部分"），并额外用夹具证明解析值。
2. `PlaceGroup(.., leashed=false, ..)` **不会**写 `leader->packSize`（`:3107-3109` 仅在 `leashed` 时写），
   也不会 `setLeader`；leader!=nullptr 只影响初始候选格取自 leader 邻格（`:3044-3049`）。
   → `squadLeashed=0` 的回退语义天然成立，无需额外代码。
3. `PlaceGroup(typeIndex, 1, nullptr, false, {})` 里 leader==nullptr → `packSize` 完全不被触碰，
   leader 保持 `packSize` 默认值；随后随从组以 `leashed=true` 调用时才写 `packSize=placed`。
   → 「不产生 packSize==0 的 leader」需要：随从组 `placed==0` 时 leader.packSize 被写成 0。
     这是 `PlaceGroup` 既有行为，本任务必须显式守卫（见下）。
4. `GetMonsterTypeIndex` 未命中返回 `LevelMonsterTypeCount`（**不可当下标**）→ partner 必须校验。
5. 测量用真实建关夹具已存在于 `test/level_roster_baseline_test.cpp`
   （`CreateDungeonForMeasurement` + `GetLevelMTypes` + `InitMonsters`）→ 每层成队率在该 TU 测。
6. 本机有 `DIABDAT.MPQ` + `HELLFIRE.MPQ`（`~/.local/share/diasurgical/devilution/`），零售路径可实跑。

- [x] **步骤 1+2 完成**：三列解析 + 校验 + 数据表 → 提交 `b6cd416e5`
- [ ] 步骤 3/4：散布循环接入 + 小队用例
- [ ] 实测每层成队率
- [ ] 门禁 + eval + 推送 + CI 跟踪

### 步骤 1/2 详情

**新增/改动**
- `LevelRosterParams` 增 `squadChance`/`squadSize`/`squadLeashed`（默认 0/0/true）
- `LoadLevelRosterParamsFromFile` 位置式读三列；`squad_leashed` 接受 `true/false/1/0`，其他值**致命拒绝**（不静默默认）
- `ValidateLevelRoster` 三条新守卫：`squad_chance>100` 拒、`squad_size>3` 拒、
  `squad_chance>0 && squad_size==0` 拒（**条件式**：`squad_chance=0` 时 `squad_size=0` 合法）
- `assets/txtdata/monsters/level_roster_params.tsv` L1-15 加 `30 / 2 / 1`（RB12 起点，CRLF 保持）
- `test/fixtures/.../level_roster_params_interleaved.tsv` 同步加列，两行**每列都不同**（L1=`40 3 true`，L2=`0 0 false`）
  → 排除「loader 把一个值写进所有行」也能过的假绿；`squad_leashed` 两种极性都被钉住

**新增用例（6 个，均在 `level_roster_test.cpp`）**
`ValidationRejectsAnOutOfRangeSquadChance`、`ValidationRejectsAnOutOfRangeSquadSize`、
`ValidationRejectsAZeroSquadSizeWhenSquadsAreEnabled`、`ValidationAcceptsAZeroSquadSizeWhenSquadsAreDisabled`（互补极性）、
`ParamsCarrySquadColumns`（真实 loader + 夹具表）、`ShippedParamsEnableSquadsOnEveryParameterisedLevel`（出厂表 L1-15 三列可用）
→ `level_roster_test` 29 → **35** 全绿。

**反证实跑（改坏 → 红 → 恢复 → 绿）**

| # | 破坏方式 | 期望 | 实测 |
|---|---|---|---|
| 1 | 三条 squad 校验全部 `if (false && ...)` | 3 个拒绝用例红、互补用例绿 | OK：3 FAILED / 1 OK |
| 2 | `squad_size==0` 校验改为**无条件** | 互补用例红、3 个拒绝用例绿 | OK：1 FAILED（`ValidationAcceptsAZeroSquadSize...`）/ 3 OK |
| 3 | `squad_chance`/`squad_size` 读列顺序**互换** | 解析用例红 | OK：夹具加载即 app_fatal「level 1 has a squad_size above 3 (40)」→ 用例未通过 |
| 4 | `squad_leashed` 的 `false` 分支返回 `true`（忽略列值） | 解析用例红 | OK：FAILED（`Value of: params2->squadLeashed`）|

恢复后 35/35 全绿；`check_drift.py` A/B/C/C2/E 全 PASS。


## 步骤 3/4：散布循环接入 + 小队用例

### 生产改动（`Source/monster.cpp` / `monster.h`）

- **文件内 static 助手**（未新增导出 roster API，避免漂移 E 噪声）：
  - `IsCoreRosterMember(level, type)`：查 `GetLevelRoster(level)` 的 `role==Core`
  - `PickCorePartner(level, leaderType)`：同层**其他** core 成员中随机取一个，且必须
    `IsMonsterAvailable`（名册行不等于可用，`level_roster.h` 已注明）；无候选返回 `MT_INVALID`
- **散布循环**（原 `:3851-3862`）：core 类型 + 剩余槽位 ≥2 + `GenerateRnd(100) < squadChance` 时
  1. `PlaceGroup(typeIndex, 1, nullptr, false, {})` 放 leader
  2. `ActiveMonsterCount == before` → **放弃小队**并降级 `PlaceGroup(typeIndex, na)` + `continue`
     （否则 `&Monsters[ActiveMonsterCount-1]` 会指向上一轮的无关怪）
  3. `PickCorePartner` → `MT_INVALID` 则 `continue`
  4. `GetMonsterTypeIndex(partner)` **校验 `< LevelMonsterTypeCount`**（未命中返回该值，不可当下标）
  5. `PlaceGroup(partnerIndex, squadSize, &leader, squadLeashed, {false,false,false})`
- `na` 的既有计算**逐轮无条件执行**（与原实现同位置同顺序），降级路径沿用
- 新增 `SquadRollCounters` + `GetSquadRollStats()`：`eligibleCoreDraws / rolls /
  leaderPlacementFailed / noPartnerAvailable / partnerNotRegistered / realised`；
  `InitLevelMonsters()` 重置；**生产消费者** = `InitMonsters()` 末尾的 `LogVerbose` 诊断（漂移 E 需要真实生产调用者）

### 排查到的两个真实缺陷（首轮实跑暴露，非猜测）

1. **unique боss 包也产生 leashed 随从**：`PlaceUniqueMonsters()` 在散布循环**之前**跑，
   用默认 `MinionOptions{}`（AI 继承、leader base 不必是 core）。首轮把所有 leashed 随从当小队统计
   → `squad_chance=0` 的表也报出 50 个"小队"、8 个"非 core leader"。
   **修正**：以 `leader->isUnique()` 为判别式（散布循环的小队路径是**唯一**会在**普通** leader 下
   造出 leashed 随从的来源）；并在"关闭小队"用例里**断言 unique 计数非零**，防止判别式写错导致 A/B 变空。
2. **leash 边界差一**：`PlaceGroup` 的 4 格约束是相对**首个候选格**量的，而首格是 leader 的**邻格**
   （`leader->position.tile + Direction(GenerateRnd(8))`），所以随从到 leader 的距离上限是 **4 而非 3**。
   首轮用 `>=4` 判越界 → L9 seed 2 误红。**修正**为 `>4`，并把推导写进注释。
3. **计数器读法**：`CreateDungeonForMeasurement` 内含 `InitLevelMonsters()`（重置计数器），
   首轮的 before/after 差值**下溢**成 `2^64-1`。修正为每轮跑完直接读（计数器本就是逐层的），
   并加 `EXPECT_LE(realised, rolls)` 作为**该缺陷的守卫**。

### 新增用例（4 个，`test/level_roster_baseline_test.cpp` 的 `SquadPlacementTest`）

走**真实放置链路** `CreateDungeonForMeasurement → GetLevelMTypes → InitMonsters`，
从**已放置怪**读小队，不重演规则。三张 fixture 覆盖表（`squads_off/always/unleashed`）
是出厂表的**整表拷贝、只改 squad 三列**，`max_image/tail_draw/class_floors` 全部held constant。

| 用例 | 断言 |
|---|---|
| `SquadFormsAroundACoreLeader` | L9-12 × 10 seed @chance=100：leader 有随从、`packSize>=1`、随从**保留自身 AI**（G2）、在 leash 内、leader 是**该层 core**、不超放置上限 |
| `SquadsAreAbsentWhenTheTableDisablesThem` | 同层同 seed @chance=0：普通 leader 的 leashed 随从 **恰为 0**，且 unique 包随从 **>0**（防判别式失效导致空断言）|
| `UnleashedFallbackPlacesNeighboursWithoutLeashing` | `squad_leashed=0` 的 leashed 随从 = 0，同 seed 的 `=1` 参照 > 0（同种子 A/B，§4.3.4 回退语义）|
| `SquadRateIsMeasuredPerLevel` | @chance=100：每层 `rolls>0`、`rolls==eligible`、`realised<=rolls`，并打印 `[ SQUADRATE ]` |
| `ShippedSquadChanceRealisesSquadsOnEveryLevel` | **出厂表**（chance=30）：每层 `eligible>0`、`realised>0`、`realised<=rolls`，每样本再查 packSize/AI/core-leader，打印 `[ SHIPPEDSQUAD ]` |

→ 实为 **5** 个新用例。`LevelRosterBaselineTest` 3 → **8**（同一二进制，两个 fixture 名）。

### 实测：每层「core 组真正成队」率（**出厂表 squad_chance=30，50 seed/层**）

`rollRate = rolls / eligibleCoreDraws`（跟随 squad_chance）；
`realisation = realised / rolls`（**纯放置损耗**：totalmonsters 钳制 / 10 次放弃 / 4 格约束）。

| 层 | 合格 core 抽取 | 成队 roll | roll 率 | 真正成队 | **实现率** | leashed 随从 | 损耗(leader/无partner/未注册) |
|---|---|---|---|---|---|---|---|
| 1 | 2105 | 640 | 30.4% | 640 | **100.0%** | 1276 | 0/0/0 |
| 2 | 1660 | 493 | 29.7% | 493 | **100.0%** | 981 | 0/0/0 |
| 3 | 1340 | 384 | 28.7% | 384 | **100.0%** | 766 | 0/0/0 |
| 4 | 1249 | 389 | 31.1% | 389 | **100.0%** | 772 | 0/0/0 |
| 5 | 1095 | 294 | 26.8% | 294 | **100.0%** | 585 | 0/0/0 |
| 6 | 1020 | 307 | 30.1% | 307 | **100.0%** | 613 | 0/0/0 |
| 7 | 1083 | 320 | 29.5% | 320 | **100.0%** | 636 | 0/0/0 |
| 8 | 1170 | 360 | 30.8% | 359 | **99.7%** | 712 | 0/0/0 |
| 9 | 859 | 260 | 30.3% | 259 | **99.6%** | 517 | 0/0/0 |
| 10 | 894 | 267 | 29.9% | 267 | **100.0%** | 532 | 0/0/0 |
| 11 | 890 | 274 | 30.8% | 274 | **100.0%** | 544 | 0/0/0 |
| 12 | 896 | 262 | 29.2% | 262 | **100.0%** | 521 | 0/0/0 |
| 13 | 1080 | 309 | 28.6% | 309 | **100.0%** | 615 | 0/0/0 |
| 14 | 1289 | 360 | 27.9% | 360 | **100.0%** | 717 | 0/0/0 |
| 15 | 1641 | 490 | 29.9% | 490 | **100.0%** | 973 | 0/0/0 |

**结论（对控制者 RB12 的回答）**：`squad_chance=30` 的**实际打折几乎为零**——15 层里 13 层
实现率 100%，L8/L9 各丢 1 次（99.6-99.7%）。原因是三条打折路径实测**全为 0**：
leader 放置失败 0、无 partner 0、partner 未注册 0。即"4 格约束 / 槽位不足 / `totalmonsters` 钳制会打折"
这一预期在**当前参数下不成立**：`squadSize=2` 太小、`na`（3-5）反而更大，
所以小队比它替代的普通组**更省槽位**，钳制反而更难触发；每队实测 ~1.99 只随从（≈满员）。

@chance=100 的对照（`[ SQUADRATE ]`，50 seed）同样是 ~99.9-100% 实现率，
每层 rolls 与 eligible 完全相等 → 分支门槛没有隐藏条件。

**给 Task 4 的移交事实**：打折不是瓶颈，因此按层分化 `squad_chance` 会**几乎线性**地
改变成队密度；若要压低成队密度应直接调 `squad_chance`，而不是指望放置损耗吸收。

## Defect 4: core squads breach the L14 placed-class-mix ceiling (spec acceptance 8)

Enabling `squad_chance=30` uniformly made `LevelRosterBaselineTest.PlacedClassMixWithinBaseline`
fail on L14: ranged share `0.615081` (14511/23592) against the ceiling `0.60876`
(baseline `0.55876` + 5pp). Same-build A/B on the shipped table, 200 seeds, seed base 9000:

| L14 config | ranged share | verdict |
|---|---|---|
| `squad_chance=0` (squads off) | 0.604908 (14271/23592) | pass, 0.39pp headroom |
| `squad_chance=30`, `class_floors=Melee=1` | 0.615081 (14511/23592) | **FAIL**, +0.63pp over |
| `squad_chance=10`, `class_floors=Melee=1` | 0.605120 (14276/23592) | pass |
| `squad_chance=30`, `class_floors=Melee=2` | 0.604474 (14077/23288) | pass here, but **breaks acceptance 9c** |
| `squad_chance=10`, `class_floors=Melee=1` | 0.605120 (14276/23592) | pass, **chosen** |

Mechanism (not a flaky bound). L14's core set is `MT_VTEXLRD` (AI `Mega` -> Melee),
`MT_SNOWWICH` (`Succubus` -> RangedTurret) and `MT_XACID` (`Acid` -> RangedKite). A squad
replaces `na` (3-5) copies of the *leader* type with 1 leader + `squad_size` partners drawn
from the **other** core types, so on L14 two of the three partner draws are ranged. The
squad path therefore shifts the mix toward ranged whenever a level's cores are
ranged-majority, and toward melee when they are melee-majority. The effect is
roster-dependent and bidirectional; L14 is the only level it pushed over, because L14 was
already sitting 0.39pp under its ceiling before squads existed.

Resolution follows R4 as the test's own comment mandates ("If a level exceeds it, the
roster or its class_floors is what changes - never this ceiling"): the ceiling and the
baseline are untouched.

The first attempt changed L14 `class_floors` `Melee=1` -> `Melee=2`, which passes acceptance
8 and keeps `squad_chance` uniform. **That attempt was rejected by the full gate**: it broke
`SamplingBaselineTest.HellUniqueBasesRemainReachable` (acceptance 9c), dropping L14 unique
base reachability from 6/6 to 2/6 - `Fangskin`, `Graywar the Slayer` and `Steelskull the
Hunter` became unplaceable, because the extra melee floor crowds out the tail slots their
base types need. Reverting to `Melee=1` restored 6/6, confirming the floors change was the
cause and not the squad path.

The shipped resolution is therefore **L14 `squad_chance` 30 -> 10** with `class_floors`
left at `Melee=1`: measured ranged share 0.605120, under the 0.60876 ceiling, with
acceptance 9c back at 6/6. L14's realised squad rate drops accordingly (rollRate 0.088,
realisation 1.000, 121 squads over 50 seeds); every other level keeps `squad_chance=30`.

Consequence for Task 4: `squad_chance` is not a free knob on ranged-majority levels, and
`class_floors` is not a free lever either - raising a melee floor on a hell level trades
acceptance 8 for acceptance 9c. Any per-level increase must be re-measured against BOTH,
not just against squad density.
