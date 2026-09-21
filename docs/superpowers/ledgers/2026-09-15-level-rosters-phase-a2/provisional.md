# 待追认账本（自主决定，可一键回滚）

> 规则：**可逆且不可见**的决定由主 agent 自定并记此账；作者可一次性追认或逐条回滚 ✓。
> 命中 artifact 面（存档格式 / eval case / 掉落与数值表 / RNG 消费 / 可观测信息量）的决定**不进此账**，而是**提问** ✓。

| # | 自主决定 | 可逆性 | 玩家可见 | 回滚方式 |
|---|---|---|---|---|
| P1 | 基线测量规模 **N=100**/层、容差 **0.05** | 可逆 ✓ | 否（仅测试时长） | 改 `test/pressure_shapes_test.cpp` 常量 ✓ |
| P2 | A 腿分带参数**草案** 25/35/45/40（chance） | 可逆 ✓ | **落地后才可见** ⚠ | 调 `level_roster_params.tsv` ✓（若落地前有异议，改草案即可 ✓） |
| P3 | 新测试落点 `test/pressure_shapes_test.cpp`；eval 用例放 `eval/cases/combat/` | 可逆 ✓ | 否 | 移动 + 改注册 ✓ |
| P4 | 规格文档结构（附录 C/E/F/G 的组织方式） | 可逆 ✓ | 否 | 重排文档 ✓ |
| P5 | 第 0 章计划的**实现路径**（意图 sidecar、原子替换、`writeGameData=true`；受已批准契约约束 ✓） | 可逆 ✓ | 否（契约已批准 ✓） | 改计划 ✓ |
| P6 | **D-2 只登记不修**（HF 专属导弹在 base 数据下越界 ⚠） | 可逆 ✓ | **潜在可见**（越界读可能崩）⚠ ⇒ 若要立刻修，请说 ✓ | 立修 ✓ |
| P7 | `docs/knowledge/` 的三条 gotcha 与索引更新 | 可逆 ✓ | 否 | 删/改 ✓ |

**本轮自主决定 N = 10 项**（全部可一键回滚 ✓）。
| P8 | 逐字节不变量守卫的**哈希算法＝FNV-1a 64 位**（非 SHA-1：仓库只有分块 API ✗；也非提交表副本 ✗） | 可逆 ✓ | 否 | 改算法 + 重新生成 `immutable_tables.fnv64` ✓ |
| P9 | 不变量守卫覆盖面由 **3 表扩到 10 表**（加 unique_monstdat、base+HF unique_itemdat、base+HF item_prefixes/suffixes） | 可逆 ✓ | 否 | 从 `tools/gen_pressure_goldens.py` 的清单里删行并重跑生成器 ✓ |
| P10 | 黄金集用**纯文本**（非 JSON ✓，免 C++ JSON 依赖）；(a) 实时等式与 (c) 白名单由**门禁步骤** `run_tests.py::run_pressure_tables()` 强制（非 C++ 测试 ✓） | 可逆 ✓ | 否 | 改生成器输出格式/移除该门禁步骤 ✓ |
