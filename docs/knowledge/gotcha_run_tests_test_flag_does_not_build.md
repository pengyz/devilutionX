---
name: run_tests.py --test 不构建，直接跑会用到陈旧二进制
description: tools/run_tests.py 的 --test <name> 只执行已存在的测试二进制，不先构建；改了源码/测试后直接用它跑定向测试，量到的是旧二进制。
type: gotcha
created: 2026-09-15
sources:
  - tools/run_tests.py:208（--test 分支直接执行，无构建步骤）
  - .superpowers/sdd/2026-09-15-level-rosters-phase-a/task-4-report.md（实现者踩坑记录）
---

**事实**：`python3 tools/run_tests.py --test <name>` **不会**先构建目标；它只是执行 `build/<name>`。改完源码/测试后直接用它，会跑到**陈旧二进制**，得到与代码不符的结果（"我改了但测试还是老样子"或反之）。

**为什么**：该脚本的设计是"跑测试 + 聚合报告"，构建由调用者负责；全量模式（不带 `--test`）才会构建。

**何时使用**：每次做定向验证时。正确姿势：
```bash
cmake --build build --target <name> -j8   # 或 ninja -C build <name>
python3 tools/run_tests.py --test <name> --filter '<Suite>.*'
```
批量改动的定向回归可用 `python3 tools/test_impact.py --diff | xargs -n1 python3 tools/run_tests.py --test`（它同样**不构建**，仍须先 build 受影响目标）。
