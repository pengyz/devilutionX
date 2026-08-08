# DevilutionX 项目指南

本项目是 DevilutionX（Diablo 1 反编译引擎）的 fork，代号 **Better D1**：面向 D1 老玩家的 QoL + 深度层 mod。

## 1. 文档导航

| 目录 | 内容 |
|---|---|
| [`docs/superpowers/specs/`](docs/superpowers/specs/) | **设计规格**（7 段结构）。宪章 = 裁决基准 |
| [`docs/superpowers/plans/`](docs/superpowers/plans/) | 实施计划 |
| [`docs/superpowers/archive/`](docs/superpowers/archive/) | 废弃文档（含已核实错误，勿参考） |
| [`docs/knowledge/`](docs/knowledge/) | **AI 共享记忆**（6 类 gotcha/pattern/decision 等，见 MEMORY.md） |
| [`eval/cases/`](eval/cases/) | eval 集成测试用例（YAML） |
| [`test/`](test/) | GoogleTest 测试源码 |

**必读**：
- 🔴 **宪章**：[`docs/superpowers/specs/2026-07-27-better-d1-design-charter.md`](docs/superpowers/specs/2026-07-27-better-d1-design-charter.md)——分类判定、红线、禁令的裁决基准
- 🔴 **实施中规格**：[`docs/superpowers/specs/2026-08-08-dark-expedition-design.md`](docs/superpowers/specs/2026-08-08-dark-expedition-design.md)——深度层「黑暗远征」
- 测试工作流：见 [`AGENTS.md`](AGENTS.md)（测试命令）与下方「测试门禁」

## 2. 必须遵守的规则

### 设计宪章（裁决基准）

所有改动必须通过宪章第 2 节**分类判定**（Base/Depth/Infra）与第 3 节**红线检查**：

- **三类改动**：Base（消除摩擦，零平衡改动，常开）/ Depth（限制信息重塑压力，单一开关，默认关）/ Infra（降低未来改动成本，玩家零感知）
- **深度层红线 9-14**：限制信息非数值膨胀、有反制、取舍非跑腿、全层段定义、开关关=原版、近战/远程分别评估
- **禁令**：不写未实现内容、不写无法落地表格、不引用无出处数值、不新增玩家必记概念、红线检查不填 ✅、不用占位测试、**不改变文件行尾**

### 行为变更必产 eval

修改引擎行为/修复 bug/新功能，必须新增或更新 `eval/cases/<category>/<id>.yaml`，提交前 `python3 -m tools.eval.backend --smoke` 跑通。

### 行尾（禁令 7）

`.editorconfig`：`[*]` 默认 CRLF；LF 例外：`.md/.py/.yml/.yaml/.sh/.json/.lua/.java/.rb/.xml` 等。改动文件必须保持既有行尾类型（漂移校验 C 检查），新文件匹配 `.editorconfig`（C2）。hook `.claude/hooks/check-line-endings.sh` 会在编辑后检查。

### 知识沉淀

完成有复用价值的排查/决策后，按 `docs/knowledge/README.md` 规则写入共享记忆（4 触发时刻 + 6 类型）。

## 3. 工作流程

### 接到任务

- 先读相关代码 + 相关规格，用要点列出方案、取舍与风险，**确认方向后再动手**
- 涉及宪章判定：先按第 2 节判定树分类，再查对应红线

### 编码完成（测试门禁，按序）

```bash
# 1. 定向（改动后快速验证）
python3 tools/test_impact.py --diff | xargs -n1 python3 tools/run_tests.py --test

# 2. eval 门禁（行为变更时）
python3 -m tools.eval.backend --smoke

# 3. 全量门禁（必跑）
python3 tools/run_tests.py --json /tmp/ci.json   # 670+ 测试 + 漂移校验
```

通过标准：JSON `ctest.passed_pct == 100`、`failed == 0`、`drift.drift_ok == true`。

### 任务结束

判断是否需要沉淀：同类错误复发 / 新架构取舍 / 解决方案无法从代码看出 / 影响后续开发 → 写 `docs/knowledge/`。

## 4. 构建命令

```bash
# 构建测试（全部，排除 benchmark）
python3 tools/run_tests.py

# 全量 ctest
cd build && ctest

# 漂移校验
python3 tools/check_drift.py --base origin/master

# eval smoke 门禁
python3 -m tools.eval.backend --smoke
```

注意：`DEVILUTIONX_SYSTEM_BENCHMARK=OFF` 规避系统 libbenchmark LTO 版本不匹配（CI 已配置）。

## 5. Git 与 CI

- 工作分支：`feature/qol-upgrades`，推送 `myrepo`（`github.com/pengyz/devilutionX.git`）
- CI：`.github/workflows/better-d1-ci.yml`（构建 + 全量测试 + 漂移；`workflow_dispatch` 可选 eval-smoke）
- **不要**直推 `master`（hook `guard-git-push.sh` 会拦截）
