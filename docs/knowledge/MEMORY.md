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

## Patterns
<!-- 代码约定、本项目开发模式 -->

- [测试用行为断言而非内部函数](pattern_test_behavioral.md) — 匿名 ns 内部函数（WitchItemOk 等）不可测；测公开路径的端到端结果（SpawnWitch 后检查 WitchItems）
- [整数百分比数学保确定性](pattern_integer_percent_math.md) — `lrad*pct/100` 而非浮点乘（x86 扩展精度 `10*0.6→5`）；全平台截断一致

## Debugging
<!-- 调试方法、关键日志位置、排查路径 -->

## Decisions
<!-- 已验证的取舍（含正反馈） -->

- [深度层旗舰 = 黑暗远征（视野限制）](decision_dark_expedition_direction.md) — 两轮 Oracle 对抗评审确认：光照是唯一引擎原生信息限制杠杆，但必须配反制（CanTarget + Infravision 卷轴预算）
