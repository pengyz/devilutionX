# /review — 独立代码复核

对分支改动做独立复核，对照设计宪章与规格验证。

## 用法

```
/review                  # 复核工作区未提交改动
/review <base>..<head>   # 复核两个 ref 之间的改动
/review HEAD~3           # 复核最近 3 个提交
```

## 工作流

### Step 1: 获取改动范围

```bash
git diff --name-only <base>..<head>
```

### Step 2: 对照宪章与规格逐项核验

- **宪章**（`docs/superpowers/specs/2026-07-27-better-d1-design-charter.md`）：
  - 分类判定（Base/Depth/Infra）是否合理
  - 红线 5-8（Base）/ 9-14（Depth）逐条核验
  - 禁令 1-7（尤其 7 行尾）
- **相关规格**：改动涉及的 `docs/superpowers/specs/` 设计文档，核对实现是否偏离

### Step 3: 运行门禁

```bash
python3 tools/test_impact.py --diff | xargs -n1 python3 tools/run_tests.py --test
python3 tools/run_tests.py --json /tmp/ci.json   # 全量
python3 -m tools.eval.backend --smoke            # eval 门禁（行为变更）
```

### Step 4: 输出

按严重度列出问题（P0 数据损坏 / P1 红线违反 / P2 质量），每条附文件:行号 + 依据。给出通过/需修复结论。
