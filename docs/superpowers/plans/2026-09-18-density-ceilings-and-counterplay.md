# 内容密度收顶与指名反制 实施计划

> **面向执行者：** 本计划**内联执行**（沿用本会话既有约定：不使用子 agent），可用 `superpower-executing-plans` 批量推进并在检查点复核。步骤用复选框（`- [ ]`）跟踪。

**目标：** 给已实施的内容密度契约补上**上限**（core 规模、单局种类、相邻层区分度三条可失败判据），把红线 10 的**指名反制**补齐并纳入验证，并整改因此暴露的三个越界层（L2/L3/L8）。

**架构：** 三条上限判据中 B1/B3 是**名册表的静态属性**（`GetLevelRoster()` 读出 core 集合即可，不需要 HF 素材）；B2 复用既有的 200-seed 测量循环（非 HF 层在 sampling 二进制，HF 层在其门控套件）。判据先落地并**当场暴露越界**，再用数据整改，最后以既有地板与占比判据确认整改没有牺牲密度或压力。

**技术栈：** C++23 / CMake+Ninja / GoogleTest / TSV 数据 / eval YAML。

**规格：** `docs/superpowers/specs/2026-09-18-density-ceilings-and-counterplay-design.md`（已批准）——论证以规格为准，执行时同读；上位文档 = 内容密度契约 `2026-09-16-content-density-contract-design.md`。

## 全局约束

- **不使用任何子 agent**（作者指示）：内联执行，遇阻停下问，不改走委派
- 行尾：C++/TSV **CRLF**；`.md/.py/.yaml/.sh/.json` **LF**；改动文件保持既有类型（漂移 C），新增文件匹配 `.editorconfig`（C2）
- **漂移只在 commit 之后跑才有意义**（C2 只检查 HEAD 里的文件）：顺序固定为 **改 → 提交 → 跑漂移 → push**（见知识条目 `gotcha_drift_check_c2_whole_file_crlf.md`）
- 改 `assets/txtdata/**.tsv` 后必须 `ninja -C build devilutionx_mpq` 再测量
- **每条新判据必须可失败**：给出"改坏→必红→恢复→绿"的实跑，证据**内联**写进报告（不得只引用 `/tmp`）
- **整改层（L2/L3/L8）整改后必须同时满足**：B1/B2 上限 ✓、并集与种类地板 ✓、占比主判据 ≤ 同种子 vanilla ✓；**任一条做不到就按内容密度契约附录 C 三选一回作者改判，不得放宽阈值**
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json`（`failed==0 && passed_pct==100 && drift_ok==true`）+ `python3 -m tools.eval.backend --smoke`；push 后跟踪 CI 到终态
- 提交信息英文 conventional commits；台账（`docs/superpowers/ledgers/**`）随提交一起进 git

---

## 文件结构

| 文件 | 职责 |
|---|---|
| `test/sampling_behavior_test.cpp` | 新增 B1/B2/B3 三条判据（B1/B3 静态 L1-24；B2 复用地板用例的测量循环） |
| `test/level_roster_baseline_test.cpp` | 仅在 B2 的 HF 部分需要时修改（HF 层上限 ≤8） |
| `assets/txtdata/monsters/level_rosters.tsv` | 整改 L2/L3/L8 的 core 行 |
| `assets/txtdata/monsters/level_roster_params.tsv` | 仅在整改需要时调 `tail_draw`（L2 可能需要） |
| `docs/knowledge/decision_content_density_contract.md` | C：加"指名反制"关联节 |
| `docs/superpowers/protocols/2026-09-18-fun-measurement.md` | C：预测 5 增加"先杀 leader 拆队"一条 |

---

## 任务 1：B3——相邻层 core 区分度（先落地，因为它是预防型守卫）

**文件：** 修改 `test/sampling_behavior_test.cpp`（`SamplingBaselineTest` 套件内，紧跟 `RosterCoreAlwaysPresent` 之后）
**接口：** 产出 `AdjacentLevelCoresRemainDistinct`；依赖 `GetLevelRoster(uint8_t) -> std::span<const LevelRosterEntry>`、`LevelRosterRole::Core`

- [ ] **步骤 1：写用例（B3）**

```cpp
TEST_F(SamplingBaselineTest, AdjacentLevelCoresRemainDistinct)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Identity needs neighbouring levels to be tellable apart. Measured 2026-09-18: the
	// largest adjacent core-set Jaccard is 0.33 (L14-L15, L11-L12), so the bound of 0.4 has
	// margin and fails only when two neighbours become near-copies of one another. The
	// core sets come straight off the roster table, so this needs no Hellfire assets.
	constexpr double kMaxAdjacentCoreJaccard = 0.4;
	std::set<_monster_id> previous;
	double worst = 0.0;
	for (uint8_t level = 1; level <= 24; level++) {
		std::set<_monster_id> cores;
		for (const LevelRosterEntry &entry : GetLevelRoster(level)) {
			if (entry.role == LevelRosterRole::Core)
				cores.insert(entry.type);
		}
		if (level > 1 && (!previous.empty() || !cores.empty())) {
			std::vector<_monster_id> shared;
			std::set_intersection(previous.begin(), previous.end(), cores.begin(), cores.end(),
			    std::back_inserter(shared));
			std::vector<_monster_id> combined;
			std::set_union(previous.begin(), previous.end(), cores.begin(), cores.end(),
			    std::back_inserter(combined));
			const double jaccard = static_cast<double>(shared.size()) / static_cast<double>(combined.size());
			worst = std::max(worst, jaccard);
			std::cout << "[ COREJACCARD ] levels " << static_cast<int>(level) << "-"
			          << static_cast<int>(level + 1) << " " << jaccard << std::endl;
			EXPECT_LE(jaccard, kMaxAdjacentCoreJaccard)
			    << "levels " << static_cast<int>(level) << " and " << static_cast<int>(level + 1)
			    << " have near-identical core sets, so their identities are indistinguishable";
		}
		previous = std::move(cores);
	}
	// Non-vacuity: if no adjacent pair shared a single type the bound above could never fire.
	EXPECT_GT(worst, 0.0) << "no adjacent pair shares any core type, so the bound proves nothing";
}
```
若缺 `<algorithm>`/`<iterator>` 包含，一并补上（该文件已用 `std::set`、`std::vector`）。

- [x] **步骤 2：运行（2026-09-18 实测：**红**，且是真实缺陷）**

运行：`cmake --build build --target sampling_behavior_test -j20 && ./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.AdjacentLevelCoresRemainDistinct'`
**结果：FAIL —— `[ COREJACCARD ] levels 19-20 = 1`**（L19 与 L20 的 core 集合**完全相同**：VENMTAIL/LASHMORM… 见下）；其余最高 0.333（L11-12、L14-15）✓。另有一起自纠：初版把标签打成 `level-(level+1)`（差一层），已修为 `(level-1)-level`。

- [x] **步骤 3：可失败性证明（用真实数据，不做人工反证）**

本用例**在真实数据上就是红的**（L19-L20 = 1.0 > 0.4）⇒ 可失败性已由现实证明，比人工 sabotage 更强；**未**再做人造反证（记录该取舍）。

- [ ] **步骤 4：提交**

```bash
git add test/sampling_behavior_test.cpp
git commit -m "test(roster): keep neighbouring levels' core sets distinguishable"
```

---

## 任务 2：B1——每层 core ≤5（预期当场暴露 L2/L3/L8）

**文件：** 修改 `test/sampling_behavior_test.cpp`（紧随任务 1 的用例）

- [ ] **步骤 1：写用例（B1）**

```cpp
TEST_F(SamplingBaselineTest, RosterCoreSizeWithinIdentityBound)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// A level's signature set has to stay small enough to be recognisable. Measured
	// 2026-09-18: every level held 2-5 cores except L2 (8), L3 (6) and L8 (6) - the three
	// the density work had inflated - so the bound of 5 is precisely the line they crossed.
	constexpr size_t kMaxCores = 5;
	for (uint8_t level = 1; level <= 24; level++) {
		size_t cores = 0;
		std::set<_monster_id> distinct;
		for (const LevelRosterEntry &entry : GetLevelRoster(level)) {
			if (entry.role != LevelRosterRole::Core)
				continue;
			cores++;
			distinct.insert(entry.type);
		}
		std::cout << "[ CORESIZE ] level " << static_cast<int>(level) << " cores " << cores << std::endl;
		EXPECT_EQ(cores, distinct.size())
		    << "level " << static_cast<int>(level) << " lists the same core type twice";
		EXPECT_LE(cores, kMaxCores)
		    << "level " << static_cast<int>(level) << " lists " << cores
		    << " core types; a signature set that large stops being recognisable";
	}
}
```

- [ ] **步骤 2：运行并确认失败（这是本轮的第一份证据）**

运行：`./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.RosterCoreSizeWithinIdentityBound'`
预期：**FAIL**，报告 **L2 = 8、L3 = 6、L8 = 6**（其余 ≤5）。把 `[ CORESIZE ]` 全表与失败行内联记录。

- [ ] **步骤 3：反证（证明阈值不是摆设）**

把 L4 临时加到 6 个 core → 预期 FAIL → 还原 → PASS。内联记录。

- [ ] **步骤 4：提交（连同预期失败的状态说明写进提交信息）**

```bash
git add test/sampling_behavior_test.cpp
git commit -m "test(roster): bound a level's core set at five types

Currently red on purpose: L2 lists 8 cores, L3 6 and L8 6. The next task reworks that data."
```

---

## 任务 3：B2——单局种类带上限（预期暴露 L2/L3）

**文件：** 修改 `test/sampling_behavior_test.cpp`（`SamplingBaselineTest` 内）

- [ ] **步骤 1：写用例（B2，复用与地板相同的 200 seeds / seed base 51000）**

```cpp
TEST_F(SamplingBaselineTest, PerRunTypesWithinBandCeiling)
{
	if (missingMpqAssets_)
		GTEST_SKIP() << "MPQ assets not found - skipping test";

	// Dilution guard: one run must not show so many kinds that nothing reads as a signature
	// any more. The ceilings are design judgements anchored ABOVE every intentional level
	// (caves currently 8, hell 7/7/6), so they only catch outliers; measured 2026-09-18 they
	// flagged L2 (13.0) and L3 (10.9). The Hellfire band's ceiling lives in its own suite.
	auto ceilingFor = [](uint8_t level) -> double {
		if (level <= 8)
			return 10.0; // Cathedral
		if (level <= 12)
			return 9.0; // Caves
		return 8.0;     // Hell (L13-16)
	};
	constexpr int kSeeds = 200;
	for (uint8_t level = 1; level <= 15; level++) {
		size_t typeSum = 0;
		for (int seed = 0; seed < kSeeds; seed++) {
			currlevel = level;
			InitLevelMonsters();
			SetRndSeed(51000 + static_cast<uint32_t>(seed));
			ASSERT_TRUE(GetLevelMTypes().has_value());
			typeSum += LevelMonsterTypeCount;
		}
		const double types = static_cast<double>(typeSum) / kSeeds;
		std::cout << "[ TYPECEILING ] level " << static_cast<int>(level) << " types " << types
		          << " ceiling " << ceilingFor(level) << std::endl;
		EXPECT_LE(types, ceilingFor(level))
		    << "level " << static_cast<int>(level) << " realises " << types
		    << " types per run, above the band ceiling";
	}
}
```

- [ ] **步骤 2：运行并确认失败**

运行：`./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.PerRunTypesWithinBandCeiling'`
预期：**FAIL**，报告 L2 = 13.0、L3 ≈10.9（其余在带内）。内联记录 `[ TYPECEILING ]` 全表。

- [ ] **步骤 3：反证**

把 L5 的 `tail_draw` 临时改成 6 → 预期 L5 越界 FAIL → 还原（`git checkout -- assets/txtdata/monsters/level_roster_params.tsv` + `ninja -C build devilutionx_mpq`）→ PASS。内联记录。

- [ ] **步骤 4：HF 带上限（L17-24 ≤8）**

在 `test/level_roster_baseline_test.cpp` 的 `HellfirePerSeedVarietyHasAtLeastTwoCombinations` 循环里补一条：
```cpp
		// Band ceiling for the Hellfire levels too (spec B2): the same dilution guard as the
		// Cathedral/Caves/Hell bands in sampling_behavior_test.cpp.
		EXPECT_LE(static_cast<double>(typeSum) / kSeeds, 8.0)
		    << "Hellfire level " << static_cast<int>(level) << " exceeds 8 types per run";
```
（该循环需要同时累加 `typeSum += LevelMonsterTypeCount;`；若原循环没有该累加，一并补上。）

- [ ] **步骤 5：重建并运行两侧**

运行：
```bash
cmake --build build --target sampling_behavior_test level_roster_baseline_test -j20
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.PerRunTypesWithinBandCeiling'
./build/level_roster_baseline_test --gtest_filter='HellfireLevelBaselineTest.HellfirePerSeedVarietyHasAtLeastTwoCombinations'
```
预期：非 HF 层 FAIL（L2/L3，待任务 4 整改）；HF 层 PASS。

- [ ] **步骤 6：提交**

```bash
git add test/sampling_behavior_test.cpp test/level_roster_baseline_test.cpp
git commit -m "test(roster): cap a single run's type count per band"
```

---

## 任务 4：整改 L2/L3/L8 **+ L19/L20**（四条约束同时满足）

> **计划修订（2026-09-18，证据驱动）**：任务 1 的 B3 守卫在真实数据上抓到 **L19 与 L20 的 core 集合完全相同（Jaccard 1.0）** ✗ —— 这是 A2/T2 时期 HF 数据留下的**身份缺陷**，必须与 L2/L3/L8 一起整改。HF 层的约束集与 L1-16 相同（B1 core ≤5、B2 带上限 ≤8、并集/种类地板、占比 ≤vanilla、unique 可达不降、组合数 ≥2）。

**文件：** 修改 `assets/txtdata/monsters/level_rosters.tsv`（必要时 `level_roster_params.tsv`）
**四条约束（缺一不可）**：B1 core ≤5、B2 种类 ≤带上限、并集与种类地板不破、占比主判据 ≤ 同种子 vanilla。

- [ ] **步骤 1：先建立整改前的快照（证据基线）**

运行并内联记录：
```bash
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.ContentDensityWithinVanillaFloor'
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.PerRunTypesWithinBandCeiling'
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.RosterCoreSizeWithinIdentityBound'
./build/level_roster_baseline_test --gtest_filter='LevelRosterBaselineTest.PlacedClassMixWithinBaseline'
```
（最后一条约 2-3 分钟，输出 L1-15 的 `[ MEASURED ]`，含 vanilla 列。）

- [ ] **步骤 2：整改 L8（core 6→5）**

从 `level_rosters.tsv` 移除 L8 的一条 core 行（优先选**非 unique base**、且类内候选充足者可继续从尾池抽到；`MT_UNSEEN` 或 `MT_STALKER` 之一），其余不动。

- [ ] **步骤 3：重建并测量 L8**

```bash
ninja -C build devilutionx_mpq
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.ContentDensityWithinVanillaFloor:SamplingBaselineTest.PerRunTypesWithinBandCeiling:SamplingBaselineTest.RosterCoreSizeWithinIdentityBound' 2>&1 | grep -E 'DENSITY \] level 8|TYPECEILING \] level 8|CORESIZE \] level 8|PASSED|FAILED'
```
预期：L8 core 5、种类 ≤10、并集 ≥17。
若并集掉到 17 以下 → 换一个被移除者（优先移除**仍能从尾池抽到**的类型）再测；**不允许**为此放宽阈值。

- [ ] **步骤 4：整改 L3（core 6→5）**

移除 L3 的一条 core 行（候选：`MT_TSKELAX`——它是我在 A4 阶段为压占比加进去的），重建并测量（把步骤 3 的命令中 `level 8` 换成 `level 3`）。
预期：core 5、种类 ≤10、**并集 ≥25**（L3 的并集恰好等于地板，需特别确认）。

- [ ] **步骤 5：整改 L2（core 8→5，最紧的一层）**

L2 的并集**恰好等于**地板（21），且此前靠"多 core 稀释"才把占比压到 ≤vanilla。按以下顺序尝试（每步都重建 + 测四条）：

1. 移除 `MT_TSKELSD`、`MT_YFALLSP`、`MT_DFALLSD` 等**非 unique base** 的追加 core，只保留 5 条；
2. 若占比回升到 vanilla 之上 → 把 L2 的 `tail_draw` 由 2 降到 1（减少随机远程抽入），再测；
3. 若并集掉到 21 以下 → 把被移除的类型确认仍在尾池（`level_rosters.tsv` 中它不应再有 core 行）并复测；仍不够则回到步骤 1 换一个被移除者。

### 任务 4 执行结果（2026-09-18）

| 层 | 整改 | core | 单局种类 | 并集 | 占比 vs vanilla |
|---|---|---|---|---|---|
| **L2** | 去 `TSKELSD`/`FIEND`/`SNEAK` | 8 → **5** ✓ | 13.0 → **10.0** ✓ | **21** = 地板 ✓ | **0.0421** ≤ 0.0916 ✓ |
| **L3** | 去 `TSKELAX` | 6 → **5** ✓ | 10.87 → **10.0** ✓ | **25** = 地板 ✓ | 0.0607 ✓ |
| **L8** | 去远程 core `YMAGMA` | 6 → **5** ✓ | 10 → **9.0** ✓ | **17** = 地板 ✓ | **0.2346** ≤ 0.2617 ✓（余量 0.4pp → **2.7pp**） |

**四条约束同时满足，未放宽任何阈值** ✓；`level_roster_test` 41/41 ✓、HF 三守卫（组合数/占比/施法者）**3/3 PASSED** ✓、L1-15 占比用例 PASSED ✓。

### 任务 4 停手项：L19/L20 **无解，已按规程回作者改判**

**问题**：L19 与 L20 的 core 集合**完全相同**（B3 = 1.0）✗。整改需要在"不新增压力"下满足三条上限 ⇒ 实测**三种组合全部失败**：

| 组合 | B3 | L19 施法者(≤0.4258) | L20 施法者(≤0.6087) | L20 远程占比(≤0.1869) |
|---|---|---|---|---|
| A：L20={NECRMORB,TORCHANT} | 0.25 ✓ | 0.3946 ✓ | **0.6187 ✗** | ✓ |
| B：L19={VENMTAIL,TORCHANT,NECRMORB} | 0.25 ✓ | **0.4965 ✗** | 0.4819 ✓ | 0.1738 ✓ |
| C：L20={SPIDLORD,TORCHANT,VENMTAIL} | 0.2 ✓ | 0.3946 ✓ | 0.2573（Kite） | **0.2573 ✗** |

**不可行的数学原因**：L19/L20 共享的**非施法者**候选只有 2 个（`VENMTAIL`、`LASHWORM`），其余 3 个（`NECRMORB`/`TORCHANT`/`SPIDLORD`）都被 RB37 计为远程施法者 ⇒ B3 ≤0.4 要求两个 3 元集合**交集 ≤1**，而 caster ceiling 又要求每层≤1 个施法者 core ⇒ **不可同时满足** ✓。

**三条出路（待作者裁决；不放宽任何阈值）**：
- **(a)** 该对层 B3 目标降级为 **0.5**，并把"仅 2 个共享非施法者候选"的实测理由写进规格（代价：两层身份仍弱）；
- **(b)** 放宽 **L20 的 caster ceiling**（+1.0pp，即接受更多施法者压力——这是压力决策，属作者）；
- **(c)** **扩池**（给 L19/L20 各引入一个**非施法者**候选：窗口/内容改动，之后需重测 HF 基线）← **我推荐**（它是唯一能同时满足 B3 与两条压力上限的路，且是"加内容"而非"砍判据"）。

**当前树状态**：L19/L20 **已回退到原状**（不引入新破坏），因此 **B3 仍红且只红在这一对**（既有缺陷）；L2/L3/L8 的整改**保留** ✓；其余守卫全绿 ✓。

- [ ] **步骤 6：四条约束的联合验证（必须一轮全绿）**

```bash
ninja -C build devilutionx_mpq
cmake --build build --target sampling_behavior_test level_roster_baseline_test -j20
./build/sampling_behavior_test --gtest_filter='SamplingBaselineTest.RosterCoreSizeWithinIdentityBound:SamplingBaselineTest.PerRunTypesWithinBandCeiling:SamplingBaselineTest.ContentDensityWithinVanillaFloor:SamplingBaselineTest.AdjacentLevelCoresRemainDistinct'
./build/level_roster_baseline_test --gtest_filter='LevelRosterBaselineTest.PlacedClassMixWithinBaseline'
./build/level_roster_test
```
预期：全部 PASS（含 `level_roster_test` 的加载期校验）。
**停手条件**：若 L2 在"core ≤5 + 种类 ≤10 + 并集 ≥21 + 占比 ≤vanilla"四条下**确实无解**（连续两种移除方案 + `tail_draw` 调整都失败），**停下**，把四组实测数列成三条路（按契约附录 C：(a) 降级该项目标并写明参数、(b) 再收紧 cap、(c) 扩池）交作者改判；**不得**改任何一个阈值。

- [ ] **步骤 7：提交**

```bash
git add assets/txtdata/monsters/level_rosters.tsv assets/txtdata/monsters/level_roster_params.tsv
git commit -m "fix(rosters): bring the inflated core sets back to a recognisable size"
```

---

## 任务 5：C——指名反制收口

**文件：** 修改 `docs/knowledge/decision_content_density_contract.md`、`docs/superpowers/protocols/2026-09-18-fun-measurement.md`

- [ ] **步骤 1：知识条目加"指名反制"节**

在 `decision_content_density_contract.md` 末尾追加：
```markdown
## 指名反制（红线 10 收口，2026-09-18）

| 新增内容 | 反制（具体玩家行为） |
|---|---|
| 核心小队（未强化） | ① 把 leader 引开 >4 格 → 随从脱队；② 先杀 leader → 拆队（普通 leader 死亡还会清掉悬挂索引） |
| 层身份与构成 | 不新增压力：远程占比主判据 = ≤ 同种子 vanilla（已断言）；既有反制不变（贴身、掩体、拉出射程） |
| L14/L15 core 调整 | 同上；该调整本身是**降低**远程占比以换回 unique 可达（4/6 → 6/6） |
```

- [ ] **步骤 2：试玩协议补一条验证项**

在 `2026-09-18-fun-measurement.md` §6 的预测 5 条目后追加：
```
预测 5 补充项：**先杀 leader 拆队**——找一支小队，先击杀 leader，观察随从是否失去归属并脱离跟随（普通 leader 死亡还会清掉悬挂索引）。记录：脱队(是/否)、随从后续行为。
```

- [ ] **步骤 3：提交**

```bash
git add docs/knowledge/decision_content_density_contract.md docs/superpowers/protocols/2026-09-18-fun-measurement.md
git commit -m "docs(counterplay): name the counterplay red line 10 has been missing"
```

---

## 任务 6：收尾（eval、门禁、CI、状态）

- [ ] **步骤 1：eval 计数同步**

新增 3 条 `SamplingBaselineTest` 用例 → 更新 `eval/cases/rng/sampling-anti-monopoly.yaml` 的 `passed_min` 与 `output_contains`（当前 33 → 33 + 3 = **36**），并把该 case 的 `description` 补一句密度上限口径。
运行：`python3 -m tools.eval.backend --smoke`（预期 exit 0）。

- [ ] **步骤 2：全量门禁**

```bash
python3 tools/run_tests.py --json /tmp/ci-ceilings.json
```
预期：`failed==0 && passed_pct==100 && drift_ok==true`。

- [ ] **步骤 3：提交后跑漂移（顺序不能反）**

```bash
git add -A && git commit -m "chore(eval): sync the sampling case counts"   # 若步骤 1 还有未提交内容
python3 tools/check_drift.py --base origin/master
```
预期：6/6 通过（**必须在 commit 之后**，C2 只检查 HEAD 里的文件）。

- [ ] **步骤 4：push 并跟踪 CI 到终态**

```bash
git push myrepo feature/qol-upgrades
gh run list -R pengyz/devilutionX --limit 2 --json databaseId,headSha,status --jq '.[]|"\(.databaseId) \(.headSha[0:9]) \(.status)"'
gh run watch -R pengyz/devilutionX <run-id> --exit-status
```
预期：`success`（纯文档提交会按 `paths-ignore: docs/**` 跳过，不算失败）。

- [ ] **步骤 5：规格状态与台账**

- 规格 §7：`已批准` → **实施中**，收尾后 → **已实施**（附实施记录：三条阈值的最终值、L2/L3/L8 的整改前后数、CI run id）；
- 台账 `docs/superpowers/ledgers/2026-09-15-level-rosters-phase-a2/progress.md` 追加本轮结论与 CI run id（**随提交进 git**）。

---

## 自检

**1. 规格覆盖度**

| 规格条目 | 任务 |
|---|---|
| §4.1 B1 core ≤5 | 任务 2（判据）+ 任务 4（整改） |
| §4.1 B2 带上限 | 任务 3（非 HF + HF）+ 任务 4 |
| §4.1 B3 相邻 Jaccard ≤0.4 | 任务 1 |
| §4.1 整改硬约束（地板不破） | 任务 4 步骤 6（联合验证） |
| §4.2 C 指名反制表 | 任务 5 |
| §6 验收 5（可失败反证） | 任务 1 步骤 3、任务 2 步骤 3、任务 3 步骤 3 |
| §6 验收 7（门禁/eval/CI） | 任务 6 |
| §7 状态与实施记录 | 任务 6 步骤 5 |

**2. 占位符扫描**：无 `TBD/TODO`；所有阈值都给数字与来源；代码步骤均含可编译代码块；`<run-id>` 是执行时从 `gh run list` 取值的**动作**，不是留白。

**3. 类型一致性**：三个新用例都使用 `GetLevelRoster(uint8_t) -> std::span<const LevelRosterEntry>` 与 `LevelRosterRole::Core`（与既有 `RosterCoreAlwaysPresent` 的访问方式一致）；B2 复用 `LevelMonsterTypeCount` / `GetLevelMTypes()` / `SetRndSeed(51000 + seed)`（与 `ContentDensityWithinVanillaFloor` 完全同口径）；HF 侧沿用 `HellfirePerSeedVarietyHasAtLeastTwoCombinations` 的 `typeSum`。

---

## 执行交接

按作者既有指示**内联执行**（`superpower-executing-plans`），检查点设在：任务 3 步骤 2（三条判据已落地并暴露越界）、任务 4 步骤 6（四条约束联合验证）、任务 6 步骤 2（全量门禁）。**不使用任何子 agent。**