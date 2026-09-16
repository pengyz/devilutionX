---
name: 改了 txtdata 下的 TSV 之后必须重建 devilutionx.mpq，否则测试量到的是旧表
description: 测试二进制从 build/devilutionx.mpq 读取数据表；直接改 assets/txtdata/**.tsv 后即使重编译测试目标，量到的仍是旧表——必须先 ninja devilutionx_mpq。
type: gotcha
created: 2026-09-15
sources:
  - .superpowers/sdd/2026-09-15-level-rosters-phase-a/task-5-fixwave-report.md（实现者实测记录）
  - CMake/Assets.cmake（数据表打包进 devilutionx.mpq）
---

**事实**：测试（如 `level_roster_baseline_test`、`level_roster_test`）读的是 `build/devilutionx.mpq` 里的表，**不是** `assets/txtdata/**` 的源文件。因此只改 `assets/txtdata/monsters/*.tsv`（或任何 `assets/txtdata/**`）而不重建 MPQ，即使重编译测试二进制，**量到的仍是旧表**——表现为"改了数据但测试结果一点没变"，极易误判为"改动无效/无影响"。

**为什么**：数据表在构建期被 `CMake/Assets.cmake` 打包进 `devilutionx.mpq`；测试运行期通过资产层（MPQ 优先）加载。

**何时使用**：任何"改 TSV → 跑测试看数值"的循环。正确顺序：
```bash
ninja devilutionx_mpq            # 先把新表打进 MPQ
ninja <test-target>              # 再构建测试
./build/<test-target> …          # 此时才量到新表
```
连带提醒：`python3 tools/run_tests.py --test <name>` **不会**先构建（`run_tests.py:208`），所以按名字跑定向测试前也要先手动 build 目标。
