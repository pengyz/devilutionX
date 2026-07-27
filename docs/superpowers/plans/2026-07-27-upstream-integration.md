# 上游集成实施计划：std::format / std::expected / C++23

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把 `origin/master` 的 41 个 commit 合入 `feature/qol-upgrades`，完成 `fmt::` → `std::format`/`FormatRuntime`、`tl::expected` → `std::expected`、C++20 → C++23 三项迁移，以 629 项测试零失败收尾。

**Architecture:** 单次 merge（非 rebase），在解冲突过程中完成迁移。31 个冲突块按解决方式分五类：24 块机械型（脚本处理）、1 块 include 合并、4 块 `tl::expected` 迁移、3 块语义冲突（手工）、1 项目录重命名。五道门禁提供中间进度信号。

**Tech Stack:** C++23（g++ 13.4，已验证 `std::format` 与 `std::expected` 可用）、CMake + Ninja、GoogleTest、Python 3（迁移脚本）

**Spec:** `docs/superpowers/specs/2026-07-27-upstream-integration-design.md`

---

## 探测阶段已确认的事实

以下来自 2026-07-27 的探测性 merge（已 `--abort` 回滚），计划基于事实而非推测：

| 事实 | 值 |
|---|---|
| 冲突文件数 | 14（13 个 C++ + 1 项 TSV 路径） |
| 冲突块总数 | 31 |
| `Source/levels/trigs.cpp` | 15 块（占近半） |
| 行尾回退过的 6 个文件 | 5 个自动合并成功；`Source/inv.cpp` 仅 1 块 6 行 |
| 上游目录重命名 | `mods/Hellfire/` → `mods/hf/` |
| 上游 `mods/hf/txtdata/spells/` | 已有 `spelldat.tsv`，无 `spelldesc.tsv` |
| merge 前 HEAD | `1634f5567` |

冲突的统一形态：我方为 `FloatingInfoString = fmt::format(fmt::runtime(X), args)`，上游为 `InfoString = FormatRuntime(X, args)`。两个正交改动叠加——我方把显示目标从 `InfoString` 迁到 `FloatingInfoString`（浮动信息 UI 设计），上游把格式化迁到 `FormatRuntime`。**解决规则：保留我方的目标变量与行结构，采用上游的调用形式。**

`FloatingInfoString` 在 merge-base 时已存在于上游 6 个文件中，本分支扩展到 15 个文件，因此这不是命名冲突而是覆盖面差异。

---

## 文件结构

### 新建的文件

| 文件 | 职责 |
|---|---|
| `tools/migrate_fmt.py` | 把 `fmt::format(fmt::runtime(X), ...)` 改写为 `FormatRuntime(X, ...)`，把 `fmt::format(` 改写为 `std::format(`。括号平衡解析，跳过字符串字面量，保留原行尾 |
| `tools/resolve_ours.py` | 按冲突块保留 HEAD 侧，只改动冲突标记之间的内容，保留 git 已自动合并的其余部分 |

两个脚本均已实测（见 Task 1）。放在 `tools/` 下而非 `/tmp`，因为后续每次上游集成都会用到。

### 冲突块分类与处置

| 类型 | 块数 | 文件 | 处置 |
|---|---|---|---|
| A 机械型 | 24 | `control_infobox.cpp`(2)、`cursor.cpp`(1)、`inv.cpp`(1)、`items.cpp`(1)、`trigs.cpp`(15)、`objects.cpp`(2)、`quests.cpp`(1)、`panels/spell_book.cpp`(1) | 脚本：保留 HEAD 侧 + 迁移 |
| B include 合并 | 1 | `control_infobox.cpp` | 手工：两侧 include 都保留 |
| C `tl::expected` 迁移 | 4 | `spell_book.cpp`、`spell_book.hpp`、`misdat.h`、`spelldat.cpp` | 手工 |
| D 语义冲突 | 3 | `panels/spell_list.cpp` | 手工 |
| E 目录重命名 | 1 | `mods/Hellfire/` → `mods/hf/` | 手工路径迁移 |

`misdat.cpp` 的 1 块单独处理（Task 7），因为它涉及弃用我方 127 行的注册表实现。

---

## Task 1: 落地迁移脚本

**Files:**
- Create: `tools/migrate_fmt.py`
- Create: `tools/resolve_ours.py`

- [ ] **Step 1: 创建 `tools/migrate_fmt.py`**

```python
#!/usr/bin/env python3
"""Rewrite fmt::format(fmt::runtime(X), ...) as FormatRuntime(X, ...).

Paren-balanced, so format strings containing calls such as
ngettext("a", "b", n) are handled correctly. Also rewrites the plain
fmt::format(...) -> std::format(...) case.
"""
import sys

NEEDLE = 'fmt::format(fmt::runtime('


def close_paren(text: str, open_idx: int) -> int:
    """Index of the ')' matching the '(' at open_idx, skipping string literals."""
    depth = 0
    i = open_idx
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"':
            i += 1
            while i < n:
                if text[i] == '\\':
                    i += 2
                    continue
                if text[i] == '"':
                    break
                i += 1
        elif c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError(f'unbalanced parens starting at {open_idx}')


def migrate_runtime(text: str) -> tuple[str, int]:
    count = 0
    while True:
        at = text.find(NEEDLE)
        if at < 0:
            return text, count
        runtime_open = at + len('fmt::format(fmt::runtime')
        runtime_close = close_paren(text, runtime_open)
        inner = text[runtime_open + 1:runtime_close]
        rest = text[runtime_close + 1:]
        text = text[:at] + 'FormatRuntime(' + inner + rest
        count += 1


def migrate_plain(text: str) -> tuple[str, int]:
    count = text.count('fmt::format(')
    return text.replace('fmt::format(', 'std::format('), count


def main(paths: list[str]) -> int:
    total_rt = total_plain = 0
    for path in paths:
        with open(path, 'rb') as f:
            raw = f.read().decode('utf-8')
        crlf = '\r\n' in raw
        text = raw.replace('\r\n', '\n') if crlf else raw

        text, n_rt = migrate_runtime(text)
        text, n_plain = migrate_plain(text)

        if n_rt or n_plain:
            out = text.replace('\n', '\r\n') if crlf else text
            with open(path, 'wb') as f:
                f.write(out.encode('utf-8'))
            print(f'{path}: runtime={n_rt} plain={n_plain}')
        total_rt += n_rt
        total_plain += n_plain
    print(f'total: runtime={total_rt} plain={total_plain}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

- [ ] **Step 2: 创建 `tools/resolve_ours.py`**

```python
#!/usr/bin/env python3
"""Resolve conflict blocks by keeping the HEAD side, leaving the rest untouched.

Only touches lines between conflict markers, so content git already
auto-merged elsewhere in the file is preserved.
"""
import sys

def resolve(text: str) -> tuple[str, int]:
    out = []
    blocks = 0
    state = 'normal'  # normal | ours | theirs
    for line in text.split('\n'):
        if state == 'normal':
            if line.startswith('<<<<<<< '):
                state = 'ours'
                blocks += 1
                continue
            out.append(line)
        elif state == 'ours':
            if line.startswith('======='):
                state = 'theirs'
                continue
            if line.startswith('>>>>>>> '):
                state = 'normal'
                continue
            out.append(line)
        else:  # theirs -- discard
            if line.startswith('>>>>>>> '):
                state = 'normal'
            continue
    if state != 'normal':
        raise ValueError('unterminated conflict block')
    return '\n'.join(out), blocks


def main(paths: list[str]) -> int:
    for path in paths:
        with open(path, 'rb') as f:
            raw = f.read().decode('utf-8')
        crlf = '\r\n' in raw
        text = raw.replace('\r\n', '\n') if crlf else raw
        text, blocks = resolve(text)
        out = text.replace('\n', '\r\n') if crlf else text
        with open(path, 'wb') as f:
            f.write(out.encode('utf-8'))
        print(f'{path}: kept HEAD side of {blocks} block(s)')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

- [ ] **Step 3: 用固定样本验证两个脚本**

```bash
mkdir -p /tmp/migverify && cd /tmp/migverify

printf 'A\tFloatingInfoString = fmt::format(fmt::runtime(ngettext("{:s} gold piece", "{:s} gold pieces", nGold)), FormatInteger(nGold));\r\n' > sample.cpp
printf 'B\tAddInfoBoxString(fmt::format(fmt::runtime(_("Hit Points {:d} of {:d}")), a >> 6, b >> 6));\r\n' >> sample.cpp
printf 'C\tstd::string s = fmt::format("plain {} and {}", x, y);\r\n' >> sample.cpp
printf 'D\treturn fmt::format(fmt::runtime(_(/* TRANSLATORS: note (with parens) */ "Trapped {:s}")), InfoString.str());\r\n' >> sample.cpp

python3 /home/peng/workspace/DevilutionX/tools/migrate_fmt.py sample.cpp
cat sample.cpp
echo "CRLF: $(grep -c $'\r' sample.cpp)/$(wc -l < sample.cpp)"

printf 'before\r\n<<<<<<< HEAD\r\nOURS line 1\r\nOURS line 2\r\n=======\r\nTHEIRS line\r\n>>>>>>> origin/master\r\nmiddle\r\n<<<<<<< HEAD\r\nOURS again\r\n=======\r\n>>>>>>> origin/master\r\nafter\r\n' > conf.cpp
python3 /home/peng/workspace/DevilutionX/tools/resolve_ours.py conf.cpp
cat conf.cpp
echo "markers left: $(grep -cE '^(<<<<<<<|=======|>>>>>>>)' conf.cpp || echo 0)"
echo "CRLF: $(grep -c $'\r' conf.cpp)/$(wc -l < conf.cpp)"
```

Expected:
```
sample.cpp: runtime=3 plain=1
total: runtime=3 plain=1
A	FloatingInfoString = FormatRuntime(ngettext("{:s} gold piece", "{:s} gold pieces", nGold), FormatInteger(nGold));
B	AddInfoBoxString(FormatRuntime(_("Hit Points {:d} of {:d}"), a >> 6, b >> 6));
C	std::string s = std::format("plain {} and {}", x, y);
D	return FormatRuntime(_(/* TRANSLATORS: note (with parens) */ "Trapped {:s}"), InfoString.str());
CRLF: 4/4
conf.cpp: kept HEAD side of 2 block(s)
before
OURS line 1
OURS line 2
middle
OURS again
after
markers left: 0
CRLF: 6/6
```

关键点：D 行证明脚本正确处理注释内的括号，A 行证明正确处理嵌套调用，CRLF 计数证明行尾未被破坏（宪章禁令 7）。

- [ ] **Step 4: 清理验证目录并提交**

```bash
rm -rf /tmp/migverify
cd /home/peng/workspace/DevilutionX
chmod +x tools/migrate_fmt.py tools/resolve_ours.py
git add tools/migrate_fmt.py tools/resolve_ours.py
git commit -m "tools: add upstream migration helpers

migrate_fmt.py rewrites fmt::format(fmt::runtime(X), ...) as
FormatRuntime(X, ...) with paren-balanced parsing, so format strings that
themselves contain calls such as ngettext(\"a\", \"b\", n) are handled
correctly. It also rewrites the plain fmt::format case to std::format.

resolve_ours.py keeps the HEAD side of each conflict block and touches
nothing outside the markers, so content git already auto-merged in the same
file survives. git checkout --ours cannot be used for this: it replaces the
whole file from the index stage and would discard those auto-merges.

Both preserve the file's existing line endings, which charter prohibition 7
requires.

Kept in tools/ rather than written ad hoc because every future upstream
integration needs them."
```

---

## Task 2: 执行 merge 并复现探测结果

**Files:** 无（git 操作）

- [ ] **Step 1: 确认干净状态并记录 HEAD**

```bash
cd /home/peng/workspace/DevilutionX
git status --short
git rev-parse HEAD | tee /tmp/pre_merge_head.txt
```

Expected: `git status --short` 无输出；HEAD 为 Task 1 的提交。

- [ ] **Step 2: 确认上游引用是最新的**

```bash
git fetch origin
git rev-parse origin/master
```

Expected: `b3e52b1ea...`。若与探测时不同，说明上游又前进了，须停止并重新执行探测（本计划的冲突清单基于 `b3e52b1ea`）。

- [ ] **Step 3: 执行 merge**

```bash
git merge --no-commit --no-ff origin/master
```

Expected: 以「自动合并失败，修正冲突然后提交修正的结果。」结束，退出码非零。这是预期结果，不是错误。

- [ ] **Step 4: 复现探测结论**

```bash
git diff --name-only --diff-filter=U | tee /tmp/conflicts.txt
echo "冲突文件数: $(wc -l < /tmp/conflicts.txt)"
total=0
while read -r f; do
  [ -f "$f" ] || continue
  n=$(grep -c '^<<<<<<<' "$f" 2>/dev/null || echo 0)
  printf "  %-44s %s\n" "$f" "$n"
  total=$((total + n))
done < /tmp/conflicts.txt
echo "冲突块总数: $total"
```

Expected:
```
冲突文件数: 14
  Source/control/control_infobox.cpp           3
  Source/cursor.cpp                            1
  Source/inv.cpp                               1
  Source/items.cpp                             1
  Source/levels/trigs.cpp                      15
  Source/objects.cpp                           2
  Source/panels/spell_book.cpp                 1
  Source/panels/spell_book.hpp                 1
  Source/panels/spell_list.cpp                 3
  Source/quests.cpp                            1
  Source/tables/misdat.cpp                     1
  Source/tables/misdat.h                       1
  Source/tables/spelldat.cpp                   1
  mods/hf/txtdata/spells/spelldesc.tsv         0
冲突块总数: 31
```

若数字不符，停止并报告——说明上游或本分支状态与探测时不同，后续任务的具体解决方案可能不适用。

- [ ] **Step 5: 记录行尾回退的实测结论（规格验收标准 9）**

```bash
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp \
         Source/lua/modules/monsters.cpp \
         assets/txtdata/classes/classdat.tsv \
         mods/Hellfire/txtdata/spells/spelldat.tsv; do
  if grep -qx "$f" /tmp/conflicts.txt 2>/dev/null; then
    printf "  %-46s 冲突 %s 块\n" "$f" "$(grep -c '^<<<<<<<' "$f" 2>/dev/null || echo 0)"
  else
    printf "  %-46s 自动合并成功\n" "$f"
  fi
done
```

Expected: `Source/inv.cpp` 冲突 1 块，其余五项「自动合并成功」（`mods/Hellfire/...spelldat.tsv` 因目录重命名不在冲突列表内，由 Task 3 处理）。

这一步的输出要保留，Task 13 会把它写进规格。

---

## Task 3: 处理目录重命名（`mods/Hellfire/` → `mods/hf/`）

**Files:**
- Move: `mods/Hellfire/txtdata/spells/spelldesc.tsv` → `mods/hf/txtdata/spells/spelldesc.tsv`
- Modify: `mods/hf/txtdata/spells/spelldat.tsv`

上游把整个 `mods/Hellfire/` 重命名为 `mods/hf/`。本分支在旧路径下有两项改动：新增的 `spelldesc.tsv`，以及对 `spelldat.tsv` 的两处修改（Task 中记为「18 条描述同步」与「3 行字段补齐」）。上游新路径下已有 `spelldat.tsv` 但没有 `spelldesc.tsv`。

- [ ] **Step 1: 确认两侧现状**

```bash
cd /home/peng/workspace/DevilutionX
echo "=== 上游新路径下有什么 ==="
git ls-tree --name-only -r origin/master mods/hf/txtdata/spells/
echo "=== 冲突登记的路径 ==="
git status --short | grep -E "mods/(hf|Hellfire)"
echo "=== 我方旧路径文件是否还在工作树 ==="
ls -l mods/Hellfire/txtdata/spells/ 2>&1 | tail -3
```

Expected: 上游只有 `mods/hf/txtdata/spells/spelldat.tsv`；`spelldesc.tsv` 以 `AA`（both added）或 `AU` 状态出现在 `mods/hf/` 路径下。

- [ ] **Step 2: 落位 `spelldesc.tsv`**

上游没有这个文件，所以我方版本直接采用。

```bash
mkdir -p mods/hf/txtdata/spells
git show HEAD:mods/Hellfire/txtdata/spells/spelldesc.tsv > mods/hf/txtdata/spells/spelldesc.tsv
git add mods/hf/txtdata/spells/spelldesc.tsv
echo "行数=$(wc -l < mods/hf/txtdata/spells/spelldesc.tsv) CRLF=$(grep -c $'\r' mods/hf/txtdata/spells/spelldesc.tsv)"
```

Expected: 行数与 CRLF 计数相等（该文件为 CRLF）。

- [ ] **Step 3: 把我方对 `spelldat.tsv` 的改动合并进上游版本**

我方改动有两项，都要重放到上游的新路径文件上：18 条法术描述（来自 `7f2de51f2`），以及 3 行缺失的 `param8` 字段（来自 `b4580a39c`）。上游版本可能已经带有 `description` 列，需要先确认。

```bash
echo "=== 上游版本的列数与空描述 ==="
git show origin/master:mods/hf/txtdata/spells/spelldat.tsv | head -1 | awk -F'\t' '{print NF" 列"}'
git show origin/master:mods/hf/txtdata/spells/spelldat.tsv | awk -F'\t' 'NR>1 && $16=="" {printf "%s ", $1}'
echo
echo "=== 上游版本是否有字段截断 ==="
git show origin/master:mods/hf/txtdata/spells/spelldat.tsv | awk -F'\t' '{gsub(/\r/,""); if (NF!=24) printf "行%d: %d 列 (%s)\n", NR, NF, $1}'
```

依结果分两种走法：

**若上游版本已是 24 列且无截断**（说明上游自己修好了）：直接采用上游版本，无需重放。

```bash
git show origin/master:mods/hf/txtdata/spells/spelldat.tsv > mods/hf/txtdata/spells/spelldat.tsv
git add mods/hf/txtdata/spells/spelldat.tsv
```

**若上游版本仍有空描述或字段截断**：用我方修好的版本，但需先确认上游没有其他改动。

```bash
diff <(git show HEAD:mods/Hellfire/txtdata/spells/spelldat.tsv | cut -f1-15) \
     <(git show origin/master:mods/hf/txtdata/spells/spelldat.tsv | cut -f1-15) \
  && echo "前 15 列一致，可直接用我方版本" \
  || echo "前 15 列有差异，需逐列合并"
```

前 15 列一致时：

```bash
git show HEAD:mods/Hellfire/txtdata/spells/spelldat.tsv > mods/hf/txtdata/spells/spelldat.tsv
git add mods/hf/txtdata/spells/spelldat.tsv
```

前 15 列有差异时停止并报告——说明上游修改了法术数值，需要人工判断如何与我方的描述列合并。

- [ ] **Step 4: 清理旧路径残留**

```bash
git status --short | grep "mods/Hellfire" || echo "旧路径已无残留"
git rm -r --cached --ignore-unmatch mods/Hellfire 2>/dev/null
rm -rf mods/Hellfire
```

- [ ] **Step 5: 验证**

```bash
echo "=== 新路径内容 ==="
ls -l mods/hf/txtdata/spells/
echo "=== 字段完整性 ==="
awk -F'\t' '{gsub(/\r/,""); if (NF!=24) printf "  行%d: %d 列 (%s)\n", NR, NF, $1}' mods/hf/txtdata/spells/spelldat.tsv || true
echo "（无输出=全部 24 列）"
echo "=== 空描述应只有 3 个废弃法术 ==="
awk -F'\t' 'NR>1 && $16=="" {printf "%s ", $1}' mods/hf/txtdata/spells/spelldat.tsv; echo
echo "=== 旧路径应已消失 ==="
ls -d mods/Hellfire 2>&1 | tail -1
```

Expected: 字段检查无输出；空描述为 `DoomSerpents BloodRitual Invisibility`；旧路径报「没有那个文件或目录」。

---

## Task 4: 机械型冲突（A 类中的 21 块）

**Files:**
- Modify: `Source/cursor.cpp`
- Modify: `Source/inv.cpp`
- Modify: `Source/items.cpp`
- Modify: `Source/levels/trigs.cpp`
- Modify: `Source/objects.cpp`
- Modify: `Source/quests.cpp`

这 6 个文件的冲突块形态一致：我方 `FloatingInfoString = fmt::format(fmt::runtime(X), args)`，上游 `InfoString = FormatRuntime(X, args)`。保留我方目标变量与行结构，采用上游调用形式。

A 类共 24 块，本任务处理其中 21 块。余下 3 块因与其他类型混在同一文件中，随该文件一并处理：`control_infobox.cpp` 的 2 块在 Task 5（该文件另有 1 块 include 冲突），`panels/spell_book.cpp` 的 1 块在 Task 6（该文件另有 `tl::expected` 迁移）。

- [ ] **Step 1: 记录处理前的冲突块数**

```bash
cd /home/peng/workspace/DevilutionX
for f in Source/cursor.cpp Source/inv.cpp Source/items.cpp \
         Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp; do
  printf "%-34s %s 块\n" "$f" "$(grep -c '^<<<<<<<' "$f")"
done
```

Expected: `cursor.cpp` 1、`inv.cpp` 1、`items.cpp` 1、`trigs.cpp` 15、`objects.cpp` 2、`quests.cpp` 1，合计 21 块。

- [ ] **Step 2: 保留 HEAD 侧**

```bash
python3 tools/resolve_ours.py \
  Source/cursor.cpp Source/inv.cpp Source/items.cpp \
  Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp
```

Expected: 六行输出，块数与 Step 1 相同，合计 21。

- [ ] **Step 3: 迁移 fmt 调用**

```bash
python3 tools/migrate_fmt.py \
  Source/cursor.cpp Source/inv.cpp Source/items.cpp \
  Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp
```

Expected: 每个文件报告 `runtime=N plain=M`，`total` 行的 `runtime` 至少为 21（这些文件里可能还有非冲突处的 `fmt::runtime` 调用需要一并迁移）。

- [ ] **Step 4: 确认 `utils/format.hpp` 已被包含**

`FormatRuntime` 定义在 `Source/utils/format.hpp`。上游在这些文件里已使用它，因此 include 应随自动合并带入。逐个确认：

```bash
for f in Source/cursor.cpp Source/inv.cpp Source/items.cpp \
         Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp; do
  if grep -q 'FormatRuntime' "$f"; then
    if grep -q '#include "utils/format.hpp"' "$f"; then
      printf "OK   %s\n" "$f"
    else
      printf "缺失 %s ← 需补 include\n" "$f"
    fi
  else
    printf "跳过 %s（未使用 FormatRuntime）\n" "$f"
  fi
done
```

对报「缺失」的文件，在其现有 `#include "utils/..."` 行附近按字母序插入：

```cpp
#include "utils/format.hpp"
```

若该文件没有 `utils/` 分组，插入到最后一个 `#include "..."` 之后。

- [ ] **Step 5: 验证无残留标记与 fmt 调用**

```bash
echo "=== 冲突标记 ==="
grep -nE '^(<<<<<<<|=======|>>>>>>>)' Source/cursor.cpp Source/inv.cpp \
  Source/items.cpp Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp \
  || echo "无残留"
echo "=== fmt:: 残留 ==="
grep -n 'fmt::' Source/cursor.cpp Source/inv.cpp Source/items.cpp \
  Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp \
  || echo "无残留"
echo "=== 行尾 ==="
for f in Source/cursor.cpp Source/inv.cpp Source/items.cpp \
         Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp; do
  n=$(wc -l < "$f"); c=$(grep -c $'\r' "$f")
  [ "$n" = "$c" ] && printf "OK   %-32s %s/%s\n" "$f" "$c" "$n" \
                  || printf "FAIL %-32s %s/%s\n" "$f" "$c" "$n"
done
```

Expected: 前两项「无残留」；行尾六行全 `OK`。

- [ ] **Step 6: 加入索引**

```bash
git add Source/cursor.cpp Source/inv.cpp Source/items.cpp \
        Source/levels/trigs.cpp Source/objects.cpp Source/quests.cpp
git status --short | grep -E "^(UU|AA)" || echo "以上 6 个文件已解决"
```

---

## Task 5: `control_infobox.cpp` 的 include 合并与迁移

**Files:**
- Modify: `Source/control/control_infobox.cpp`

3 块冲突：第 1 块是 include，第 2、3 块是 A 类机械型。

- [ ] **Step 1: 查看 include 冲突块**

```bash
cd /home/peng/workspace/DevilutionX
awk '/^<<<<<<</{n++} n==1 && /^<<<<<<</,/^>>>>>>>/{print}' Source/control/control_infobox.cpp | head -10
```

Expected:
```
<<<<<<< HEAD

#include "diablo.h"
=======
#include "controls/control_mode.hpp"
>>>>>>> origin/master
```

- [ ] **Step 2: 手工解决 include 冲突——两侧都保留**

把上述整块替换为：

```cpp
#include "controls/control_mode.hpp"
#include "diablo.h"
```

我方的 `#include "diablo.h"` 是浮动信息 UI 工作引入的，上游的 `#include "controls/control_mode.hpp"` 是其自身重构引入的，二者不冲突，按字母序排列。

- [ ] **Step 3: 处理剩余 2 块并迁移**

```bash
python3 tools/resolve_ours.py Source/control/control_infobox.cpp
python3 tools/migrate_fmt.py Source/control/control_infobox.cpp
```

Expected: `resolve_ours` 报 `kept HEAD side of 2 block(s)`；`migrate_fmt` 报 `runtime` 至少 3。

- [ ] **Step 4: 确认 include 与验证**

```bash
echo "=== include 两侧都在 ==="
grep -n '#include "controls/control_mode.hpp"\|#include "diablo.h"\|#include "utils/format.hpp"' \
  Source/control/control_infobox.cpp
echo "=== 残留检查 ==="
grep -nE '^(<<<<<<<|=======|>>>>>>>)|fmt::' Source/control/control_infobox.cpp || echo "无残留"
echo "=== 行尾 ==="
echo "$(grep -c $'\r' Source/control/control_infobox.cpp)/$(wc -l < Source/control/control_infobox.cpp)"
```

Expected: 三个 include 都在（若 `utils/format.hpp` 缺失则按 Task 4 Step 4 的方式补上）；无残留；行尾计数相等。

- [ ] **Step 5: 加入索引**

```bash
git add Source/control/control_infobox.cpp
```

---

## Task 6: `spell_book` 的 `tl::expected` 迁移

**Files:**
- Modify: `Source/panels/spell_book.cpp`
- Modify: `Source/panels/spell_book.hpp`

我方在这两个文件里新增了 `GetSpellRequirementText` 与 `CanLearnSpell`（`24de4002f` 接入法术需求文本），并保留了 `tl::expected` 签名；上游把签名迁到 `std::expected`。需要两者兼得。

- [ ] **Step 1: 解决 `spell_book.cpp`**

把冲突块整体替换为：

```cpp
std::string GetSpellRequirementText(const SpellData &spellData, const Player &player)
{
	int required = spellData.minInt;
	int current = player._pMagic;
	int max = GetClassAttributes(player._pClass).maxMag;

	return FormatRuntime(_("Need {:d} Magic (current {:d}/{:d})"), required, current, max);
}

bool CanLearnSpell(SpellID spell, const Player &player)
{
	const SpellData &spellData = GetSpellData(spell);
	return player._pMagic >= spellData.minInt;
}

std::expected<void, std::string> InitSpellBook()
```

三处改动：保留我方两个函数、`fmt::format(fmt::runtime(...))` 改 `FormatRuntime(...)`、`tl::expected` 改 `std::expected`。

- [ ] **Step 2: 解决 `spell_book.hpp`**

把冲突块整体替换为：

```cpp
struct Player;

std::expected<void, std::string> InitSpellBook();
```

我方的 `struct Player;` 前向声明是 `GetSpellRequirementText` / `CanLearnSpell` 的签名需要的，必须保留。

- [ ] **Step 3: 确认头文件的 include 与声明**

```bash
cd /home/peng/workspace/DevilutionX
echo "=== spell_book.hpp 的 expected include ==="
grep -n '#include <expected>\|#include <expected.hpp>' Source/panels/spell_book.hpp
echo "=== 我方两个函数的声明是否还在 ==="
grep -n 'GetSpellRequirementText\|CanLearnSpell' Source/panels/spell_book.hpp
echo "=== spell_book.cpp 的 format include ==="
grep -n '#include "utils/format.hpp"' Source/panels/spell_book.cpp || echo "缺失，需补"
```

若 `spell_book.hpp` 仍是 `#include <expected.hpp>`（tl 的头），改为 `#include <expected>`。
若 `spell_book.cpp` 缺 `utils/format.hpp`，按 Task 4 Step 4 的方式补上。

- [ ] **Step 4: 验证**

```bash
grep -nE '^(<<<<<<<|=======|>>>>>>>)|fmt::|tl::expected' \
  Source/panels/spell_book.cpp Source/panels/spell_book.hpp || echo "无残留"
for f in Source/panels/spell_book.cpp Source/panels/spell_book.hpp; do
  n=$(wc -l < "$f"); c=$(grep -c $'\r' "$f")
  [ "$n" = "$c" ] && printf "OK   %s\n" "$f" || printf "FAIL %s %s/%s\n" "$f" "$c" "$n"
done
git add Source/panels/spell_book.cpp Source/panels/spell_book.hpp
```

Expected: 无残留；两个文件行尾 `OK`。

---

## Task 7: `misdat` — 采用上游实现，弃用我方注册表

**Files:**
- Modify: `Source/tables/misdat.cpp`
- Modify: `Source/tables/misdat.h`

我方把 `ParseMissileAddFn` / `ParseMissileProcessFn` 的 if 链改成了 `std::unordered_map` 注册表（144 增 / 119 删，其中 127 行是注册表本身）；上游保留 if 链，只做了 `tl::expected` → `std::expected` 迁移。

**裁决：采用上游的 if 链，弃用我方注册表。** 依据宪章：

- Infra 红线 I2 要求有消费者。注册表的唯一消费者是 `ParseMissile*Fn` 自身——`RegisterMissileAddFn` / `RegisterMissileProcessFn` 两个入口点已在决策 16 中删除，因为它们只有测试调用者
- Infra 红线 I3 要求指名它让哪个未来改动变便宜。入口点删除后无从指名
- 注册表与 if 链功能等价，但注册表使 `misdat.cpp`（上游频繁改动的文件）永久成为冲突面

这是决策 16 的一致延续：既然对外注册能力已判定不该存在，支撑它的内部结构也没有留存理由。

- [ ] **Step 1: 确认我方注册表确实只服务于 Parse 函数**

```bash
cd /home/peng/workspace/DevilutionX
echo "=== 注册表符号的使用者 ==="
grep -n 'g_addFnRegistry\|g_processFnRegistry\|InitDefaultMissileRegistries' \
  Source/tables/misdat.cpp Source/tables/misdat.h
echo "=== 入口点应已不存在（决策 16）==="
grep -rn 'RegisterMissileAddFn\|RegisterMissileProcessFn' Source/ test/ || echo "已无入口点"
```

Expected: 注册表符号只出现在 `misdat.cpp` 内；入口点报「已无入口点」。

若入口点仍存在，停止并报告——说明 merge 把上游或旧版本的声明带了回来，需要重新确认决策 16 的执行状态。

- [ ] **Step 2: 采用上游版本**

```bash
git checkout --theirs Source/tables/misdat.cpp
git checkout --theirs Source/tables/misdat.h
```

此处用 `--theirs` 是安全的：我方对这两个文件的改动只有注册表（已裁决弃用）与 `tl::expected` 签名（上游已迁移），没有需要保留的内容。

- [ ] **Step 3: 确认我方无其他改动被一并丢弃**

```bash
BASE=$(git merge-base origin/master $(cat /tmp/pre_merge_head.txt))
echo "=== 我方对 misdat.cpp 的改动摘要 ==="
git diff "$BASE" "$(cat /tmp/pre_merge_head.txt)" -- Source/tables/misdat.cpp \
  | grep '^+' | grep -v '^+++' \
  | grep -vE 'g_addFnRegistry|g_processFnRegistry|InitDefaultMissileRegistries|^\+$|RegisterMissile' \
  | head -20
echo "=== 我方对 misdat.h 的改动摘要 ==="
git diff "$BASE" "$(cat /tmp/pre_merge_head.txt)" -- Source/tables/misdat.h \
  | grep '^+' | grep -v '^+++' | head -10
```

Expected: 两条命令的输出应只剩注册表相关的残余行、`tl::expected` 签名行，或空。若出现其他实质改动，停止并报告——需要把那部分手工重放到上游版本上。

- [ ] **Step 4: 验证并加入索引**

```bash
grep -nE '^(<<<<<<<|=======|>>>>>>>)|fmt::|tl::expected' \
  Source/tables/misdat.cpp Source/tables/misdat.h || echo "无残留"
grep -c 'if (value == ' Source/tables/misdat.cpp
git add Source/tables/misdat.cpp Source/tables/misdat.h
```

Expected: 无残留；`if (value == ` 计数大于 100（上游的 if 链已就位）。

---

## Task 8: `spelldat.cpp` 的 include 清理

**Files:**
- Modify: `Source/tables/spelldat.cpp`

冲突块是我方新增的两个 include 对上游的删除：

```
<<<<<<< HEAD
#include <expected.hpp>
#include <fmt/format.h>

=======
>>>>>>> origin/master
```

- [ ] **Step 1: 采用上游侧（删除两个 include）**

把该冲突块整体删除，不留任何一行。`<expected.hpp>` 是 tl 的头文件，`<fmt/format.h>` 是 libfmt 的头文件，两者在迁移后都不应存在。

- [ ] **Step 2: 检查该文件是否仍需格式化头文件**

```bash
cd /home/peng/workspace/DevilutionX
echo "=== 该文件是否使用格式化 ==="
grep -n 'FormatRuntime\|std::format' Source/tables/spelldat.cpp || echo "未使用"
echo "=== 是否使用 expected ==="
grep -n 'std::expected\|tl::expected' Source/tables/spelldat.cpp || echo "未使用"
```

若使用 `FormatRuntime` 或 `std::format` 但无对应 include，补上 `#include "utils/format.hpp"` 或 `#include <format>`。
若使用 `std::expected` 但无 include，补上 `#include <expected>`。

- [ ] **Step 3: 验证并加入索引**

```bash
grep -nE '^(<<<<<<<|=======|>>>>>>>)|fmt::|expected\.hpp' Source/tables/spelldat.cpp \
  || echo "无残留"
n=$(wc -l < Source/tables/spelldat.cpp); c=$(grep -c $'\r' Source/tables/spelldat.cpp)
[ "$n" = "$c" ] && echo "行尾 OK" || echo "行尾 FAIL $c/$n"
git add Source/tables/spelldat.cpp
```

---

## Task 9: `spell_list.cpp` 的语义冲突（3 块）

**Files:**
- Modify: `Source/panels/spell_list.cpp`

这是唯一的真实语义冲突。我方用 `BuildSpellListTooltip()`（法术 tooltip v2 系统）替换了上游按类型逐个构造 `InfoString` 的逻辑；上游同时重写了同一区域，改用 `FormatRuntime`、给 `switch` 分支加了大括号、并新增了 `HolyBolt` 与 `Spell Level` 的说明。

**裁决：保留我方的 `BuildSpellListTooltip` 路径**，因为它是 `2026-07-05-spell-tooltip-system-v2-design.md` 的实现，是对上游逻辑的有意替换（该 spec 已标注「已实施」）。上游新增的 `HolyBolt` 与 `Spell Level` 文本由 `spelldesc.tsv` 数据驱动提供，不需要在 C++ 中硬编码。

- [ ] **Step 1: 逐块查看**

```bash
cd /home/peng/workspace/DevilutionX
awk '/^<<<<<<</{n++} {if (n>=1 && n<=3 && /^<<<<<<</,/^>>>>>>>/) print}' \
  Source/panels/spell_list.cpp | head -60
```

前两块的我方侧为空（上游新增了 `InfoString = FormatRuntime(...)` 行，我方删除了它们），第三块是大段替换。

- [ ] **Step 2: 保留 HEAD 侧**

```bash
python3 tools/resolve_ours.py Source/panels/spell_list.cpp
```

Expected: `kept HEAD side of 3 block(s)`。前两块因我方侧为空，效果是删除上游新增的那些行——这正是我方设计意图（tooltip 内容由 `BuildSpellListTooltip` 统一产生）。

- [ ] **Step 3: 迁移剩余 fmt 调用**

```bash
python3 tools/migrate_fmt.py Source/panels/spell_list.cpp
```

Expected: `runtime` 至少 2（第三块里的 `ngettext` 卷轴数与充能数两处）。

- [ ] **Step 4: 检查 switch 结构完整性**

上游给 `switch` 分支加了 `{ }` 与 `} break;`，我方版本是无大括号的 `break;`。保留 HEAD 侧后大括号平衡可能被破坏。

```bash
echo "=== 编译该文件单独验证语法 ==="
g++ -std=c++23 -fsyntax-only \
  -I Source -I 3rdParty -I build/_deps \
  Source/panels/spell_list.cpp 2>&1 | head -20 || true
```

单文件语法检查很可能因缺少大量 include 路径而失败，那不代表结构有问题。更可靠的判据是 Task 11 的库编译。此处只做结构目视检查：

```bash
awk '/case SpellType::/{print NR": "$0}' Source/panels/spell_list.cpp
```

确认每个 `case` 分支的结构自洽（要么全用大括号，要么全不用），且 `SpellType::Invalid` 分支存在。

- [ ] **Step 5: 验证并加入索引**

```bash
grep -nE '^(<<<<<<<|=======|>>>>>>>)|fmt::' Source/panels/spell_list.cpp || echo "无残留"
grep -n 'BuildSpellListTooltip' Source/panels/spell_list.cpp
n=$(wc -l < Source/panels/spell_list.cpp); c=$(grep -c $'\r' Source/panels/spell_list.cpp)
[ "$n" = "$c" ] && echo "行尾 OK" || echo "行尾 FAIL $c/$n"
git add Source/panels/spell_list.cpp
```

Expected: 无残留；`BuildSpellListTooltip` 仍在；行尾 `OK`。

---

## Task 10: 全仓迁移扫尾

**Files:**
- Modify: `Source/spell_tooltip.cpp`、`Source/spell_tooltip.h`、`Source/panels/level_info.cpp` 及任何仍含 `fmt::` / `tl::expected` 的文件
- Modify: `Source/CMakeLists.txt`

冲突文件之外，本分支新增的文件也需迁移：`spell_tooltip.cpp`（21 处 `fmt::` + 1 处 `tl::expected`）、`spell_tooltip.h`（1 处 `tl::expected`）、`panels/level_info.cpp`（1 处 `fmt::`）。

- [ ] **Step 1: 列出所有残留**

```bash
cd /home/peng/workspace/DevilutionX
echo "=== 仍含 fmt:: 的文件 ==="
grep -rl 'fmt::' Source/ test/ 2>/dev/null | tee /tmp/fmt_left.txt
echo "文件数: $(wc -l < /tmp/fmt_left.txt)"
echo "=== 仍含 tl::expected 的文件 ==="
grep -rl 'tl::expected\|expected\.hpp\|tl::make_unexpected' Source/ test/ 2>/dev/null \
  | tee /tmp/tl_left.txt
echo "文件数: $(wc -l < /tmp/tl_left.txt)"
```

- [ ] **Step 2: 批量迁移 fmt**

```bash
[ -s /tmp/fmt_left.txt ] && xargs -a /tmp/fmt_left.txt python3 tools/migrate_fmt.py
```

- [ ] **Step 3: 手工迁移 tl::expected**

`tl::expected` 的迁移无法脚本化，因为 `tl::make_unexpected` 与 `std::unexpected` 的构造语义不同。逐文件按三条规则改：

| 旧 | 新 |
|---|---|
| `#include <expected.hpp>` | `#include <expected>` |
| `tl::expected<T, E>` | `std::expected<T, E>` |
| `tl::make_unexpected(x)` | `std::unexpected(x)` |

```bash
while read -r f; do
  echo "=== $f"
  grep -n 'tl::expected\|expected\.hpp\|tl::make_unexpected' "$f"
done < /tmp/tl_left.txt
```

按上表逐处替换。

- [ ] **Step 4: 补齐 include**

```bash
for f in $(grep -rl 'FormatRuntime' Source/ test/ 2>/dev/null); do
  grep -q '#include "utils/format.hpp"' "$f" || printf "缺 format.hpp: %s\n" "$f"
done
for f in $(grep -rl 'std::format' Source/ test/ 2>/dev/null); do
  grep -q '#include <format>\|#include "utils/format.hpp"' "$f" \
    || printf "缺 <format>: %s\n" "$f"
done
for f in $(grep -rl 'std::expected\|std::unexpected' Source/ test/ 2>/dev/null); do
  grep -q '#include <expected>' "$f" || printf "缺 <expected>: %s\n" "$f"
done
```

对每个报缺的文件补上对应 include，位置按该文件既有 include 的分组与排序惯例。

- [ ] **Step 5: 清除 CMake 中的 fmt 依赖**

```bash
grep -n 'fmt' Source/CMakeLists.txt CMakeLists.txt
```

Expected（merge 后）：上游版本的 `CMakeLists.txt` 已无 fmt。若 `Source/CMakeLists.txt` 仍有 `fmt::fmt` 行，逐行删除——它们是我方在 4 个新文件所属 target 上加的链接依赖，而 `FormatRuntime` 与 `std::format` 都不需要外部库。

删除后确认我方 4 个新文件仍在正确的 target 中注册：

```bash
for f in spell_tooltip.cpp panels/level_info.cpp lua/modules/world.cpp lua/modules/spells.cpp; do
  printf "%-30s %s 处注册\n" "$f" "$(grep -c "$f" Source/CMakeLists.txt)"
done
```

Expected: 四行各 1 处。

- [ ] **Step 6: 验证清零**

```bash
echo "=== fmt:: 应清零 ==="
grep -rn 'fmt::' Source/ test/ || echo "OK"
echo "=== tl:: 应清零 ==="
grep -rn 'tl::expected\|expected\.hpp\|tl::make_unexpected' Source/ test/ || echo "OK"
echo "=== CMake 中 fmt 应清零 ==="
grep -n 'fmt' Source/CMakeLists.txt CMakeLists.txt || echo "OK"
echo "=== C++ 标准应为 23 ==="
grep -n 'CMAKE_CXX_STANDARD' CMakeLists.txt
```

Expected: 前三项 `OK`；标准为 `set(CMAKE_CXX_STANDARD 23)`。

- [ ] **Step 7: 加入索引**

```bash
git add -A Source/ test/ CMakeLists.txt
git status --short | grep -E "^(UU|AA|AU|UA)" || echo "无未解决冲突"
```

---

## Task 11: 门禁 1-2（CMake 配置 + 库编译）

- [ ] **Step 1: 确认无残留冲突**

```bash
cd /home/peng/workspace/DevilutionX
git diff --name-only --diff-filter=U
echo "（空=全部已解决）"
git grep -nE '^(<<<<<<<|=======|>>>>>>>)$' -- Source/ test/ CMake/ CMakeLists.txt \
  || echo "无冲突标记"
```

- [ ] **Step 2: 门禁 1 — CMake 重新配置**

上游改了 C++ 标准与依赖，需要重新配置而非增量。

```bash
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -25
```

Expected: 以 `-- Generating done` 与 `-- Build files have been written to` 结束，无 `fmt::fmt` 找不到目标的错误。

若报缺少依赖（上游可能新增了库），按报错安装后重试；若报 C++23 不支持，停止并报告——工具链前提已在规格中验证过，出现此错说明环境有变。

- [ ] **Step 3: 门禁 2 — 库目标编译**

```bash
timeout 3600 ninja -C build libdevilutionx 2>&1 | grep -E 'error:|FAILED' | head -40
echo "--- 结束 ---"
```

Expected: 无输出。

若有错误，按以下顺序判断：
- `FormatRuntime` 未声明 → 该文件缺 `#include "utils/format.hpp"`，回到 Task 10 Step 4
- `std::expected` 未声明 → 缺 `#include <expected>`
- 格式串编译期检查失败（`std::format` 要求编译期常量格式串）→ 该处应改用 `FormatRuntime` 而非 `std::format`
- 其他 → 逐条处理，不要批量猜测

`std::format` 与 `FormatRuntime` 的选择判据：格式串是字面量则用 `std::format`，来自 `_()` / `ngettext()` / 变量则必须用 `FormatRuntime`。

---

## Task 12: 门禁 3-4（链接 + 测试目标编译）

- [ ] **Step 1: 门禁 3 — 主程序链接**

```bash
cd /home/peng/workspace/DevilutionX
timeout 3600 ninja -C build devilutionx 2>&1 | grep -E 'error:|FAILED|undefined' | head -30
echo "--- 结束 ---"
ls -l build/devilutionx
```

Expected: 无错误输出；`build/devilutionx` 存在。

若出现 `undefined reference`，多为 CMake 中删除 fmt 依赖时误删了其他链接项，检查 Task 10 Step 5 的改动。

- [ ] **Step 2: 门禁 4 — 全部测试目标编译**

benchmark 目标因环境问题必然失败（宪章待立项 #5），用 `-k 0` 跳过继续。

```bash
timeout 3600 ninja -C build -k 0 2>&1 | grep '^FAILED' | sort -u
echo "--- 结束 ---"
```

Expected: 只有 benchmark 目标失败：
```
FAILED: clx_render_benchmark
FAILED: crawl_benchmark
FAILED: dun_render_benchmark
FAILED: light_render_benchmark
FAILED: palette_blending_benchmark
FAILED: path_benchmark
```

若有非 benchmark 目标失败，逐个处理后重试。

- [ ] **Step 3: 确认测试二进制齐全**

上游 41 个 commit 可能增删测试，因此以 `CMake/Tests.cmake` 的当前内容为准。

```bash
awk '/^set\(tests/{f=1;next} /^\)/{f=0} f{gsub(/[ \t\r]/,"");if($0!="")print}' \
  CMake/Tests.cmake > /tmp/testlist.txt
echo "测试目标数: $(wc -l < /tmp/testlist.txt)"
miss=0
while read -r t; do
  [ -x "build/$t" ] || { echo "缺失: $t"; miss=1; }
done < /tmp/testlist.txt
[ $miss -eq 0 ] && echo "全部测试二进制已生成"
```

Expected: 「全部测试二进制已生成」。注意提取时必须剥除 `\r`——`CMake/Tests.cmake` 是 CRLF 文件。

---

## Task 13: 门禁 5（全量测试）与提交

- [ ] **Step 1: 门禁 5 — 全量测试**

```bash
cd /home/peng/workspace/DevilutionX/build
timeout 3600 ctest 2>&1 | tail -12
```

Expected: `100% tests passed, 0 tests failed out of N`。

`N` 的基线是 629。上游 41 个 commit 可能增删测试，若 `N` 不等于 629，记录差值与原因（对比 `CMake/Tests.cmake` 在 merge 前后的条目变化），但门禁判据是**零失败**而非固定数量。

若有失败，逐个判断是迁移引入还是上游既有：

```bash
timeout 3600 ctest --output-on-failure 2>&1 \
  | grep -E "^[[:space:]]+[0-9]+ - .*\(Failed\)" | sed 's/(Failed)//' | awk '{print $3}' | sort
```

对每个失败项，先确认它在 `origin/master` 上是否也失败（可在临时 worktree 上验证），再决定修复还是记录。

- [ ] **Step 2: 验证规格的九项验收标准**

```bash
cd /home/peng/workspace/DevilutionX
echo "【1】无残留冲突标记"
git grep -nE '^(<<<<<<<|=======|>>>>>>>)$' -- Source/ test/ CMake/ CMakeLists.txt || echo "  OK"
echo "【2】fmt 依赖清零"
grep -rn 'fmt::' Source/ test/ || echo "  OK"
echo "【3】tl::expected 清零"
grep -rn 'tl::expected' Source/ test/ || echo "  OK"
echo "【4】CMake 中无 fmt 目标"
grep -n 'fmt' Source/CMakeLists.txt CMakeLists.txt || echo "  OK"
echo "【5】C++ 标准"
grep -n 'CMAKE_CXX_STANDARD' CMakeLists.txt
echo "【6】构建（仅 benchmark 失败）"
timeout 3600 ninja -C build -k 0 2>&1 | grep '^FAILED' | grep -v benchmark || echo "  OK"
echo "【7】全量测试"
(cd build && timeout 3600 ctest 2>&1 | tail -3)
echo "【8】行尾未破坏"
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp \
         Source/lua/modules/monsters.cpp assets/txtdata/classes/classdat.tsv; do
  n=$(wc -l < "$f"); c=$(grep -c $'\r' "$f")
  [ "$n" = "$c" ] && printf "  OK   %s\n" "$f" || printf "  FAIL %s %s/%s\n" "$f" "$c" "$n"
done
[ -f mods/hf/txtdata/spells/spelldat.tsv ] && {
  n=$(wc -l < mods/hf/txtdata/spells/spelldat.tsv)
  c=$(grep -c $'\r' mods/hf/txtdata/spells/spelldat.tsv)
  [ "$n" = "$c" ] && echo "  OK   mods/hf/txtdata/spells/spelldat.tsv" \
                  || echo "  FAIL mods/hf/txtdata/spells/spelldat.tsv $c/$n"
}
echo "【9】行尾回退收益：见 Task 2 Step 5 的记录"
```

- [ ] **Step 3: 把验收标准 9 的实测结论写进规格**

在 `docs/superpowers/specs/2026-07-27-upstream-integration-design.md` 的「事实基础」一节末尾追加：

```markdown
### 探测与实测补记（2026-07-27）

规格初稿的事实基础遗漏了目录重命名：上游把 `mods/Hellfire/` 整体重命名为 `mods/hf/`，且新路径下已有 `spelldat.tsv` 而无 `spelldesc.tsv`。静态内容比对无法发现重命名，需要实际 merge 才暴露。

探测性 merge（`--no-commit` 后 `--abort`）的实测结果：

| 项 | 值 |
|---|---|
| 冲突文件数 | 14 |
| 冲突块总数 | 31 |
| `Source/levels/trigs.cpp` | 15 块 |

**验收标准 9 的结论：行尾回退的收益成立。** `c843f4240` 处理过的 6 个文件中，5 个自动合并成功（含 `diablo.cpp` 与 `qol/stash.cpp`），唯一冲突的 `Source/inv.cpp` 只有 1 个 6 行的行级冲突块。若未做行尾回退，这 6 个文件会全部整文件冲突，约 13000 行。
```

- [ ] **Step 4: 提交 merge**

```bash
git add -A
git status --short | head -20
git commit --no-edit
```

`--no-edit` 采用 git 生成的 merge commit 信息并追加解决说明。若需要自定义，用：

```bash
git commit -F - <<'MSG'
merge: integrate upstream std::format, std::expected and C++23 migrations

Brings in 41 upstream commits (298 files), including the three one-way
migrations: libfmt to std::format (ac2d7fc73), tl::expected to std::expected
(63e85d9b1), and C++23 (7a75c9e66).

31 conflict blocks across 14 files, resolved as follows.

24 mechanical blocks had a single shape: ours set FloatingInfoString via
fmt::format(fmt::runtime(X), args) while upstream set InfoString via
FormatRuntime(X, args). Two orthogonal changes overlapping -- this branch
migrated display targets from InfoString to FloatingInfoString for the
floating info UI, upstream migrated formatting. Resolved by keeping our
target variable and line structure and adopting upstream's call form.

One include conflict in control_infobox.cpp kept both sides.

Four tl::expected blocks migrated to std::expected while preserving this
branch's additions, notably GetSpellRequirementText and CanLearnSpell in
spell_book.

misdat.cpp took upstream's if-chain and dropped this branch's 127-line
registry. The registry's only consumer was ParseMissile*Fn itself once the
Register*Fn entry points were deleted in charter decision 16, so it fails
Infra red lines I2 and I3, and it made a file upstream touches often into a
permanent conflict surface.

spell_list.cpp kept this branch's BuildSpellListTooltip path, which the spell
tooltip v2 spec deliberately substitutes for upstream's per-type InfoString
construction. Upstream's new HolyBolt and Spell Level strings are supplied by
spelldesc.tsv instead of hardcoded C++.

Upstream renamed mods/Hellfire to mods/hf. Our spelldesc.tsv moved to the new
path; our spelldat.tsv changes were replayed onto upstream's version there.

Two migration helpers are kept in tools/ because every future integration
needs them.
MSG
```

- [ ] **Step 5: 更新规格状态并提交**

把规格的「状态」一节改为：

```markdown
## 7. 状态

**已实施。** 2026-07-27 完成，九项验收标准全部通过。
```

```bash
git add docs/superpowers/specs/2026-07-27-upstream-integration-design.md
git commit -m "docs: mark upstream integration as implemented

All nine acceptance criteria pass. The fact basis gains a note on the
mods/Hellfire to mods/hf rename, which static content comparison could not
have found, and records the line-ending revert result: five of the six files
it touched merged cleanly and inv.cpp had a single six-line conflict, against
roughly 13000 lines of whole-file conflict without it."
```

- [ ] **Step 6: 推送**

```bash
git push myrepo feature/qol-upgrades
git rev-parse HEAD myrepo/feature/qol-upgrades | uniq -c
```

Expected: `uniq -c` 输出以 `2` 开头的单行，表示两者一致。

---

## 自审

**规格覆盖**（对照规格第 4 节方案与第 6 节验收标准）：

| 规格条目 | 对应任务 |
|---|---|
| 集成方式 merge 非 rebase | Task 2 Step 3 |
| 合入先行，解冲突时迁移 | Task 4–10 |
| 门禁 1 CMake 配置 | Task 11 Step 2 |
| 门禁 2 库编译 | Task 11 Step 3 |
| 门禁 3 主程序链接 | Task 12 Step 1 |
| 门禁 4 测试目标编译 | Task 12 Step 2–3 |
| 门禁 5 全量测试 | Task 13 Step 1 |
| 验收标准 1–8 | Task 13 Step 2 |
| 验收标准 9（行尾回退实测） | Task 2 Step 5 记录，Task 13 Step 3 写入规格 |
| 迁移规则表（5 条） | Task 4（前两条脚本化）、Task 10 Step 3（`tl::expected`）；`fmt::format_int` / `fmt::join` / `dynamic_format_arg_store` 由 Task 10 Step 1 的残留扫描兜住 |

规格未覆盖而计划补上的：目录重命名（Task 3）、`misdat` 注册表裁决（Task 7）。两者都在 Task 13 Step 3 写回规格。

**占位符扫描**：无 TBD/TODO。每个脚本步骤给出完整可运行代码，每个手工步骤给出确切的替换后代码，每个命令步骤给出预期输出。Task 3 Step 3 与 Task 11 Step 3 含条件分支，但每个分支都给出了具体命令与判据，不是「视情况处理」。

**类型一致性**：
- `tools/migrate_fmt.py` 与 `tools/resolve_ours.py` 的函数签名在 Task 1 定义，Task 4/5/9/10 的调用方式与之一致（均为 `python3 tools/X.py <文件列表>`）
- `std::expected<void, std::string> InitSpellBook()` 在 Task 6 Step 1（定义）与 Step 2（声明）中签名一致
- `FormatRuntime(X, args)` 的参数顺序在 Task 4、5、6、9 中一致：格式串在前，参数在后
- `/tmp/pre_merge_head.txt` 在 Task 2 Step 1 写入，Task 7 Step 3 读取
- `/tmp/conflicts.txt` 在 Task 2 Step 4 写入，Task 2 Step 5 读取
