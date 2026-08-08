# Agent 测试工作流（AI 全自动集成测试）

本文件为在 DevilutionX 仓库工作的 AI agent 定义**测试运行约定**。任何代码变更后都必须按此流程验证。

## 测试基础设施速览

| 组件 | 说明 |
|---|---|
| 框架 | GoogleTest + GoogleMock（`test/main.cpp` 自定义 Skip 聚合） |
| 构建 | CMake/Ninja，构建目录 `build/` |
| 测试二进制 | 共享库 `libdevilutionx_so.so` + 每测试独立可执行（`build/*_test`） |
| 数据资源 | MPQ 文件位于 `~/.local/share/diasurgical/devilution/`（经 `SDL_GetPrefPath` 解析，测试自动发现） |
| 全量基线 | 666 项测试，0 失败（2 个上游既有 Skip：`VisualStoreTest.Pagination_*`，白名单内） |

## 命令约定（AI 必须使用）

### 1. 全量门禁（代码变更后必跑）

```bash
python3 tools/run_tests.py                # 构建所有测试 + 全量 ctest + 漂移校验
python3 tools/run_tests.py --json /tmp/ci.json   # 输出 JSON 报告（AI 解析）
```

等价 shell 封装：`tools/ci_test.sh [--no-build] [--json PATH]`

**通过标准**：JSON 中 `ctest.passed_pct == 100` 且 `failed == 0`，`drift.drift_ok == true`。退出码 0。

### 2. 定向测试（AI 改代码后快速验证）

```bash
# 单测试二进制
python3 tools/run_tests.py --test inv_test

# gtest filter 跨整个套件（所有二进制跑该 filter）
python3 tools/run_tests.py --filter "InvTest.*"

# 只跑受影响测试（推荐：根据改动文件自动推荐）
python3 tools/test_impact.py Source/items.cpp Source/spells.cpp
```

`test_impact.py` 输出受影响测试二进制列表（stdout 每行一个）。用法：
```bash
# 从 git diff 推荐
python3 tools/test_impact.py --diff
# 组合：改动文件 → 受影响测试
python3 tools/test_impact.py --diff | xargs -n1 python3 tools/run_tests.py --test
```

### 3. 手动运行单个测试

```bash
cd build && ./inv_test                          # 全量
cd build && ./spelldat_test --gtest_filter="SpelldatTest.*"
```

## AI 自动化循环（RECOMMENDED）

1. **改动前**：`git status` + `git diff` 确认范围
2. **改动后**：`python3 tools/test_impact.py --diff` 得到受影响测试
3. **快速验证**：跑受影响测试（`--filter` 或逐个 `--test`）
4. **全量门禁**：`python3 tools/run_tests.py --json /tmp/ci.json`
5. **失败处理**：
   - 读 JSON `failures` 列表 + `build/Testing/Temporary/LastTest.log`
   - 单测失败：`cd build && ./<test> --gtest_filter=<case>` 复现
   - 修复 → 重跑该测试 → 重跑全量
6. **提交前**：确认漂移校验 PASS（`run_tests.py` 自动包含）

## 注意事项

- **不要**用 `cmake --build --target test` 全量构建（会触发 benchmark 目标；已用 `DEVILUTIONX_SYSTEM_BENCHMARK=OFF` 规避 LTO 链接问题，但逐个构建更稳）
- **不要**修改 `tools/check_drift.py` 白名单除非确知上游行为
- 行尾约束：所有改动文件必须 CRLF（漂移校验 C 检查）；新文件写完后需转换（`sed -i 's/$/\r/'`）
- timedemo 测试（`Timedemo.WarriorLevel1to2`）验证存档格式与 RNG 确定性，改动存档/玩家状态后必跑
- 新增测试：在 `CMake/Tests.cmake` 的 `tests` 列表注册 + `test/<name>_test.cpp`，漂移校验检查 A 强制
- benchmark 目标（`clx_render_benchmark` 等）是性能基准，非正确性测试，`run_tests.py` 默认不跑
