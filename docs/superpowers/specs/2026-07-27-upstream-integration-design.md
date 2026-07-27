# 上游集成：std::format / std::expected / C++23

**日期**：2026-07-27
**分类**：Infra
**状态**：已批准
**评判基准**：`2026-07-27-better-d1-design-charter.md`

---

## 1. 问题陈述

本分支落后 `origin/master` 41 个 commit、298 个文件。其中三个是不可回避的一次性全局迁移：

| commit | 内容 |
|---|---|
| `ac2d7fc73` | Replace libfmt with std::format |
| `63e85d9b1` | Replace tl::expected with std::expected |
| `7a75c9e66` | Update to C++23 |

具体症状（不是"感觉该同步了"）：

- **上游已完全移除 `fmt` 依赖。** `origin/master:Source/CMakeLists.txt` 中 `fmt` 出现 0 次，而本分支出现 10 次（`Source/CMakeLists.txt:197,229,258,400,504,548,562,629,651,691`）。合入后这些链接目标不存在，CMake 配置即失败。
- **上游 `CMAKE_CXX_STANDARD` 已设为 23**（`origin/master:CMakeLists.txt:344`），本分支代码需在该标准下编译通过。
- **迁移成本单调增长。** 本分支每新增一处 `fmt::` 或 `tl::expected` 调用，迁移工作量加一。腰带堆叠改背包（宪章待立项 #2）会改动 `inv.cpp`、`pack.cpp`、`loadsave.cpp`、`msg.cpp`，四者全在重叠清单内，因此该项若先行，本集成成本会上升。

不集成的代价：永久失去上游 bug 修复。这是一个需要明示的选择，而非可以靠拖延默认进入的状态。

## 2. 分类判定

按宪章第 2 节判定树逐步：

**第一步**——是否触及 6 条平衡规则？

| 规则 | 触及？ | 依据 |
|---|---|---|
| 1 TSV 数值字段 | 否 | 迁移只改 C++ 代码与构建配置 |
| 2 掉落概率或掉落表构成 | 否 | 同上 |
| 3 玩家/怪物属性、伤害、命中、抗性、生命、法力计算 | 否 | 同上 |
| 4 光照半径、视野、怪物激活或仇恨判定 | 否 | 同上 |
| 5 商店库存、价格、可购买清单 | 否 | 同上 |
| 6 战斗中可即时使用的资源量 | 否 | 同上 |

**第二步**——玩家可感知的行为是否有变化？**否。** 格式化字符串的输出内容不变，错误处理语义不变。

**结论：Infra。**

上游那 41 个 commit 中确实包含玩家可感知的改动（例如 `5141d0d18` 修复速记法术书快捷键显示、`7413e6e49` 修正法术书中 Healing 与 HealOther 的说明），但那些是上游的既有行为，不是本次集成引入的设计决策。本规格的分类针对的是集成动作本身。

## 3. 事实基础

均于 2026-07-27 核实。

### 工具链

| 事实 | 值 | 出处 |
|---|---|---|
| 本机编译器 | g++ 13.4.0（Ubuntu 13.4.0-6ubuntu1~22~ppa2） | `g++ --version` |
| `-std=c++23` 下 `std::format` | 可用 | 实测编译并运行 `std::format("value={} hex={:#x} pad={:>6}", ...)` 成功 |
| `-std=c++23` 下 `std::expected` | 可用 | 实测 `std::expected<int, std::string>` 与 `std::unexpected` 成功 |
| `<print>` | **不可用**（需 GCC 14+） | 实测 `fatal error: print: 没有那个文件或目录` |
| 上游是否使用 `<print>` | 否 | `git grep -l "#include <print>" origin/master -- Source/` 无输出 |
| clang++ | 14.0.0，过旧不可用 | `clang++ --version` |
| 上游要求的 C++ 标准 | 23 | `origin/master:CMakeLists.txt:344` `set(CMAKE_CXX_STANDARD 23)` |
| 上游 CMake 中的 fmt 引用 | 0 处 | `git show origin/master:Source/CMakeLists.txt \| grep fmt` 无输出 |

### 规模

| 事实 | 值 |
|---|---|
| merge-base | `95acc82d89e6ce34cf54cda429d794d52eba3459` |
| 落后 commit 数 | 41 |
| 上游改动文件数 | 298 |
| 本分支领先 commit 数 | 157 |
| 重叠文件数（双方都改过） | 36 |
| 本分支新增文件数（`Source/` + `test/`） | 18 |

### 需自行迁移的调用点

表面用量为 `fmt::` 486 处 / `tl::expected` 425 处，但绝大部分是与上游共享的代码，merge 会带来上游已迁移的版本。需自行处理的仅为：

| 来源 | `fmt::` | `tl::expected` |
|---|---|---|
| 新增文件：`Source/spell_tooltip.cpp` | 21 | 1 |
| 新增文件：`Source/spell_tooltip.h` | 0 | 1 |
| 新增文件：`Source/panels/level_info.cpp` | 1 | 0 |
| 重叠文件新增行：`Source/levels/trigs.cpp` | 15 | 0 |
| 重叠文件新增行：`Source/items.cpp` | 5 | 0 |
| 重叠文件新增行：`Source/objects.cpp` | 2 | 0 |
| 重叠文件新增行：`Source/control/control_infobox.cpp` | 1 | 0 |
| 重叠文件新增行：`Source/panels/spell_book.cpp` | 1 | 0 |
| 重叠文件新增行：`Source/quests.cpp` | 1 | 0 |
| 重叠文件新增行：`Source/tables/spelldat.cpp` | 1 | 0 |
| 重叠文件新增行：`Source/tables/misdat.h` | 0 | 2 |
| **合计** | **48** | **4** |

### 上游给定的迁移规则

出自 `ac2d7fc73` 的 commit message，无需自行推导：

| 旧写法 | 新写法 |
|---|---|
| `fmt::format(字面量, ...)` | `std::format(字面量, ...)` |
| `fmt::format(fmt::runtime(f), ...)` | `FormatRuntime(f, ...)` |
| `fmt::format_int` | `std::to_chars` |
| `fmt::join` | 手写拼接 |
| `fmt::dynamic_format_arg_store` | 基于 `std::variant` 的逐字段格式化器，返回 `std::expected` |

`FormatRuntime` 定义于上游新增的 `Source/utils/format.hpp`，是 `std::vformat` 的封装，并满足 P2905 对 `std::make_format_args` 的 lvalue 要求。本分支的 202 处 `fmt::format(fmt::runtime(...))` 中，属于我们新增行的部分按此规则逐一替换。

### 本分支新文件的 CMake 注册

`Source/spell_tooltip.cpp`、`Source/panels/level_info.cpp`、`Source/lua/modules/world.cpp`、`Source/lua/modules/spells.cpp` 各在 `Source/CMakeLists.txt` 中注册 1 处，合入后需确认其所属 target 的链接列表中不再引用 `fmt::fmt`。

## 4. 方案

### 集成方式：merge，非 rebase

保留 157 个 commit 的历史，产生一个 merge commit，冲突一次性解决。

不选 rebase 的理由：157 个 commit 逐个重放，每个都可能撞上同一批迁移冲突。其中 14 个是 2026-07-27 的清理提交，重放时会反复触发相同的 `fmt::` 冲突而无额外收益。

### 迁移时机：合入先行

直接 merge，在解冲突过程中完成本分支 48 处的迁移，形成单一编译-修复循环。

「迁移先行」不可行：当前基线没有 `Source/utils/format.hpp`，也没有 C++23，无处可迁。

### 分批验证门禁

一次合入 298 个文件后编译会产生大量错误，需要中间进度信号。按依赖顺序设四道门禁：

| # | 门禁 | 判据 |
|---|---|---|
| 1 | CMake 配置通过 | `cmake -S. -Bbuild` 成功，无 `fmt::fmt` 找不到目标的错误 |
| 2 | `libdevilutionx` 编译通过 | 库目标全部编译，无 `fmt::` 或 `tl::expected` 相关错误 |
| 3 | 主程序链接通过 | `devilutionx` 可执行文件生成 |
| 4 | 测试目标编译通过 | 50 个 `tests` 目标全部生成二进制 |
| 5 | 全量测试通过 | 629 项零失败 |

### 行尾回退的实测

`Source/diablo.cpp`、`Source/inv.cpp`、`Source/qol/stash.cpp` 三者既在重叠清单内，又是 `c843f4240` 行尾回退处理过的文件。本次 merge 是对该回退声称收益的实测：冲突应以行为单位出现，而非整文件冲突。

若仍为整文件冲突，说明行尾回退未达预期效果，须如实记录，不得掩饰。

## 5. 红线检查

### 通用红线

| # | 判定 | 依据 |
|---|---|---|
| 1 | 分类已判定 | Infra，第 2 节给出判定树逐步结果 |
| 2 | 问题陈述指向具体症状 | 是：上游 CMake 的 fmt 引用为 0 而本分支为 10 处（行号已列），`CMAKE_CXX_STANDARD 23` 位于 `CMakeLists.txt:344`。非"感觉该同步了" |
| 3 | 数值标注出处 | 是，见第 3 节，每项均有文件路径或命令依据 |
| 4 | 「已实施」需有非测试调用者且验收标准全部通过 | 本规格状态为「已批准」，未标「已实施」。达成第 4 节五道门禁后方可改标 |

### 基础设施红线

| # | 判定 | 依据 |
|---|---|---|
| I1 | 玩家可感知行为是否有变化？**否** | 格式化输出内容与错误处理语义均不变。上游 41 个 commit 中含玩家可感知的既有改动，但那不是本集成引入的设计决策 |
| I2 | 是否有消费者？**有，类型 (a)** | 消费者是仓库内的全部生产代码：合入后 `Source/` 下所有使用格式化与错误返回的代码都依赖新 API。不是为假想的未来消费者铺路 |
| I3 | 它让哪个未来改动变便宜？**已指名** | 宪章待立项 #2 腰带堆叠改背包堆叠，将改动 `inv.cpp` / `pack.cpp` / `loadsave.cpp` / `msg.cpp`，四者全在 36 个重叠文件内。先集成可使该项直接在新 API 上编写，避免二次迁移 |
| I4 | 是否引入新的运行时失败模式？**是，已定义并将测试** | `FormatRuntime` 在格式串非法时抛 `std::format_error`，禁用异常时终止程序；`fmt::format(fmt::runtime(...))` 的行为与之等价。运行时格式串全部来自翻译文件，由现有 629 项测试覆盖其调用路径 |

## 6. 验收标准

| # | 验收项 | 命令或方法 | 通过标准 |
|---|---|---|---|
| 1 | merge 完成且无残留冲突标记 | `git grep -nE "^(<<<<<<<\|>>>>>>>\|=======)$" -- Source/ test/ CMake/ CMakeLists.txt` | 无输出 |
| 2 | fmt 依赖清零 | `grep -rn "fmt::" Source/ test/` | 无输出 |
| 3 | `tl::expected` 清零 | `grep -rn "tl::expected" Source/ test/` | 无输出 |
| 4 | CMake 中无 fmt 目标 | `grep -n "fmt" Source/CMakeLists.txt CMakeLists.txt` | 无输出 |
| 5 | C++ 标准为 23 | `grep -n "CMAKE_CXX_STANDARD" CMakeLists.txt` | `set(CMAKE_CXX_STANDARD 23)` |
| 6 | 构建通过 | `ninja -C build -k 0` | 除 6 个 benchmark 目标外无 `FAILED`（benchmark 失败为环境问题，见宪章待立项 #5） |
| 7 | 全量测试通过 | `cd build && ctest` | 629 项零失败 |
| 8 | 行尾未被破坏 | 对 `c843f4240` 处理过的 6 个文件检查 CRLF 行数等于总行数 | 全部相等 |
| 9 | 行尾回退收益的实测结论 | 记录 `diablo.cpp` / `inv.cpp` / `qol/stash.cpp` 的冲突是行级还是整文件级 | 已如实记录，无论结论正负 |

第 7 项的 629 是当前基线。上游 41 个 commit 可能新增或删除测试，届时以「零失败」为准，并记录数量变化及原因。

## 7. 状态

**已批准。** 未实施。

实施计划见 `plans/2026-07-27-upstream-integration.md`。
