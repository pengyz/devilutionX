---
title: 构建并发与行尾的操作规则（本会话连续踩坑后固化）
type: gotcha
tags: [build, concurrency, line-endings, drift, process]
date: 2026-09-18
---

# 构建并发与行尾：五条硬规则

## 1. `job_kill` 只是**请求**，不是终止保证 ✗

现象：请求取消后进程仍存活（本会话 `bash-367` 一直跑），于是"以为已停"又启动新门禁 ⇒
**两个 `run_tests.py` 并发争用 `build/`** ⇒ `ninja: build stopped: interrupted by user`、
二进制被半写、`smoke` 报 `PermissionError: build/xxx_test`。

**规则**：终止后必须用 `ps`/`pgrep` **验证归零**，才能启动任何构建/门禁 ✓。

## 2. 同一个 `build/` 只能有一个执行者 ✓

包括：全量门禁、单目标 `cmake --build`、**允许自行构建的子 agent**。
⇒ 派"可以构建"的复核/子 agent 时，主 agent **不得**同时跑门禁 ✓（本会话教训 ✓）。

## 3. `pgrep -f "a|b"` 的匹配陷阱 ✗

- **自匹配**：`pgrep -f "run_tests.py"` 会匹配**自己这条命令行** ⇒ 括号技巧 `[r]un_tests.py` 可解 ✓；
- **但括号技巧对"兄弟进程/自身作业包装"无效** ✗：包装行里是明文 `run_tests.py`（本会话首行打印 `残留: 2` 即自证 ✓）。
⇒ 判定"是否只有一个实例"要看**PID 关系与启动时间**（`ps -eo pid,lstart,etime,cmd` ✓），不要只数行数 ✓。

## 4. CRLF：python 文本往返之后，行尾归一化必须是**最后一步** ✗

现象：`read_text()`/`write_text()` 往返会把 CRLF 文件写成 LF；本会话在同一项内踩了 **3 次**，
每次都被漂移 C/C2 抓住（`Source/monster.cpp` CRLF→LF、`test/ai_registry_test.cpp` 应为 CRLF 却 LF）。

**规则**：①改动 CRLF 文件后**立即**单跑一次漂移 ✓；②若中间还有"反证/恢复"步骤，
行尾归一化放在**所有编辑之后** ✓；③`.md/.py/.yaml/.sh/.json` 保持 LF ✓。

## 5. 漂移：只认 `drift_ok` 标志 ✗ 禁止用 grep 计数代替

现象：用 `grep -c "^PASS"` 得到 4 就当成绿灯 ✗，而实际上 **C/C2 是 FAIL**，
真正的绿灯来自 `run_tests.py` 的 JSON：`drift.drift_ok == true && drift.passes == 6` ✓。
⇒ 结论：**报告任何"通过"必须引用 `drift_ok` 标志与其 passes 数**，不得只引用 PASS 行数 ✓。
