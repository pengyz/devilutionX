# Task 4 修复轮次 1 报告

评审 4 项待修（1 重要 F1 + 3 轻微 F2/F3/F4），全部落地于
`test/level_roster_baseline_test.cpp`（未改任何生产代码、未改任何出厂表格数据、
未改 `kRangedShareCeiling`/`kSquadFormationFloor` 阈值数值）。

## 状态：完成

---

## F1（重要，RB23）：`ShippedSquadChanceRealisesSquadsOnEveryLevel` 的零卷探测器改为 leash 无关

### 问题
该用例原用 `EXPECT_GT(realised, 0u)` 作为「本层至少形成过一次小队」的探测器。
`realised` 只在 `obs.leadersWithMinions`（遍历 `getLeader()` 回指）计数，而
`getLeader()`/`setLeader()` 只在 `PlaceGroup(..., leashed=true, ...)` 时才建立回指
（`Source/monster.cpp` `PlaceGroup`）。一旦某层按 spec 4.3.4 的兜底把
`squad_leashed` 设为 `0`（正是 Task 4 自己的 `SquadFormationRate` 门禁在低于
`kSquadFormationFloor` 时给出的补救路径），`realised` 会**按构造永远读 0**，与该层
是否真的形成过小队无关。此时旧断言会把一个**符合 spec 的合规兜底**误报为
「本层完全没形成过小队」。

### 修复
把探测器的分子换成 `SquadRollCounters::formed`（leash 无关：只要该轮 roll 的
`ActiveMonsterCount` 相比 roll 前有增长就计数，见 `Source/monster.cpp:3999-4003`），
断言改为 `EXPECT_GT(formed, 0u)`；同时保留原有的 `EXPECT_LE(realised, rolls)`
（未删除、未削弱），并新增 `EXPECT_LE(formed, rolls)` 作为 `formed` 自身的上界不变式。

### 新形式证明（要求：不是「打断验证变红」，而是真实场景下两条用例都保持绿）

**步骤 1-2：用夹具表构造某层 `squad_leashed=0`，两条用例在该夹具下必须保持绿**

创建临时夹具 `test/fixtures/txtdata/monsters/level_roster_params_l10_unleashed.tsv`
（出厂 `level_roster_params.tsv` 的逐行拷贝，仅将 L10 的 `squad_leashed` 列由 `1`
改为 `0`），临时将两条用例的 `LoadLevelRoster()` 换成
`LoadSquadParams("txtdata\\monsters\\level_roster_params_l10_unleashed.tsv")`
后重跑（`ninja level_roster_baseline_test` 先把新夹具拷进
`build/test/fixtures/`，为此临时把该夹具加进了 `test/Fixtures.cmake` 的
`devilutionx_fixtures` 列表，收尾时已连同该 cmake 改动一起完全撤销，
见下文「临时改动撤销确认」）：

```
[ RUN      ] SquadPlacementTest.ShippedSquadChanceRealisesSquadsOnEveryLevel
...
[ SHIPPEDSQUAD ] level 10 eligible 883 rolls 261 rollRate 0.295583 realised 0 realisation 0 formed 261 minions 0 lost(leader/partner) 0/0
...
[       OK ] SquadPlacementTest.ShippedSquadChanceRealisesSquadsOnEveryLevel (26648 ms)
[ RUN      ] SquadPlacementTest.SquadFormationRate
...
[ SQUADFORM ] level 10 leashed 0 rolls 2684 formed 2684 rate 1 noPartnerAvailable 0 leaderPlacementFailed 0 floor 0.95
...
[       OK ] SquadPlacementTest.SquadFormationRate (288944 ms)
[==========] 2 tests from 1 test suite ran. (315604 ms total)
[  PASSED  ] 2 tests.
```

L10 在该夹具下 `leashed 0`（兜底生效），`realised 0 realisation 0`（旧分子按构造归
零），但 `formed 261`／`formed 2684 rate 1`（新分子完全不受影响）。两条用例均
**PASSED**，证明修复后的断言在 spec 4.3.4 兜底场景下不会误判。

**步骤 3：控制/反证——旧断言在同一场景下会变红**

日志中 L10 那一行本身就是反证：`realised 0`（若旧断言仍是
`EXPECT_GT(realised, 0u)`，`0 > 0` 为假，必定 FAIL，输出
`"level 10 realised no squad at all over 50 seeds despite 261 rolls"`）。
未额外做「改回旧代码重跑再变红」的第二次动作，因为该场景下 `realised` 的读数
（0）已经是旧断言判定式的直接输入，从代码逐行推导（`PlaceGroup` 只在
`leashed=true` 时写 `packSize`/`setLeader` → `obs.leadersWithMinions` 遍历
`getLeader()` → 对应 `realised` 计数）与实测输出完全一致，故以「实测 0 + 代码路径
推导」作为反证证据，未做重复劳动。

### 恢复确认
- 两处 `LoadSquadParams("...l10_unleashed.tsv")` 均已改回
  `LoadLevelRoster(); // shipped tables (squad_chance 30)`。
- 临时夹具文件 `test/fixtures/txtdata/monsters/level_roster_params_l10_unleashed.tsv`
  已删除。
- `test/Fixtures.cmake` 中为该临时夹具新增的一行已撤销；
  `git diff --stat test/Fixtures.cmake` 为空（与撤销前完全一致，零差异）。
- `build/test/fixtures/txtdata/monsters/level_roster_params_l10_unleashed.tsv`
  这一构建产物也已手动清理。
- `cmake . && ninja level_roster_baseline_test` 重新配置+构建成功（无该文件的
  拷贝步骤）。
- 收尾后 `git status --short` 只剩 `M test/level_roster_baseline_test.cpp`
  一个改动，无夹具/cmake 残留。

---

## F2（轻微）：`else` 分支（L10 未涉及）注释诚实化

`if (params->squadLeashed) { ... } else { EXPECT_EQ(leashedRealised, 0u) ... }`
的逻辑与断言强度均**未改动**（未删除、未弱化）。仅重写了 `else`
分支上方的注释：明确说明在当前出厂表（全层 `squad_leashed=1`）下，
`leashedRealised` 恒为 0 是 `PlaceGroup` 只在 `leashed=true` 时写 `packSize` 的
构造性结果，因此这个 `else` 分支在**今天的数据下是死代码/不具区分力的
tautology**；只有当未来某层的 `squad_leashed` 真的被翻成 `0`（正是 F1 falsification
用的那种场景）时，这个分支才会从死代码变成活跳的判别器——那一天 `formed`
会 > 0 而 `leashedRealised` 仍为 0，这条断言正是唯一能确认「兜底产生了小队但
没有加 leash」而非「形成崩溃」的地方。

（该分支已在 F1 的 L10-unleashed 夹具跑中被实际执行且通过：见上方
`SquadFormationRate` 在 `leashed 0` 时对应代码路径进入 `else` 分支，
`EXPECT_EQ(leashedRealised, 0u)` 通过——间接证实了本条注释描述的场景已经过真实
验证，不是纯理论推导。）

---

## F3（轻微）：`[ SQUADFORM ]` 日志补充归因计数器 + 失败诊断按真实原因分支

### 问题
原诊断信息无条件建议「把本层 `squad_leashed` 设为 0」，但当 `formed` 低于
`rolls` 的真实原因是 `noPartnerAvailable`（名册缺第二个可用核心类型）或
`leaderPlacementFailed`（领队自身放置失败，发生在 leash 判断之前）时，
unleash 完全不能修复问题——该建议会把评审导向错误的根因。

### 修复
- `[ SQUADFORM ]` 打印行新增 `noPartnerAvailable`/`leaderPlacementFailed` 两个字段
  （之前只在 `ShippedSquadChanceRealisesSquadsOnEveryLevel` 打印，
  `SquadFormationRate` 没有）。
- `EXPECT_GE(rate, kSquadFormationFloor[level])` 保持**同一条**、**同一阈值**
  不变（未削弱），但失败附言从单一提示改为按计数器优先级分支的 `diagnosis`
  字符串：`noPartner > 0` → 指向名册缺搭档；否则 `leaderFailures > 0` → 指向
  领队放置失败；两者都为 0 才建议 `squad_leashed`。

### 验证（构造计数器值驱动同一段诊断逻辑，逐分支核实指向正确根因）

任务简报明确允许「fixture 或构造的计数器值」两种验证路径。鉴于在真实游戏放置
链路里人为构造 `noPartnerAvailable>0`（需要一个核心类型只有 1 个成员且
`IsMonsterAvailable` 过滤后仍然唯一的关卡组合）或 `leaderPlacementFailed>0`
（该分支在 `PlaceGroup` 今天的实现下是防御性的、不可达的死代码，见
`Source/monster.cpp:3954-3976` 的注释）代价过高且脆弱，采用了「构造计数器值」
路径：把测试文件里 `diagnosis` 构建那段 if/else-if/else 逐字抽出（分支文案完全
一致，仅在断言内嵌入可比较的分支标签），用四组构造输入驱动，编译并实跑：

```cpp
// /tmp/f3_diagnosis_probe.cpp —— 与测试文件内 diagnosis 构建逻辑逐字一致
```

实跑输出：

```
[CASE A noPartner=7,leaderFailures=0] ok=1 -> noPartner-branch: 7 roll(s) had no available core partner ... NOT squad_leashed: ...
[CASE B noPartner=0,leaderFailures=4] ok=1 -> leaderFailures-branch: 4 roll(s) failed to place the LEADER itself ... squad_leashed cannot fix it
[CASE C noPartner=0,leaderFailures=0] ok=1 -> squad_leashed-branch: leader placement and partner selection both succeeded on every roll ... squad_leashed goes to 0 ...
[CASE D noPartner=3,leaderFailures=3] ok=1 -> noPartner-branch: ...
ALL CASES PASSED (0 failures)
EXIT: 0
```

四个场景全部命中期望分支：
- A（仅 `noPartner>0`）→ 指名 roster 缺搭档，明确排除 `squad_leashed`；
- B（仅 `leaderFailures>0`）→ 指名领队放置失败，明确排除 `squad_leashed`；
- C（两者皆 0，今天出厂表的实际状态）→ 才建议 `squad_leashed`；
- D（两者皆非 0）→ 验证 if/else-if 的优先级顺序，`noPartner` 优先于
  `leaderFailures` 被报告（符合代码里 `if (noPartner > 0) ... else if
  (leaderFailures > 0) ...` 的书写顺序）。

（该探测脚本 `/tmp/f3_diagnosis_probe.cpp`/`/tmp/f3_diagnosis_probe` 为
`/tmp` 下的一次性验证产物，不在仓库内，无需撤销。）

---

## F4（轻微）：`rolls==0` 分支诊断文案改为报告真实值

### 问题
原 `std::cout` 无条件打印
`"squads disabled by the table (squad_chance 0)"`，但真正判定「squad_chance 是否
为 0」的是紧邻的 `EXPECT_EQ(params->squadChance, 0u)`——若该断言本身失败
（`squadChance != 0` 但 `rolls==0`），日志里这行诚实的 EXPECT 失败消息旁边却站着
一句自相矛盾的「squad_chance 确实是 0」的错误陈述。

### 修复
`EXPECT_EQ(params->squadChance, 0u) << ...` 本身**完全未改**，仍能在
`squadChance != 0` 时变红。`std::cout` 改为无条件打印真实的 `squadChance` 值和
`rolls`（`0 rolls in kSeeds seeds`），不再对「为什么 rolls 是 0」下断言式的
结论。

### 验证
出厂表下没有任何层的 `rolls==0`（15 层全部 `squad_chance=30`
或 `10`，均 > 0，且 50/500 次种子跑下 `rolls` 均 > 0，见前述实跑日志），因此
该分支在真实回归中天然不会触发。已通过阅读代码确认：`EXPECT_EQ` 判定式与
`squadChance` 值的读取路径未变，唯一改动是 `std::cout` 的字符串拼接从固定文案
换成 `<< static_cast<int>(params->squadChance) << ...`，不影响任何断言求值。

---

## 完整实跑证据（永久修复，出厂表，非临时夹具）

### `level_roster_baseline_test` 全 10 用例

```
[==========] Running 10 tests from 2 test suites.
...
[==========] 10 tests from 2 test suites ran. (453654 ms total)
[  PASSED  ] 10 tests.
```

关键行（`SquadPlacementTest.SquadFormationRate`，出厂表 L10 `leashed 1`，
恢复后再次确认）：
```
[ SQUADFORM ] level 10 leashed 1 rolls 2718 formed 2715 rate 0.998896 noPartnerAvailable 0 leaderPlacementFailed 0 floor 0.95
[       OK ] SquadPlacementTest.SquadFormationRate (271675 ms)
```

`ShippedSquadChanceRealisesSquadsOnEveryLevel`（出厂表，恢复后）：
```
[ SHIPPEDSQUAD ] level 10 eligible 894 rolls 267 rollRate 0.298658 realised 267 realisation 1 formed 267 minions 532 lost(leader/partner) 0/0
[       OK ] SquadPlacementTest.ShippedSquadChanceRealisesSquadsOnEveryLevel (26616 ms)
```

### 门禁

```
python3 tools/run_tests.py --json /tmp/ci.json
```
```json
{
  "ctest": { "passed": 764, "failed": 0, "skipped": 3, "total": 764, "passed_pct": 100, "returncode": 0 },
  "drift": {
    "drift_ok": true, "passes": 5,
    "output": "PASS A ...\nPASS B ...\nPASS C ...\nPASS C2 ...\nPASS E ..."
  }
}
```

```
python3 -m tools.eval.backend --smoke
```
```
evaluated: 36  passed: 36  failed: 0  skipped: 0  pass_rate: 1.0
```
（`level-rosters.yaml` 不在 smoke 集合内，属于按 `mpq_required`/耗时单独跑的
case；其 `passed_min: 10` 与 `output_contains` 的 `"[  PASSED  ] 10 tests."`
本轮**测试数未变**，无需同步。）

---

## 未做的事 / 约束遵守确认

- 未修改任何生产代码（`Source/monster.cpp`、`Source/monster.h` 均只读理解，
  未落笔）。
- 未修改任何出厂表格数据（`assets/txtdata/monsters/level_roster_params.tsv`
  等），临时夹具改动已全部撤销，`git status --short` 确认无残留。
- 未修改 `kRangedShareCeiling`/`kSquadFormationFloor` 的**阈值数值**，只改了
  相邻的诊断文案/注释。
- 未新增/删除任何 `TEST_F`，测试计数保持 10，`eval/cases/rng/level-rosters.yaml`
  无需同步。
- 未使用任何子代理（subagent/workflow/agent_teams_*）；全部验证由本 agent 直接
  用 `bash`/`read`/`edit` 完成。
- 未写入 `docs/`。
- C++ 文件行尾确认为 CRLF（`file test/level_roster_baseline_test.cpp` →
  `with CRLF line terminators`）。
