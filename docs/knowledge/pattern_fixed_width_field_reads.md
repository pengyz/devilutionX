---
name: "定长 wire 字段必须按 sizeof 读取（禁止裸指针转 string_view）"
description: "存档/网络结构体里的 char[N] 字段没有 NUL 保证；隐式转 std::string_view/std::string 或 strcpy 都会走 strlen 越界读。规则=一律 std::string_view(field, sizeof(field))；验证必须把整个堆分配填满非 NUL，否则 ASan 抓不到"
type: "pattern"
created: "2026-09-15"
sources:
  - "b4dfc8d26 (上游：PlayerPack/PlayerNetPack::pName OOB，fork 移植于 4d8fd3f5b)"
  - "a82c0a827 (fork：TEar::heroname[17] OOB，上游 master 仍未修)"
  - "Source/pack.h:44,95 (pName), Source/msg.h:554 (heroname), Source/pfile.cpp:706 (strcpy)"
---

**规则**：从存档/网络结构体读取定长 `char[N]` 字段（`PlayerPack::pName`、`PlayerNetPack::pName`、`TEar::heroname[17]`、`ItemNetPack` 等）时，**一律显式定长**:

```cpp
CopyUtf8(dest, std::string_view(packed.pName, sizeof(packed.pName)), sizeof(dest));  // 对
CopyUtf8(dest, packed.pName, sizeof(dest));                                          // 错：strlen 越界
strcpy(hero_names[i], pkplr.pName);                                                  // 错：越界读 + 越界写
```

**根因**：线格式是定宽的，字段**没有终止符保证**（`PrepareEarForNetwork` 之类的发送端会终止，但恶意/损坏/跨版本 peer 不会；`DeltaImportItem` 更是直接 `memcpy` 整块 `TCmdPItem`）。而 `std::string_view`/`std::string`/`strcpy` 接 `char*` 时都走 `strlen` → 读到字段（乃至整个分配）之外。`RecreateEar(..., std::string_view)`、`SyncDropEar(..., std::string_view)` 这类形参是 `string_view` 的函数，**调用点的隐式转换就是越界点**。

**实例（同一类，两次）**：上游 `b4dfc8d26` 修了玩家名 3 处（本 fork 原样存在，已移植 `4d8fd3f5b`），但**没修耳朵名**——`TEar::heroname` 在 `msg.cpp`（抄写、`RecreateEar` ×2、`SyncDropEar`）与 `pack.cpp`（`UnPackNetItem`）共 5 处同样越界，**上游 master 至今仍是**（fork 修于 `a82c0a827`）。

**为什么：** 越界读不会崩溃（读到 NUL 就停），只会把相邻内存当名字读出来（物品名变成垃圾、潜在信息泄露），或走到未映射页时崩——**没有编译期信号、正常游玩也不触发**，只有恶意/损坏输入才暴露。C++ 的 `char*` → `string_view` 隐式转换把这种不安全读写得很自然，是这类 bug 反复出现的原因。

**何时使用：** 任何把定长字段交给 `std::string_view`/`std::string`/`CopyUtf8`/`FormatRuntime`/`strcpy`/`strcat` 的地方。审计现有代码：

```bash
grep -rn "strcpy(\|strcat(\|strlen(" Source/ --include=*.cpp --include=*.h
grep -rn "std::string_view(" Source/ --include=*.cpp        # 看实参是不是裸字段名
```

注意**发送侧通常安全**（源是已终止的 `item._iIName`，`loadsave.cpp:288-291` 有 `TerminateUtf8`）——判据是**源的终止保证**，不是"看起来像裸数组"。

**验证方法（本会话踩过的坑）**：只把字段本身填成非 NUL **不够**——若结构体内紧随字段之后就有 NUL（如 `PlayerPack` 的 `pName` 后面是 `pClass` 等零值字段），`strlen` 停在结构体内，**ASan 不会报**，测试会"通过"从而变成假测试。要让 sanitizer 真正证伪，必须把**整个分配**填满非 NUL 并放在堆上：

```cpp
auto packed = std::make_unique<ItemNetPack>();
std::memset(packed.get(), 'A', sizeof(ItemNetPack));   // 含 padding
packed->def.wIndx = static_cast<_item_indexes>(Swap16LE(IDI_EAR));
```

然后 pre-fix 构建必然报 `heap-buffer-overflow`（栈：`strlen` → `basic_string_view(char const*)` → 调用点），post-fix 干净。取证方式：`git worktree add --detach /tmp/prefix HEAD` + 把带测试的工作区文件 `cp` 进去，用 `-DCMAKE_CXX_FLAGS=-fsanitize=address` 单独构建该测试目标（`pack_test` 约 3 分钟）——**不要**在主工作区回退源码，容易污染正在进行的改动。