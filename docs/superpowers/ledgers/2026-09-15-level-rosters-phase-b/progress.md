# SDD ledger — plan: docs/superpowers/plans/2026-09-15-level-rosters-phase-b.md

**工作区**：`docs/superpowers/ledgers/2026-09-15-level-rosters-phase-b/`
**规格**：`docs/superpowers/specs/2026-09-15-level-rosters-design.md` 的**阶段 B**（§4.3 + §6 验收 5/5b/5c/6 + 附录 D 的 3/5）
**分支**：`feature/qol-upgrades`（远端 `myrepo`）
**前置**：阶段 A 已完成、CI 绿（run `35075979571`）

## 飞行前裁决（Rulings）

| # | 裁决 | 原因 | 若错的代价 |
|---|---|---|---|
| RB1 | 继续在 `feature/qol-upgrades` 上执行，**不建 worktree** | 沿用阶段 A 的 R1 与项目长期约定 | 低 |
| RB2 | 计划里那句"Task 2 先标 `DISABLED_` 再在 Task 3 拆掉"**已由控制者删除**：小队路径的断言（`SquadMinionsKeepOwnAi`）整体归 Task 3 | 仓库禁令 6 禁止占位测试；留一个 `DISABLED_` 用例等于把禁令欠在一处 | 低（少一条中间态用例） |
| RB3 | Task 1 步骤 3 的代码块含**草稿版**（用 `any_of` 扫描）与**推荐版**（`ReleaseMinions(monster, !monster.isUnique())`）→ 执行者**必须采用推荐版** | 草稿版的 `std::any_of(&Monsters[ActiveMonsters[0]], …)` 依赖 `ActiveMonsters` 连续且非空，脆弱且多余；`ReleaseMinions` 自身已按 `getLeader()` 过滤 | 中（照抄草稿可能引入新的越界/空区间问题） |
| RB4 | T3/T4 改 TSV 后必须 `ninja devilutionx_mpq`；T3 的小队接入会改变放置 RNG → 若 `timedemo` 等夹具失配，按既有流程**重生成**（沿用阶段 A R3） | 两条均为本会话已沉底的知识/既有流程（`docs/knowledge/gotcha_tsv_edits_need_mpq_rebuild.md`） | 低-中 |
| RB5 | 每次 push 后按 R43 跟踪 CI 到终态；CI 只有 `spawn.mpq`，需零售/HF 素材的用例必须**贴真实依赖**地 `GTEST_SKIP`（阶段 A 的 `genrl.trn` 探测写法） | 阶段 A 曾 6 连红 | 高 |

## 冲突扫描表

| 涉及 | 产出 → 消费 | 发现 | 处置 |
|---|---|---|---|
| T1 ↔ T2 | 都改 `Source/monster.cpp` 与 `sampling_behavior_test.cpp` | T1 改 `ReleaseMinions`/`M_UpdateRelations`；T2 加 `MinionOptions`/改 `PlaceGroup` | 串行；T2 不得触碰 `M_UpdateRelations`，T1 不得引入 `MinionOptions` |
| T2 ↔ T3 | `MinionOptions{tough,inheritAi,inheritIntelligence}` → 小队调用 | 命名一致 | 无需裁决 |
| T3 ↔ T4 | 都改 `level_roster_params.tsv` | T3 加三列，T4 只改 `squad_leashed` 值 | 串行 |
| T3 ↔ T5 | 用例数变化 → YAML `passed_min` | 计划未在 T3 里同步 | 由 T5 步骤 2 收口；若 T3 结束即 push，须先确认 eval smoke 仍绿 |
| T1 内部 | 步骤 3 两版实现 | 见 RB3 | 采纳推荐版 |
| 全局 ↔ T3/T4 | TSV 改动需重建 MPQ；RNG 变化需夹具重生成 | 已写入计划与台账 | RB4 |

## 任务状态

| 任务 | 状态 |
|---|---|
| Task 1：G1（普通 leader 死亡释放随从） | dispatched |
| Task 2：G2（`MinionOptions`） | pending |
| Task 3：小队接入 + 三列 | pending |
| Task 4：形成率守卫与回退 | pending |
| Task 5：收尾（预测验收/eval/台账/CI） | pending |

## 执行记录

| 时间点 | 事件 |
|---|---|
| start | Task 1 BASE=`08b1db0ab`；实现者已分发（模型 claude-opus-5：改动落在死亡路径、且涉及悬挂索引正确性，属高风险集成） |

| Task 1 | DONE_WITH_CONCERNS（提交 `38e9b58e1`/`2d425d806`/`5cadd5df4`）：门禁 747/0/100%/drift ok、eval smoke exit 0；遵守 RB3（推荐版、无 `any_of` 草稿、无 `MinionOptions`）；`ReleaseMinions` 加第二参默认值（原为文件内 static，未动头文件） |
| RB6 | **接受**越界修改 `eval/cases/rng/sampling-anti-monopoly.yaml`（`passed_min: 26→28` + `output_contains 26→28`）：该文件自身的 R35 维护注释即要求新增用例时同步计数，不同步则 eval 门禁 FAIL——属"必须的连带修正"而非越界；同时记录：**计数硬编码在两处**（`passed_min` 与 `output_contains`）正是 R35 记下的工具层脆弱点 | 中（若不同步：eval 静默失效或误红） |
| RB7 | **接受**用 `MonsterDeath(monster, md, sendmsg)` 取代简报骨架里的 `KillMonster`：该符号在本仓库**不存在**（控制者写计划时凭记忆，属计划缺陷）；`M_StartKill`/`StartMonsterDeath` 均汇聚到 `MonsterDeath`，直接调它可避开 `GetDirection` 对玩家位置的依赖，且路径等价。**控制者已修正计划文本** | 低 |
| RB8 | `setLeader(nullptr)` 保留 `leader` 索引（为 monhealthbar 给被 buff 随从上色）**仍是悬挂来源**，unique 路径按规格刻意保留 → **本任务不扩范围**；记入延期，交 Task 2/3 评估"是否改为让 monhealthbar 不再依赖该索引" | 低（unique 槽位复用风险与既有一致） |
| RB9 | **接受**夹具新增 `PrepareDeathPathPrerequisites()`（`LoadItemData` + 1 个 `Player` + `MyPlayer`，顺序对齐 `diablo.cpp`）：不加载则会 `MonsterDeath`→`SpawnLoot`/`PlayEffect` 在 `items.cpp:1390` ASan SEGV。**但属夹具级改动，其是否扰动既有 26 条用例必须由任务评审独立核实** | 中（`Players.resize(1)`/`MyPlayer` 是进程级全局，可能影响同二进制其它用例） |
| 参考 | 关注点 3（G1 目前无生产触发点）符合设计：普通怪小队在 Task 3 才接入；`Source/msg.cpp:888` 的 delta 载入路径行为一致。关注点 6（无既有测试依赖旧行为）有 grep 证据 |
| Task 1 | 任务评审已分发（agent `c1ce3e97`，claude-opus-5）；重点核对 RB3 遵守、G1 双向语义（非 unique 清索引 / unique 不变）、测试是否驱动真实死亡路径、**RB9 的夹具改动是否扰动既有 26 条用例**、eval 双处计数一致性 |
| Task 1 | CI 跟踪已启动（`d60e0a590`，R43） |
| Task 1 | **CI 绿**（R43 已核实远端）：run `35082081489` on `d60e0a590` → `success` |
| Task 1 | **complete**（commits `08b1db0ab..d60e0a590`，review clean：0 严重/0 重要/3 轻微） |
| Task 1 | 评审唯一未独立核实项=全量门禁 → **由 CI 闭合**：run `35082081489`（同一 SHA `d60e0a590`）在全量 ctest + drift 上 100% 通过 |
| Task 1 | minor (deferred)：①`RunEngineDeath` 复制了 `MonsterDeath` 末帧分支两行（将来该分支新增动作时测试不会跟随）→ 建议加一行"改 `monster.cpp:1529-1539` 时同步此处"的注释；②`LeaderDeathReleasesMinions` 未覆盖"槽位复用后指向活怪"的端到端场景（当前断言足以锁住修复）；③`PrepareDeathPathPrerequisites()` 改进程级全局却无对称复原（当前无害，属隐性耦合）；④普通怪 leader 死后 `packSize` 未归零（`isInvalid` 且槽位会被重置，无实际风险） |
| Task 2 | 分发中（BASE=`d60e0a590`）；把 minor③ 作为**可选加固**带入（同文件），并要求不得触碰 `M_UpdateRelations`/`ReleaseMinions`（T1 的产物） |
| Task 2 | 已分发（agent `ad5febe9`，claude-sonnet-5：计划含近乎完整的代码，属"转录+测试"档）；BASE=`d60e0a590`；交付 `MinionOptions` + `SquadMinionsUnbuffed` + `UniqueMinionsBehaviourUnchanged`；要求同步 eval 双处计数；禁止引入 `DISABLED_` 占位；不得触碰 T1 的 `M_UpdateRelations`/`ReleaseMinions` |
| Task 2 | 首位实现者因**上下文耗尽被停止**：实现已留在工作区（未提交、无报告）——`monster.h` 定义 `MinionOptions` + 新签名；`monster.cpp` 的 `PlaceGroup` 已正确 gated 三个开关（含 `setLeader` 前存 `ownAi` 后恢复）。**控制者逐行审阅确认实现正确**，缺测试/eval 同步/门禁/提交 |
| Task 2 | 已派接手实现者（保留工作区、不回退）；要求：补 2 条用例（其中"随从 AI 不被覆写"必须选 leader 与随从 AI **不同**的组合，否则断言恒真）、同步 eval 双处计数、门禁 + eval、分块提交、边做边写报告 |
| Task 3 | 简报已预生成（`task-3-brief.md`）；分发时须带：① TSV 加三列后必须 `ninja devilutionx_mpq`；② 小队接入改变放置 RNG → 夹具失配按既有流程重生成；③ 不得写 `DISABLED_`/跳过式占位（RB2）；④ 新增用例后同步 eval 双处计数；⑤ 复用 T1/T2 的夹具辅助；⑥ `IsCoreRosterMember`/`PickCorePartner` 用 `GetLevelRoster` 实现为**文件内 static**，不新增导出 API（避免漂移检查 E 噪声） |
| Task 3 | **评审重点（预置）**：① 小队路径不得超 `totalmonsters`、不得产生 `packSize==0` 的 leader；② partner 的索引必须合法（`GetMonsterTypeIndex` 对**已在表中**的 core 成员返回有效下标，否则会是 `LevelMonsterTypeCount`）；③ `squad_leashed=false` 回退的语义（仍传 leader → 相邻初置有保证，但不设 `setLeader`/`packSize`）；④ 是否改变了既有 `na` 与降级路径；⑤ 放置 RNG 变化后夹具是否**按流程重生成**（而非手改数字）；⑥ `IsCoreRosterMember`/`PickCorePartner` 是否真读生产数据（禁自证）；⑦ 新增用例后 eval 双处计数是否同步 |
| 备注 | Hindsight 知识库在本部署**不可用**（三次尝试均 401：未配置 API key）→ 项目记忆继续落在 `docs/knowledge/`、`docs/superpowers/` 与本 SDD 台账；本次 initiative 摘要已写入本台账 |
| R43 细化 | 本仓**只有 `better-d1-ci.yml` 会跑在 `feature/**`**（已核实）；`clang-format-check.yml`（clang-format **18**，check-path `Source`+`test`）与 `clang-tidy-check.yml` **仅 master/PR 触发** → 分支"CI 绿"≠"格式合规"，开 PR 前需自备 18.x 自查；另 `docs/**` 提交不触发 CI。已沉淀 `docs/knowledge/reference_ci_workflows_on_feature_branches.md` |
| RB10（工具层加固，进行中） | `tools/run_tests.py` 的 `--test` 分支已改为**先构建该目标**（新增 `build_target()`，镜像 `build_tests()` 的 generator 处理；目标不在本次配置内则**明确失败**而非静默跑旧二进制）。已 `py_compile` 通过、`--no-build` 路径实跑 exit 0；**因 Task 2 接手者正在同一 build 目录构建，验证（含非目标名的失败路径）与提交延后到它交回后**，避免 ninja 并发冲突 |
| Task 2 | **接手者 `8b2d4e31` 静默结束**（`list_agents` = ready、无提交、无报告、测试未写）→ 按 R42 先确认无 agent 在跑，再**重派全新实现者**（不回退工作区；要求"第一分钟就落盘报告"）
| Task 2 | **收尾完成**（`8ffee8da9` 实现逐字提交 + `4ea6b63cc` 2 条用例与 eval 计数 28→30）：`sampling_behavior_test` 32/32；门禁 **749/0/100%/drift ok**；eval smoke 36/36；**避开恒真断言**（leader `MT_WSKELAX` 近战 vs 随从 `MT_TSKELBW` 远程，用 `ASSERT_NE` 先钉住 AI 类别不同）；自查出草稿 bug（HP 断言误以为该类型 `min==max`，实际 `InitMonster` 随机 roll）→ 改为断言落在未翻倍的 `[min,max]<<6` 区间 |
| RB10 | **工具层加固已验证并提交**（`1a97e178a`）：`--test` 先构建自身目标（正常目标 build ok + 29 条通过；**非目标名 rc=1 明确失败**；带 filter 新用例 build ok + 1 条通过）；知识条目改为 `decision_run_tests_test_builds_its_target.md` 并更新 MEMORY |
| Task 2 | 任务评审已分发（agent `9c17fc78`，claude-opus-5）；三条自报关注点要求独立核实：①"AI 断言非恒真"的反证是否成立 ②HP 区间断言能否区分"被翻倍"③内联复制 `IsMonsterAvailable` 前提检查是否算重抄生产逻辑 |
| 推送 | `d60e0a590..1a97e178a`（含 Task 2 两提交 + 控制者文档与工具提交）；CI 跟踪已启动（R43） |
| Task 2 | **CI 红**（run `35103847109` on `1a97e178a`）：唯一失败项 `SamplingBaselineTest.UniqueMinionsBehaviourUnchanged`（0 ms 即失败）→ 与阶段 A 同因：unique 路径要读零售/HF 的 `monsters\monsters\genrl.trn`，CI 只有 `spawn.mpq`。**已确认控制者的 `tools/run_tests.py` 改动未影响 CI**（失败是单条测试） |
| RB11 | 修法：新增的 unique 回归用例必须**贴真实依赖**探测（`OpenAsset("monsters\\monsters\\genrl.trn")`，同阶段 A 的既有写法）并在缺失时 `GTEST_SKIP`，**不得**改成"少断言"或"跳过整个套件"；`SquadMinionsUnbuffed` 在 CI 通过，无需守卫（但要求实现者确认理由）。另：CI 的 push 触发路径**不跑 eval**（`eval-smoke/eval-nightly` 为 skip），但 nightly/dispatch 跑 eval 时该用例会 skip → 需在报告里说明 `passed_min` 的口径（本环境 30） | 中（不修则分支持续红） |
| Task 2 | CI 修复已分发（agent `035ba63b`，claude-sonnet-5，RB11）；已给在跑的评审 `9c17fc78` 补发"新用例 CI 安全性"视角（用 `send_message`——`agent_teams_send_message` 不适用，本会话非 AgentTeams 团队） |
| Task 2 | 任务评审裁决**需修复**（agent `9c17fc78`，claude-opus-5；生产代码与规格/默认行为完全对齐，问题都在**用例**）：**S1** `UniqueMinionsBehaviourUnchanged` 的 AI 断言**恒真**（选中 idx13 `MT_TSKELAX`，unique ai == base ai == SkeletonMelee → `inheritAi` 改坏也过；gdb 实测）；**S2** CI 不安全（spawn-only 下 `bhka.trn` 缺失硬失败）；I3 HP 断言区间重叠（[256,512] vs [512,1024] 在 512 重叠，当前能过是运气，注释论证不成立）；I4 内联前提检查删了 dunLvl 且 spawn 分支是死代码 → 筛不掉缺素材的零售 unique（S2 成因），但**不判自证**；轻微：`test:424` 自相矛盾注释 |
| RB11 修正 | 我原指令让探测 `genrl.trn` **过窄**：CI 报的是 idx12 `MT_NAKRUL` 的 `genrl.trn`，但该 unique 已不在 `monstdat.tsv`（availability 默认 Never）被过滤 → 实际落到 idx13 的 `bhka.trn`。**必须改为通用探测**：从被测 unique 的 `UniqueMonstersData[...]` 取它实际要读的 TRN 名再 `OpenAsset` 探测 |
| 评审对我三问的结论 | ① unique 用例**应**按同法探测后跳过；② `SquadMinionsUnbuffed` **真不依赖**零售素材（两类型 `availability=Always`，且从不进 unique 路径；评审在隔离环境实跑通过）；③ 除 I3 的区间重叠外无其它静默失效通道（placement 装不满等由 `ASSERT_EQ(ActiveMonsterCount, …)` 响亮兜住） |
| Task 2 | 已把**完整发现列表**发给在跑的修复者 `035ba63b`（S1 反证要求、S2 通用探测、I3 与区间无关的判据、I4 注释、轻微项），一份指令一次修完 |
| 观察 | 这是本会话第 **4** 次由独立评审抓到"**看起来严谨实则可失败性缺失**"的缺陷：①2a 的自证用例（删掉生产逻辑仍通过）②2b 的恒真 caps 断言（豁免来自被测名册）③S1 的固定组合盲区（五道守卫都盯不到）④本轮的恒真 AI 断言（选中的 unique 与其 base 同 AI）。四次都不是"实现写错"，而是**验证写得像真的却不具备区分力** |
| 流程加固 | 把"**素材依赖检查**"与"**断言可失败性反证**"写进 Task 3/4/5 的**分发固定清单**：①任何新用例若依赖零售/HF 素材，必须从被测对象的真实依赖（如 `UniqueMonstersData[...]` 的 TRN 名）探测后 `GTEST_SKIP`，不得硬编码文件名；②任何新守卫必须做一次"改坏生产 → 必红 → 恢复 → 绿"的反证并把两次实跑写进报告；③区间型断言优先改为相对/A-B 比较 |
| 知识沉淀 | `docs/knowledge/pattern_assertions_must_be_failable.md`（4 种复发形态 + 修法 + 区间重叠附带形态），已索引 MEMORY |
| 备注 | Hindsight 知识库**第四次确认不可用**（本次 `list_knowledge_pages` 亦 401：未配置 API key）→ 不再重试；项目记忆以 `docs/knowledge/`（本次新增 `pattern_assertions_must_be_failable.md`，提交 `b61995f4e`）+ `docs/superpowers/` + SDD 台账为准 |
| Task 3 | 分发稿已预写（`task-3-dispatch.md`）：含**已核验的符号事实表**（`LevelMonsterTypes`/`GetMonsterTypeIndex` 未命中语义/`MT_INVALID`/`IsMonsterAvailable` 同 TU 可用/`MinionOptions` 默认值）+ **10 条固定清单** + 8 条评审重点；符号审计确认计划 Task 3 的 17 个候选符号全部存在 |
| Task 2 | 修复者 `035ba63b` **静默结束且只做了 16 行方向错误的改动**（硬编码 `genrl.trn`，正是评审否掉的窄做法）；S1/I3/I4/轻微项均未做、无报告、无提交。按 R42 先 `list_agents` 确认无 agent 在跑，再**改派最强档位**（`claude-opus-5`，第三次接手）——这是测试正确性任务第二次失败，值得用最强模型；工作区那 16 行要求**改造为通用探测**而非保留 |
| Task 2 | 修复完成（提交 `e4abc02e3`，仅 `test/sampling_behavior_test.cpp` +175/-71 与 eval YAML 注释）：S1 扫描加 `mAi != base.ai` → 选中项由 idx13 `MT_TSKELAX`（同 AI，恒真）变为 idx17 `MT_WSKELAX`/Boneripper（Bat vs SkeletonMelee）；S2 删掉硬编码、改为从 `UniqueMonsterData::mTrnName` 拼路径（探测与使用同一引用）；I3 改用**同种子 A/B**（`doubled == 2 * unbuffed`）+ `ASSERT_GT(…,0)`；I4 标注"Candidate PRE-FILTER only"；轻微矛盾注释删除 |
| Task 2 | **三变异体反证**：A 强制非继承→unique 用例红；B 去掉恢复（强制总继承）→`SquadMinionsUnbuffed` 红；C 忽略 `opts.tough`→**两条都红**（384vs768 / 96vs192，证明新判据能抓住旧区间判据抓不住的情形）；恢复后全绿 |
| Task 2 | 两种环境实测：有素材 32/32 不跳过；模拟 spawn-only → 该用例 **Skipped**（消息为实际 TRN `br.trn`）、零 FAILED；**并对照 HEAD 在同环境 FAILED**（证实 S2 真实）；三个 MPQ 已恢复（`grep -c bak_spawnonly`=0） |
| Task 2 | 门禁 749/0/100%/drift ok；eval smoke exit 0；`passed_min: 30` **未下调**（有素材口径），spawn-only 下 `output_contains` 不成立一事按"CI 侧决策"上报，未为迁就改断言 |
| RB12 | **小队参数起点由控制者给定，但验收看"实测实现率"而非输入旋钮**：`squad_chance` 起点 **30%**、`squad_size = 2`、`squad_leashed = 1`（阶段 B 目标=可辨识但非主导；对应附录 D 预测 3："能看到一只 + 1-2 只贴身同行的小队"）。Task 3 必须**实测并报告每层"core 组中真正成队（≥1 随从）"的实现率**（不只是写进表的概率），Task 4 再据实测钉阈值/回退 | 若只写概率不测实现率：表里的 30% 与实际观感脱节（4 格约束/槽位不足/`totalmonsters` 钳制都会打折） | 低-中 |
| Task 2 | **CI 转绿**（R43 已核实远端）：run `35121001274` on `e4abc02e3` → `success`；`100% tests passed, 0 tests failed out of 749`；**`316 SamplingBaselineTest.UniqueMinionsBehaviourUnchanged (Skipped)`**——S2 的修复被**真实 CI 环境**证实（此前该用例是 FAILED），其余跳过项与设计一致（`Timedemo` + HF 2 条 + 零售口径基线 2 条 + `VisualStoreTest` 2 条） |
| U1（复审 Important） | **已关闭（控制者实现，提交 `369438ebe`）**：给 eval 框架加**真实数据依赖门控** `setup.retail_or_hf_required`（`models.py` 的 `_SETUP`+属性、`assertions.classify_skip` 新分支、`backend.retail_or_hf_present()`：查 `DIABDAT.MPQ`/`diabdat.mpq`/`hellfire.mpq`/`HELLFIRE.MPQ`）；两个 rng case 声明之。**双向实证**：有素材 `level-rosters` → `[PASS] (30/30)`、`sampling-anti-monopoly-cap` → `1/1`；模拟 spawn-only（隔离 HOME）→ 两者 `skipped: 1 / failed: 0`（整案跳过，不再判失败）。`sync_case_sets --check` = OK 67 cases；drift 5/5 PASS |
| U1 附带 | 同一缺陷在 **`level-rosters`** 也存在（复审只点了 `sampling-anti-monopoly-cap`；我先做了全量排查，发现它在 spawn-only 下 `passed_min:3` 只会得到1 → 同修）。之所以门控是 `retail_or_hf_required` 而非"retail"：TRN 由 `DIABDAT.MPQ` **或** `hellfire.mpq` 提供，这也解释了"本地能跑（有 hellfire）、CI 不能跑（只有 spawn）" |
| Task 2 | **complete**（生产代码 + 用例 + CI 修复 + U1 全部关闭；任务评审"需修复"→ 修复轮 `e4abc02e3` → 范围化复审"5 项全 ADDRESSED、I3 前提经独立推导为结构性成立、无新增破坏"，剩 U1 → 本轮关闭） |
| U1 | **端到端验证通过**：eval `--smoke` exit 0（render 2/2、combat 5/5、utility 9/9 …）；**CI 绿**（run `35123854177` on `369438ebe` → success，R43 已核实远端）。即：依赖由门控表达、断言强度不变、nightly 不再会因缺素材而红 |
| Task 3 | 实现者 `80441e7a` 正在 BASE `369438ebe` 上工作（三列 + 散布循环组队 + 实测成队率） |
| Task 3 | DONE_WITH_CONCERNS（提交 `b6cd416e5`/`1ea3dfda3`/`9d46b0ef1`）：门禁 **760/0/100%/drift ok**、eval smoke 36/36；**实测每层成队率**（50 seeds）rollRate 26.8-31.1% + 实现率≈100%（L1-13/L15），**L14 为 8.8%**（其 squad_chance 从 30 降到 10） |
| RB13 | **接受 L14 的 `squad_chance=10`**：L14 cores = 1 近战 + 2 远程 → 成队把 na(3-5) 个 leader 换成 1 leader + 2 个从**其他 core** 抽的 partner（2/3 是远程）→ chance30 时 placed 远程占比 0.615081 > ceiling 0.60876（squads-off 已 0.604908，仅 0.39pp 余量）。**按 R4 改数据未动 ceiling** ✓；其首次尝试（Melee floor 1→2）虽过了验收8却**打断验收9c**（L14 unique 可达 6/6→2/6）→ **自行回退** ✓ 这正是我们要的跨验收取舍纪律 |
| RB14 | 交 Task 4：**`class_floors` 不是自由杠杆**（在地狱层抬高近战 floor 会在验收8 与 9c 之间互换）；`squad_chance` 与密度近似线性（因 `squadSize=2 < na`，成队反而**少占槽位**，故"4格拴系/槽位不足"的折扣并未出现）→ 任何逐层调整必须**同时**对验收8 与 9c 重测 |
| RB15 | `SquadMinionsKeepOwnAi` 目前只是 `SquadFormsAroundACoreLeader` 内的内联断言（每样本 `minionsWithOverwrittenAi==0`）→ **要求拆成具名用例**（独立失败归因 + 与分发清单可追溯） |
| RB16 | 要求补写两条**引擎行为知识**（实现者已发现但未落盘）：①unique boss pack 也会产生拴系随从（`PlaceUniqueMonsters` 在散布循环前、用默认 `MinionOptions`）→ 小队度量须以 `leader->isUnique()` 区分；②`PlaceGroup` 的 4 格拴系是**从 leader 的邻格**量起，故真实边界是 >4 而非 >=4 |
| Task 3 评审 | 裁决**需修复**（agent `4807d157`，claude-opus-5）：机制忠实/下标安全/计数由生产驱动/ceiling 未动/A-B 经其**实测确可失败** ✓；但 **RB15/RB16 未执行**（严重·台账缺口），且**两条规格要求目前无任何可失败守卫**（重要）：①`UnleashedFallbackPlacesNeighboursWithoutLeashing` 只断言"unleashed 侧 0 拴系"，**完全没测邻近性**（把 `&leader` 改成 `nullptr` 全部用例仍绿）②"partner 必须与 leader 异类型"无用例可失败（删掉 `entry.type == leaderType` 跳过，8 条全绿） |
| RB17 | **L14 在 Task 4 冻结**：本步把 L14 余量从 0.604908 推到 0.605120，距 ceiling 0.60876 仅 **0.36pp**——比 R34 曾以"太贴边"否掉的 1.2pp（`tail_draw=2`）还小 3 倍，而 L14 只拿到 8.8% roll 率≈几乎不享受本特性。裁决：**不动 ceiling**（R4/宪章），**Task 4 不得再改 L14 的任何旋钮**；该取舍**正式记入规格附录 E 并作为阶段 B 收尾时向作者呈报的首选项**（三选一：接受 L14≈8.8% 小队 / 恢复 `tail_draw=2` / 提高 L14 ceiling 上限——后者是规格变更） |
| Task 3 fix round 1 | 范围：RB15 具名 `SquadMinionsKeepOwnAi` + 修 finding3（把回退路径的**相邻性**变成可失败守卫）+ finding4（`SquadObservation` 记录随从/leader 类型关系并断言异类型，且以删除跳过作反证）+ 轻微8（`squad_leashed` 拼写错误必须报错，补用例）；RB16 由控制者直接做（文档） |
| RB16 | **控制者已完成**：两条引擎行为知识落盘（unique boss pack 也会产生拴系随从；`PlaceGroup` 的 4 格拴系从 **leader 的邻格**量起） |
| 观察（第 5 次） | 本会话第 **5** 次由独立评审抓到"**要求存在但没有可失败守卫**"：①2a 自证用例 ②2b 恒真 caps 断言 ③S1 固定组合盲区 ④恒真 AI 断言 ⑤**本轮两条规格要求**（回退路径"相邻初置"、partner 必须异类型）——都是"改坏了测试仍全绿"。已据此把"**每条要求必须有一条能指名的可失败守卫**"作为分发与评审的固定检查项 |
| RB16 | 控制者已落盘两条引擎行为知识并索引 MEMORY（提交 `4ef3a65f8`）：unique boss pack 也产生拴系随从（度量须用 `isUnique()` 区分 + 断言 unique 随从数>0 防 A/B 空转）；`PlaceGroup` 的 4 格拴系从 **leader 的邻格**量起（真实边界 >4） |
| Task 3 fix round 1 | 已分发（agent `a9289c58`，claude-opus-5）：F1 具名 `SquadMinionsKeepOwnAi`、F2 回退路径**相邻性**可失败守卫（含把 `&leader` 改 `nullptr` 的反证）、F3 "partner 异类型"可失败守卫（含删除跳过的反证）、F4 `squad_leashed` 拒错用例；**四条反证都要两次实跑**；RB17（L14 冻结、不动 ceiling、不写 docs） |
| Task 3 fix round 1 | DONE（`29efdda31`/`cbc51d053`/`c93a2664b`/`fb31c8218`）：`level_roster_baseline_test` 9/9（新增具名 `SquadMinionsKeepOwnAi`）、`level_roster_test` 37/37（新增 2 条 F4 拒错死亡测试）；门禁 **763/0/100%/drift ok**；eval smoke exit 0；`level-rosters` 计数 8→9 同步 + 新增 `output_contains: "[ SQUADFALLBACK ]"`。**四条反证均有实跑**：F1 改 `inheritAi=true` → 每 seed 报 30+ 随从 AI 被覆写（红）；F2 `&leader`→`nullptr` → 邻近性断言红（实测 max 距离 12 vs 界 32）；F3 删异类型跳过 → 异类型断言红（6/6）；F4 把 `std::unexpected` 换成静默 `return false` → 2 条红 |
| RB18 | **接受 F2 的界 32**：代码只能证到 101（100 步无界游走 +1），但 **101 不可失败**（散布区 `[16,96)` → 全图任意两格 Chebyshev ≤80，连"完全不传 leader"也能过 101）→ 实现者先实测两侧（出厂 25204 个随从 max **17**；`nullptr` 同口径 **66**）再取 32（≈1.9× 实测、不到 66 的一半），推导与两次实测写进注释，并**无条件打印** `[ SQUADFALLBACK ] … max N bound 32` 让漂移先可见。接受；**该界与 `try2` 上限、散布区范围、`squad_size` 绑定**——任一变化须复核（已写入规格附录 E） |
| RB19 | **接受在产品代码加两个测量计数**（`SquadRollCounters::partnersPlaced` / `maxPartnerLeaderDistance`）：unleashed 随从 `leaderRelation==None`/`leader==NoLeader`，**放置后无法与 leader 重新配对** → 邻近性只能在放置那一刻记录（正是 F2 指令里"必要时扩展观察器"的情形）；与既有 roll 计数同源、由 verbose 诊断/测量读取、**不改变放置行为**。替代方案（给 unleashed 随从留反向指针）会改 §4.3.4 语义，代价更大 → 不采纳。**约束**：仅诊断/测量消费者，不得影响放置/RNG/状态 |
| RB20 | **接受 F1 具名用例与内联断言并存**（同一断言两处）：保留内联是为了 `SquadFormsAroundACoreLeader` 失败时也能报出 AI 违规；要求两处各有注释说明**这是有意重复**（防止将来被"DRY"掉一处而丢守卫） |
| Task 3 fix round 1 | 范围化复审已分发（agent `819b5ab0`，claude-opus-5）：逐条验 F1-F4 的**可失败性**、F2 的界与无条件打印、**产品侧两个计数是否真的不改变放置行为**（不得多一次 `GenerateRnd`/改状态）、eval 计数同步、未越界（L14 冻结/ceiling 未动/无 docs/无占位） |
| Task 3 | **complete**（`b6cd416e5`/`1ea3dfda3`/`9d46b0ef1` + 修复轮 `29efdda31`/`cbc51d053`/`c93a2664b`/`fb31c8218`）；范围化复审**全部 8 项 ADDRESSED、无未关闭项**（F1 具名用例含防空转 + 两处"有意重复"注释；F2 邻近性真断言 + 无条件打印 + 界推导自洽；F3 异类型断言依赖被删逻辑故会红；F4 钉住解析器自有文案；**产品两计数经 `grep -c GenerateRnd`=0 证明无新 RNG/状态改写**；eval 计数实跑一致；未越界；无新破坏）。复审另注明：F2/F3 的"改坏→红"由**实现者实跑**、复审为静态推导——两者互补 |
| Task 3 | 附录 E 第 7 条（界的参数绑定）已提交（复审发现我未提交，已补） |
| Task 4 | 已分发（BASE=当前 HEAD）：形成率阈值（由实测导出）+ 按层回退；**L14 冻结（RB17）**、必须**同时对验收8 与 9c 重测（RB14）**、附 `[ MEASURED ]` 9c 本轮证据 |
| Task 3 | **CI 绿**（R43 已核实远端）：run `35144671104` on `1a90a16ae` → `success`（含 Task 3 三次修复提交 + 规格附录 E） |
| Task 4 | DONE_WITH_CONCERNS（`a7a56dcd1` counter + `bb3a17326` 守卫/eval，BASE `1a90a16ae`）：`level_roster_baseline_test` 9→10；**500 seed/层实测形成率 99.890-100%**（最差 L10 99.890），统一 floor **0.95**；**验收 9c 本轮证据** `level 14 unique bases reachable 6/6`（findings6 满足）；验收 8 本轮 L1-15 全在 ceiling（L14 0.60512 ≤ 0.60876）；门禁 764/0/100%/drift ok；eval smoke exit0 + `--run level-rosters` PASS(50/50)；**CI run `35151879181` ✓ 5m11s**；**无层需回退、params 表零改动**（RB17 遵守） |
| RB21 | **接受 leash 无关的 `SquadRollCounters::formed`**：既有 `realised` 的判定含 `squadLeashed && packSize>0`，而 `PlaceGroup` 只在 leashed 时写 `packSize` → 用它做分子会让**任何采用 §4.3.4 回退的层恒读 0%**，即"守卫因它自己要求的补救被采用而变红"。改用按 roll 计的 `formed` 是正确修正；分母沿用循环自己的 `rolls`（L14 chance=10 按实际 roll 数评判） |
| RB22 | 运行时：新用例 500seed×15层 ≈290s（二进制 184→~475s），eval case `timeout: 900` 余量不宽 → **Task 5 若再加长用例必须同步上调**；500 seed 是简报硬要求故不下调 |
| RB23 | **要求把 `ShippedSquadChanceRealisesSquadsOnEveryLevel` 的 `EXPECT_GT(realised, 0)` 改用 `formed`**：该断言的语义是"小队真的成队"，而未来某层一旦采用 §4.3.4 的 `squad_leashed=0` 回退（规格明文允许的补救），leash 相关的 `realised` 会对该层变红——那是"**在规格允许的补救上埋雷**"，属自相矛盾守卫家族。实现者以"不写未实现内容"未预改 → 我判定这**不是**未实现内容，而是让既有守卫对已文档化的补救保持正确（`formed` 语义更准）。并入 Task 4 修复轮 |
| RB24 | **接受统一阈值**（各层差异 <0.11pp，逐层分化只会把噪声固化成规格）；数组仍按层索引保留分化能力 |
| Task 4 | 控制者核实：`formed` 计数已加、阈值数组 `kSquadFormationFloor`（size 17，注释含"MEASUREMENT, not chosen"与全 15 层实测）；**`level_roster_params.tsv` 相对 BASE 零 diff**（RB17 遵守）；CI run `35151879181` 经远端核实（见台账下方）；规格已补 7b 实测块（`e9ad6ffc6`，docs-only 故不触发 CI） |
| Task 4 | 任务评审已分发（agent `bfafff83`，claude-opus-5）：8 个核对点含**分子/分母语义**、**豁免逻辑是否真被强制**、两条反证、RB23 未做项、本轮 8/9c 证据真伪、未越界、运行时余量 |
| Task 4 评审 | 裁决**通过**（agent `bfafff83`，claude-opus-5；无严重）：①分子/分母语义有代码支撑（`formed` 在 `ActiveMonsterCount > beforePartners` 时自增、leash 无关；`realised` 确被 `squadLeashed && packSize>0` 门控）②**豁免前提被真正强制**（`rolls==0` 前先 `EXPECT_EQ(params->squadChance,0)`，且解析器兜住 `chance>0 && size==0` 拒表）③阈值可在实测为空的间隙失败、L16 哨兵取 0.0 方向正确④**本轮证据为真**（`LastTest.log` 时间戳 + 独立算 `0.6087595` 吻合 + 逐层数值与注释逐位一致）⑤未越界/门禁同步/运行时 475.32s 对 900s 余量 47% |
| RB23（确认） | 仍未做：`ShippedSquadChanceRealisesSquadsOnEveryLevel` 用 `leadersWithMinions`（依赖 `getLeader()` 链，仅在 leashed 时由 `setLeader` 建立）→ 未来某层按 §4.3.4 置 `squad_leashed=0` 会**恒 0 → 变红**，且失败文案会把**合规回退误报成"小队完全没形成"**（正是新用例特意避开的陷阱留在邻案里）→ 改用 `GetSquadRollStats().formed` |
| RB25 | 三项轻微并入同一修复轮（均属**诊断质量**）：①非 leashed 分支的 `EXPECT_EQ(leashedRealised,0)` 在现码下**同义反复且该分支今日为死码** → 保留断言但**如实改注释**（说明它在出厂表下为空、存在意义是未来回退层）②`[ SQUADFORM ]` 只打 rate **不含丢失归因**，而失败文案只开"该层 `squad_leashed→0`"一味处方 → 若真因是 `noPartnerAvailable`/`totalmonsters` 钳制，unleash 并不能修，**诊断会引错方向** → 打印丢失计数器并让文案按计数器指路③豁免失败时仍打印"disabled by the table (chance 0)"而表里是 30 → 日志自相矛盾 → 按实际 chance/rolls 打印 |
| 新证明形态 | RB23 的证明不是"改坏→红"，而是"**施加规格允许的补救→必须仍绿**"：用夹具把某层 `squad_leashed=0` → 修复后 `ShippedSquadChance...` 与 `SquadFormationRate` 都应保持绿（`formed` 与 leash 无关），而旧断言会红 |
| Task 4 fix round 1 | DONE（提交 `bb24de4d2`，CI run `35162721917` success）：F1-F4 修完；**F1 的新形态证明拿到实证**——临时"L10 无拴系"夹具下守卫**仍绿**，日志对照 `formed=261` vs 旧断言会红（`realised=0`）；夹具已完全恢复、出厂数据零永久改动；10/10 目标用例通过；门禁 764/764 100%/drift ok；eval smoke 36/36 |
| Task 4 fix round 1 | 范围化复审已分发（agent `←`，claude-sonnet-5）：核 F1 分子语义与断言仍非恒真、F1 证明的对照是否可信、F2 注释是否如实且未删断言、F3 打印/文案是否按计数器指路（不再一味开 unleash 处方）、F4 日志是否与事实一致、未越界（L14 冻结/阈值未动/无 docs/无占位） |
| Task 4 fix round 1 | 控制者核实：`bb24de4d2` **只动 `test/level_roster_baseline_test.cpp`**（+99/-16，无夹具/出厂数据/阈值/docs 改动）；`formed` 分子在位（:934/:946）并新增丢失计数器累计与打印（`:863-864`/`:943-946`/`:963`）；树干净；**CI `35162721917` 已远端核实 = success on `bb24de4d2`** |
| Task 4 | **complete**（`a7a56dcd1`/`bb3a17326` + 修复轮 `bb24de4d2`；CI `35151879181`/`35162721917` 均绿且经远端核实）；范围化复审 **7/7 ADDRESSED**（F1 分子改 `formed` 且**仍非恒真**、失败文案不再误报合规回退；F1 证明具体数字复审无法只读重放但**夹具无残留已核实**、代码路径支持；F2 注释如实且断言未删；F3 文案按计数器**三路分派**（伙伴不足/leader 放置失败/否则才开 unleash 处方）无残留旧处方；F4 日志与事实一致；未越界；10 用例未变；两目标用例实跑 PASS 26.5s + 290.5s） |
| Task 5 | 已分发（阶段 B 收尾）：冻结树上的全量验证 + 附录 D 两条可证伪预测的**如实覆盖声明** + 存档格式台账行 + 计划实施记录 + eval 收口 + 推送并跟踪 CI |
| 阶段 B 进度 | Task 1-4 **全部 complete**（各经任务评审 + 修复循环 + 范围化复审，四次 CI 均绿并经远端核实）；**Task 5（收尾）** 进行中（agent `454076ff`）→ 之后为**阶段 B 最终全分支评审** |
| Task 5 | **complete**（`3a404677b`/`a5475fdb1`/`dfbef1f1b`，CI `35166422076` 已由实现者跟到 success；控制者再核实一遍）：冻结树**新跑**验证 764/0/100% + drift 5/5、eval smoke 36/36、`level-rosters` 50/50（~450-460s vs timeout900）。**预测 3 覆盖**（6 个用例 + 逐层形成率 99.89-100% + 随从距离 max 12 << 回退界 32）；**预测 5 未覆盖**（`nm -C` 实证 `GroupUnity`/`FollowTheLeader`/`IsLineNotSolid` 为匿名命名空间内部符号、唯一调用点在 `ProcessMonsters()` tick 内、仓库无任何测试驱动完整 tick；已写人工验证方法）——**如实声明，非假装通过** |
| 阶段 B | 5/5 任务完成 → 进入**最终全分支评审** |

## 阶段 B 收尾呈报稿（待最终评审返回后提交给作者）

### 1. L14 的取舍（RB17 遗留，三选一 + 我的建议）
事实：L14 cores = **1 近战 + 2 远程**；成队的 partner 有 2/3 概率抽到远程 → `squad_chance=30` 时 placed 远程占比 **0.615081 > ceiling 0.60876**；而"抬 Melee floor"虽过了验收 8 却把验收 9c 从 6/6 打到 2/6（Task 3 实测）→ 自行回退。现状 `squad_chance=10`（远程占比 0.605120，9c 6/6），**余量仅 0.36pp**（比 R34 曾以"太贴边"否掉的 1.2pp 方案更紧），且 L14 只拿到 8.8% roll 率≈几乎不享受本特性。
- **A（维持现状）**：两守卫完好、L14 近乎不参与小队；余量 0.36pp 需长期看护。
- **B（恢复 `tail_draw=2`）**：物种 4→5、余量 1.2pp（R34 曾否掉，理由是"太贴边"）——但它**不改善** L14 的小队参与度。
- **C（提高 L14 的 ceiling）**：能让 L14 用 chance30，但**这是规格变更**（放宽红线 14 的守卫），我不建议。
- **D（我的建议：先做的一次低成本实测）**：**重塑 L14 的 core 构成**（例如 2 近战 + 1 远程，而非抬 floor）——让成队的 partner 不再 2/3 是远程，从而在**不放宽任何守卫**的前提下容纳 chance30。**成功则采纳；失败则维持 A**（并把实测可行域写入附录 E）。这条之所以不同于已被否的"抬 Melee floor"：改的是 core **名单**而非 floor 约束，代价面不同，须实测 8 与 9c。
### 2. 下一步主线优先级（我的建议）
1. **A2（HF overlay 的 L17-24 名册表）**：数据为主、风险低，且**闭合红线 12「全层段定义」**（目前只对 L1-16 成立）并消除 R28 legacy 特例；
2. **O2（加载期校验是否计入 quest 无条件预加）**：规格精确性；
3. **工具层剩余**：`passed_min` 硬编码计数机制（R35）、`drlg_l2.cpp:2072` 既存越界单独开单；
4. **（阶段 C 候选）**地狱段多样性恢复：附录 E 的四条路径（放宽 cap / 提占比上限 / 扩 Melee 池 / 编组层）——建议在 A2/O2 之后用实测数据再决策。
| 阶段 B 最终评审 | 裁决**修复后可合并**（agent `24ed88d8`，claude-opus-5）：计划/规格本身无需返工；唯一显著偏离（L14 30→10）合理且已按 R4 只改数据 |
| **S1【严重】** | **G1 的索引清理漏掉 `LeaderRelation::Separated`**：`ReleaseMinions` 只过滤 `Leashed`，而 `GroupUnity`（:1653-1655）在视线被挡时把随从改成 `Separated` **并保留 `leader` 索引** → 普通 leader 死后该随从被跳过 → 槽位复用后其索引指向**无关怪**（视线通且<4格时还会 `leader.packSize++` 并翻回 `Leashed`，随后 `DirOK`(:4662) 把移动锁死在陌生怪 4 格内、`FollowTheLeader` 同步其 `position.last`）。评审实证全仓**无任何路径**清 `Separated` 的索引。**这正是 §4.3.1 声称要消除的场景，只是换成 `Separated`，且因阶段 B 引入普通 leader 而新变可达**。路径＝"引开 leader >4 格→脱队"（正常玩法；虽落在预测 5 未覆盖区，但**不豁免**——用例可绕开 tick：直接置 `Separated` 后走 `RunEngineDeath`） |
| RB26（S1 修法） | 过滤放宽到 `!= None`；但 **`Separated` 分支只在 `clearReference`（非 unique）为真时清索引**，保证 unique 逐字节不变；`ShrinkLeaderPacksize` 对 `Separated` **不减** `packSize` 是**正确的**（分离时已减过）→ **勿动**。补两条用例：普通（含 `Separated` 变体）清除 / unique（含 `Separated` 变体）保留 |
| **S2【严重】** | **小队随从被 `monhealthbar` 染成蓝名**：判据 `leader != Monster::NoLeader`，而 `setLeader` 的注释明示保留索引就是为了给"buffed minions"上色 → 蓝名的既有含义＝**HP×2 强化随从**；小队随从必走 `setLeader` → **一律蓝名**，但按 G2 `tough=false` **毫无强化** → **UI 宣告与"只买构成不买数值"（红线 9/11 核心承诺）相反**，且规格 §4.3/验收 5-9c/eval 全无此项 |
| RB27（S2 裁决） | **采纳评审方案 (a)：把着色判据改成真实"被强化"判据**（推荐 `leader->isUnique()`——G2 后只有 unique 包的随从会被强化），并要求抽成**可测试的谓词**（`IsBuffedMinion` 之类）以便无头验证；不采纳 (b)"接受蓝名=有 leader"（那是未记录的语义漂移）。附注：若将来出现其它强化路径，该判据须重评。规格 §4.3 与 §5 补记（控制者做） |
| RB28 | **I1/I2 同轮修**（纯测试）：I1 `ObserveSquads` 的 `minionsOutsideLeash` 用 `dx>4` 判越界而通过钳制的候选必然 `dx<=4` → **恒真**，改成可失败的紧界 4 或删除并注明由 `maxPartnerLeaderDistance` 统一守；I2 **`squad_size` 旋钮当前无任何守卫**（把 `PlaceGroup` 第 2 参改成常量 1，10 条用例全绿）→ 加弱但可失败的断言（如 `minions >= realised*1.5`，出厂实测 1.98，改 1 会掉到 1.0 必红）+ 反证 |
| RB29 | **A2 升为最高优先级**，理由比台账更强：L17-24 无 params 行 → `rosterParams == nullptr` → **完全没有小队**，红线 12「全层段定义」的缺口从"名册"扩大到"小队"；L14 三选一同意"**先试 D（改 core 名单而非 floor）、失败退 A**"，并**明确排除 C**（提 ceiling＝放宽红线 14 守卫） |
| 延期（评审分拣） | M1 夹具注释称"只差 squad_chance"但实际改了两列（A/B 单变量立论不该在此失准）→ **并入本轮**；M2 RB14 的"`squadSize=2<na` 故成队**反而少**占槽位"不精确（`na` 半数=1、均值≈2.5，小队占3 槽是**略多**；结论由 500-seed 实测支撑不受影响）→ **控制者改写台账措辞**；M3 `SquadRateIsMeasuredPerLevel` 与 `ShippedSquadChance...` 重叠（各 15 层×50 seed）→ 建议**降重而非再上调 timeout**（延期）；M4 `Timedemo` 仍隔离而本轮改了 L1/L2 放置序列与 RNG 流 → **重录必要时上升，列入呈报缺口**；M5 `MT_NAKRUL` 早退不回滚 `AMC++`（既有、本轮不可达）→ 仅记录 |
| 评审改进建议 | ①修 S1 时落成知识：**"leader 关系三态 + 8 个消费者清单"**（`GroupUnity`/`FollowTheLeader`/`DirOK`/`ShrinkLeaderPacksize`/`ScavengerAi`/`SyncPackSize`/`monhealthbar`/`msg.cpp` delta）——同一处踩两次的根因就是没有这份清单（**控制者本轮落盘**）；②规格 §5 红线检查加固定问句"**是否改变任何玩家可见的标识/提示/着色语义**"（S2 漏检的结构性原因：验收全在量放置构成与数值）（**控制者本轮落盘**）；③`monster.h` 新增 98 行几乎全是注释 → 决策性说明移到 `docs/knowledge/`；④6 次反证证据都在被 gitignore 的报告里 → 每条守卫在代码注释里一句话写明"什么改动会让它红" |
| 最终修复轮 | DONE（`a54666156` S1+S2 引擎 / `00ad9173f` 用例+夹具 / `a4d0eebd0` I1+I2+M1 / `542afe208` 门禁目标清单修复 + 两条知识；起点 `3dc643820`）：**6 条反证全部实跑**；用例数 `SamplingBaselineTest` 30→33（`sampling-anti-monopoly.yaml` 已同步）、baseline 过滤集维持 10；门禁 **767/0/100%/drift ok**；eval smoke exit0、`level-rosters` 60/60、`sampling-anti-monopoly-cap` 15/15；**CI `35178605691` 绿** |
| **重大发现（它自挖）** | `tools/run_tests.py` 的 `TEST_TARGETS` 是**手抄清单**，`level_roster_test`/`level_roster_baseline_test` **从未登记** → **全量门禁从不重建这两个二进制**，ctest 跑磁盘陈旧产物（含它反证时留下的变异产物）。**这意味着我此前的门禁数字对这两个二进制的"新鲜度"无保证**（定向 `--test` 跑是会构建的——RB10 之后——但全量门禁不会）。已补入并验证（touch 后出现两行 Linking；差集只剩既有条件目标） |
| **重大发现 2（它自挖）** | 新用例挂住 + `items.cpp:648` UBSan：`PrepareDeathPathPrerequisites` 未加载 `SpellsData` → 死亡掉落抽到书时 `GetBookSpell` 的 `GenerateRnd(0)+1` 使 `rv==1` 永不归零、环绕判据永不命中 → **死循环**。按 `diablo.cpp:2809` 顺序补 `LoadSpellData()`，**未用"换个种子"绕开** |
| 最终修复轮 | 范围化复审已分发（agent `911b086c`，claude-opus-5）：除 5 项修复外，要求做**系统性核对**——把 `CMake/Tests.cmake` 注册目标与 `run_tests.py` 的 `TEST_TARGETS` **逐一比对**（是否仍有遗漏/多余；条件目标 `text_render_integration_test` 的处理是否合理）；并核 `LoadSpellData()` 的插入顺序与**是否扰动既有用例**（本会话反复出现的夹具污染风险） |
| 诚实说明 | `TEST_TARGETS` 漏登记的后果：**我此前"全量门禁"的数字对那两个二进制的"新鲜度"无保证**（定向 `--test` 在我 RB10 改动后会构建、实现者也各自显式构建过，故**内容验证不受影响**；受影响的是"全量门禁 = 全新构建"这一表述）。已修（`542afe208`）并有 CI 全新构建佐证 |
| 最终修复轮 | 范围化复审裁决 **可合并（是）**（agent `911b086c`）：5 项 + 2 处自挖缺陷全部 ADDRESSED；**系统性核对通过**（`Tests.cmake` 70 = `TEST_TARGETS` 70、双向差集空）；它还**纠正了修复者的说法**（条件目标 `text_render_integration_test` 其实在清单里）。4 项 Low：L1 yaml 措辞（"逐字节不变"仅对索引成立）、L2 谓词解引用内存安全（leader<200 恒在界内）、L3 清单手抄无自动守卫、L4 未实跑 290s 用例与全量门禁（但它核实了二进制 mtime 新于源码） |
| L1/L3 已由控制者修 | **L1**：yaml 措辞改准（unique 路径"**索引**保留、relation 现统一置 None；旧码在 Separated+unique 时保持 Separated，新码改 None 属**修正**，已被用例断言）。**L3**：给"清单漂移→假绿"加了**自动化守卫** `check_drift.py` 的 **检查 F**（`CMake/Tests.cmake` 的 tests+standalone_tests ↔ `run_tests.py` 的 TEST_TARGETS，含 `list(APPEND ...)` 声明形式），并**用两次 sabotage 证明它可失败** |
| 第 6 次"假守卫"（自己踩） | 我第一版检查 F 的**正则写错**（要求行末有右括号，而 `Tests.cmake` 实际写 `set(tests`、右括号在块尾）→ 只收集到 1 个目标（来自 `list(APPEND)` 支）→ `cmake - runner` **恒空** → **F 是假守卫**。抓住它的是我按纪律做的 sabotage；修正后两侧 70=70、两类 sabotage 均 FAIL。**这是本会话第 6 次"看起来在守、实际不守"，且这次发生在我自己的守卫上** |
| 阶段 B 终局 | **完整门禁 exit 0**：ctest **767/767** failed 0 pct 100、`drift_ok True` **passes 6**（A/B/C/C2/E + **新增 F**）；**CI run `35180257042` = success on `350902cbe`**（远端核实）。阶段 B 完全收口 |
| 后续（作者认可"按建议来"） | **D（L14 重塑 core 名单）→ A2（HF L17-24 名册，最高优先级）→ O2 → 工具层**；D 与 A2 都改 `level_rosters.tsv`/params 表 → **必须串行** |
| 任务 D | **采纳**（提交 `080963871`）：删掉 L14 的 **RangedKite core**（`MT_XACID`）→ cores 变 1 Melee + 1 RangedTurret；`squad_chance` 10→30；placed 远程占比 **0.605120 → 0.543672**（余量 0.36pp → **6.5pp**）、9c **6/6 → 6/6**；门禁 767/767 + drift A-F、eval smoke exit0、`level-rosters` 60/60；CI `35189391029` success |
| RB30 | **接受 D**：它**未动任何守卫**（ceiling/floor/baseline/tolerance、`class_floors` 仍 Melee=1、`squad_size`/`squad_leashed`、caps、其它层均未动；test 侧 diff 仅注释）。关键结构判据（代码+实测双证）：partner 从**其他 core** 抽 → 3 core 里 2 远程即每队 2/3 偏远程；**不能加 Melee 也不能加第二个 Turret**——L14 的 6 个 unique base 是 4 Melee + 2 Turret，cap=2/类，core 计入 `classCounts`，某类 core 到 2 就把该类整类从尾抽池剔除 → **该类 unique base 永久不可达**（这正是 Task 3 掉到 2/6 的机制，故 RB14 的结论对**任何同类 core 增量**成立）；而 **RangedKite 在 L14 无 unique base** → kite core 是唯一可动的杠杆 |
| RB31 | **解除 RB17 的 L14 冻结**：`squad_chance` 现**全 15 层统一 30**；L14 rollRate 0.088→**0.313406**，每 seed 组合数实测 **51**（远高于 ≥2），玩家侧多样性**升**；若将来想恢复"L14 4-5 物种"，须走**抬 `tail_draw`**而非加 core，且重测占比 |
| 备注 | 实现者提醒：`gh` 默认仓库不是 myrepo，`gh run list` 必须显式 `-R pengyz/devilutionX`（否则查到上游返回空）——控制者此前的命令均已带 `--repo`，后续沿用 |
| 任务 D | **complete**（`080963871`，评审通过：三条结构判据在代码/数据层独立成立；验收 8 `0.543672`、9c `6/6`、组合数 51、rollRate `0.313406` 逐位复现；守卫文件 **md5 与基线一致**＝零放宽） |
| RB32 | 采纳评审的 3 条轻微（**文档同步**）：①规格附录 C 的物种数（L14 3→**2**，我上一轮漏掉的过期行）②附录 E 的 `tail_draw=2` 条文化语境更新（旧论据基于 3-core；现余量 6.5pp，且 RB31 已指 `tail_draw` 为恢复物种数的**正途**）③`RosterQuotaAllowanceIsBinding` 的"贴限饱和层"现仅由 L15 承担（判别力未失，记录耦合） |
| 备注 | 评审未实跑对照组 A@30（只读不能改 TSV 重建 mpq），但该数值与分发稿/台账**三处一致**且属"判红"方向的不利读数 → 接受为静态一致性核实 |
| 备注（我的操作失误） | 附录 E 第 5 条的首次补丁**锚点未匹配**（我凭印象写锚点，实际措辞不同；脚本如实报 `MISSES`）→ 改为**整行替换**（先读出真实行再改）后落地。教训与文档编辑一致：**不要用记忆中的字符串做锚点** |
