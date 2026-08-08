# /drift-check — 事实漂移机械校验

运行宪章的事实漂移校验脚本，确认文档/代码/测试/行尾未漂移。

## 用法

```
/drift-check            # 标准基线（origin/master）
/drift-check <ref>      # 指定基线 ref
```

## 工作流

### 1. 运行校验

```bash
python3 tools/check_drift.py --base origin/master
```

5 项检查：
- **A** Tests.cmake 条目有对应源文件
- **B** 无占位测试（`EXPECT_TRUE(true)`）
- **C** 改动文件保持行尾类型（CRLF/LF）
- **C2** 新增文件匹配 `.editorconfig`
- **E** 无仅测试调用的生产函数（白名单除外）

### 2. 通过标准

5 项全 PASS，退出码 0。

### 3. 失败处理

- **A/B/E**：检查 `CMake/Tests.cmake` 注册、测试断言、生产代码调用点
- **C**：被改动文件的行尾被改变 → `git diff --name-only --diff-filter=M <base> HEAD` 定位，转回原类型
- **C2**：新增文件行尾不符 `.editorconfig` → 按后缀规则转换（`.md/.py/.yml/.yaml/.sh/.json` 为 LF，C++ 为 CRLF）

### 4. 集成

漂移校验已内置于 `tools/run_tests.py`（全量门禁自动跑）和 CI（`better-d1-ci.yml` Drift check 步骤，基线为 upstream/master）。
