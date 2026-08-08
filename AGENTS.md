# Multi-Agent Project Guide

## Canonical Project Instructions

@CLAUDE.md

- Treat `CLAUDE.md` as the shared project guidance source of truth.
- Treat `.claude/skills/*` and `.claude/commands/*` as the canonical workflow documents.
- When a referenced workflow says `Claude`, `Claude Code`, `Skill tool`, `Bash`, `Read`, `Write`, `Glob`, or `Grep`, map it to the equivalent behavior in the current session.
- Hooks are declared in `.claude/settings.json` (shared) and reuse `.claude/hooks/*.sh` scripts.

## 测试命令速查（AI 必须使用）

本仓库是 C++ 引擎（Diablo 反编译），测试 = GoogleTest 二进制 + eval YAML case。

### 1. 全量门禁（代码变更后必跑）

```bash
python3 tools/run_tests.py --json /tmp/ci.json   # 构建所有测试 + 全量 ctest + 漂移校验
```

通过标准：JSON `ctest.passed_pct == 100` && `failed == 0` && `drift.drift_ok == true`。退出码 0。

### 2. 定向测试（改代码后快速验证）

```bash
python3 tools/test_impact.py --diff | xargs -n1 python3 tools/run_tests.py --test  # 受影响测试
python3 tools/run_tests.py --test inv_test --filter "InvTest.*"                      # 单二进制+filter
```

### 3. Eval 集成测试（行为变更）

```bash
python3 -m tools.eval.backend --smoke          # 提交前门禁（exit 0=全过）
python3 -m tools.eval.backend --run <case-id>  # 单用例
python3 -m tools.eval.backend --set nightly    # 全量回归
```

YAML case 在 `eval/cases/<category>/<id>.yaml`，规则见 `.claude/skills/d1-eval/SKILL.md`。断言权威永远是 gtest 二进制，LLM 不当判官。

### 4. 失败处理

- 读 JSON `failures` + `build/Testing/Temporary/LastTest.log`
- 单测失败：`cd build && ./<test> --gtest_filter=<case>` 复现
- 修复 → 重跑该测试 → 重跑全量
- 排查经验 → 沉淀到 `docs/knowledge/`

## 注意事项

- **不要**用 `cmake --build --target test` 全量构建（触发 benchmark 目标导致 LTO 链接失败；`DEVILUTIONX_SYSTEM_BENCHMARK=OFF` 已规避，但逐个构建更稳）
- **不要**修改 `tools/check_drift.py` 白名单除非确知上游行为
- 行尾：改动文件保持既有类型（C++ CRLF；`.md/.py/.yml/.yaml/.sh/.json` LF）；新文件匹配 `.editorconfig`
- timedemo（`Timedemo.WarriorLevel1to2`）验证存档格式与 RNG 确定性，改动存档/玩家状态后必跑
- 新增测试：注册到 `CMake/Tests.cmake` + `test/<name>_test.cpp`（漂移检查 A 强制）
- benchmark 目标（`clx_render_benchmark` 等）是性能基准，非正确性测试，默认不跑

## Windows 兼容

- hooks 仅在 Claude Code 交互会话触发；CI 不跑 hooks（行尾由 `check_drift.py` 覆盖）
- Windows 上 `.sh` hook 需 Git Bash；装 WSL 可能误解析到 WSL bash（已知 bug）——显式配 `CLAUDE_CODE_GIT_BASH_PATH` 指向 Git Bash 的 `bash.exe`
- CI 模式：`CI_MODE=1 CI_FILE=<path>` 供复用
