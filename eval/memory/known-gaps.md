# D1-Eval 已知缺口

本文件记录 eval case 的已知失败及其根因。规则：绝不修改失败的 case 来"通过"；报告根因并修代码。缺口关闭时在此记录验证。

## 2026-09-15 timedemo 夹具过期（quarantine）

| case | 状态 | 根因 | 修复方向 |
|---|---|---|---|
| flow-timedemo-warrior | **FAIL（已知缺口，已 quarantine）** | `f474a64c0`（宪章决策 31 删除 Dark Expedition 开关）使 `DarkExpeditionDropOk` 掉落排除无条件生效 → `GetItemIndexForDroppableItem` 的 `cumulativeWeight` 改变 → 同一次 RNG 抽取得到不同物品 → 回放 RNG 流分叉 → 上游录制的 demo 在 `interfac.cpp:363` `isOnActiveLevel` 断言崩溃（`Subprocess aborted`）。**不是 case 写错，是夹具过期** | 重录 `test/fixtures/timedemo/WarriorLevel1to2/`（真人 `--record 0 --create-reference`），随后删除 `test/timedemo_test.cpp` 的 `GTEST_SKIP()` 并清掉本条。详见 `docs/knowledge/gotcha_timedemo_isOnActiveLevel_failure.md` |

对应 gtest 用例已由 `GTEST_SKIP` 临时隔离（ctest 退出码 0），因此**该 case 在 nightly 会失败、smoke 不受影响**（timedemo 不在 smoke 集）。
本文件条目与 case 本身都不做「改到能过」的处理——按本文件首行规则执行。

## 2026-08-09 Windows 本机首次闭环

| case | 状态 | 根因 | 修复方向 |
|---|---|---|---|
| render-palette | FAIL（防空跑守卫拦截） | palette_blending_test 在 Windows/MSVC 下 gtest 注册 0 测试（"does NOT link in any test case"）。已尝试：TEST/MATCHER 移出匿名 ns、SDL_MAIN_HANDLED、_DVL_EXPORTING 排除、test_main 替换 gtest_main（后者 LNK1169 重复符号回滚）——均未解。obj 符号正确（External 类 + MakeAndRegisterTestInfo 引用），exe 运行 static init 未注册。Linux CI 正常（GCC 保留匿名 ns 注册器）。疑似 MSVC /OPT:REF + gtest 静态注册器 COMDAT 丢弃 + SDL2maind/gmock 交互 | 用调试器（cdb/gflags）断点 MakeAndRegisterTestInfo 跟踪 static init；或检查 /OPT:REF,ICF 影响 |
| render-text_render_integration | FAIL | SEH 异常 0xc0000005（访问违例）在测试体抛出。SDL 渲染相关（GoldenTests/TextRenderIntegrationTest）。SDL_MAIN_HANDLED 已修复注册（列出测试），但渲染执行崩溃。Windows 特有（Linux CI 正常） | 检查 SDL 视频/渲染子系统初始化（headless 环境）；用调试器定位崩溃栈 |
| flow-timedemo-warrior | 曾 SKIP（MPQ）→ 已 PASS → **2026-09-15 转 FAIL（见上节）** | eval 的 mpq_present() 只检测 ~/.local/share/diasurgical/devilution/{spawn.mpq,DIABDAT.MPQ,diabdat.mpq}（home 目录），不检测构建目录 build/spawn.mpq（CI 下载位置）。把 spawn.mpq 复制到 home 目录后 timedemo 通过（存档/RNG 确定性验证 OK）；该 P1 已由 d62d6ecea 修复（mpq_present 检测 build dir） | 改进 mpq_present() 也检测 --build-dir 下的 spawn.mpq（P1，已修） |

## 建议（未实施）

- run_tests.py TEST_TARGETS 手工列表与 CMake/Tests.cmake 漂移——已修复缺 2 个（dark_expedition_drop_test、consume_scroll_test），建议加自动漂移检测
- test_impact.py IMPACT_MAP 手工映射表同样会漂移
