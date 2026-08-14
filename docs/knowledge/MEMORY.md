# DevilutionX 知识索引

## Architecture
<!-- 设计决策、模块边界、为什么选 A 不选 B -->

## Gotchas
<!-- 平台坑、反直觉行为（Diablo 引擎/存档/渲染/Lua 等） -->

- [SpellsData 位置索引 — 删行即错位（已修复）](gotcha_spelldata_positional_index.md) — 曾按行序 emplace 删行即错位；现已改名称键控加载，size 保持 max-enum+1 语义
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

## Patterns
<!-- 代码约定、本项目开发模式 -->

- [测试用行为断言而非内部函数](pattern_test_behavioral.md) — 匿名 ns 内部函数（WitchItemOk 等）不可测；测公开路径的端到端结果（SpawnWitch 后检查 WitchItems）
- [整数百分比数学保确定性](pattern_integer_percent_math.md) — `lrad*pct/100` 而非浮点乘（x86 扩展精度 `10*0.6→5`）；全平台截断一致
- [测试套件结构性盲区](pattern_test_structural_blindspot.md) — UT 只测模板隔离不测真实路径；Oracle 用运行时探针抓到 3 个盲区 bug。新功能测试应经公开 API 触发完整路径
- [CI 分支保护是合入门禁](pattern_ci_branch_protection.md) — 盯 CI 状态治标不治本；master 保护（PR+审查+build-and-test 必选+strict+enforce_admins）才是失败无法合入的保障

## Debugging
<!-- 调试方法、关键日志位置、排查路径 -->

## Decisions
<!-- 已验证的取舍（含正反馈） -->

- [深度层旗舰 = 黑暗远征（视野限制）](decision_dark_expedition_direction.md) — 两轮 Oracle 对抗评审确认：光照是唯一引擎原生信息限制杠杆，但必须配反制（CanTarget + Infravision 卷轴预算）
- [密度塌缩修复框架 = B1 采样约束 + A1/A3 克隆区分 + E1 地狱重组](decision_density_fix_package.md) — Oracle 对抗评审收敛的「最小连贯包」；关键洞察「池≠体验」（GetLevelMTypes 随机采样，B1 是唯一逐层体验杠杆）；排除 B2/C3/E2。**后续演进（2026-08-10 定稿）**：E1→悬崖评估（REJECT 重构）、A2→激励者（REJECT 重构）、全部 6 规格经多 agent 独立复核后 v4/v5 修订完成（A1/A3/B1/A2 v4、C2/E1 v5）——以框架总览为准
- [密度修复框架独立复核结果（4 agent 发现系统性引擎事实错误）](decision_density_fix_review_findings.md) — 冲锋伤害路径断裂（special 列=0→1 伤害）、B1 27.8% 夸大 7×（Golem 预算遗漏，真实 4.13%）、E1 GetMinHit 证明错误（漏 base toHit 90-130，GetMinHit 是下限非钳制）、狂乱者 no-op 造假（goal=Attack 是纯 AI 攻击性）、C2 死亡钩子 MP 缺陷、A2 rate 引用错误（AnimStruct 静态字段）。教训：跨文档自洽 ≠ 正确，只有忠实引擎模拟能抓到。**修订状态（2026-08-10）**：全部规格已按复核结果修订（v4/v5），框架 §8.2 验证表自身 2 处错误已修正
- **B1 采样反垄断已定稿（2026-08-13）**：Oracle 对抗评审 PASS（8/8 AC，cap 零 RNG 消费、1-8 层字节级 vanilla、L16 早退）；4 条非阻塞建议落地（BoneDemon 注记/quest 双计数单测/非 Boss 映射断言/L16 注释）。实现零回归——全量门禁 4 失败全部由 f474a64c0（开关移除）引入而非 B1，详见 [gotcha_vendor_predicate_seed_drift](gotcha_vendor_predicate_seed_drift.md) 与 [gotcha_timedemo_isOnActiveLevel_failure](gotcha_timedemo_isOnActiveLevel_failure.md)

## References
<!-- 外部资源指针、关键文件位置 -->

- [怪物配置图谱：各层段怪物池与密度诊断](analysis_monster_config_landscape.md) — 系统扫描 monstdat：教堂12类/墓穴13类/洞穴10类(远程垄断38%)/地狱6类(多样性骤降)；密度修复框架（A1/A2/A3/B1/C2/E1）各规格落地依据

## Gotchas
<!-- 平台坑、反直觉行为、踩雷记录 -->

- [timedemo_test 本机断言失败（isOnActiveLevel）](gotcha_timedemo_isOnActiveLevel_failure.md) — `Timedemo.WarriorLevel1to2` 在本机 `interfac.cpp:363` 断言崩溃（`plrlevel != currlevel`）；已确认与 B1 cap 无关（stash 回退后仍在 HEAD 基线失败），CI 不受影响。待单独排查（git bisect + 回放路径时序）
