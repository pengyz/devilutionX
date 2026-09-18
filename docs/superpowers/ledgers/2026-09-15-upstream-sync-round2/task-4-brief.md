## Task 4: 解决 `Source/engine/render/scrollrt.cpp`

**文件：**
- 修改：`Source/engine/render/scrollrt.cpp`（1 处冲突）

**接口：**
- 依赖输入：任务 2 的冲突状态
- 对外产出：`DrawMain` 调用点与上游新签名一致（含 `drawInfoBox` 参数），并保留上游插入的 `this_sdl_thread::yield();`

- [ ] **步骤 1：确认上游的 `DrawMain` 签名与 `drawInfoBox` 变量来源**

```bash
grep -n "drawInfoBox" Source/engine/render/scrollrt.cpp | head
git show origin/master:Source/engine/render/scrollrt.cpp | grep -n "drawInfoBox" | head
```

预期：上游文件里 `drawInfoBox` 在同一函数作用域内已有定义（若没有，说明签名改动还有别的调用点要改——**停止并回报**）。

- [ ] **步骤 2：采用上游两行**

把冲突块替换为：

```cpp
	this_sdl_thread::yield();
	DrawMain(hgt, drawInfoBox, drawHealth, drawMana, drawBelt, drawControlButtons);
```

- [ ] **步骤 3：确认无残留标记、且 fork 侧该文件的其它改动仍在**

```bash
grep -nE "^(<<<<<<<|>>>>>>>|=======)$" Source/engine/render/scrollrt.cpp   # 预期：无输出
git diff origin/master -- Source/engine/render/scrollrt.cpp | head -40
```

预期：与上游的差异只包含 fork 侧既有改动（不应出现 `DrawMain(hgt, false, ...)` 这一旧形态）。

- [ ] **步骤 4：提交（与其余冲突解决合并提交，见任务 6 步骤 3）**

---

