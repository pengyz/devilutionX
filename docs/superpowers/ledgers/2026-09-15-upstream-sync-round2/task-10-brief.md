## Task 10: eval smoke 与漂移校验

**文件：**
- 修改：无

**接口：**
- 依赖输入：任务 9 的全量门禁
- 对外产出：`eval smoke exit 0`、drift 5 项 PASS 及与基线的口径对比结论

- [ ] **步骤 1：eval smoke 门禁**

```bash
python3 -m tools.eval.backend --smoke; echo "exit=$?"
```

预期：exit 0，36/36 PASS。

- [ ] **步骤 2：漂移校验并与基线对比**

```bash
python3 tools/check_drift.py --base origin/master 2>&1 | tee /tmp/drift-after-sync.txt
diff /tmp/drift-before-sync.txt /tmp/drift-after-sync.txt
```

预期：5 项 PASS；与基线的唯一差异应是 `merge-base:` 行从 `b3e52b1ea` 变为 `e00b7260f`。**若 PASS/FAIL 集合发生变化，逐项说明原因并追加到实施记录**（规格 §4 风险表第 5 行）。

- [ ] **步骤 3：确认没有行尾被改动**

```bash
python3 tools/check_drift.py --base origin/master 2>&1 | grep -E "PASS C |PASS C2 "
```

预期：两项都 PASS。（若 FAIL，最常见原因是把 `Source/quests.h` 写回了 CRLF——回到任务 3 步骤 5 的判据修正。）

---

