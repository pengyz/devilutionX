## Task 3: 解决 `Source/quests.h`（整文件冲突 + fork 增量迁移）

**文件：**
- 修改：`Source/quests.h`（改为上游版本）、`Source/tables/questdat.hpp`（补 fork 增量）
- 测试：`Source/quests.cpp:286-287`（`questData.scriptName` 的消费者，编译即验证）

**接口：**
- 依赖输入：任务 2 的冲突状态
- 对外产出：`QuestData`（现在声明于 `Source/tables/questdat.hpp`）含成员 `std::string scriptName;`，供 `Source/quests.cpp` 的 `lua::OnQuestCheck(questData.scriptName, &quest)` 使用

- [ ] **步骤 1：取上游版本覆盖冲突文件**

```bash
git checkout --theirs -- Source/quests.h
git add Source/quests.h
```

- [ ] **步骤 2：确认取到的是上游版本且行尾为 LF**

```bash
wc -l Source/quests.h                      # 预期：32
grep -c $'\r' Source/quests.h              # 预期：0（LF）
grep -n "questdat.hpp" Source/quests.h     # 预期：命中 include 行
```

- [ ] **步骤 3：确认上游 `QuestData` 缺的正是 fork 的那一行**

```bash
sed -n '/struct QuestData/,/};/p' Source/tables/questdat.hpp
grep -n "scriptName" Source/tables/questdat.hpp || echo "EXPECTED-ABSENT"
```

预期：结构体末尾是 `std::string _qlstr;`，且 `scriptName` 不存在。

- [ ] **步骤 4：把 fork 增量落到它的新家**

在 `Source/tables/questdat.hpp` 的 `struct QuestData` 中，`std::string _qlstr;` 之后加一行：

```cpp
	std::string scriptName;
```

- [ ] **步骤 5：确认落地正确且行尾未变**

```bash
sed -n '/struct QuestData/,/};/p' Source/tables/questdat.hpp   # 预期：末行为 std::string scriptName;
python3 -c "b=open('Source/tables/questdat.hpp','rb').read(); print('CRLF', b.count(b'\r\n'), 'LF', b.count(b'\n'))"
```

预期：`CRLF 141 LF 141`（**上游该文件本身就是 CRLF**——本轮唯一的 LF 文件是 `Source/quests.h` 与上游新增的 7 个文件，`questdat.hpp` 不在其中）。判据是「与该文件在上游的行尾一致」，不是固定值；本例的一致值就是 CRLF。

- [ ] **步骤 6：确认消费者的引用链完整**

```bash
grep -n "scriptName" Source/quests.cpp Source/quests.h Source/tables/questdat.hpp
```

预期：`questdat.hpp` 定义、`quests.cpp:286-287` 使用；`quests.h` 不再出现（内容已迁走）。

- [ ] **步骤 7：提交（与其余冲突解决合并提交，见任务 6 步骤 3）**

---

