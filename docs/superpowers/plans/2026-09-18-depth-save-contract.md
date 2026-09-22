# 深度层第 0 章（存档契约）实施计划

> **面向执行者：** 本计划**内联执行**（沿用本会话约定：不使用子 agent），可用 `superpower-executing-plans` 批量推进并在检查点复核。步骤用复选框（`- [ ]`）跟踪。

**目标：** 让单机也满足"**存档＝提交 / 时间线唯一 / 死亡留痕 / 提交失败不得提供死前 Load**"：补上**单机的死亡提交**、给提交加**原子性与兜底**，并以**可失败**的 gtest 逐条验收。

**架构：** 三块：①**提交点**——单机死亡后走**单机可用的完整保存**（`SaveGame()`，含 `writeGameData=true`，因为死亡态与掉落物存在 game data 里）；②**意图先行（journal）**——先落一个"死亡意图"侧车文件并 fsync，保存成功后再删除；③**启动闸门**——启动时若发现未完成的死亡意图 ⇒ **禁用死前存档的加载**（复用 `gbValidSaveFile` 这道既有闸门）。

**技术栈：** C++23 / CMake+Ninja / GoogleTest / 现有存档栈（`pfile.cpp`、`loadsave.cpp`、`mpq_writer.cpp`）

**规格：** `docs/superpowers/specs/2026-09-18-depth-first-principles-design.md`（**已批准 v2**；只做第 0 章；§4.3 各自单独立项）

## 全局约束

- **不使用任何子 agent**：内联执行，遇阻停下问
- 行尾：C++/TSV **CRLF**；`.md/.py/.yaml/.sh/.json` **LF**；新增文件匹配 `.editorconfig`（漂移 C2）
- **顺序固定**：改 → 提交 → 跑漂移 → push（漂移 C2 只检查 HEAD 里的文件，见 `docs/knowledge/gotcha_drift_check_c2_whole_file_crlf.md`）
- 新测试必须注册到 `CMake/Tests.cmake`（漂移 A/F 强制）
- **规格约束（不得绕过）**：①只在**本地、仅单机**触发（远端不得替其写档 ✓）②提交必须 `writeGameData=true` ③**提交失败不得留下可用的死前存档** ④不提供任何"回滚到死前"的入口（**无开关** ✓）
- 每条新测试必须**可失败**：给出"改坏→必红→恢复→绿"的实跑，证据**内联**记录
- 门禁：`python3 tools/run_tests.py --json /tmp/ci.json`（`failed==0 && passed_pct==100 && drift_ok==true`）+ `python3 -m tools.eval.backend --smoke`；push 后跟踪 CI 到终态
- 台账（`docs/superpowers/ledgers/**`）随提交进 git

---

## 文件结构

| 文件 | 职责 |
|---|---|
| `Source/save_commit.h` / `.cpp` | **新建**：死亡提交的意图（journal）与原子替换封装；对上层暴露 `BeginDeathCommit()/CompleteDeathCommit()/HasPendingDeathCommit()` |
| `Source/player.cpp` | 修改：单机死亡路径（`SyncPlrKill`）触发提交 |
| `Source/diablo.cpp` | 修改：启动时读取闸门（`gbValidSaveFile` 附近）拒绝死前存档 |
| `Source/utils/file_util.cpp` | 可能修改：暴露/复用原子重命名与 fsync 辅助（**实现前先读该文件确认可用函数名**） |
| `CMake/Tests.cmake` | 修改：注册新测试目标 |
| `test/save_commit_test.cpp` | **新建**：验收 1/2/3/4/5 的 gtest |
| `docs/superpowers/protocols/2026-09-19-death-penalty-experiment.md` | **新建**：规格 §6-7 的**预注册产品实验**（N/人群/阈值先写死；不进验收、不进 CI） |

---

## 任务 1：死亡提交的意图（journal）

**文件：** 新建 `Source/save_commit.h`、`Source/save_commit.cpp`
**接口：** 产出 `bool BeginDeathCommit()`、`bool CompleteDeathCommit()`、`bool HasPendingDeathCommit()`（**签名以此为准，后续任务直接使用**）

- [ ] **步骤 1：先读既有文件工具（确认可用函数名与 fsync 手段）**

运行：`grep -n "RenameFile\|fsync\|FlushFileBuffers\|OpenFile" Source/utils/file_util.cpp Source/utils/file_util.h | head -20`
把确认到的真实函数名记进本任务注释（**不要凭空取名**）。

**实测结论（2026-09-18 ✓）**：`file_util.h` 可用者为 `FileExists` ✓ / `FileExistsAndIsWriteable` ✓ / `RenameFile(from,to)` ✓（内部 `std::filesystem::rename` ✓）/ `RemoveFile` ✓ / `OpenFile` ✓；**不存在** `fsync`/`FlushFileBuffers`/`_commit` 封装 ✗（`Source/`+`3rdParty/` 全仓 0 命中 ✓），`MpqWriter`（`Source/mpq/mpq_writer.hpp` ✓）只有 `WriteFile`/`RenameFile`/`HasFile` 等 ✓、无落盘屏障 ✗。⇒ 本计划的"复用既有持久化封装"假设**不成立** ✗，需作者在 (i)/(ii) 间裁决 ✓。

- [ ] **步骤 2：写接口与最小实现（意图文件 + 原子替换）**

```cpp
// Source/save_commit.h
#pragma once
#include <string_view>

namespace devilution {

/** Persist the intent that a death must be committed before the old save may be loaded. */
bool BeginDeathCommit(std::string_view reason);

/** Remove the intent after the save has been durably replaced. */
bool CompleteDeathCommit();

/** True when a death intent exists without a completed commit (checked at start-up). */
bool HasPendingDeathCommit();

/** Absolute path of the intent file (for tests and diagnostics). */
std::string DeathCommitIntentPath();

} // namespace devilution
```

```cpp
// Source/save_commit.cpp（骨架：写意图 → fsync → 返回；完成后删除）
#include "save_commit.h"
#include "utils/file_util.h"
#include "utils/paths.h"
#include "utils/log.hpp"
#include <fstream>

namespace devilution {
namespace {
constexpr std::string_view IntentFileName = "death_commit.intent";
}

std::string DeathCommitIntentPath() { return paths::PrefPath() + std::string(IntentFileName); }

bool BeginDeathCommit(std::string_view reason)
{
	std::ofstream out(DeathCommitIntentPath(), std::ios::binary | std::ios::trunc);
	if (!out)
		return false;
	out << reason;
	out.flush();
	// 关键：必须落到磁盘，否则崩溃后意图丢失、旧档仍可加载（规格 §4.1⑤）
	// ⚠ 更正（2026-09-18 步骤 1 实测 ✓）：本仓**没有**任何 fsync / FlushFileBuffers / _commit
	// 封装（全仓 0 命中 ✗；file_util 只有 FileExists / RenameFile / RemoveFile / OpenFile ✓），
	// 存档栈（MpqWriter / SDL_RWops）也只有**重命名式原子替换** ✓、无落盘屏障 ✗。
	// 故这里**不能**调用 FlushFileToDisk（该名字不存在 ✗）。两条路（待作者裁决 ✓）：
	//   (i)  新增可移植的 FlushFileToDisk（POSIX open+fsync+目录 fsync / Windows FlushFileBuffers）
	//        ⇒ 忠实规格 §4.1⑤，但引入平台代码面；
	//   (ii) 先只写意图文件（不保证崩溃级持久化）⇒ 仍防"保存失败后旧档可加载"，掉电/崩溃不保证。
	// 在本任务完成前，**不得**假装已持久化 ✗。
}

bool CompleteDeathCommit()
{
	return RemoveFile(DeathCommitIntentPath()); // 同样使用任务 1 步骤 1 确认到的真实函数
}

bool HasPendingDeathCommit() { return FileExists(DeathCommitIntentPath()); }
} // namespace devilution
```

- [ ] **步骤 3：编译**

运行：`cmake --build build --target libdevilutionx -j20`
预期：编译通过（若 `FlushFileToDisk`/`RemoveFile`/`FileExists` 名字与仓库不符，**按步骤 1 的真实名字改**，不要新建重复工具）。

- [ ] **步骤 4：提交**

```bash
git add Source/save_commit.h Source/save_commit.cpp
git commit -m "feat(save): add the durable death-commit intent"
```

---

> **执行前更正（2026-09-18 实测 ✓，务必先读）**
>
> 1. **依赖不是线性** ✗：任务 2 正文用到 `DisableLoadingOldSaveForThisSession()`（**任务 3** 才提供 ✗）与
>    **`SaveGameReporting()`**（**步骤 3b** ✗）⇒ 必须先做 **3b（让存档能报告成功）** 与 **任务 3**，
>    再做任务 2；否则无法 **fail-closed**（不知道保存是否成功就删意图 ✗ = 违反规格 §4.1⑤ ✗）。
> 2. **漂移 E 会挡住单独落地任务 1** ✗✓：`check_drift.py` 的 E 检查扫**工作树**，新生产函数
>    `BeginDeathCommit` 若只有测试调用 ⇒ `FAIL E "has test callers but no production caller"` ✓
>    （实测 ✓，提交被拦 ✓）。⇒ **任务 1 必须与任务 2 的生产调用点一起落地** ✓（同一次提交 ✓），
>    不可先提交"只有接口 + 测试"的版本 ✗。CLAUDE.md 亦禁止"写未实现内容" ✓。
> 3. **任务 1 的实现已完成并已验证，暂存于 `/tmp/task1_parked/`** ✓（`save_commit.h` / `save_commit.cpp` /
>    `save_commit_test.cpp` / `commit-intent.yaml` ✓）：编译通过 ✓、测试 **2 passed** ✓、
>    **两处可失败性反证已实测为红并恢复** ✓（去掉 `RemoveFile` ⇒ "must remove the intent" ✓；
>    `HasPendingDeathCommit` 恒假 ⇒ 断言红 ✓）。落回仓库时需**同时**改 `Source/CMakeLists.txt`
>    （**显式列表非 glob** ✗）✓ + `CMake/Tests.cmake` ✓ + `tools/run_tests.py::TEST_TARGETS` ✓。
> 4. `SyncPlrKill` 是**唯一死亡漏斗** ✓（`Source/player.cpp`，6+ 调用点 ✓）；多人标志真实名＝
>    **`gbIsMultiplayer`** ✓（`Source/game_mode.hpp:41` ✓）；`LogError` **首参必须是 `LogCategory`** ✓。

## 任务 2：单机死亡触发提交

**文件：** 修改 `Source/player.cpp`（`SyncPlrKill`，`:2845`）、`Source/loadsave.h`（无需新增，`SaveGame()` 已声明）
**接口：** 依赖任务 1 的 `BeginDeathCommit()` / `CompleteDeathCommit()`

- [ ] **步骤 1：写失败的测试（先证明它现在不提交）**

新建 `test/save_commit_test.cpp`：
```cpp
#include <gtest/gtest.h>
#include "player.h"
#include "save_commit.h"

namespace devilution {
TEST(SaveCommitTest, SinglePlayerDeathLeavesACommittedSave)
{
	// 该用例由任务 4 的夹具驱动真实死亡路径后调用；此处先断言"意图接口可用"
	EXPECT_FALSE(HasPendingDeathCommit()); // 干净启动时不应有遗留意图
}
} // namespace devilution
```
并在 `CMake/Tests.cmake` 的 `set(tests` 列表中加入 `save_commit_test`（漂移 A/F 要求）。

- [ ] **步骤 2：运行并确认失败/通过基线**

运行：`python3 tools/run_tests.py --test save_commit_test`
预期：构建通过；用例 PASS（此时它只断言"干净启动"），**任务 4 会把它扩成真断言**。

- [ ] **步骤 3：最小实现（在单机死亡路径触发）**

在 `Source/player.cpp` 的 `SyncPlrKill` 中（**仅本地、仅单机**）：
```cpp
void SyncPlrKill(Player &player, DeathReason deathReason)
{
	SetPlayerHitPoints(player, 0);
	StartPlayerKill(player, deathReason);

	// 规格 §4.1①：单机的死亡必须**提交**（否则读档/退出重开可回到死前）。
	// 只在本地玩家、仅单机触发；远端玩家由各自的客户端提交（避免重复写档）。
	// 规格 §4.1①⑤：单机死亡必须**提交**；提交必须 **fail-closed**——
	// 只有在保存**确实成功**后才删除意图；任何一步失败都要 ①保留意图
	// ②在本会话内禁用"读取旧档"入口 ③响亮记录。
	// 触发范围：仅本地玩家、仅单机（远端由各自客户端提交，避免重复写档）。
	if (gbIsMultiplayer || &player != MyPlayer)
		return;
	if (!BeginDeathCommit("death")) {
		DisableLoadingOldSaveForThisSession(); // 任务 3 提供（内存态禁用）
		LogError("Death commit could not be journalled; the old save must not be loaded");
		return;
	}
	if (!SaveGameReporting()) {               // 步骤 3b：让存档**返回**成功与否
		DisableLoadingOldSaveForThisSession();
		LogError("Death commit save failed; keeping the intent so the old save stays unusable");
		return;                               // 意图**保留** ⇒ 启动闸门会拒绝旧档
	}
	CompleteDeathCommit();                    // 仅保存成功后撤销意图
}
```
（`SaveGame()` 声明于 `Source/loadsave.h:42`，定义 `Source/loadsave.cpp:2948`：`gbValidSaveFile = true; pfile_write_hero(true); sfile_write_stash();` ✓）

- [ ] **步骤 3b：让存档能报告成功（`SaveGameReporting()`）**

现状**丢弃写结果**：`Source/loadsave.cpp:249-255 SaveHelper::~SaveHelper` 调用 `m_mpqWriter.WriteFile(...)` 且**不使用返回值** ✓；`SaveGame()` 亦无返回值 ✓。因此提交无法判断成败 ✗：
1. 让 `SaveHelper` **记录**写结果（成员或 out 参数）；
2. 新增 `bool SaveGameReporting()`，返回该结果；`SaveGame()` 保留原签名，内部委托（既有调用方不变 ✓，如 `Source/gamemenu.cpp:353`）；
3. 失败注入测试：让写入失败 ⇒ `SaveGameReporting()` 必须返回 **false** ⇒ 意图保留 ⇒ 启动闸门继续拒绝旧档 ✓。

- [ ] **步骤 4：重建并运行**

运行：`cmake --build build --target save_commit_test -j20 && python3 tools/run_tests.py --test save_commit_test`
预期：PASS。

- [ ] **步骤 5：提交**

```bash
git add Source/player.cpp CMake/Tests.cmake test/save_commit_test.cpp
git commit -m "feat(save): commit a single-player death to the save"
```

---

## 任务 3：启动闸门（提交失败不得提供死前 Load）

**文件：** 修改 `Source/diablo.cpp`（启动校验 `gbValidSaveFile` 附近，`:2725` / `:2734`）
**接口：** 依赖任务 1 的 `HasPendingDeathCommit()`

- [ ] **步骤 1：写失败的测试**

在 `test/save_commit_test.cpp` 追加：
```cpp
TEST(SaveCommitTest, PendingDeathCommitDisablesLoadingTheOldSave)
{
	ASSERT_TRUE(BeginDeathCommit("injected"));
	// 模拟"意图已写、存档未替换"的中断：启动闸门必须判定该存档不可加载
	EXPECT_FALSE(IsSaveLoadableAfterPendingCommit());
	ASSERT_TRUE(CompleteDeathCommit());
	EXPECT_TRUE(IsSaveLoadableAfterPendingCommit());
}
```
（`IsSaveLoadableAfterPendingCommit()` 是**任务 3 步骤 2** 产出的查询函数——先按此签名实现，测试与实现同任务内闭环。）

- [ ] **步骤 2：运行并确认失败，然后最小实现**

在 `Source/diablo.cpp` 的启动校验处加入闸门（**读档前**）：
```cpp
	// 规格 §4.1⑤：存在未完成的死亡意图 ⇒ 不得加载旧档（否则玩家可借崩溃/写失败规避不可逆）
	if (gbValidSaveFile && HasPendingDeathCommit()) {
		gbValidSaveFile = false;   // 与"存档不可用"同一语义，复用既有 UI/菜单路径（gamemenu.cpp:94）
		LogError("Pending death commit found; refusing to load the old save");
	}
```
并提供测试查询（同一任务内）：
```cpp
bool IsSaveLoadableAfterPendingCommit() { return !HasPendingDeathCommit(); }
```

- [ ] **步骤 3：重建 + 运行 + 反证**

运行：`cmake --build build --target save_commit_test -j20 && python3 tools/run_tests.py --test save_commit_test`
**反证**：临时注释掉闸门 ⇒ 用例**必须红** ⇒ 恢复 ⇒ 绿。**两次输出内联记录**。

- [ ] **步骤 4：提交**

```bash
git add Source/diablo.cpp test/save_commit_test.cpp
git commit -m "feat(save): refuse to load a save with a pending death commit"
```

---

## 任务 4：验收 1/2/4（死因矩阵 · 重载状态 · 掉落持久）

**文件：** `test/save_commit_test.cpp`（扩展）；需要能驱动"死亡 → 保存 → 重新加载"的夹具（**沿用 `test/level_roster_baseline_test.cpp` 的夹具模式**：加载归档、建关、初始化怪物）

- [ ] **步骤 1：死因矩阵（规格 §6-1）**

逐条断言（每条都可失败，改坏对应分支即红）：
```cpp
// 怪/陷阱致死 ⇒ 装备离开 InvBody 且落到死亡地点周围、金币被扣（DropHalfPlayersGold）
// 玩家杀（DeathReason::Player）⇒ 只掉"耳朵"；已存在耳朵时不再掉
// 镇上死亡 ⇒ 仍掉金/物
// 多人 L16/竞技场例外 ⇒ 不掉（单机无此例外）
```

- [ ] **步骤 2：死亡提交可观测（规格 §6-2）**

```cpp
// 死亡 → 重新加载存档 ⇒ 断言：_pmode == PM_DEATH、HP == 0、装备在**死亡地点**、
// _pLvlVisited 与关卡数据已持久（**不断言 mtime** ✗）
```

- [ ] **步骤 3：掉落持久（规格 §6-4）**

```cpp
// 死亡 → 离开该层 → 重进 ⇒ 掉落物仍在（依赖层持久分支）
```

- [ ] **步骤 4：运行 + 每条反证**

运行：`python3 tools/run_tests.py --test save_commit_test`
**反证**：分别改坏 ①`DeadItem`/`dropItems` 分支 ②跳过死亡提交 ③`_pLvlVisited`/持久物品序列化 ⇒ 各自必红 ⇒ 恢复 ⇒ 绿。内联记录。

- [ ] **步骤 5：提交**

```bash
git add test/save_commit_test.cpp
git commit -m "test(save): assert the death-reason matrix, the committed state and drop persistence"
```

---

## 任务 5：验收 5（提交失败兜底的注入测试）

- [ ] **步骤 1：注入"意图已写、存档未替换"**

```cpp
TEST(SaveCommitTest, InterruptedCommitNeverLeavesALoadablePreDeathSave)
{
	// 场景 A：意图写入后进程在保存前中断
	ASSERT_TRUE(BeginDeathCommit("crash-before-save"));
	EXPECT_FALSE(IsSaveLoadableAfterPendingCommit());
	// 场景 B：保存写入失败（复用仓库既有的写失败注入点：MPQ 先删后写失败 / UNPACKED 截断失败）
	// ⇒ 意图仍在 ⇒ 仍不得加载
}
```

- [ ] **步骤 2：运行 + 反证**

**反证**：去掉"完成才撤销意图"的顺序（改成先 `CompleteDeathCommit()` 再 `SaveGame()`）⇒ 用例**必须红** ⇒ 恢复 ⇒ 绿。内联记录。

- [ ] **步骤 3：提交**

```bash
git add test/save_commit_test.cpp
git commit -m "test(save): fail the commit at both interruption points"
```

---

## 任务 6：产品实验的预注册（**不进验收、不进 CI**）

**文件：** 新建 `docs/superpowers/protocols/2026-09-19-death-penalty-experiment.md`

- [ ] **步骤 1：写预注册（N/人群/阈值先写死）**

内容至少包含：观测项（死亡后 3 分钟内**主动再进同层**的比例；"惩罚过重"自陈）；样本与人群；**结论规则**（保留下限 / 撤销上限，均为具体数字）；以及"未达样本量不得下结论"。

- [ ] **步骤 2：提交**

```bash
git add docs/superpowers/protocols/2026-09-19-death-penalty-experiment.md
git commit -m "docs(protocol): pre-register the death-penalty experiment"
```

---

## 任务 7：收尾（门禁 / 漂移 / CI / 规格实施记录）

- [ ] **步骤 1：全量门禁**

```bash
python3 tools/run_tests.py --json /tmp/ci-save.json
python3 -m tools.eval.backend --smoke
```
预期：`failed==0 && passed_pct==100 && drift_ok==true`、smoke exit 0。

- [ ] **步骤 2：提交后跑漂移（顺序不能反）**

```bash
git add -A && git commit -m "chore: tidy"   # 若有未提交内容
python3 tools/check_drift.py --base origin/master
```
预期：6/6 通过。

- [ ] **步骤 3：push + 跟踪 CI 到终态**

```bash
git push myrepo feature/qol-upgrades
gh run list -R pengyz/devilutionX --limit 2 --json databaseId,headSha,status --jq '.[]|"\(.databaseId) \(.headSha[0:9]) \(.status)"'
gh run watch -R pengyz/devilutionX <run-id> --exit-status
```

- [ ] **步骤 4：规格实施记录 + 台账**

规格 §7 追加实施记录（改动的可观测结果、验收 5 条的实测、CI run id）；台账追加本轮结论（随提交进 git）。

---

## 自检

**1. 规格覆盖度**

| 规格条目 | 任务 |
|---|---|
| §4.1① 死亡提交 | 任务 2 |
| §4.1② 切层提交语义 | 任务 4 步骤 3（切层/重进后状态仍在） |
| §4.1③ 时间线唯一（无回滚入口） | 任务 3（闸门）+ 任务 5（失败兜底） |
| §4.1④ 死亡不终结角色 | 任务 4 步骤 2（重载后处死亡态但可复活） |
| §4.1⑤ 提交失败的语义 | 任务 1（意图）+ 任务 3（闸门）+ 任务 5（注入） |
| §4.1⑥⑦⑧ 边界/多人/旧档 | **范围声明**：任务 2 的"仅本地、仅单机"实现 + 任务 7 记录（不设验收，规格已声明 ✓） |
| §6-1…§6-5 | 任务 4、5 |
| §6-6（④ 的逐层×职业表） | **不在本计划**（属 §4.3 ④ 的独立立项 ✓） |
| §6-7 产品实验 | 任务 6（移出验收 ✓） |
| §4.3 路线图 | **不在本计划**（各自立项 ✓） |

**2. 占位符扫描**：无 `TBD/TODO`；`<run-id>` 是执行时从 `gh run list` 取值的动作；任务 1 步骤 1 的"确认真实函数名"是**显式执行动作**（并已给出候选骨架 ✓）。

**3. 类型一致性**：`BeginDeathCommit/CompleteDeathCommit/HasPendingDeathCommit/DeathCommitIntentPath`（任务 1）在任务 2/3/4/5 中同名使用；`IsSaveLoadableAfterPendingCommit()`（任务 3 步骤 2 定义）在任务 3 步骤 1 与任务 5 中使用；`SaveGame()` 使用既有声明（`loadsave.h:42` ✓），不新造。

---

## 可见性证明（本章专属，v2：经独立复核修正）

**命题 A′（成立，范围已收窄）**：
> **在"存在可加载旧档"且"玩家会在死亡后重载或重开"的条件下**，本改动**改变玩家可见的结局**，且该差异**可机械验证**。
> 对"死亡后原地复活、且永不读档/退出"的玩家，本改动**不改变任何可见结局** ✗（现状下掉落与扣金已经发生 ✓ `Source/player.cpp:2733` ✓）。

### ① 演绎（全部经本会话核验）

| 事实 | 锚点 |
|---|---|
| 单机**死亡不写档**；且**死亡期间手动保存被禁用** | `Source/player.cpp:2845 SyncPlrKill` → `:2672 StartPlayerKill`（无写档）✓；`Source/gamemenu.cpp:341`（`PM_DEATH/MyPlayerIsDead` ⇒ `gamemenu_off()`）✓ |
| 单机**切层只写 temp 关卡档、不写英雄** | `Source/interfac.cpp` ×8 → `pfile_save_level()` → `SaveLevel(saveWriter)`（写 `GetTempLevelNames()`，`Source/loadsave.cpp:1956` ✓；**无** `SavePlayer`/`pfile_write_hero` ✓） |
| 单机的**英雄写档只有手动 `SaveGame()`**（另有建角与转档例外，均**非死亡路径**） | `Source/gamemenu.cpp:353` ✓；建角 `Source/pfile.cpp:746` ✓、HF/Diablo 转档载入 `Source/loadsave.cpp:2701` ✓（**例外已核验为非死亡路径** ✓）；`pfile_update` 单机首行 `return`（`Source/pfile.cpp:828-830`）✓；退出写档仅多人（`Source/init.cpp:116-118`）✓ |
| **死亡时按 ESC 在单机直接读旧档**（一键撤销） | `Source/diablo.cpp:524-532`（`MyPlayerIsDead` + `SDLK_ESCAPE` + `!gbIsMultiplayer` + `gbValidSaveFile` ⇒ `gamemenu_load_game(false)`）✓ |

**结论（可直接观察的差异）**：现状 ⇒ **装备穿在身上、金币回来**（死亡被一键撤销 ✗）；改动后 ⇒ **装备在死亡地点、金币已扣**（死亡被兑现 ✓）。

### ② 被独立复核推翻的旧推论（已删除，留档）

- ~~"死后切层会让同一件装备重复"~~ ✗ **不成立**：死后切层写的是 **temp 关卡档**（`Source/loadsave.cpp:1956` ✓），而**读档与新开都会 `pfile_remove_temp_files()`**（`Source/loadsave.cpp:2510` ✓、`Source/interfac.cpp:331` ✓）⇒ 地面副本**不会**参与合并 ✓。
  ⇒ 因此"**推论 2 是缺陷、无需口味实验**"的说法**也随之作废** ✗：能免于实验的只有"存档违反契约/时间线不一致"这类**机械正确性** ✓；**死亡惩罚强度仍必须由预注册实验判定** ✓。

### ③ 可失败断言（工程形式，**只保留能证明改动的那些**）

| 断言 | 是否证明本改动 |
|---|---|
| 重载后装备**在死亡地点**（不在身上）、金币已扣 | ✅ **直接可见 + 只由本改动产生** ✓ |
| 死亡后**按 ESC / 退出重开** ⇒ 得到死亡态而非死前态 | ✅ 直接可见 ✓（现状该入口直接读旧档 ✓） |
| 死亡提交后**英雄档与关卡档一致**（无"穿戴与地面并存"） | ✅ 机械正确性 ✓ |
| ~~死因矩阵（怪/陷阱/玩家杀/镇上/多人例外）~~ | ❌ 修复前**也 PASS**（`Source/player.cpp:2683/2733/2766` ✓）⇒ **不构成改动的证据**，仅作回归护栏 ✓ |
| ~~同会话"离开再回来掉落仍在"~~ | ❌ 修复前**也 PASS**（`Source/diablo.cpp:3163/3199` ✓）⇒ 仅作回归护栏 ✓ |
| ~~`_pmode == PM_DEATH`、HP == 0~~ | ⚠️ 内部状态，**须给状态→呈现映射**（死亡动画/血条/无法行动）才可称"可见" ✓ |

### ④ 未列出的可见后果（v2 补）

1. **菜单语义改变**：现状死亡时 ESC 直接读旧档（`Source/diablo.cpp:528` ✓）；改动后该入口必须指向**死亡态**，且在**提交失败**时该入口必须被**禁用** ✓（规格 §4.1⑤ ✓）。
2. **窗口**：死亡发生到提交完成之间存在极短窗口，其间强杀进程仍可能规避 ⇒ 由**意图文件**在下次启动时兜底 ✓（规格 §4.1⑤ ✓），该窗口必须在验收里显式测试 ✓。
3. **不改变的场景**：玩家不读档/不退出 ⇒ 无可见差异 ✓（已写入命题 A′ 的范围 ✓）。

## 执行交接

按作者既有指示**内联执行**（`superpower-executing-plans`）。检查点：任务 3 步骤 3（闸门反证）、任务 5 步骤 2（失败注入反证）、任务 7 步骤 1（全量门禁）。**不使用任何子 agent。**