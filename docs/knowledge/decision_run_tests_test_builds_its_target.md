---
name: run_tests.py --test 现在会先构建目标（历史坑与现行语义）
description: 历史坑：--test 只执行已存在的二进制、不构建，改完代码后跑定向测试会量到陈旧二进制。现已修复：--test 会先构建该目标，目标不在本次配置内则明确失败。本文记录语义与老版本上的注意事项。
type: decision
created: 2026-09-15
updated: 2026-09-15
sources:
  - tools/run_tests.py（build_target() 与 --test 分支，2026-09-15 起）
  - .superpowers/sdd/2026-09-15-level-rosters-phase-b/progress.md（RB10）
---

**现行语义（2026-09-15 起）**：`python3 tools/run_tests.py --test <name>` **会先构建 `<name>` 目标**（`build_target()`，镜像 `build_tests()` 的 generator 处理，但只构建这一个目标，避免 `test_impact.py --diff | xargs -n1 run_tests.py --test` 每次重建全部目标），然后才执行它。若 `<name>` 不在本次构建配置内（`build.ninja` 的目标列表里没有），它会**明确报错并返回 1**，不会去跑一个陈旧的二进制。`--no-build` 可跳过构建。

**历史坑（在此修复之前）**：`--test` 分支在 `not args.test` 条件下才构建，因此它**只执行** `build/<name>`——改了源码/测试后直接用它，量到的是**旧二进制**，表现为"我改了但结果没变"，极易误判。当时全量模式（不带 `--test`）才会构建。

**为什么重要**：定向测试是"改一处 → 立刻验证"的常用路径（`AGENTS.md` 也把它列为标准用法）；陈旧二进制会让验证与工作树脱节。

**仍需注意**：① 测试读的是 `build/devilutionx.mpq`——改 `assets/txtdata/**.tsv` 后仍必须先 `ninja devilutionx_mpq`（见 `gotcha_tsv_edits_need_mpq_rebuild.md`），`build_target()` 只构建测试目标本身；② 若你工作在**较早的提交**上，`--test` 仍是不构建的旧语义，请自行先 build。
