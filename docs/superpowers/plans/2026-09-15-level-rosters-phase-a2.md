# 逐层名册 阶段 A2 实施计划（HF overlay 的 L17-24 名册，闭合红线 12「全层段定义」）

> **面向 Agent 执行者：** 必需子技能：使用 superpower-subagent-driven-development（推荐）或 superpower-executing-plans 按任务逐项执行本计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 让 HF 的 L17-24（Nest/Crypt）也拥有**逐层名册、配额与核心小队**，从而：①红线 12「全层段定义」对**全部层段**成立（目前只对 L1-16）；②消除 R28 的 legacy 特例（L17-24 现在 `GetLevelRosterParams()==nullptr` → 无 core、**无小队**、尾池不设上限）。

**架构（控制者已核实的机制，务必按此设计）：**
- **不新建 overlay 表**。HF 的数据覆盖机制是「mod 以**同路径**提供同名文件」（`mods/hf/txtdata/monsters/monstdat.tsv` 是完整表：base 112 行 → HF 138 行，base 里 24 只 `Never` 的怪在 HF 下变为 `Retail`/`Always`）。因此**同一份 `level_rosters.tsv`/`level_roster_params.tsv` 直接扩到 L17-24**即可；在非 HF 安装下这些行的怪不可用，由**校验的层范围化**放行（不 fatal、也不生效——那 8 层在非 HF 下不存在）。
- `ValidateLevelRoster(entries, params)` 现为**纯函数**（`Source/tables/level_roster.cpp:180`），由 `LoadLevelRoster()`（`:360`）调用 → 层范围**作为参数传入**，便于测试直接驱动。
- HF 的层数：`Source/diablo.cpp:2732` 有 `giNumberOfLevels = gbIsHellfire ? 25 : 17`（**执行者必须自行定位该符号的声明头文件**，并在报告里记录；不要照抄本计划的文件名猜测）。
- HF 素材在 CI 不可用（CI 只下载 `spawn.mpq`）→ **所有 L17-24 相关用例必须贴真实依赖探测后 `GTEST_SKIP`**（先例：阶段 A 的 `genrl.trn` 探测、`LevelRosterBaselineTest::missingRetailTrn_`、`HellfireNoParamsSamplingTest::missingHellfire_`）。

**技术栈：** C++23、CMake+Ninja、GoogleTest、TSV 数据驱动、eval 集成测试。

**规格：** `docs/superpowers/specs/2026-09-15-level-rosters-design.md` §4.2/§4.3/§4.4/§6 与**附录 E 第 2/4/7/7b 条**；计划前置 `2026-09-15-level-rosters-phase-b.md`。

## 全局约束

- 行尾：C++/TSV **CRLF**；`.md/.yaml/.py` LF
- 禁令 6：不得占位/自证用例；**断言必须可失败**（能指名"什么改动会让它红"，并实做反证——见 `docs/knowledge/pattern_assertions_must_be_failable.md`）
- 改 `assets/txtdata/**.tsv` 后必须 `ninja devilutionx_mpq` 再测量（`docs/knowledge/gotcha_tsv_edits_need_mpq_rebuild.md`）
- `run_tests.py --test` 会先构建自身目标；**新增测试二进制必须同时登记 `CMake/Tests.cmake` 与 `tools/run_tests.py` 的 `TEST_TARGETS`**（否则全量门禁跑陈旧产物——已有 `check_drift.py` 检查 F 兜住）
- 需零售/HF 素材的 eval case 必须声明 `setup.retail_or_hf_required: true`
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json`（`failed==0 && passed_pct==100 && drift_ok==true`，含检查 F）；eval：`python3 -m tools.eval.backend --smoke` exit 0
- 每次 push 后 `gh run list` + `gh run watch` 跟到终态（台账 R43）
- 不得修改任何既有 ceiling/floor 阈值（R4）；不得改 L1-16 的既有数据（除本计划明确允许的）
- 提交信息英文 conventional commits；**绝不**让实现者分发子代理

---

## 文件结构

| 文件 | 职责 |
|---|---|
| `Source/tables/level_roster.{h,cpp}` | 修改：`ValidateLevelRoster` 增加**层范围**参数；`LoadLevelRoster` 传入当前游戏的最高地下层 |
| `assets/txtdata/monsters/level_rosters.tsv` | 修改：新增 L17-24 的 core/tail 行 |
| `assets/txtdata/monsters/level_roster_params.tsv` | 修改：新增 L17-24 行（含 `squad_*`） |
| `test/level_roster_test.cpp` | 修改：层范围校验的合成用例 |
| `test/level_roster_baseline_test.cpp` | 修改：L17-24 的测量/验收用例（资产门控） |
| `test/sampling_behavior_test.cpp` | 修改：L17-24 的守卫（资产门控）与形成率 floor 数组扩展 |
| `eval/cases/rng/*.yaml` | 修改：必要时新增/扩展 HF 层段的 case（资产门控） |

---

## Task 1: 校验层范围化 + L17-24 前置基线实测

**接口：**
```cpp
// level_roster.h（保持既有两参调用可用：默认 maxLevel = 最高地下层）
std::optional<std::string> ValidateLevelRoster(
    std::span<const LevelRosterEntry> entries,
    std::span<const LevelRosterParams> params,
    uint8_t maxLevel);
```

- [ ] **步骤 1：写失败的合成用例**（`test/level_roster_test.cpp`）
  构造一张含 **L17-24** 行、但其怪在基础表里 `availability=Never` 的表，`maxLevel = 16` → 断言**校验通过**（这些层在非 HF 下不存在，不应 fatal）；再以 `maxLevel = 24` → 断言**校验失败**（HF 下必须真实可用）。
- [ ] **步骤 2：跑测试确认失败**（当前实现会因 L17-24 怪不可用而 fatal）
- [ ] **步骤 3：实现层范围化**
  - 逐层检查（存在性/可用性/floors 可满足性/`core` 非空）**只对 `level <= maxLevel`** 生效；
  - 全局检查（重复行、`max_image>0`、`tail_draw>=0`、`squad_*` 范围、哨兵拒绝、unique base 白名单——**更正：该检查实为逐层语义**（签名 `IsUniqueBaseForLevel(level,type)`），应由层范围约束；评审实测 4 条 ≥L17 的 base unique 两表皆不可用/窗口外，故两种归属都不漏检，但合同文字以"逐层"为准）**；其余全局检查不受层范围影响**（它们与层是否可达无关）；
  - 保留既有两参重载（默认 `maxLevel`＝当前游戏最高地下层）以免破坏既有调用与测试——**若因此产生新的导出符号，须确认不触发漂移检查 E**（必要时把默认值写成调用方显式传参）。
- [ ] **步骤 4：`LoadLevelRoster()` 传入真实层上限**（自行定位 `giNumberOfLevels`/等价常量，并在报告记录来源）
- [ ] **步骤 5：L17-24 的前置基线实测**（资产门控；无 HF 素材则 `GTEST_SKIP`）
  在 `level_roster_baseline_test.cpp` 增加一个**测量**用例：HF 下 L17-24 的 placed class mix / 远程占比 / 类型数，**无条件打印**（`[ A2BASELINE ] …`）并写入报告。**这是后续验收 8（相对改动前基线 ≤ +5pp）的基线来源**——没有它就不许改数据。
- [ ] **步骤 6：提交**（`feat(roster): validate rosters only up to the active level range`）

---

## Task 2: L17-24 的 core/tail 数据与逐层参数

- [ ] **步骤 1：候选池盘点**（HF 载入下的 L17-24 可用怪、`ai`→行为类别、unique base、Σimage）
- [ ] **步骤 2：写数据**（`level_rosters.tsv` 加 L17-24 core 行；`level_roster_params.tsv` 加 L17-24 行含 `max_image`/`tail_draw`/`class_floors`/`squad_chance`/`squad_size`/`squad_leashed`）
  约束：core ≤ cap（L17-24 的 caps 需按 `BehaviorClassCapForLevel` 的实际分段确认并记录）、floors 在 caps 下可满足、unique base 白名单、`Σimage` 低于 `max_image`（**口径更正 RB41**：这**不是**加载期硬校验——`ValidateLevelRoster` 只查 `max_image > 0`，预算在**采样期**生效且 **core 预加不受其约束**；越界由占比/构成守卫兜住）。
- [ ] **步骤 3：校验回归**：`level_roster_test` 全绿（含新层范围用例）；加载期不再对 L17-24 fatal（HF 载入时）。
- [ ] **步骤 4：提交**

---

## Task 3: L17-24 的核心小队与形成率守卫扩展

- [ ] **步骤 1：扩展守卫覆盖面**
  - `kSquadFormationFloor` 数组**必须覆盖到 L24**（当前 size 17、L16 为 unconstrained 哨兵）；L17-24 用**实测**导出阈值（500 seed/层，资产门控）；
  - `RosterPerSeedVariety`/`RosterQuotasSatisfied`/`HellUniqueBasesRemainReachable` 的**层循环**扩展到 L17-24（资产门控；HF 缺席时跳过）；
  - 逐层 `[ SQUADFORM ]`/`[ SHIPPEDSQUAD ]` 打印需覆盖新层。
- [ ] **步骤 2：反证**：至少一条新守卫做"改坏→必红→恢复→绿"。
- [ ] **步骤 3：验收 8 对 L17-24**：相对 Task 1 基线 ≤ +5pp（HF 下实测；写入用例或与既有阈值用例同构）。
- [ ] **步骤 4：门禁 + 提交**

---

## Task 4: 收尾

- [ ] **步骤 1：eval**：为 HF 层段新增/扩展 case（或扩展既有 `level-rosters` 的 filter 与计数），**声明 `retail_or_hf_required: true`**；`passed_min`/`output_contains` 与实跑一致。
- [ ] **步骤 2：R28 legacy 的现状说明**：L1-24 都有 params 行后，legacy 分支对**出厂数据**不再可达——**保留代码**（mod/缺行仍需它），并保留/新增一条**合成表**用例证明它仍工作（不得删除）。
- [ ] **步骤 3：全量门禁（含检查 F）+ eval smoke + 行尾**。
- [ ] **步骤 4：push 并跟踪 CI 到终态**（CI 只有 `spawn.mpq` → 所有 HF 相关用例应**跳过**而非失败；若红则按日志定位）。
- [ ] **步骤 5：提交**

---

## 实施记录（执行者填写）

| 项 | 值 |
|---|---|
| `giNumberOfLevels`/层上限的符号与头文件 | （执行者定位后填写） |
| L17-24 前置基线（远程占比/类型数） | （Task 1 步骤 5 的 `[ A2BASELINE ]` 实测） |
| L17-24 的 core 名单与 caps/floor 组合 | （Task 2） |
| 逐层小队形成率（L17-24） | （Task 3 的 500-seed 实测） |
| eval case 与计数 | （Task 4） |
| CI run 与结论 | （Task 4） |

---

## 自检

**1. 规格覆盖度**

| 规格/台账条目 | 对应任务 |
|---|---|
| 红线 12「全层段定义」 | Task 1（层范围化）+ Task 2（L17-24 数据） |
| §4.3 小队机制对全部层段成立 | Task 3 |
| §6 验收 7（形成率阈值参数化）/8（远程占比）/9b/9c | Task 3 |
| 附录 E 第 2/4 条（HF overlay 名册、A2 范围） | 全计划 |
| 附录 E 第 7/7b 条（界的参数绑定/形成率实测） | Task 3 |
| R28（无名册参数行退回旧行为） | Task 4 步骤 2（保留 + 合成用例） |

**2. 占位符扫描：** 无待补项；不确定的**事实**（如层上限符号的头文件）已显式标为"执行者必须自行定位并记录"，而不是编造。

**3. 类型一致性：** `ValidateLevelRoster(...)` 的 `maxLevel` 参数、`kSquadFormationFloor` 的索引范围、`[ A2BASELINE ]`/`[ SQUADFORM ]` 打印前缀在 Task 1-4 中命名一致。

---

## 执行交接

计划已保存至 `docs/superpowers/plans/2026-09-15-level-rosters-phase-a2.md`。执行方式：
1. **子代理驱动（推荐）**——每任务分发全新子代理 + 任务间评审
2. **内联执行**——用 `superpower-executing-plans` 批量执行

**前置**：任务 D（L14 core 重塑）与本金计划**都改 `level_rosters.tsv`/params 表**，必须**串行**（D 先完成或明确搁置后再开始 Task 2）。