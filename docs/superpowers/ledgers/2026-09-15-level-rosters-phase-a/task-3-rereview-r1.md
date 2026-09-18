# Task 3 修复轮次 1 复审（S1/R29, I1/R30, I2/R31）

基线 `c62a07e0f` → Head `9095a2397`。只读评审，未改动工作区/暂存区/HEAD。

## 问题裁决

### S1【严重｜R29】caps 断言恒真 — ADDRESSED

**① 加载期 core 自洽校验：落地且位置正确。**
`Source/tables/level_roster.cpp:239-257`，紧接 core-non-empty 循环之后、`if (gbIsSpawn)`
早退（`:259`）**之前**。逐 level × `BehaviorClass` 统计 core 计数，`cap != 0` 且
`coreClassCounts[i] > cap` 时返回错误，文案含 level / 计数 / class 名 / cap 四项。
`levels` 向量（`:215-221`）同时来自 entries 与 params，故只有 roster 行没有 params 行的
层（L16）也被覆盖。cap 来源是 `BehaviorClassCapForLevel`（`:138-148`），与采样循环
`Source/monster.cpp:3569` 同一真相源，不会漂移。

**② 断言豁免真的不依赖被测名册：是。**
`test/sampling_behavior_test.cpp:745-762` 的 `EnginePreAddClassCount` 只读两个引擎源：
`MonstersData[MT_GOLEM].ai` 与 `UniqueMonstersData` 中 `mlevel == level` 的行（经
`GetBehaviorClass(unique.mtype)` 归类）。旧函数里对 `GetLevelRoster(level)` 的遍历已完全
删除，新函数体内无任何 `GetLevelRoster` / `LevelRosterEntry` 引用（已逐行核对 diff
`:250-260` 的删除块）。断言改为 `EXPECT_LE(counts[i], cap + enginePreAdds)`
（`:869-871`），豁免与被测表解耦成立。

**③ 负向用例：驱动真实校验函数，且判别力已独立确认。**
`test/level_roster_test.cpp:161-186`（L10 三 RangedKite）与 `:188-206`（L14 三 Melee，
spawn 模式）都直接调用 `ValidateLevelRoster(entries, params)`，非复制逻辑。L10 用例还断言
「降到恰好 2 个必须通过」（`:184-185`），把「因超 cap 被拒」与「这几个类型本身非法」区分开。

关于报告的 `if (false)` 反证法：该法只证明「删掉该分支两例会失败」，属必要而非充分
（它不排除断言被别的路径顺带满足）。我不依赖它，改用**独立正向核实**：手工按
`monstdat.tsv` 的 ai 列复算三例的 `GetBehaviorClass`——`MT_BMAGMA`/`MT_WMAGMA`=Magma、
`MT_RSTORM`=Storm 三者同为 RangedKite（cap(10,·)=2 → 3>2 命中）；`MT_RSNAKE`/`MT_BSNAKE`
=Snake、`MT_NBLACK`=SkeletonMelee 三者同为 Melee（cap(14,·)=2 命中）。且实跑
`./level_roster_test --gtest_filter='*ThreeSameClassCore*'` → 2/2 PASS，全套 29/29 PASS。
再对**出厂表**离线复算全部 level×class core 计数：15 组受 cap 约束的组合无一越界，
即新校验不会误伤已发布数据（这也解释了报告中的加载仍绿）。

**④ 断言是否仍能发现 core/tail 越界：能，但覆盖不均——见范围外观察 O1。**
按候选池离线复算，收紧后的 allowance 在 L10-12 RangedKite、L14/L15 Melee 与
RangedTurret 上严格小于该类可实现上限（有判别力）；在 L9 RangedKite、L13 全类与
L14/15 若干空类上，`cap + enginePreAdds` 已 ≥ 该类候选池上限，断言在这些格子上无法失败。
这是引擎 unique 数据（L13 有 7 个 Melee unique）的客观结果，不是本次修复引入的自证，
且 core 侧的越界已由 ①（加载期，与候选池无关）刚性兜住，故不构成 S1 未闭合。

### I1【重要｜R30】Hellfire 套件泄漏全局态 — ADDRESSED

**① 还原清单覆盖：与 `TestInitGame` 的实际写入面逐项对齐。**
读 `test/drlg_test.hpp:55-75`，`TestInitGame` 写入：`Players`（resize + `MyPlayer` +
`pOriginalCathedral`）、`sgGameInitInfo.fullQuests`、`gbIsMultiplayer`、mod 归档、
以及 `InitQuests()`。快照（`test/sampling_behavior_test.cpp:402-412`）覆盖
`Quests`、`Players.size()`、`MyPlayer` 是否指向 `Players[0]`、`pOriginalCathedral`、
`sgGameInitInfo`、`gbIsMultiplayer`、`gbIsHellfire`、`gbIsSpawn`；TearDown
（`:428-446`）逐项还原并复位 mod/monstdat/roster。采用保存-还原而非臆造初值，符合要求。
`snapshotTaken_` 守卫（`:428`）确保 MPQ 缺失早退时不会用零值污染全局——这是必要的细节，
做对了。

**② 泄漏是否真的复现过：报告给出了可验证的具体失败。**
报告 `task-3-report.md:183-198` 的复现是「删 `Quests` 还原行 + 让 Hellfire 先跑」，
失败点为 `HellL13SameClassTailBaseline` tail=100 vs 0。该因果链我独立核对成立：
`Q_WARLORD` 的 unique base 是 `MT_BTBLACK`（`unique_monstdat.tsv`，level 13），ai 属
Melee；`GetLevelMTypes()` 在 `Quests[Q_WARLORD].IsAvailable()` 时预加它
（`Source/monster.cpp:3474-3475`），而 L13 core 已有 `MT_NBLACK`/`MT_GUARD` 两个 Melee
→ 3 个同类 → 该 EXPECT_EQ(0.0) 必炸。方向与量级都自洽。

**③ shuffle 实跑输出在报告里（`:200-215`，3 轮 24/24，种子 54396-54398），并已独立复跑。**
我另跑 `./sampling_behavior_test --gtest_shuffle --gtest_repeat=2`（种子 10139/10140）
→ 两轮各 24/24 PASSED。顺序无关性确认。

**④ `Player` 不可按值快照的处理合理。**
`Player` 的拷贝赋值已删除，只快照规模与被改写字段是可行的最小正确集；
`MyPlayer` 用「是否指向 `Players[0]`」的布尔重建而非裸指针，避免 resize 后悬垂，是正确取舍。

### I2【重要｜R31】层范围无注释 + L16 params 永不读 — ADDRESSED

**① 三处范围注释均已加。** `RosterCoreAlwaysPresent`（`test/sampling_behavior_test.cpp:772-779`，
跑 1-16，并在 `:801-804` 对 L16 跳过 realised-core 断言）、`RosterTailDrawBounded`
（`:822-826`，1-15）、`RosterQuotasSatisfied`（`:855-859`，1-15）。三处都点明 L16 走硬编码
分支并在 roster pre-add 前 return（已对 `Source/monster.cpp:3443-3448` 核实：`currlevel == 16`
在 core 预加 `:3503` 之前 `return {}`）。

**② L16 params 行确已删除。** `level_roster_params.tsv` 现有表头 + L1-15 共 16 行，
`awk` 取第一列得 `1..15`，无 16。CRLF 行尾保持不变。

**③ L16 名册行仍在且注释到位。** `level_rosters.tsv` 的 L16 四行 core
（`MT_GSNAKE`/`MT_BTBLACK`/`MT_CABALIST`/`MT_ADVOCATE`）保留；
`Source/tables/level_roster.h:95-101` 说明 registration-only、引用 spec 4.2.7/R31，并解释
为何写在头文件（TSV 无注释语法）。

**④ 删行不影响任何消费者。** 全仓 `GetLevelRosterParams` 调用点共 6 处：
`Source/monster.cpp:3520`（L16 在 `:3447` 已提前 return，走不到）、
`Source/tables/level_roster.cpp:307`（诊断按 `Params` 迭代，L16 自然不在其中）、
测试 4 处均为 L1-15 或 L17-24 断言 nullptr。`ValidateLevelRoster` 的
core-non-empty 与新 core-vs-cap 循环仍覆盖 L16（`levels` 含 entries 的层），
校验力未因删行下降。加载与全套用例实跑仍绿（`level_roster_test` 29/29、
`sampling_behavior_test` 24/24）。

## 修复 Diff 中的新增破坏

无 Critical / Important。

一处 **Minor（格式，非阻塞）**：`test/sampling_behavior_test.cpp:38` 新增的
`#include "tables/questdat.hpp"` 未按 `SortIncludes: true` 排在 `tables/level_roster.h`
之后，clang-format 18 CI（`.github/workflows/clang-format-check.yml` check-path `test`）
会在该行报差异。该文件在**基线** `c62a07e0f` 上已有一处既存 clang-format 差异
（`A1A3VariantsAreCore` 的初始化列表换行），故这不是本轮引入的**新**门禁状态变化，
但排序差异本身确由本轮新增行造成。修法是把该 include 移到 `tables/monstdat.h` 之前一行。
（该 include 本身是必需的：`std::vector<Quest>` 快照需要 `Quest` 的完整定义，
它定义在 `Source/tables/questdat.hpp:102`。）

## 范围外观察（非阻塞）

- **O1｜`RosterQuotasSatisfied` 的判别力在部分格子上仍为空。** 收紧后
  `cap + enginePreAdds` 在 L9 RangedKite（2+4=6 = 候选上限 6）、L13 全部类别
  （Melee 2+7=9 = 上限 9 等）上已达或超过该类可实现类型数，断言在这些格子恒真。
  根因是 `EnginePreAddClassCount` 计入**该层全部** `UniqueMonstersData` 行，而
  `GetLevelMTypes()` 实际只无条件预加 6 个 quest unique 的 base
  （Butcher/Garbud/Zhar/SnotSpill/Lachdanan/Warlord，`Source/monster.cpp:3464-3475`），
  其余 unique 由 `PlaceUniqueMonsters()` 在**已有类型**中挑选（`:511-513`），并不新增
  `LevelMonsterTypes` 槽位。按后者收紧豁免可让 L13 等重新获得判别力。这是保守过头
  （方向上只会漏报、不会误拒），且 core 侧已由加载期校验兜住，故仅记录。
- **O2｜core-vs-cap 校验只覆盖 core，不覆盖 core+quest-unique 的合计。**
  L13 core 有 2 个 Melee，`Q_WARLORD` 的 `MT_BTBLACK` 预加后即为 3 —— 出厂表能过校验，
  但真实组合已破 cap（这正是 I1 复现出的现象，只是被测试的「无任务」条件掩盖）。
  这是规格层面的问题（cap 是否应把 quest 预加计入预算），建议在阶段 B 与 spec 4.2.2 一并裁决。
- **O3｜L16 的 `MT_BTBLACK` 同时是 `Q_WARLORD` 的 base（其 `mlevel` 为 13）**，
  与 L16 roster 行并列存在；`IsUniqueBaseForLevel` 按 `mlevel == level` 比较故不冲突，属预期。

## 裁决

**修复轮次：全部问题已处理且无新增 Critical/Important 破坏。**

- S1 — ADDRESSED（三项要求全部落地；负向用例判别力经独立正向核实，不依赖报告的 `if (false)` 反证）
- I1 — ADDRESSED（还原清单与 `TestInitGame` 写入面对齐；泄漏因果链独立核对成立；shuffle 已独立复跑）
- I2 — ADDRESSED（params L16 行确已删除，只余 L1-15；名册行保留且注释到位；6 处消费者全部核查无影响）

未关闭项：无。唯一遗留是一条 Minor 的 include 排序（会被 clang-format CI 标出），
以及 3 条范围外观察，均不阻塞本轮。
