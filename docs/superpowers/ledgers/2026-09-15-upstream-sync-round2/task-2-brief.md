## Task 2: 执行 merge 并复现 4 处冲突

**文件：**
- 修改：由 merge 自动（100 个文件自动合并 + 4 个冲突文件）

**接口：**
- 依赖输入：任务 1 确认的 `origin/master` tip
- 对外产出：处于冲突待解决状态的索引（供任务 3-6 使用）

- [ ] **步骤 1：执行 merge（不自动提交）**

```bash
git merge --no-commit --no-ff origin/master
```

预期：退出码 1，输出「自动合并失败，修正冲突然后提交修正的结果」。

- [ ] **步骤 2：确认冲突集合与探测结果一致**

```bash
git diff --name-only --diff-filter=U | sort
```

预期恰好 4 个文件：

```
Source/controls/plrctrls.cpp
Source/engine/render/scrollrt.cpp
Source/quests.cpp
Source/quests.h
```

**若出现第 5 个冲突文件：停止并回报**（探测结果不成立，说明上游 tip 已变或 fork 侧状态变了）。

- [ ] **步骤 3：确认无冲突标记遗留在自动合并的文件里**

```bash
git grep -nE "^(<<<<<<<|>>>>>>>|=======)$" -- Source/ test/ CMake/ CMakeLists.txt | grep -v -E "quests\.h|plrctrls\.cpp|scrollrt\.cpp|quests\.cpp"
```

预期：无输出。

- [ ] **步骤 4：本任务不单独提交**（与任务 3-6 的冲突解决一起提交为一个 merge commit）

---

