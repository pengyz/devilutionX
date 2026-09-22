# DevilutionX 知识索引

## Architecture
<!-- 设计决策、模块边界、为什么选 A 不选 B -->

## Gotchas
- [`gotcha_unpacked_mpqs_mod_manifest_include_guard.md`](gotcha_unpacked_mpqs_mod_manifest_include_guard.md) — `UNPACKED_MPQS=ON` 曾因 `mod_identity.h` 被并进 guard 而**整引擎编不过**（CI 默认 OFF 故不暴露）；改动 `assets.hpp` 时两配置都要编
- [`gotcha_testing_hellfire_levels_headlessly.md`](gotcha_testing_hellfire_levels_headlessly.md) — 无头测 HF 层：别调 `LoadHellfireArchives()`（会 FatalExit 整个二进制）、补齐 `.til`+触发器、快照还原全局、以真实依赖门控跳过
- [`gotcha_drift_check_c2_whole_file_crlf.md`](gotcha_drift_check_c2_whole_file_crlf.md) — 漂移 **C2** 对"相对 merge-base 新增"的文件要求**整文件 CRLF**（不是只看你加的行）
- [`gotcha_unique_boss_packs_are_also_leashed.md`](gotcha_unique_boss_packs_are_also_leashed.md) — unique boss pack 也带拴系随从（默认 MinionOptions），度量小队必须用 `isUnique()` 区分，并断言 unique 随从数 >0 防 A/B 空转
- [`gotcha_placegroup_leash_is_measured_from_a_neighbour.md`](gotcha_placegroup_leash_is_measured_from_a_neighbour.md) — `PlaceGroup` 的 4 格拴系从 **leader 的邻格**量起，真实边界 >4；断言邻近性别照抄 4
- [`reference_ci_workflows_on_feature_branches.md`](reference_ci_workflows_on_feature_branches.md) — 推 `feature/**` 只触发 `better-d1-ci.yml`；**格式/tidy 检查仅在 master/PR**（"CI 绿"≠"格式合规"）；`docs/**` 提交不触发 CI
- [`gotcha_tsv_edits_need_mpq_rebuild.md`](gotcha_tsv_edits_need_mpq_rebuild.md) — 改 `assets/txtdata/**.tsv` 后必须先 `ninja devilutionx_mpq`，否则测试量到旧表
- [`decision_run_tests_test_builds_its_target.md`](decision_run_tests_test_builds_its_target.md) — `run_tests.py --test` **现在会先构建该目标**（历史坑与老版本注意事项）；改 TSV 仍需 `ninja devilutionx_mpq`
- [`gotcha_run_tests_target_list_misses_new_binaries.md`](gotcha_run_tests_target_list_misses_new_binaries.md) — 全量门禁只构建 `run_tests.py` 手抄的 `TEST_TARGETS`，ctest 却跑 `Tests.cmake` 全部用例；漏登记的二进制会被当**陈旧产物**执行（实例：两个 level_roster 目标；指纹是产物 mtime 早于源码）
- [`gotcha_max_image_bump_exposes_drlg_oob.md`](gotcha_max_image_bump_exposes_drlg_oob.md) — 抬高 `max_image` 会曝光 `drlg_l2.cpp:2072` 的**既存**越界（改变生成期分支覆盖），先判"曝光 vs 引入"再决定回退
- [`gotcha_level_state_when_bypassing_loadgamelevel.md`](gotcha_level_state_when_bypassing_loadgamelevel.md) — 测试里自行建关必须补齐 SOLData / tile 元数据 / trigs；症状指纹：**所有层放置数完全相同**
<!-- 平台坑、反直觉行为（Diablo 引擎/存档/渲染/Lua 等） -->

- [SpellsData 位置索引 — 删行即错位（已修复）](gotcha_spelldata_positional_index.md) — 曾按行序 emplace 删行即错位；现已改名称键控加载，size 保持 max-enum+1 语义
- [`gotcha_death_path_tests_need_loadspelldata.md`](gotcha_death_path_tests_need_loadspelldata.md) — 夹具走死亡/掉落路径必须先 `LoadSpellData()`；空 `SpellsData` 让 `GetBookSpell` 死循环 → `items.cpp:648` 整型溢出，**症状是挂住不是失败**，别靠换种子绕开
- [光照双重计数 bug](gotcha_light_double_count.md) — 撤销前 `10*mult + (_pLightRad-10)` 装备加成叠加两次；正确是倍率作用于总量 `trunc(_pLightRad*mult)`（截断非四舍五入，见规格 §4.2）
- [存档/网络 bId 位覆写](gotcha_save_bid_overwrite.md) — 堆叠数存 `bId` 位域曾覆写物品品质与鉴定标志；存档须存 `count-1` 保持上游逐字节兼容
- [SaveItem 追加/LoadItem 条件读不对称](gotcha_save_stack_append.md) — 无条件追加 `_iStackCount` 而主存档路径从不设标志 → 偏移累积越界写；改存对齐填充字节
- [Infravision 是渲染级法术](gotcha_infra_render_only.md) — `_pInfraFlag` 只设渲染标志不改 tile Lit 位；暗处怪物可见但不可选。修复须新函数 `CanTarget` 而非改 `IsTileLit`（后者同时驱动渲染/自动地图）
- [IsTileLit 三用途耦合](gotcha_istilelit_gates.md) — 选择(cursor/track)、渲染(scrollrt)、自动地图(Explored) 共用；改它破坏三者。目标选择修复须独立函数
- [WitchItemOk 在匿名命名空间](gotcha_witchitemok_anon_ns.md) — `items.cpp:2009` 位于匿名 ns，测试不可直接调；测行为结果而非函数本身
- [网络物品更新要网格索引非 InvList 索引](gotcha_inv_grid_vs_list_index.md) — `NetSendCmdChInvItem` 期望网格索引；传 InvList 索引会 OOB 读 + 多人损坏。用 `NetSyncInvItem` 反查
- [多计数堆叠合并会静默丢物品](gotcha_multi_count_stack_merge.md) — `TryStackInInventory` 曾 +1 合并，叠 3 放到叠 1 变 2；多计数应开新格整放
- [怪物等级对战斗压力无贡献（GetMinHit 钳制）](gotcha_monster_level_no_combat_pressure.md) — `hit = 2×(怪级−玩家级) + 30 − AC` 被 `GetMinHit()` 钳制，地狱层 AC≥16 角色悬崖项完全无效；降怪级是错误杠杆（只影响掉落/XP/被钳制命中）
- [供应商谓词改动导致种子物品生成漂移](gotcha_vendor_predicate_seed_drift.md) — 改 `WitchItemOk`/`RndVendorItem` 过滤谓词 = 改 RNG 重试路径 = 同种子生成不同物品；pack 测试数组/golden SHA/demo 回放全部静默过期（实例：f474a64c0 开关移除后 War Staff→Book of Flame Wave）
- [timedemo 回放断言失败（isOnActiveLevel）](gotcha_timedemo_isOnActiveLevel_failure.md) — `f474a64c0` 的 `DarkExpeditionDropOk` 无条件生效改变掉落池权重 → 同一次 RNG 抽取得到不同物品 → 上游录制的 demo 回放 RNG 流分叉 → `interfac.cpp:363` 断言。**CI 自 2026-08-10 起一直红**（原文「CI 未受影响」已被 CI 日志证伪）；2026-09-15 起 `GTEST_SKIP` quarantine，待重录夹具解除
- [手工解冲突的文件不会继承上游同一 hunk 里自动合并进来的新增行](gotcha_conflict_file_loses_auto_merged_lines.md) — `plrctrls.cpp` 是本轮 4 个手工冲突文件之一，冲突块只处理两行 include 取舍，漏带上游同一次 quest_log 迁移新增的 `panels/quest_log.hpp` include → 7 处编译错误（`91e805149` 修复）；「不重叠即安全」的推断对冲突文件集合失效，需整文件三方核对

## Patterns
- [pattern_sdd_ledger_location.md](pattern_sdd_ledger_location.md) — 台账 canonical 位置：docs/superpowers/ledgers/（进 git）；.superpowers/sdd/ 仅草稿
- [`pattern_assertions_must_be_failable.md`](pattern_assertions_must_be_failable.md) — **断言必须具备可失败性**（本会话 4 次复发的头号缺陷模式：自证/恒真/测量无断言/区间重叠）；写守卫必须能指名"什么改动会让它变红"
<!-- 代码约定、本项目开发模式 -->

- [定长 wire 字段必须按 sizeof 读取](pattern_fixed_width_field_reads.md) — `PlayerPack::pName`/`TEar::heroname` 等定长 `char[N]` 无 NUL 保证；裸指针转 `string_view`/`strcpy` 走 strlen 越界（上游 b4dfc8d26 只修了玩家名，耳朵名 5 处上游至今未修）。验证必须把整个堆分配填满非 NUL，否则 ASan 抓不到
- [测试用行为断言而非内部函数](pattern_test_behavioral.md) — 匿名 ns 内部函数（WitchItemOk 等）不可测；测公开路径的端到端结果（SpawnWitch 后检查 WitchItems）
- [整数百分比数学保确定性](pattern_integer_percent_math.md) — `lrad*pct/100` 而非浮点乘（x86 扩展精度 `10*0.6→5`）；全平台截断一致
- [测试套件结构性盲区](pattern_test_structural_blindspot.md) — UT 只测模板隔离不测真实路径；Oracle 用运行时探针抓到 3 个盲区 bug。新功能测试应经公开 API 触发完整路径
- [CI 分支保护是合入门禁](pattern_ci_branch_protection.md) — 盯 CI 状态治标不治本；master 保护（PR+审查+build-and-test 必选+strict+enforce_admins）才是失败无法合入的保障

## Debugging
<!-- 调试方法、关键日志位置、排查路径 -->

## Decisions
- [decision_content_density_contract.md](decision_content_density_contract.md) — 内容密度四轴地板、非对称 cap、R4 反转（密度优先）、D4 退役
- [`decision_minion_options_and_formed_counter.md`](decision_minion_options_and_formed_counter.md) — 小队**只买构成不买数值**（MinionOptions 全 false）；形成率用 leash 无关的 `formed`（否则合规回退会让守卫变红）；蓝名＝真的被强化
- [`decision_save_format_policy.md`](decision_save_format_policy.md) — 存档格式策略（初版功能阶段不做兼容）+ **格式变更台账**（动序列化前先查、动完记一行）
<!-- 已验证的取舍（含正反馈） -->

- [深度层旗舰 = 黑暗远征（视野限制）](decision_dark_expedition_direction.md) — 两轮 Oracle 对抗评审确认：光照是唯一引擎原生信息限制杠杆，但必须配反制（CanTarget + Infravision 卷轴预算）
- [密度塌缩修复框架 = B1 采样约束 + A1/A3 克隆区分 + E1 地狱重组](decision_density_fix_package.md) — Oracle 对抗评审收敛的「最小连贯包」；关键洞察「池≠体验」（GetLevelMTypes 随机采样，B1 是唯一逐层体验杠杆）；排除 B2/C3/E2。**后续演进（2026-08-10 定稿）**：E1→悬崖评估（REJECT 重构）、A2→激励者（REJECT 重构）、全部 6 规格经多 agent 独立复核后 v4/v5 修订完成（A1/A3/B1/A2 v4、C2/E1 v5）——以框架总览为准
- [密度修复框架独立复核结果（4 agent 发现系统性引擎事实错误）](decision_density_fix_review_findings.md) — 冲锋伤害路径断裂（special 列=0→1 伤害）、B1 27.8% 夸大 7×（Golem 预算遗漏，真实 4.13%）、E1 GetMinHit 证明错误（漏 base toHit 90-130，GetMinHit 是下限非钳制）、狂乱者 no-op 造假（goal=Attack 是纯 AI 攻击性）、C2 死亡钩子 MP 缺陷、A2 rate 引用错误（AnimStruct 静态字段）。教训：跨文档自洽 ≠ 正确，只有忠实引擎模拟能抓到。**修订状态（2026-08-10）**：全部规格已按复核结果修订（v4/v5），框架 §8.2 验证表自身 2 处错误已修正
- **B1 采样反垄断已定稿（2026-08-13）**：Oracle 对抗评审 PASS（8/8 AC，cap 零 RNG 消费、1-8 层字节级 vanilla、L16 早退）；4 条非阻塞建议落地（BoneDemon 注记/quest 双计数单测/非 Boss 映射断言/L16 注释）。实现零回归——全量门禁 4 失败全部由 f474a64c0（开关移除）引入而非 B1，详见 [gotcha_vendor_predicate_seed_drift](gotcha_vendor_predicate_seed_drift.md) 与 [gotcha_timedemo_isOnActiveLevel_failure](gotcha_timedemo_isOnActiveLevel_failure.md)

## References
- [`reference_leader_relation_states_and_consumers.md`](reference_leader_relation_states_and_consumers.md) — **leader 关系三态（None/Leashed/Separated）× 8 个消费者**必查清单；含"只过滤 Leashed 会漏 Separated 索引"的已知坑与蓝名着色语义
<!-- 外部资源指针、关键文件位置 -->

- [怪物配置图谱：各层段怪物池与密度诊断](analysis_monster_config_landscape.md) — 系统扫描 monstdat：教堂12类/墓穴13类/洞穴10类(远程垄断38%)/地狱6类(多样性骤降)；密度修复框架（A1/A2/A3/B1/C2/E1）各规格落地依据

- [`gotcha_pseudo_guards_and_assumed_state.md`](gotcha_pseudo_guards_and_assumed_state.md) — 三类"假验证"：假守卫 / 先测量后断言 / 没读准就断言（含修法与硬规则）
- [`gotcha_build_concurrency_and_line_endings.md`](gotcha_build_concurrency_and_line_endings.md) — 构建并发与行尾五条硬规则：`job_kill` 只是请求 / build 单执行者 / `pgrep` 自匹配与兄弟匹配 / CRLF 最后归一化 / 漂移只认 `drift_ok`
- [`pattern_decision_protocol.md`](pattern_decision_protocol.md) — 决策提问协议：按 artifact 面判定是否问作者 / ≤5 条批量带默认 / 五要素格式 / 不停等 + 待追认账本 / 反"以批准之名改设计"
- [`gotcha_data_changes_need_asset_rebuild.md`](gotcha_data_changes_need_asset_rebuild.md) — 改 txtdata 源表却"无效果"：定向构建不刷新数据资产（需 `ninja -C build devilutionx_mpq`）；`OverridePaths` 在 `TestInitGame()` 内快照，夹具覆盖件须放 build 侧
- [`decision_contract_baselines_are_measurements_not_targets.md`](decision_contract_baselines_are_measurements_not_targets.md) — 契约守卫变红时：基线是"实测"不可抬高；正确做法是回调机制（本案 L4 越顶 ⇒ 回归规格的 L1-8 30/2）
- [`gotcha_ci_is_the_terminal_authority_check_after_every_push.md`](gotcha_ci_is_the_terminal_authority_check_after_every_push.md) — `gh` 须 `-R <fork>`；本地绿≠CI绿（缺 `missingRetailTrn_` 守卫的用例在 spawn-only CI 会照常运行并红）；每次 push 后都看 CI
- [`gotcha_test_metrics_depend_on_the_global_rng_stream.md`](gotcha_test_metrics_depend_on_the_global_rng_stream.md) — 逐种子指标必须每次建关 `InitLevelMonsters()+SetRndSeed()`，否则"隔离跑绿、同进程全量/filter 红"；ctest 逐用例隔离会掩盖它 ✗
