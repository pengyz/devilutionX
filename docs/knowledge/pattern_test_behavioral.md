---
name: 测试用行为断言而非内部函数
description: 匿名 ns 内部函数不可测；测公开路径的端到端结果
type: pattern
created: 2026-08-08
sources: [test/dark_expedition_scroll_test.cpp, Source/items.cpp]
---

DevilutionX 大量内部函数（`WitchItemOk`、`RndAllItems`、`CalcPlrLightRadius` 等）在匿名命名空间，测试不可直接调用。

正确模式：
1. 优先测**公开路径**的端到端结果（如 `SpawnWitch` 后检查 `WitchItems`）
2. 确需测内部函数时，在头文件用 `DVL_API_FOR_TEST` 导出（`items.h` 有先例：`CalcPlrLightRadius`）
3. 断言用 `passed_min` 防空跑（filter 匹配 0 个测试 = FAIL）

**为什么：** 测内部函数使测试与实现耦合，重构即碎；行为断言验证的是玩家可感知的结果。

**何时使用：** 给引擎功能写测试时。随机性路径（如卷轴库存）用多轮采样 + 确定性断言（开关关恒出现、开关开恒不出现）。
