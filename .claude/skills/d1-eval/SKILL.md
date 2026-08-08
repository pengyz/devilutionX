---
name: d1-eval
description: DevilutionX 集成测试评估。YAML 定义 eval case，随时跑自动化集成测试。支持创建用例、单用例调试、冒烟自检（提交前门禁）、回归验证。
allowed-tools: Bash, Read, Write, Glob, Grep
argument-hint: "创建用例 | 运行用例 | 冒烟自检 | 回归测试 | 查看结果"
---

# D1-Eval — DevilutionX 集成测试评估

对 DevilutionX 游戏引擎做端到端集成验证：YAML case 选择真实断言机制（gtest 二进制 + filter），
规则评分，JSON 报告。**断言权威永远是 gtest 二进制，绝不使用 LLM 判分**。

## 架构

```
YAML case → tools/eval/backend.py → 直接执行 gtest 二进制（捕获 stdout + 解析 passed/failed/skipped）
          → assertions.py 应用 expected{} → scoring → writer 输出 eval-summary.json + 表格
```

- 引擎：`tools/eval/`（models/runner/assertions/scoring/writer/backend/sync_case_sets）
- 用例：`eval/cases/<category>/<id>.yaml`（category 必须 = 目录名）
- 套件：`eval/cases/_smoke.yaml`（快速门禁）、`_nightly.yaml`（全量，自动生成）
- 结果：`eval/results/<timestamp>/`（3 日滚动保留）

## 四个工作流

### 1. 创建用例

用户描述要验证的场景，AI 生成 YAML case。

**步骤：**
1. `python3 -m tools.eval.backend --list` 看现有 case，避免重复 id
2. 选目标测试二进制（`build/*_test`）+ filter，参照 `tools/eval/README.md` 库存表
3. 按下方模板写 YAML，category = 目录名
4. `python3 -m tools.eval.backend --check <path>` 校验 schema
5. `python3 -m tools.eval.backend --run-file <path>` 试跑一次确认通过
6. 若快速且 MPQ 无关，加入 `_smoke.yaml`；否则加入 `_nightly.yaml`（或跑 `sync_case_sets --write` 全量）

**YAML 模板：**

```yaml
id: "category-short-name"          # kebab-case，--run 匹配
name: "中文描述"
category: "mechanics"              # 必须 = 目录名
difficulty: "basic"                # basic|medium|advanced|expert
tags: ["tag1"]

setup:
  mpq_required: false              # true → 缺 MPQ 时 skip（非 fail）
  side_effect: false
  timeout: 60

run:
  backend: "gtest"
  binary: "inv_test"               # build/ 下的测试二进制
  filter: "InvTest.*"              # gtest filter
  # args: ["--gtest_repeat=3"]

expected:                          # 全部通过才 PASS（AND）
  exit_code: 0
  failed: 0
  passed_min: 1                    # 防空跑：filter 匹配 0 个测试 → FAIL
  skipped_max: 0                   # MPQ 依赖时可容忍的 skip 数
  output_contains: ["[  PASSED  ]"] # stdout 必含
  output_not_contains: []
```

### 2. 运行用例

```bash
python3 -m tools.eval.backend --run <id>       # 单用例（glob 匹配）
python3 -m tools.eval.backend --run-file <path> # 显式文件
python3 -m tools.eval.backend --dir dark-expedition  # 目录内全部
python3 -m tools.eval.backend --set smoke       # 套件
python3 -m tools.eval.backend --smoke           # = --set smoke
python3 -m tools.eval.backend --json /tmp/r.json  # JSON 报告
```

**exit code**：0=全过，1=有失败，2=无匹配，3=无效 YAML，4=基础设施错误。

### 3. 冒烟自检（提交前门禁）

```bash
python3 -m tools.eval.backend --smoke
```

**门禁语义**：evaluated>0 且全部 PASS → exit 0。全 skip → exit 1（防空跑）。
每次代码变更、提交前必跑。

### 4. 回归验证

```bash
python3 -m tools.eval.backend --set nightly --json /tmp/nightly.json
```

- 读 `eval/memory/known-gaps.md` 已知缺口
- 对比上次 `eval/results/<ts>/eval-summary.json`，找新失败
- 用 `python3 tools/test_impact.py --diff` 定位改动范围
- 根因修复后重跑，更新 `known-gaps.md`，追加 `eval-log.md`

## 评分规则

| 项 | 权重 |
|---|---|
| 基础 PASS | 10 |
| 每个 output_contains 命中 | +5 |
| 每个 output_not_contains 满足 | +3 |

分数是「多绿」辅助指标，**PASS/FAIL 是权威**。

## 纪律（移植自 osbot）

- **绝不修改失败的 case 来「通过」**——报告根因并修代码
- case 必须自包含（无跨 case 依赖）
- **skip ≠ pass**（MPQ 缺失是 skip，不是成功）
- 每个报告记录 `git_head`
- 未知 YAML 字段 = 硬错误（models.py 强校验），防止拼写错误静默通过
- 新 case 提交前跑 `sync_case_sets --check`（category=目录 + suite 成员校验）

## 参考

- 测试基础设施：`tools/run_tests.py`、`tools/test_impact.py`（见 AGENTS.md）
- 全量门禁：`tools/ci_test.sh`（构建 + 666 测试 + 漂移）
- osbot 参考：`~/workspace/osbot/.claude/skills/osbot-eval/`
