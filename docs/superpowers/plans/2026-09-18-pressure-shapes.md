# A 腿（逐层交战形态）实施计划

> **面向执行者：** 内联执行（沿用本会话约定：**不使用子 agent** 做实现），每完成一项派**独立复核** ✓。
> 步骤用复选框（`- [ ]`）跟踪；**每条断言都必须能"改坏即红"** ✓。

**目标**：用**既有机制**（每层小队参数 + 放置策略 + 构成）为四个层段塑形"交战形态"，使深处的战斗重新逼迫玩家**准备与撤回**；**不改任何数值** ✗。

**架构**：三种手段，全部已存在——①`level_roster_params.tsv` 的 `squad_chance/squad_size/squad_leashed`（**现状全层统一 30/2/1** ✓）；②名册侧小队放置（`Source/monster.cpp:3960-3995`：一个 leader + `squadSize` 个**同层不同 core** 随从 ✓）；③构成（`level_rosters.tsv` ✓）。守卫用既有仪器 `GetSquadRollStats()`（`monster.h:596` ✓，重置于 `InitLevelMonsters():3555` ✓）。

**规格**：`docs/superpowers/specs/2026-09-18-pressure-shapes-design.md`（**已批准** ✓；§4.2b 落实方式、§6.1b 守卫清单、附录 C/E/F/G 基线）

## 全局约束

- **不使用子 agent 实现**；每完成一项派**独立复核**（失败＝未复核，必须改派 ✓）
- **不改数值** ✗：`monstdat` 数值字段、掉落表、物价**逐字节不变**（由 (g) 守卫保证 ✓）
- **L16 原子性**：补参数行与守卫改动**必须同一次提交** ✓（§4.2b 已决 ✓）
- 行尾：TSV/C++ **CRLF** ✓；`.md/.py/.yaml/.sh/.json` **LF** ✓；**所有编辑（含反证与恢复）完成后行尾归一化必须是最后一步** ✓
- **提交前对"提交信息声称写入的每一处"做计数 grep** ✓；校验失败**必须非零退出**（`if not all(checks): sys.exit(1)` ✓）
- **同一 `build/` 只能有一个执行者** ✓；杀进程后用 `ps` 验证归零再启动新构建 ✓
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json`（**只看 `ctest.failed==0`、`passed_pct==100`、`drift.drift_ok==true`** ✓）+ `python3 -m tools.eval.backend --smoke`；push 后跟踪 CI 到终态 ✓
- 台账（`docs/superpowers/ledgers/**`）随提交进 git ✓

---

## 文件结构

| 文件 | 职责 |
|---|---|
| `assets/txtdata/monsters/level_roster_params.tsv` | **改**：补 **L16 行** ✓ + 按层段调 `squad_chance/squad_size/squad_leashed` |
| `test/fixtures/txtdata/monsters/level_roster_params_squads_*.tsv` | **已有**（复核确认 ✓）：反证用的坏参数 fixture |
| `test/pressure_shapes_test.cpp` | **新建**：守卫 (a)(b)(c)(d)(f)(f2)(g) |
| `CMake/Tests.cmake` | **改**：注册 `pressure_shapes_test`（漂移 A/F 强制 ✓） |
| `tools/gen_pressure_tables.py` | **新建**：生成附录 E/F/G 的表与测试黄金集（(e) 要求"表由脚本生成" ✓） |
| `test/fixtures/pressure/*.json` | **新建**：黄金集（类型集合、AI→类型映射、数值表哈希） |
| `test/sampling_behavior_test.cpp` | **改**（T7）：输出"遭遇同时 ≥2 类型"比例 |
| `eval/cases/combat/pressure_shapes.yaml` | **新建**：把守卫接入 eval |

---

## 任务 1：L16 参数行 + 守卫原子改动

**文件**：`assets/txtdata/monsters/level_roster_params.tsv`、`test/pressure_shapes_test.cpp`（新建）、`CMake/Tests.cmake`

- [ ] **步骤 1：确认 L16 现状（读，不改）**

运行：`awk -F'\t' '{print $1}' assets/txtdata/monsters/level_roster_params.tsv | tr '\n' ' '`
预期：`level 1 2 … 15 17 … 24`（**无 16** ✓）

- [ ] **步骤 2：写失败的测试（先证明 L16 缺失会红）**

```cpp
#include <gtest/gtest.h>
#include "tables/level_roster.h"

using namespace devilution;

TEST(PressureShapesTest, EveryLevelHasSquadParameters)
{
	// Author decision (2026-09-18): level 16 gets its row so the squad mechanism is not
	// silently bypassed there. The roster still holds L16 cores, so adding the row later
	// without this guard would make squads appear on that level unnoticed.
	for (uint8_t level = 1; level <= 24; level++) {
		EXPECT_NE(GetLevelRosterParams(level), nullptr) << "level " << static_cast<int>(level) << " has no squad parameter row";
	}
}
```
在 `CMake/Tests.cmake` 的测试列表加入 `pressure_shapes_test`（漂移 A/F ✓）。

- [ ] **步骤 3：运行并确认失败**

运行：`cmake --build build --target pressure_shapes_test -j8 && ./build/pressure_shapes_test`
预期：**FAIL**，且消息含 `level 16 has no squad parameter row` ✓

- [ ] **步骤 4：补 L16 行（数据）**

在 `level_roster_params.tsv` 的 L15 与 L17 之间插入（列序：`level / max_image / tail_draw / class_floors / squad_chance / squad_size / squad_leashed`）：
```
16	6000	1	Melee=2	30	2	1
```
（`tail_draw=1`、`class_floors=Melee=2` 沿用 L19/L20 的深层口径 ✓；**数值字段一律不动** ✗）

- [ ] **步骤 5：重建并确认通过 + 行尾归一化**

运行：`cmake --build build --target pressure_shapes_test -j8 && ./build/pressure_shapes_test`
预期：**PASS** ✓
⚠ TSV 必须保持 **CRLF** ✓（python 往返后立刻归一化 ✓）

- [ ] **步骤 6：反证**

临时把 L16 行删掉 ⇒ 重跑 ⇒ **必须红** ✓ ⇒ 恢复 ⇒ 绿 ✓（两次输出内联记录 ✓）

- [ ] **步骤 7：提交（原子 ✓）**

```bash
git add assets/txtdata/monsters/level_roster_params.tsv test/pressure_shapes_test.cpp CMake/Tests.cmake
git commit -m "feat(roster): give level 16 squad parameters and guard it"
```

---

## 任务 2：按层小队比率守卫（(f)，N=100）

**文件**：`test/pressure_shapes_test.cpp`

- [ ] **步骤 1：写测试（先跑通骨架，阈值先取现状基线）**

```cpp
TEST(PressureShapesTest, SquadRateMeetsBaselinePerLevel)
{
	// Baseline fixture (spec appendix G): chance=30 on every level, so rolls/eligible >= 0.30.
	// N=100 builds per level (author decision); L16 included now that it has a row.
	constexpr int N = 100;
	constexpr double Baseline = 0.30 - 0.05; // tolerance
	for (uint8_t level = 1; level <= 24; level++) {
		size_t eligible = 0;
		size_t rolls = 0;
		for (int seed = 0; seed < N; seed++) {
			ASSERT_TRUE(BuildLevelForMeasurement(level, seed)); // existing test helper
			ASSERT_TRUE(InitMonsters());
			const SquadRollCounters &stats = GetSquadRollStats();
			eligible += stats.eligibleCoreDraws;
			rolls += stats.rolls;
		}
		ASSERT_GT(eligible, 0u) << "level " << static_cast<int>(level) << " produced no eligible squad draws";
		EXPECT_GE(static_cast<double>(rolls) / static_cast<double>(eligible), Baseline)
		    << "level " << static_cast<int>(level) << " squad rate below baseline";
	}
}
```
（`BuildLevelForMeasurement` 是本仓既有测试的建关辅助；若签名不同，**照 `test/level_roster_baseline_test.cpp:844-858` 的 `RunLevel` 取真实写法** ✓，不得凭空取名 ✗。）

- [ ] **步骤 2：运行（现状应 PASS：chance=30 ⇒ 约 0.30）**

运行：`./build/pressure_shapes_test --gtest_filter='PressureShapesTest.SquadRateMeetsBaselinePerLevel'`
预期：PASS ✓（若某层红 ⇒ 记录该层实测值，**先修正基线而不是放宽阈值** ✓）

- [ ] **步骤 3：反证（用既有 fixture ✓）**

把某层 `squad_chance` 由 30 改成 5（可在 fixture `level_roster_params_squads_*.tsv` 内做 ✓）⇒ 该层**必须红** ✓ ⇒ 恢复 ⇒ 绿 ✓

- [ ] **步骤 4：提交**

```bash
git commit -am "test(pressure): assert the per-level squad rate baseline"
```

---

## 任务 3：类型等式 / 名册闭包 / 发射点白名单（(a)(b)(c)）

**文件**：`tools/gen_pressure_tables.py`（新建）、`test/fixtures/pressure/*.json`（新建）、`test/pressure_shapes_test.cpp`

- [ ] **步骤 1：写生成器（(e)：表与黄金集必须脚本生成 ✓）**

生成器职责（**扫描而非手写** ✓）：①扫 `Source/monster.cpp` 的 `GetMissileType` 的 `case` 与各 `XxxAi` 内的 `StartRangedAttack/StartRangedSpecialAttack/AddMissile` 字面 `MissileID`（复现附录 E.1/E.1b 的口径 ✓）；②联 `mods/hf/.../misdat.tsv` 的 flags ⇒ AI→类型 ✓；③联 `level_rosters.tsv`×`monstdat`⇒ 各层段类型集合与反制数 ✓；④输出 `test/fixtures/pressure/ai_types.json`、`ai_emission_sites.json`、`segment_types.json` ✓；⑤并用同一份数据**重写规格附录 E/F/G 的表格**（禁止手抄 ✗）。

运行：`python3 tools/gen_pressure_tables.py --write`
预期：三个 JSON + 附录表被（重）生成；`git diff` 若只剩表格内容则说明**文档与代码一致** ✓

- [ ] **步骤 2：写守卫测试**

```cpp
TEST(PressureShapesTest, SegmentTypeSetsMatchGolden) { /* (a) 等式断言 */ }
TEST(PressureShapesTest, RosterAisAreMapped)      { /* (b) 闭包断言 */ }
TEST(PressureShapesTest, EmissionSitesMatchWhitelist) { /* (c) 扫描源码 vs 黄金集 */ }
```
（每条：比较**实际计算结果**与 JSON 黄金集；差异即红 ✓）

- [ ] **步骤 3：逐条反证**

分别：①从某层段类型集里删一个类型 ⇒ (a) 红 ✓；②往名册塞一个未映射 AI ⇒ (b) 红 ✓；③在某 `Ai*` 里加一行 `AddMissile(..., MissileID::X, ...)` ⇒ (c) 红 ✓；④手改 JSON 黄金集 ⇒ 重跑生成器 ⇒ 与源码不一致 ⇒ 红 ✓

- [ ] **步骤 4：提交**

```bash
git add tools/gen_pressure_tables.py test/fixtures/pressure test/pressure_shapes_test.cpp docs/superpowers/specs/2026-09-18-pressure-shapes-design.md
git commit -m "test(pressure): generate and assert the type, mapping and emission-site tables"
```

---

## 任务 4：前提断言（(d)）

- [ ] **步骤 1：写测试**

```cpp
TEST(PressureShapesTest, CounterPoolHasNoCursesAndNoAcidResistance)
{
	// (d) premise: curses are not counters; D1 has no acid resistance family (verified 2026-09-18).
	// If either changes, the guard thresholds in the spec must be re-derived.
	EXPECT_FALSE(AffixFamilyExists("ACIDRES"));
	for (const auto &family : CounterFamilies())
		EXPECT_EQ(family.find("_CURSE"), std::string::npos) << family;
}
```

- [ ] **步骤 2：反证**：临时加一条 `ACIDRES` 到 fixture 词缀表 ⇒ 红 ⇒ 恢复 ⇒ 绿 ✓
- [ ] **步骤 3：提交**

---

## 任务 5：逐字节不变量（(g)）

- [ ] **步骤 1：记录黄金哈希**（`test/fixtures/pressure/immutable_tables.sha256`）

覆盖：`assets/txtdata/monsters/monstdat.tsv`、`mods/hf/txtdata/monsters/monstdat.tsv`、`assets/txtdata/items/itemdat.tsv`、掉落相关表、`Source/stores.cpp` 的价格常量所在表 ✓

- [ ] **步骤 2：写测试**：逐文件比对 sha256 ✓（**合法改动必须显式更新该文件**，更新本身即"我越界了"的可见记录 ✓）
- [ ] **步骤 3：反证**：改任一被覆盖文件的一行 ⇒ 红 ⇒ 恢复 ⇒ 绿 ✓
- [ ] **步骤 4：提交**

---

## 任务 6：形态调参（数据为主）+ L1-8 可绕性

- [ ] **步骤 1：按层段定参数（草案 ⚠ 需实测校准）**

| 层段 | squad_chance | squad_size | squad_leashed | 意图 |
|---|---|---|---|---|
| L1-8 | 25 | 2 | 1 | 略降（教学段）+ **通路可绕** |
| L9-12 | 35 | 2 | 1 | 略升（狭窄压制） |
| L13-16 | 45 | 3 | 1 | 消耗战 |
| L17-24 | 40 | 3 | 1 | 精英抉择（**以小队为主** ✓） |

- [ ] **步骤 2：写 L1-8 可绕性断言**（(e) 的放置侧 ✓）：对该段每层采样，断言"至少存在一条不经过小队锚点的通路"（用既有地图遍历辅助；**若现有辅助不足，先停下报告**，不得自造判据 ✗）
- [ ] **步骤 3：重跑任务 2 的 (f) 守卫并**按新基线重钉阈值** ✓（`0.30-0.05` → 按各层段实测值，逐层写死 ✓）
- [ ] **步骤 4：反证**：把 L13-16 的 `squad_chance` 调回 30 ⇒ 该段 (f) 断言**必须红** ✓ ⇒ 恢复 ⇒ 绿 ✓
- [ ] **步骤 5：提交**

---

## 任务 7：补上"遭遇同时 ≥2 类型"仪器 ⚠

**文件**：`test/sampling_behavior_test.cpp`（或 `test/pressure_shapes_test.cpp`）

- [ ] **步骤 1：定义并实现**：一次建关后，取同时活跃（`MFLAG_ACTIVE`/与玩家同层）的怪物集合，计算"存在 ≥2 种需求类型"的**层-种子**比例 ✓；用与附录 G.4b 相同的 N=100 采样 ✓
- [ ] **步骤 2：打印 + 断言**：先打印现状比例（记录为基线 ✓），再按"不低于现状 − 容差"写断言 ✓（**不得**先写断言再调数据 ✗）
- [ ] **步骤 3：反证**：把某层构成改成单类型（fixture）⇒ 红 ⇒ 恢复 ⇒ 绿 ✓
- [ ] **步骤 4：提交**

---

## 任务 8：收尾（eval / 门禁 / 漂移 / CI / 规格实施记录）

- [ ] **步骤 1：eval 用例**：`eval/cases/combat/pressure_shapes.yaml`（照既有 schema ✓：`setup/run/expected`；`passed_min` 与 `output_contains` 计数必须同步 ✓，含 R35 维护注释 ✓）
- [ ] **步骤 2：eval smoke**：`python3 -m tools.eval.backend --smoke` ⇒ **exit 0** ✓（失败即停，不提交 ✗）
- [ ] **步骤 3：全量门禁**：`python3 tools/run_tests.py --json /tmp/ci.json` ⇒ `failed==0 && passed_pct==100 && drift_ok==true` ✓（**只看标志** ✓）
- [ ] **步骤 4：提交后跑漂移**（顺序不能反 ✓）：`python3 tools/check_drift.py --base origin/master` ⇒ 6/6 ✓
- [ ] **步骤 5：push + CI 到终态**：`git push myrepo feature/qol-upgrades` → `gh run list` → `gh run watch <id> --exit-status` ✓
- [ ] **步骤 6：规格实施记录 + 台账**：A 腿规格 §7 追加实测（各层段最终参数、各守卫实测值、CI run id ✓）；台账追加本轮结论 ✓

---

## 自检

**1. 规格覆盖度**

| 规格条目 | 任务 |
|---|---|
| §4.2b 四形态落实（含 L16 已决 ✓） | 任务 1（L16 行）+ 任务 6（分带参数 + 可绕性） |
| §6.1b (a)(b)(c) 类型/闭包/发射点 | 任务 3 |
| §6.1b (d) 前提（无 `*_CURSE`/无 `ACIDRES`） | 任务 4 |
| §6.1b (e) 表由脚本生成 | 任务 3 步骤 1 |
| §6.1b (f) 按层小队比率（N=100，含 L16） | 任务 2 + 任务 6 步骤 3 |
| §6.1b (f2) L16 参数行存在 | 任务 1 |
| §6.1b (g) 逐字节不变量 | 任务 5 |
| §6.4 仍缺的"遭遇同时类型"仪器 | 任务 7 |
| §6.2/§6.3 试玩与撤销 | **不在本计划**（需人执行 ✓，随规格 §6.2 单独进行） |

**2. 占位符扫描**：无 `TBD/TODO`；任务 2/6/7 的**阈值与参数为"先测后钉"**（明确写了"先记录基线，再钉阈值"✓，不是占位符 ✓）。
**3. 类型一致性**：`GetLevelRosterParams(uint8_t)` ✓、`GetSquadRollStats()` ✓、`SquadRollCounters` 字段名以 `monster.h:544/548/553/555/568` 为准 ✓；测试辅助 `BuildLevelForMeasurement` 若不存在则**照 `test/level_roster_baseline_test.cpp:844-858` 取真实写法** ✓（不得凭空取名 ✗）。

---

## 执行交接

按作者既有指示**内联执行** ✓，**每完成一项派独立复核** ✓。
检查点：任务 1 步骤 6（L16 原子性与反证）、任务 2 步骤 3（fixture 反证）、任务 5 步骤 3（不变量反证）、任务 6 步骤 4（调参反证）、任务 8 步骤 3（全量门禁只看标志）。
**不使用任何子 agent 做实现** ✓。