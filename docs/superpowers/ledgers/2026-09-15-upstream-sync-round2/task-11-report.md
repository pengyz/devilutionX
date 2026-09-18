# Task 11 报告：复核 heroname 越界读在上游的分叉状态并留档

- 任务：复核 `TEar::heroname[17]`（定长 char 数组当裸指针读 → `strlen` 越界）在上游的修复状态，并以实测更新知识库中的「上游状态」表述
- 工作区：`/home/peng/workspace/DevilutionX`，分支 `feature/qol-upgrades`
- 复核日期：2026-09-15
- 起始 HEAD：`91e805149`（`fix(build): add panels/quest_log.hpp include after the upstream quest split`）
- 结束时 HEAD：`d1e122ab6`（见文末提交 SHA）
- 结论：**上游仍未修**（与 2026-09-15 首轮探测结论一致）→ 按简报步骤 2 的「仍未修」分支处理，**未改任何源码**

---

## 1. 复核命令与真实输出（判据 = 上游 tip，不是本地）

### 1.0 先固定判据（上游 tip 的 commit）

```bash
git rev-parse origin/master
git log -1 --format='%H %ci %s' origin/master
```

真实输出：

```
e00b7260fff69f42a3d16b60353abced6ee04ace
e00b7260fff69f42a3d16b60353abced6ee04ace 2026-09-15 05:32:22 +0200 Prevent inlining of blitter operations
```

→ 本轮复核所依据的上游 commit = `e00b7260fff69f42a3d16b60353abced6ee04ace`（`origin/master` tip）。

### 1.1 `git show origin/master:Source/msg.cpp | grep -n "heroname"`

真实输出：

```
1094:				CopyUtf8(delta.ear.heroname, message.ear.heroname, sizeof(delta.ear.heroname));
1360:		RecreateEar(item, Swap16LE(message.ear.wCI), Swap32LE(message.ear.dwSeed), message.ear.bCursval, message.ear.heroname);
1368:		RecreateEar(item, Swap16LE(message.ear.wCI), Swap32LE(message.ear.dwSeed), message.ear.bCursval, message.ear.heroname);
1398:	    ear.heroname);
2695:	CopyUtf8(ear.heroname, item._iIName, sizeof(ear.heroname));
```

### 1.2 `git show origin/master:Source/pack.cpp | grep -n "heroname"`

真实输出：

```
445:		RecreateEar(item, Swap16LE(packedItem.ear.wCI), Swap32LE(packedItem.ear.dwSeed), packedItem.ear.bCursval, packedItem.ear.heroname);
```

### 1.3 上游上下文（确证 5 处都是「读」侧、且都未加 `sizeof` 限定）

```bash
git show origin/master:Source/msg.cpp | sed -n '1090,1096p'
git show origin/master:Source/msg.cpp | sed -n '1356,1370p'
git show origin/master:Source/msg.cpp | sed -n '1393,1400p'
git show origin/master:Source/pack.cpp | sed -n '440,448p'
```

真实输出（节选，保留行号语义）：

```
1109 行区:
			if (message.def.wIndx == IDI_EAR) {
				delta.ear.bCursval = message.ear.bCursval;
				CopyUtf8(delta.ear.heroname, message.ear.heroname, sizeof(delta.ear.heroname));
			} else {

1356-1370 行区:
void RecreateItem(const Player &player, const TCmdPItem &message, Item &item)
{
	if (message.def.wIndx == Swap16LE(IDI_EAR))
		RecreateEar(item, Swap16LE(message.ear.wCI), Swap32LE(message.ear.dwSeed), message.ear.bCursval, message.ear.heroname);
	else
		RecreateItem(player, message.item, item);
}

void RecreateItem(const Player &player, const TCmdChItem &message, Item &item)
{
	if (message.def.wIndx == Swap16LE(IDI_EAR))
		RecreateEar(item, Swap16LE(message.ear.wCI), Swap32LE(message.ear.dwSeed), message.ear.bCursval, message.ear.heroname);
	else
		RecreateItem(player, message.item, item);

1393-1400 行区:
	return SyncDropEar(
	    position,
	    Swap16LE(ear.wCI),
	    Swap32LE(ear.dwSeed),
	    ear.bCursval,
	    ear.heroname);
}

pack.cpp 440-448 行区:
	item = {};
	const auto idx = static_cast<_item_indexes>(Swap16LE(packedItem.def.wIndx));
	if (idx < 0 || idx >= static_cast<_item_indexes>(AllItemsList.size()))
		return true;
	if (idx == IDI_EAR) {
		RecreateEar(item, Swap16LE(packedItem.ear.wCI), Swap32LE(packedItem.ear.dwSeed), packedItem.ear.bCursval, packedItem.ear.heroname);
		return true;
	}
```

### 1.4 `RecreateEar` 形参在两边的签名（说明「调用点隐式转换 = 越界点」仍成立）

```bash
git show origin/master:Source/items.h | grep -n "RecreateEar"
grep -rn "RecreateEar" Source/items.h
```

真实输出：

```
535:void RecreateEar(Item &item, uint16_t ic, uint32_t iseed, uint8_t bCursval, std::string_view heroName);   # 上游
538:void RecreateEar(Item &item, uint16_t ic, uint32_t iseed, uint8_t bCursval, std::string_view heroName);   # fork
```

→ 上游形参已是 `std::string_view`，但调用点仍传裸 `char[17]`，隐式转换走 `strlen`——与 fork 修复前完全同构，缺陷未消除。

---

## 2. 逐条比对：上游 vs fork（5 个调用点）

| # | 位置（上游行号） | 上游 tip 写法 | fork 写法 | 状态 |
|---|---|---|---|---|
| 1 | `Source/msg.cpp:1094` | `CopyUtf8(delta.ear.heroname, message.ear.heroname, sizeof(delta.ear.heroname))` | `CopyUtf8(delta.ear.heroname, std::string_view(message.ear.heroname, sizeof(message.ear.heroname)), sizeof(delta.ear.heroname))` | 上游是裸字段；fork 已限长 |
| 2 | `Source/msg.cpp:1360` | `RecreateEar(..., message.ear.heroname)` | `RecreateEar(..., std::string_view(message.ear.heroname, sizeof(message.ear.heroname)))` | 同上 |
| 3 | `Source/msg.cpp:1368` | `RecreateEar(..., message.ear.heroname)` | `RecreateEar(..., std::string_view(message.ear.heroname, sizeof(message.ear.heroname)))` | 同上 |
| 4 | `Source/msg.cpp:1398` | `SyncDropEar(..., ear.heroname)` | `SyncDropEar(..., std::string_view(ear.heroname, sizeof(ear.heroname)))` | 同上 |
| 5 | `Source/pack.cpp:445` | `RecreateEar(..., packedItem.ear.heroname)` | `RecreateEar(..., std::string_view(packedItem.ear.heroname, sizeof(packedItem.ear.heroname)))` | 同上（fork 侧因本地分叉，行号已移至 453） |

说明：上游 `Source/msg.cpp:2695` 是**写**侧（`CopyUtf8(ear.heroname, item._iIName, sizeof(ear.heroname))`，目标被 `sizeof` 限定、源为已终止的 `item._iIName`），不属该缺陷；fork 对应行号为 `2700`。上游行号 1094/1360/1368/1398 + pack.cpp 445 **与首轮探测结论完全一致**，未被本轮同步改变。

---

## 3. 上游结论

**上游仍未修（结论与 2026-09-15 首轮探测一致）。**

- 判据 commit：`e00b7260fff69f42a3d16b60353abced6ee04ace`（`origin/master` tip，2026-09-15 05:32:22 +0200）
- 仍为裸字段的确切位置（5 处，全部未加 `sizeof` 限定）：
  - `Source/msg.cpp:1094`（`CopyUtf8` 源 `message.ear.heroname`）
  - `Source/msg.cpp:1360`、`Source/msg.cpp:1368`（`RecreateItem` ×2 的 `RecreateEar` 实参 `message.ear.heroname`）
  - `Source/msg.cpp:1398`（`SyncDropEar` 实参 `ear.heroname`）
  - `Source/pack.cpp:445`（`UnPackNetItem` 的 `RecreateEar` 实参 `packedItem.ear.heroname`）
- 因此走简报步骤 2 的「仍未修」分支：**保留 fork 修复、不改任何源码**，仅更新知识库状态。
- 简报里「已修」分支的 `git checkout origin/master -- Source/pack.cpp Source/pfile.cpp Source/msg.cpp` **未执行**（会牵动门禁，属控制者裁决范围）。上游侧 `git show origin/master:Source/pack.cpp | grep -n "pName"` 亦未作为决策依据，因为该分支前提不成立。

---

## 4. fork 修复仍在的证据（合并后未被上游覆盖）

命令与真实输出：

```bash
grep -n "std::string_view(message.ear.heroname" Source/msg.cpp
grep -n "std::string_view(ear.heroname" Source/msg.cpp
grep -n "std::string_view(packedItem.ear.heroname" Source/pack.cpp
```

```
1094:				CopyUtf8(delta.ear.heroname, std::string_view(message.ear.heroname, sizeof(message.ear.heroname)), sizeof(delta.ear.heroname));
1360:		RecreateEar(item, Swap16LE(message.ear.wCI), Swap32LE(message.ear.dwSeed), message.ear.bCursval, std::string_view(message.ear.heroname, sizeof(message.ear.heroname)));
1368:		RecreateEar(item, Swap16LE(message.ear.wCI), Swap32LE(message.ear.dwSeed), message.ear.bCursval, std::string_view(message.ear.heroname, sizeof(message.ear.heroname)));
1398:	    std::string_view(ear.heroname, sizeof(ear.heroname)));
453:		RecreateEar(item, Swap16LE(packedItem.ear.wCI), Swap32LE(packedItem.ear.dwSeed), packedItem.ear.bCursval, std::string_view(packedItem.ear.heroname, sizeof(packedItem.ear.heroname)));
```

→ 三组命令均命中，合计覆盖 **5 个调用点**（`msg.cpp` 4 处 + `pack.cpp` 1 处），与预期一致，merge 未回退 fork 修复。

补充审计（只读）：

```bash
grep -n "heroname" Source/msg.cpp Source/pack.cpp   # 见 1.1/1.2 的 fork 对应输出
grep -rn "heroname" Source/ --include=*.cpp --include=*.hpp --include=*.h | grep -v "^Source/msg.cpp\|^Source/pack.cpp"
```

```
Source/inv.cpp:1899:int SyncDropEar(Point position, uint16_t icreateinfo, uint32_t iseed, uint8_t cursval, std::string_view heroname)
Source/inv.cpp:1905:	RecreateEar(item, icreateinfo, iseed, cursval, heroname);
Source/msg.h:554:	char heroname[17];
Source/inv.h:237:int SyncDropEar(Point position, uint16_t icreateinfo, uint32_t iseed, uint8_t cursval, std::string_view heroname);
```

→ fork 侧不存在遗留的裸 `heroname` 读调用点；`inv.cpp:1899/1905` 与 `inv.h:237` 的形参本就是 `std::string_view`（属下游，已由上溯 5 处限长），`msg.h:554` 是字段声明本身。即 merge **未引入新的裸读**。

fork 修复提交（历史留档）：

```bash
git show --stat --format='%H%n%ci%n%s' a82c0a827
```

```
a82c0a8275e85af8e89609366b0bbd81f68ac2a6
2026-09-15 15:44:11 +0800
fix(msg): bound ear-name reads from network/save data

 Source/msg.cpp  | 8 ++++----
 Source/pack.cpp | 2 +-
 2 files changed, 5 insertions(+), 5 deletions(-)
```

---

## 5. 我改动的确切文本

唯一改动文件：`docs/knowledge/pattern_fixed_width_field_reads.md`（在「实例（同一类，两次）」段之后插入 1 个空行 + 1 行状态；原有那句「上游 master 至今仍是」被保留，由新增状态行提供实测出处）。

新增文本（逐字）：

```
**上游状态（2026-09-15 同步后复核）**：仍**未修**。判据是上游 tip 而非本地：以 `origin/master` = `e00b7260fff69f42a3d16b60353abced6ee04ace`（`2026-09-15 05:32:22 +0200`，`Prevent inlining of blitter operations`）实测，裸字段读取恰好仍是 5 处——`Source/msg.cpp:1094`（`CopyUtf8` 的源 `message.ear.heroname`）、`1360` 与 `1368`（`RecreateItem` ×2 的 `RecreateEar` 实参）、`1398`（`SyncDropEar` 实参 `ear.heroname`）、`Source/pack.cpp:445`（`UnPackNetItem` 的 `RecreateEar` 实参 `packedItem.ear.heroname`），均未加 `sizeof` 限定。fork 侧同 5 个调用点已收敛为 `std::string_view(field, sizeof(field))`（`a82c0a827`，本轮 merge 后未被上游覆盖）——即该分歧在本轮同步后依然存在，fork 修复需继续保留。
```

`git diff` 摘要：

```
 docs/knowledge/pattern_fixed_width_field_reads.md | 2 ++
 1 file changed, 2 insertions(+)
```

**未改动**：任何源码文件（`Source/**` 全部未动）、frontmatter（yaml 头部保持原样）、任何其他文档。**未新增** `docs/superpowers/plans/` 下的 PR 文案文件——简报把该项与提交留档并列，但对外 PR 文案属对外动作，且硬约束限定「不得修改除该 .md 之外的任何文件」，故本次不创建，留待控制者确认（见第 7 节顾虑）。

---

## 6. 行尾自查（`.editorconfig` 要求 `.md` = LF）

改动前：

```bash
python3 -c "b=open('docs/knowledge/pattern_fixed_width_field_reads.md','rb').read();print('CRLF',b.count(b'\r\n'),'LF',b.count(b'\n'))"
# CRLF 0 LF 42
```

改动后（工作区）：

```bash
python3 -c "b=open('docs/knowledge/pattern_fixed_width_field_reads.md','rb').read();print('CRLF',b.count(b'\r\n'),'LF',b.count(b'\n'))"
# CRLF 0 LF 44
```

提交后（HEAD blob，独立复核）：

```bash
git show HEAD:docs/knowledge/pattern_fixed_width_field_reads.md | python3 -c "import sys;b=sys.stdin.buffer.read();print('CRLF',b.count(b'\r\n'),'LF',b.count(b'\n'))"
# CRLF 0 LF 44
```

→ **CRLF = 0**，符合 `.editorconfig`（`.md` → `end_of_line = lf`）。LF 由 42 增至 44，与新增 1 行文本 + 1 个空行一致。

---

## 7. 提交与状态

```bash
git add docs/knowledge/pattern_fixed_width_field_reads.md
git commit -m "docs(knowledge): record heroname divergence state after the upstream sync"
```

真实输出与提交信息：

```
[feature/qol-upgrades d1e122ab6] docs(knowledge): record heroname divergence state after the upstream sync
 1 file changed, 2 insertions(+)
```

- 提交 SHA（完整）：`d1e122ab6e0475f5b3b0659901d422bfa4cbc784`
- 提交 SHA（短）：`d1e122ab6`
- 提交标题：`docs(knowledge): record heroname divergence state after the upstream sync`
- 父提交：`91e805149`
- `git status --short` 提交后为空（工作区干净）
- `git show --stat HEAD` 确认**仅** `docs/knowledge/pattern_fixed_width_field_reads.md`（1 file changed, 2 insertions）

### 约束遵守

| 硬约束 | 状态 |
|---|---|
| 只改 `docs/knowledge/pattern_fixed_width_field_reads.md` | ✅ `git show --stat HEAD` = 1 file |
| 不改源码 | ✅ `Source/**` 零改动 |
| 不推送 | ✅ 未执行任何 push |
| 不重跑测试 | ✅ 未执行 `run_tests.py` / `ctest` / eval |
| 不分发子代理 | ✅ 未调用 subagent |
| `.md` 为 LF | ✅ CRLF 0（工作区与 HEAD blob 均实测） |

### 顾虑 / 待控制者裁决

1. **简报「仍未修」分支里还要求把「上游 PR 的文案与 ASan 复现步骤」整理进 `docs/superpowers/plans/`**，与本次硬约束「不得修改除该 .md 之外的任何文件」直接冲突，且对外 PR 属需单独确认的动作。本次**未创建**该文件；如需该文案，请单独授权后再做。
2. 突发情况**未发生**：上游确实仍未修，因此未触发「已修 → NEEDS_CONTEXT」路径，也未执行简报里会牵动门禁的 `git checkout origin/master -- ...`。
3. fork 与上游的 `pack.cpp` 行号已错位（上游 445 vs fork 453），未来若上游修复并 merge，需按内容而非行号比对，避免误判为「上游未修」。
4. 该缺陷是**静默越界读**（仅恶意/损坏 peer 数据触发），上游未修意味着 fork 需长期携带这 5 处修复；每次上游同步都应重跑本复核。