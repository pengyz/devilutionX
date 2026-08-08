# /eval-check — Eval 集成测试门禁

运行 D1-eval 集成测试，验证行为变更符合预期。

## 用法

```
/eval-check                    # smoke 门禁（快速，exit 0=全过）
/eval-check nightly            # 全量回归
/eval-check <case-id>          # 单用例
```

## 工作流

### 1. Smoke 门禁（提交前必跑）

```bash
python3 -m tools.eval.backend --smoke --json /tmp/eval.json
```

通过标准：`evaluated > 0` 且全部 PASS（exit 0）。全 skip = 失败（防空跑）。

### 2. 单用例调试

```bash
python3 -m tools.eval.backend --run <case-id>
python3 -m tools.eval.backend --run-file eval/cases/<cat>/<id>.yaml
```

### 3. 全量回归

```bash
python3 -m tools.eval.backend --set nightly --json /tmp/nightly.json
```

对比上次 `eval/results/<ts>/eval-summary.json`，定位新失败 → 用 `test_impact.py --diff` 缩小范围 → 根因修复 → 重跑。

### 4. 新增用例

```bash
python3 -m tools.eval.backend --list              # 查现有 id 避免重复
python3 -m tools.eval.backend --check <path.yaml> # schema 校验
python3 -m tools.eval.sync_case_sets --check      # 目录/套件一致性
```

## 纪律

- **绝不修改失败 case 来「通过」**——报告根因修代码
- 断言权威永远是 gtest 二进制，不用 LLM 判分
- case 自包含、skip ≠ pass
