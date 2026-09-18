# Task 1 报告：G1 —— 普通 leader 死亡释放随从并清除悬挂索引

**状态：** DONE_WITH_CONCERNS（实现与测试完成、三道门禁全绿；唯一关注点是必须同步 eval case 计数，超出简报文件清单，见下）

## 提交

| SHA | 说明 |
|---|---|
| `38e9b58e1` | `test(monster): failing G1 cases for ordinary leader death (red)`（TDD red，独立提交以留下失败证据） |
| `2d425d806` | `fix(monster): release minions when an ordinary leader dies (G1)`（简报指定的提交信息） |
| `5cadd5df4` | `test(eval): sync sampling case count for the two G1 cases (26 -> 28)` |

## 改动文件

- `Source/monster.cpp`：`ReleaseMinions`、`M_UpdateRelations`
- `test/sampling_behavior_test.cpp`：新增 2 个用例 + 2 个测试辅助函数

`Source/monster.h` **未改动**：`ReleaseMinions` 原本就在 `monster.cpp` 的匿名命名空间内（无头文件声明），
两个调用点（`M_UpdateRelations`、`ScavengerAi` 一侧的 `ShrinkLeaderPacksize` 不涉及）都在同 TU 内，
按简报「若它原本是文件内 static，则本任务可以让它保持 static」处置，因此新增第二参用**默认实参**
（`bool clearReference = false`）保持既有调用语义，不导出到头文件。

## 实现（采用控制者裁决 RB3 的推荐版）

```cpp
void ReleaseMinions(const Monster &leader, bool clearReference = false)
{
	for (size_t i = 0; i < ActiveMonsterCount; i++) {
		Monster &minion = Monsters[ActiveMonsters[i]];
		if (minion.leaderRelation == LeaderRelation::Leashed && minion.getLeader() == &leader) {
			minion.setLeader(nullptr);
			if (clearReference)
				minion.leader = Monster::NoLeader;
		}
	}
}

void M_UpdateRelations(const Monster &monster)
{
	ReleaseMinions(monster, /*clearReference=*/!monster.isUnique());
	ShrinkLeaderPacksize(monster);
}
```

未采用含 `std::any_of(&Monsters[ActiveMonsters[0]], …)` 的草稿版（依赖 `ActiveMonsters` 区间连续且非空，
脆弱且是重复扫描；`ReleaseMinions` 自身已按 `getLeader() == &leader` 过滤，对无随从的怪物是空循环，
语义与旧行为一致）。

## 测试用例

两条用例都走**引擎真实死亡路径**（禁令 6：不得占位/自证）：

- `SamplingBaselineTest.LeaderDeathReleasesMinions`（普通 leader）
- `SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged`（unique 回归：relation 置 None、索引**保留**）

驱动方式：`MonsterDeath(leader, dir, false)`（`StartMonsterDeath` / `M_StartKill` 汇聚到的公共入口）
起死亡动画 → 按 `ProcessMonsters` 的方式推进 `animInfo.processAnimation()` 到最后一帧 →
执行与 `monster.cpp` 内 file-static `MonsterDeath(Monster&)` 最后一帧分支相同的动作
（`isInvalid = true; M_UpdateRelations(...)`）。

**为何不直接调 `KillMonster(leader, false)`（简报骨架的写法）：** 本仓库**不存在** `KillMonster` 符号
（`grep` 全仓无匹配）。等价公共入口是 `M_StartKill(monster, player)` / `StartMonsterDeath(monster, player, sendmsg)`，
两者都要求一个真实 `Player`，且最终都落到 `MonsterDeath(monster, md, sendmsg)`。测试直接用
`MonsterDeath(...)`（`monster.h:540` 导出）以避免 `GetDirection` 对玩家位置的额外依赖，路径本身完全一致。
`M_UpdateRelations` 在 `monster.h:544` 已导出，故最后一帧的 relation 更新可以直接驱动，无需复制生产逻辑。

死亡路径的前置：`MonsterDeath` 会跑 `SpawnLoot -> SpawnItem -> AllItemsList` 与 `PlayEffect`（读 `MyPlayer`）。
sampling 夹具原本只加载怪物/名册数据，故新增 `PrepareDeathPathPrerequisites()`（`LoadItemData()` +
`Players.resize(1)` + `MyPlayer`），对齐 `diablo.cpp` 中「item 数据与玩家先于任何怪物死亡」的真实顺序。
未加此前置时 ASan 在 `items.cpp:1390`（`GetItemIndexForDroppableItem` 空 `AllItemsList`）SEGV。

## 命令与真实输出

### TDD 步骤 2：确认失败（红）

```
$ cmake --build build --target sampling_behavior_test -j8
$ ./sampling_behavior_test --gtest_filter='SamplingBaselineTest.LeaderDeathReleasesMinions:SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged'
[ RUN      ] SamplingBaselineTest.LeaderDeathReleasesMinions
test/sampling_behavior_test.cpp:1711: Failure
Expected: (minion.leaderRelation) != (LeaderRelation::Leashed), actual: 1-byte object <01> vs 1-byte object <01>
an ordinary leader's death must not leave its minion leashed
test/sampling_behavior_test.cpp:1713: Failure
Expected equality of these values:
  minion.leader
    Which is: '\0'
  Monster::NoLeader
    Which is: '\xFF' (255)
a stale leader index can point at a live stranger after slot reuse
test/sampling_behavior_test.cpp:1715: Failure
Expected equality of these values:
  minion.getLeader()
    Which is: 0x60333a9cc540
  nullptr
    Which is: (nullptr)
getLeader() must stop resolving to the dead leader's slot
[  FAILED  ] SamplingBaselineTest.LeaderDeathReleasesMinions (12 ms)
[ RUN      ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged
[       OK ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged (11 ms)
[  PASSED  ] 1 test.
[  FAILED  ] 1 test
```

失败形态与简报预期一致：随从仍 `Leashed`，且 `leader` 索引仍是 0（指向 leader 槽位）。
注意 unique 回归用例在**修复前就通过**——这正是它作为回归守卫的意义（unique 路径本就正确，
本次改动必须不动它）。

### TDD 步骤 4：确认通过（绿）

```
$ cmake --build build --target sampling_behavior_test -j8
$ ./sampling_behavior_test --gtest_filter='SamplingBaselineTest.LeaderDeathReleasesMinions:SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged'
[ RUN      ] SamplingBaselineTest.LeaderDeathReleasesMinions
[       OK ] SamplingBaselineTest.LeaderDeathReleasesMinions (11 ms)
[ RUN      ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged
[       OK ] SamplingBaselineTest.UniqueLeaderDeathBehaviourUnchanged (10 ms)
[  PASSED  ] 2 tests.
```

### 全量 sampling 二进制

```
$ ./sampling_behavior_test
[==========] 30 tests from 2 test suites ran. (5045 ms total)
[  PASSED  ] 30 tests.
```

### 行尾（禁令 7）

```
$ file Source/monster.cpp test/sampling_behavior_test.cpp
Source/monster.cpp:              C++ source, ASCII text, with CRLF line terminators
test/sampling_behavior_test.cpp: C++ source, Unicode text, UTF-8 text, with CRLF line terminators
```

字节级核对（`monster.cpp`）：改前 crlf=5144 / cr=5144 / lf=5144，改后 crlf=5160 / cr=5160 / lf=5160
——全文件纯 CRLF，无孤立 CR、无 LF 泄漏。

### 全量门禁

```
$ python3 tools/run_tests.py --json /tmp/ci.json
=== SUMMARY ===
{
  "timestamp": "2026-09-16 17:46:16",
  "build_dir": "/home/peng/workspace/DevilutionX/build",
  "steps": {
    "build": { "ok": true, "message": "build ok" },
    "ctest": {
      "passed": 747, "failed": 0, "skipped": 3, "not_run": 0,
      "total": 747, "failures": [], "passed_pct": 100, "returncode": 0
    },
    "drift": {
      "drift_ok": true, "passes": 5,
      "output": "PASS A  Tests.cmake entries have source files\nPASS B  no placeholder assertions in tests\nPASS C  modified files keep line endings\nPASS C2 added files match .editorconfig\nPASS E  no test-only production functions\n"
    }
  }
}
[exit code: 0]
```

`passed_pct == 100`、`failed == 0`、`drift_ok == true`，达标。漂移检查 B（无占位断言）与 E（无测试专用生产函数）均 PASS。

### eval 门禁

```
$ python3 -m tools.eval.backend --smoke
  by category:
    dark-expedition      4/4 (1.0)
    save-load            2/2 (1.0)
    mechanics            8/8 (1.0)
    data                 6/6 (1.0)
    render               2/2 (1.0)
    combat               5/5 (1.0)
    utility              9/9 (1.0)
EXIT=0
```

## eval case 同步（超出简报文件清单，需披露）

简报的可改文件清单只有 `Source/monster.cpp` / `Source/monster.h` / `test/sampling_behavior_test.cpp`，
但 `eval/cases/rng/sampling-anti-monopoly.yaml` **必须**同步：该 case 的 `output_contains`
硬编码了 `[  PASSED  ] 26 tests.` 与 `passed_min: 26`（SamplingBaselineTest 的用例总数）。
新增 2 条用例后计数变 28，未同步时 eval 门禁报
`[FAIL] [rng] sampling-anti-monopoly-cap (0/15) stdout missing: '[  PASSED  ] 26 tests.'`。
该文件自身的 R35 维护注释即要求「新增 SamplingBaselineTest 用例时同步此计数」，故按其指示改为 28
并补了一段说明新增覆盖的描述。文件为 LF，改后仍为 LF。

提交：`5cadd5df4` `test(eval): sync sampling case count for the two G1 cases (26 -> 28)`（独立提交，便于回溯）。

同时也满足 CLAUDE.md「行为变更必产 eval」：G1 是引擎行为变更，其覆盖记录在该 case 的描述里，
断言权威仍是 gtest 二进制。

## 自检结论

1. **RB3 裁决已遵守**：`M_UpdateRelations` 用推荐版（无 `hasLeashedMinions()` 闸门、无 `std::any_of` 草稿），
   未引入 `MinionOptions`（属 Task 2）。
2. **unique 路径逐字节不变**：`clearReference = !isUnique()` 对 unique 为 `false`，
   走的仍是纯 `setLeader(nullptr)`（relation → None、索引保留）；
   `UniqueLeaderDeathBehaviourUnchanged` 修复前后都通过，即回归守卫成立。
3. **测试驱动生产路径**：走 `MonsterDeath(...)` + 真实动画推进 + 导出的 `M_UpdateRelations`，
   不是对被测逻辑的复述；修复前红、修复后绿，红色形态与简报预期一致。
4. **无既有测试/eval 依赖旧行为**：`grep -E 'Leashed|setLeader|getLeader|leaderRelation|packSize'` 扫
   `test/` 与 `eval/`，除本测试文件外只命中 `test/fixtures/memory_map/monster.txt` 的字段布局行
   （仅记录偏移，不断言释放语义）。故简报第 6 步的「冲突上报」无需触发。
5. **现网暂不可见（设计如此）**：目前 `PlaceGroup` 只在 `PrepareUniqueMonst`（`Source/monster.cpp:3423`）
   一处传入 leader，出货内容里没有普通怪当 leader，所以 G1 是**真实的正确性修复但尚无生产触发点**，
   等 Task 2/3 引入普通怪小队后才会被实际走到。另一处 `M_UpdateRelations` 调用点
   `Source/msg.cpp:888`（delta 载入路径）现在行为一致。

## 遗留关注点

- 上述 eval YAML 的**文件清单越界**（已说明必要性与依据）。
- `Monster::setLeader(nullptr)` 保留索引的设计（为 monhealthbar 给被 buff 随从上蓝色）本身仍是悬挂来源；
  本任务只在普通怪路径上清了它。unique 路径的悬挂索引按简报要求**刻意保留**，
  若后续要统一收口，需要先给 monhealthbar 换一个不依赖 `leader` 索引的信息源——建议留给 Task 2/3 评估。
- `sampling-anti-monopoly.yaml` 里硬编码用例计数这种耦合，后续每加一条 SamplingBaselineTest 用例都要改；
  可考虑改成 `>=` 语义，但那超出本任务范围。
