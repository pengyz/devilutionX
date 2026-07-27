# Better D1 宪章清理 — 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修复损坏的测试构建、回退行尾污染、重组设计文档、撤销四项违反宪章的代码改动、删除七处死代码，使 Base 层的承诺（平衡完全等于原版）成立且可验证。

**Architecture:** 本计划全部是删除与回退，不新增任何功能。执行顺序由依赖关系决定：测试构建必须先修好，否则后续每一步都无法验证；行尾必须紧接着回退，否则后续 commit 会把噪声埋得更深。文档重组与代码撤销之间无依赖。

**Tech Stack:** C++20、CMake + Ninja、GoogleTest、Perl（行尾转换）

**Spec:** `docs/superpowers/specs/2026-07-27-better-d1-design-charter.md`

---

## 环境前提

构建目录已存在并已配置：

- 路径 `build`，生成器 Ninja，`CMAKE_BUILD_TYPE=Debug`，`BUILD_TESTING=ON`
- 单目标构建：`ninja -C build <target>`
- 全量构建：`ninja -C build`
- 运行单个测试：`./build/<target>`
- 运行全部测试：`ninja -C build && (cd build && ctest --output-on-failure)`

`CMake/Tests.cmake` 变更后 Ninja 会自动重跑 CMake，无需手动 configure。

---

## 文件结构

### 删除的文件

| 文件 | 职责（删除原因） |
|---|---|
| `test/world_state_test.cpp` | 测试已删除的 `world_state.h`，导致测试构建失败 |
| `test/gold_drop_test.cpp` | 唯一断言为 `EXPECT_TRUE(true)`，违反宪章禁令 6 |
| `test/room_decoration_test.cpp` | 同上 |
| `test/stack_limit_test.cpp` | 三个测试是 `consumable_stack_test.cpp` 前三个的重复，且全部断言被撤销的职业加成 |
| `test/light_suppression_test.cpp` | 测试被撤销的光照压制 |
| `test/monster_activation_test.cpp` | 测试被删除的 `GetMonsterActivationRadius` |
| `test/combat_integration_test.cpp` | 整篇只测上述两个被删函数，非独立集成测试 |
| `test/quest_reward_test.cpp` | 测试被删除的三个任务奖励函数 |
| `docs/superpowers/specs/better-d1/` 整个目录 | 见任务 3 |
| `docs/superpowers/plans/2026-07-07-better-d1-implementation.md` | 内容错误，不归档 |

### 修改的文件

| 文件 | 修改内容 |
|---|---|
| `CMake/Tests.cmake` | 移除 8 个测试目标条目 |
| `Source/items.cpp` | 撤销金币 +25%、职业堆叠加成、`floatingInfoBox` 硬编码 |
| `Source/items.h` | `GetMaxStackCount` 参数改为无名 |
| `Source/lighting.cpp` | 删除两个压制函数，回退 1 处调用点 |
| `Source/lighting.h` | 删除两个声明与 `struct Player;` 前向声明 |
| `Source/msg.cpp` | 回退 1 处调用点 |
| `Source/loadsave.cpp` | 回退 1 处调用点 |
| `Source/player.cpp` | 回退 2 处调用点 |
| `Source/monster.cpp` | 删除 `GetMonsterActivationRadius`、`FallbackAiImpl`、`RegisterAiFunction` |
| `Source/monster.h` | 删除 `GetMonsterActivationRadius` 声明 |
| `Source/tables/monstdat.h` | 删除 `RegisterAiFunction` 声明，修正注释 |
| `Source/tables/misdat.cpp` | 删除两个 `Register*Fn` |
| `Source/tables/misdat.h` | 删除两个声明 |
| `Source/stores.cpp` | 删除三个任务奖励函数 |
| `Source/stores.h` | 删除三个声明及其文档注释 |
| `Source/diablo.cpp`、`Source/inv.cpp`、`Source/qol/stash.cpp`、`Source/lua/modules/monsters.cpp` | 行尾 LF → CRLF |
| `test/consumable_stack_test.cpp` | 删除 6 个测试，修正 2 个断言 |
| `test/ai_registry_test.cpp` | 删除 3 个测试及其辅助函数 |
| `test/missile_registry_test.cpp` | 删除 2 个测试及其辅助函数 |
| `test/lua_integration_test.cpp` | 删除 2 个测试及其辅助函数 |

### 新建的文件

| 文件 | 职责 |
|---|---|
| `docs/superpowers/archive/specs/` | 归档被取代或含错误的设计规格 |
| `docs/superpowers/archive/plans/` | 归档已执行的实施计划 |

---

## Task 1: 修复测试构建

**Files:**
- Delete: `test/world_state_test.cpp`
- Modify: `CMake/Tests.cmake`

- [ ] **Step 1: 确认构建当前是坏的**

Run:
```bash
ninja -C build world_state_test 2>&1 | grep -E "error|FAILED"
```

Expected:
```
FAILED: CMakeFiles/world_state_test.dir/test/world_state_test.cpp.o
test/world_state_test.cpp:5:10: fatal error: world_state.h: No such file or directory
```

（错误信息可能为中文「没有那个文件或目录」，取决于 locale。）

- [ ] **Step 2: 确认被引用的头文件确实不存在**

Run:
```bash
ls Source/world_state.h Source/world_state.cpp; git log --oneline -1 ddda610f6
```

Expected: `ls` 报两个文件都不存在；`git log` 输出 `ddda610f6 chore: remove dead world_state skeleton code (no consumers)`

- [ ] **Step 3: 删除测试文件**

```bash
git rm test/world_state_test.cpp
```

- [ ] **Step 4: 从 Tests.cmake 移除注册项**

在 `CMake/Tests.cmake` 的 `set(tests ...)` 列表中删除这一行：

```cmake
  world_state_test
```

删除后该列表末尾应为：

```cmake
  room_decoration_test
  quest_reward_test
  combat_integration_test
)
```

- [ ] **Step 5: 验证测试构建恢复**

Run:
```bash
ninja -C build 2>&1 | tail -5
```

Expected: 构建成功，无 `FAILED` 行。

- [ ] **Step 6: 运行全部测试建立基线**

Run:
```bash
cd build && ctest --output-on-failure 2>&1 | tail -15
```

Expected: 全部通过。若有失败，记录失败的测试名——它们是本计划之前就存在的问题，需在继续前报告，不要静默跳过。

- [ ] **Step 7: 提交**

```bash
git add CMake/Tests.cmake test/world_state_test.cpp
git commit -m "fix(test): remove dangling world_state_test

Source/world_state.{cpp,h} were deleted in ddda610f6 but the test file
and its CMake registration were left behind, breaking the test build
since 2026-07-08."
```

---

## Task 2: 回退行尾污染

四个文件被从 CRLF 转为 LF，违反 `.editorconfig`（`end_of_line = crlf`）与 `.gitattributes`（`* -text`，注释原文「Do not let git change line endings」）。

**Files:**
- Modify: `Source/diablo.cpp`
- Modify: `Source/inv.cpp`
- Modify: `Source/qol/stash.cpp`
- Modify: `Source/lua/modules/monsters.cpp`

- [ ] **Step 1: 记录修复前的噪声量**

Run:
```bash
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  t=$(git diff origin/master...HEAD --numstat -- "$f" | awk '{print $1+$2}')
  w=$(git diff origin/master...HEAD --numstat -w -- "$f" | awk '{print $1+$2}')
  printf "%-40s 总改动=%s 实际=%s\n" "$f" "$t" "${w:-0}"
done
```

Expected:
```
Source/diablo.cpp                        总改动=7063 实际=31
Source/inv.cpp                           总改动=4651 实际=301
Source/qol/stash.cpp                     总改动=1556 实际=84
Source/lua/modules/monsters.cpp           总改动=150 实际=40
```

- [ ] **Step 2: 确认四个文件当前为纯 LF**

Run:
```bash
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  printf "%-40s 总行=%s CRLF行=%s\n" "$f" "$(wc -l < "$f")" "$(grep -c $'\r' "$f" || echo 0)"
done
```

Expected: 四个文件 CRLF 行数均为 `0`。

- [ ] **Step 3: 备份内容用于事后比对**

```bash
mkdir -p /tmp/lf-revert
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  cp "$f" "/tmp/lf-revert/$(basename "$f").bak"
done
```

- [ ] **Step 4: 转换为 CRLF**

该命令幂等：`\r?\n\z` 匹配行尾的 LF 或 CRLF，统一替换为 CRLF，重复执行不会产生 `\r\r\n`。

```bash
perl -pi -e 's/\r?\n\z/\r\n/' \
  Source/diablo.cpp \
  Source/inv.cpp \
  Source/qol/stash.cpp \
  Source/lua/modules/monsters.cpp
```

- [ ] **Step 5: 验证行尾已全部转换且内容未变**

Run:
```bash
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  b="/tmp/lf-revert/$(basename "$f").bak"
  n=$(wc -l < "$f"); c=$(grep -c $'\r' "$f")
  if [ "$n" = "$c" ] && diff --strip-trailing-cr -q "$b" "$f" > /dev/null; then
    echo "OK   $f （$c/$n 行 CRLF，内容一致）"
  else
    echo "FAIL $f"
  fi
done
```

Expected: 四行全部为 `OK`。

- [ ] **Step 6: 验证噪声已清除**

Run:
```bash
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  t=$(git diff origin/master...HEAD --numstat -- "$f" | awk '{print $1+$2}')
  printf "%-40s 总改动=%s\n" "$f" "${t:-0}"
done
```

Expected: 与 Step 1 的「实际」列一致，即 31 / 301 / 84 / 40（±数行，取决于 diff 算法对相邻改动的合并）。判定标准：每个文件总改动 < 400。

- [ ] **Step 7: 验证构建未受影响**

Run:
```bash
ninja -C build 2>&1 | tail -3
```

Expected: 构建成功。

- [ ] **Step 8: 清理备份并提交**

```bash
rm -rf /tmp/lf-revert
git add Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp
git commit -m "style: restore CRLF line endings in four files

.editorconfig requires end_of_line = crlf and .gitattributes declares
'* -text', so stored bytes are final. These four files had been converted
to LF, producing 12964 lines of whitespace-only diff noise (diablo.cpp
alone showed 7063 changed lines of which 31 were real) and guaranteeing
line-by-line conflicts on every future upstream merge of the two most
active files in the project."
```

---

## Task 3: 文档重组

**Files:**
- Create: `docs/superpowers/archive/specs/`、`docs/superpowers/archive/plans/`
- Delete: 7 项
- Move: 12 项
- Modify: 4 项

- [ ] **Step 1: 建立归档目录**

```bash
mkdir -p docs/superpowers/archive/specs docs/superpowers/archive/plans
```

- [ ] **Step 2: 删除零可落地内容的文档**

```bash
git rm docs/superpowers/specs/better-d1/designs/narrative-system.md \
       docs/superpowers/specs/better-d1/designs/combat-system.md \
       docs/superpowers/specs/better-d1/designs/living-dungeon.md \
       docs/superpowers/specs/better-d1/designs/resource-system.md \
       docs/superpowers/specs/better-d1/design-decision-template.md \
       docs/superpowers/specs/better-d1/README.md \
       docs/superpowers/plans/2026-07-07-better-d1-implementation.md
```

删除理由逐项对应宪章第 5 节。`better-d1-implementation.md` 不归档：它含两个冲突的 `GetMaxStackCount` 签名、两处 `EXPECT_TRUE(true)` 占位测试，以及勾选了「All test code included」的假自审清单，留着会被当参考。

- [ ] **Step 3: 归档被取代的规格**

```bash
git mv docs/superpowers/specs/2026-07-05-skill-descriptions-design.md \
       docs/superpowers/archive/specs/
git mv docs/superpowers/specs/2026-07-06-better-d1-design-system.md \
       docs/superpowers/archive/specs/
git mv docs/superpowers/specs/2026-07-06-wire-up-skeleton-code.md \
       docs/superpowers/archive/specs/
```

- [ ] **Step 4: 归档含已核实错误的规格**

```bash
git mv docs/superpowers/specs/better-d1/designs/environment-lighting.md \
       docs/superpowers/archive/specs/environment-lighting.md
git mv docs/superpowers/specs/better-d1/designs/consumable-system.md \
       docs/superpowers/archive/specs/consumable-system.md
git mv docs/superpowers/specs/better-d1/designs/spell-system.md \
       docs/superpowers/archive/specs/spell-system.md
git mv docs/superpowers/specs/2026-07-07-consumable-stacking-design.md \
       docs/superpowers/archive/specs/
```

- [ ] **Step 5: 归档已执行的实施计划**

```bash
git mv docs/superpowers/plans/2026-06-29-floating-info-ui.md \
       docs/superpowers/plans/2026-07-05-skill-descriptions.md \
       docs/superpowers/plans/2026-07-05-spell-tooltip-system-v2.md \
       docs/superpowers/plans/2026-07-06-skill-descriptions-v2.md \
       docs/superpowers/plans/2026-07-07-consumable-stacking.md \
       docs/superpowers/archive/plans/
```

- [ ] **Step 6: 删除空的 better-d1 目录**

```bash
rmdir docs/superpowers/specs/better-d1/designs docs/superpowers/specs/better-d1
ls docs/superpowers/specs/
```

Expected: `ls` 输出仅含五个 `.md` 文件（宪章 + 四篇保留），无 `better-d1` 目录。

- [ ] **Step 7: 为归档文档加取代说明**

在以下每个文件的**第一行之前**插入对应的横幅。

`docs/superpowers/archive/specs/2026-07-05-skill-descriptions-design.md`：

```markdown
> **已废弃**：被 `2026-07-05-spell-tooltip-system-v2-design.md` 与 `2026-07-06-skill-descriptions-v2-design.md` 取代。

```

`docs/superpowers/archive/specs/2026-07-06-better-d1-design-system.md`：

```markdown
> **已废弃**：被 `specs/2026-07-27-better-d1-design-charter.md` 取代。本文档的五支柱与红线清单为不可判定版本，且与 `design-decision-template.md` 中的另一套五支柱冲突。

```

`docs/superpowers/archive/specs/2026-07-06-wire-up-skeleton-code.md`：

```markdown
> **已废弃**：一次性清理任务，光照与法术需求文本的接线已执行；`world_state` 与 `passiveDescription` 的删除已执行；其接线 `GetMonsterActivationRadius` 的计划项已随光照压制撤销而作废。

```

`docs/superpowers/archive/specs/environment-lighting.md`：

```markdown
> **已废弃 — 内容含已核实错误，勿作参考。**
>
> - 表格以玩家基础光半径 8 计算，实际为 10（`Source/player.cpp:2320`）
> - 「火把半径」列为虚构：静态火把烘焙在地图数据中，代码从未压制
> - 标注「已实现」的怪物激活半径零生产调用者
> - 压制层段未覆盖 17–20 层（Nest），落入 `default` 返回 `1.0f`，比 Hell 的 `0.6f` 更亮
>
> 对应机制已于宪章清理中撤销。重写见待立项清单。

```

`docs/superpowers/archive/specs/consumable-system.md`：

```markdown
> **已废弃 — 内容含已核实错误，勿作参考。**
>
> 「掉落设计」一节为回城/识别卷轴的掉落提供设计理由（「回城卷轴稀有 → 决策重量」），与「取消符文及伤害卷轴掉落」的决策方向相反。
>
> 重写见待立项清单中的消耗品经济重构。

```

`docs/superpowers/archive/specs/spell-system.md`：

```markdown
> **已废弃 — 内容含已核实错误，勿作参考。**
>
> - ManaShield 写作需 45 魔法，实际 25（`assets/txtdata/spells/spelldat.tsv:minIntelligence`）
> - Elemental 写作需 70，实际 68
> - Fireball（48）与 Mana Shield（25）被列为法师专属，实际魔法上限 50 的战士均可学
>
> 重写见待立项清单中的法术实用性。

```

`docs/superpowers/archive/specs/2026-07-07-consumable-stacking-design.md`：

```markdown
> **已废弃 — 内容含已裁决撤销的设计，勿作参考。**
>
> - 职业差异化堆叠上限（战士 +3/+1、法师 −2）已撤销：它违反深度层红线 11，法师是最依赖蓝瓶的职业而被削减上限，产出的是跑腿而非取舍
> - 腰带堆叠形态已裁决改为背包堆叠：腰带 8 格的红瓶/蓝瓶配比是真决策，堆叠取消了它，违反底座层支柱 B2
>
> 重写见待立项清单优先级 2。

```

五篇已执行的实施计划（`docs/superpowers/archive/plans/` 下全部）统一插入：

```markdown
> **已执行并归档。**

```

- [ ] **Step 8: 为保留的四篇补分类与状态标注**

在以下每个文件的**第一行之前**插入。

`docs/superpowers/specs/2026-06-29-floating-info-ui-design.md`：

```markdown
> **分类**：Base　**状态**：已实施　**评判基准**：`2026-07-27-better-d1-design-charter.md`

```

`docs/superpowers/specs/2026-07-05-spell-tooltip-system-v2-design.md`：

```markdown
> **分类**：Base　**状态**：已实施　**评判基准**：`2026-07-27-better-d1-design-charter.md`
>
> 记账口径：本系统属 Base（玩家看到不同的 tooltip），但体验收益弱——D1 的法术获取是书本驱动的，玩家学他捡到的那本，知道下一级伤害变化不改变决策。其主要实际价值是让 tooltip 可数据驱动配置，即降低未来改动成本。

```

`docs/superpowers/specs/2026-07-05-spell-tooltip-ui-language.md`：

```markdown
> **分类**：规范类（豁免宪章第 4 节 7 段结构，仍受禁令清单约束）　**状态**：生效　**评判基准**：`2026-07-27-better-d1-design-charter.md`

```

`docs/superpowers/specs/2026-07-06-skill-descriptions-v2-design.md`：

```markdown
> **分类**：Base　**状态**：已实施　**评判基准**：`2026-07-27-better-d1-design-charter.md`

```

- [ ] **Step 9: 验证目录结构**

Run:
```bash
find docs/superpowers -name "*.md" | sort
```

Expected:
```
docs/superpowers/archive/plans/2026-06-29-floating-info-ui.md
docs/superpowers/archive/plans/2026-07-05-skill-descriptions.md
docs/superpowers/archive/plans/2026-07-05-spell-tooltip-system-v2.md
docs/superpowers/archive/plans/2026-07-06-skill-descriptions-v2.md
docs/superpowers/archive/plans/2026-07-07-consumable-stacking.md
docs/superpowers/archive/specs/2026-07-05-skill-descriptions-design.md
docs/superpowers/archive/specs/2026-07-06-better-d1-design-system.md
docs/superpowers/archive/specs/2026-07-06-wire-up-skeleton-code.md
docs/superpowers/archive/specs/2026-07-07-consumable-stacking-design.md
docs/superpowers/archive/specs/consumable-system.md
docs/superpowers/archive/specs/environment-lighting.md
docs/superpowers/archive/specs/spell-system.md
docs/superpowers/plans/2026-07-27-charter-cleanup.md
docs/superpowers/specs/2026-06-29-floating-info-ui-design.md
docs/superpowers/specs/2026-07-05-spell-tooltip-system-v2-design.md
docs/superpowers/specs/2026-07-05-spell-tooltip-ui-language.md
docs/superpowers/specs/2026-07-06-skill-descriptions-v2-design.md
docs/superpowers/specs/2026-07-27-better-d1-design-charter.md
```

- [ ] **Step 10: 验证支柱名称只在宪章中出现**

Run:
```bash
grep -rl "稀缺是美德\|环境是角色\|不安感是核心体验" docs/superpowers/specs/ docs/superpowers/plans/
```

Expected: 无输出。（旧支柱名称只应存在于 `archive/` 与宪章的「废弃的旧支柱」表中。）

- [ ] **Step 11: 提交**

```bash
git add -A docs/superpowers
git commit -m "docs: reorganize design docs per charter verdicts

Delete seven documents with no actionable content: narrative-system,
combat-system, living-dungeon and resource-system were wish lists;
design-decision-template is folded into the charter; better-d1/README
was a drifted index; better-d1-implementation.md is not archived because
its content is wrong.

Archive twelve documents. Four carry an errata banner because their
mechanisms still matter but their content has verified errors.

Annotate the four retained specs with classification and status.

Flatten the tree: better-d1/ is gone, all specs live in specs/ with date
prefixes, the directory listing is the index."
```

---

## Task 4: 撤销金币掉落 +25%

**Files:**
- Modify: `Source/items.cpp`
- Delete: `test/gold_drop_test.cpp`
- Modify: `CMake/Tests.cmake`

分类判定：触及判定树第 2 条（掉落概率或掉落表构成）→ Depth。当前无条件生效于 Base 层，违反 B3。

- [ ] **Step 1: 确认改动存在**

Run:
```bash
grep -n "rndv \* 5 / 4" Source/items.cpp
```

Expected: `3178:	rndv = rndv * 5 / 4;`

- [ ] **Step 2: 删除加成**

在 `Source/items.cpp` 的 `GetItemAttrs` 中，将：

```cpp
	if (leveltype == DTYPE_HELL)
		rndv += rndv / 8;

	// Better D1: Increase gold drop rate by 25% to compensate for reduced consumable drops
	rndv = rndv * 5 / 4;

	item._ivalue = std::min(rndv, GOLD_MAX_LIMIT);
```

改为：

```cpp
	if (leveltype == DTYPE_HELL)
		rndv += rndv / 8;

	item._ivalue = std::min(rndv, GOLD_MAX_LIMIT);
```

- [ ] **Step 3: 删除占位测试**

`test/gold_drop_test.cpp` 的唯一断言是 `EXPECT_TRUE(true)`，违反宪章禁令 6。

```bash
git rm test/gold_drop_test.cpp
```

在 `CMake/Tests.cmake` 的 `set(tests ...)` 中删除这一行：

```cmake
  gold_drop_test
```

- [ ] **Step 4: 验证撤销生效**

Run:
```bash
grep -c "rndv \* 5 / 4" Source/items.cpp || echo "0 matches"
git diff origin/master...HEAD -- Source/items.cpp | grep -c "rndv" || echo "0 gold diff lines"
```

Expected: 第一条输出 `0 matches`；第二条输出 `0 gold diff lines`（金币计算已与上游一致）。

- [ ] **Step 5: 构建并运行相关测试**

Run:
```bash
ninja -C build items_test && ./build/items_test
```

Expected: PASS

- [ ] **Step 6: 提交**

```bash
git add Source/items.cpp CMake/Tests.cmake test/gold_drop_test.cpp
git commit -m "revert(items): remove 25% gold drop increase

Classified Depth by charter rule 2 (drop values) but was active
unconditionally in the Base layer, violating pillar B3.

The compensation it was meant to provide is also the wrong shape: removing
scroll drops hurts the early game most, while scaling gold income by a
percentage adds the least where compensation is needed and the most where
it is not. The economy rework will derive its own compensation.

gold_drop_test asserted only EXPECT_TRUE(true), violating prohibition 6."
```

---

## Task 5: 撤销职业差异化堆叠上限

**Files:**
- Modify: `Source/items.cpp`
- Modify: `Source/items.h:592`
- Delete: `test/stack_limit_test.cpp`
- Modify: `test/consumable_stack_test.cpp`
- Modify: `CMake/Tests.cmake`

分类判定：触及判定树第 6 条（战斗中可即时使用的资源量）→ Depth。另违反深度层红线 11：法师少带 2 个蓝瓶没有替代策略，只能多跑一趟。

- [ ] **Step 1: 确认当前实现**

Run:
```bash
sed -n '/^int GetMaxStackCount/,/^}/p' Source/items.cpp
```

Expected: 输出含 `case HeroClass::Warrior:` 与 `case HeroClass::Sorcerer:` 分支的完整函数。

- [ ] **Step 2: 简化实现，移除职业加成**

在 `Source/items.cpp` 中，将整个函数：

```cpp
int GetMaxStackCount(const Item &item, const Player &player)
{
	// 根据物品类型确定基础堆叠数
	int base;
	switch (item._iMiscId) {
	case IMISC_HEAL:
	case IMISC_MANA:
	case IMISC_REJUV:
	case IMISC_FULLREJUV:
		base = 5;
		break;
	case IMISC_SCROLL:
		base = 3;
		break;
	default:
		return 1; // 不可堆叠
	}

	// 职业加成
	switch (player._pClass) {
	case HeroClass::Warrior:
		if (item._iMiscId == IMISC_HEAL || item._iMiscId == IMISC_MANA)
			base += 3; // 战士药水堆叠+3
		else if (item._iMiscId == IMISC_SCROLL)
			base += 1; // 战士卷轴堆叠+1
		break;
	case HeroClass::Sorcerer:
		if (item._iMiscId == IMISC_HEAL || item._iMiscId == IMISC_MANA)
			base -= 2; // 法师药水堆叠-2
		break;
	default:
		break;
	}

	return base;
}
```

替换为：

```cpp
int GetMaxStackCount(const Item &item, const Player &)
{
	switch (item._iMiscId) {
	case IMISC_HEAL:
	case IMISC_MANA:
	case IMISC_REJUV:
	case IMISC_FULLREJUV:
		return 5;
	case IMISC_SCROLL:
		return 3;
	default:
		return 1;
	}
}
```

`Player` 参数保留但改为无名：两个生产调用点（`Source/inv.cpp:502`、`Source/inv.cpp:1350`）与 16 个测试调用点无需改动，且待立项的背包堆叠改造仍可能需要该参数。无名参数是本仓库既有写法（见 `test/ai_registry_test.cpp` 的 `void TestFirstFn(Monster &)`）。

- [ ] **Step 3: 更新头文件注释**

在 `Source/items.h:592` 处，将：

```cpp
int GetMaxStackCount(const Item &item, const Player &player);
```

替换为：

```cpp
/**
 * @brief Maximum stack count for a consumable. Potions 5, scrolls 3, others 1.
 * @param item The item to query
 * @param player Unused. Retained for call-site stability; class differentiation
 *               was reverted because it produced round trips, not tradeoffs.
 */
int GetMaxStackCount(const Item &item, const Player &player);
```

- [ ] **Step 4: 删除重复的 stack_limit_test**

`test/stack_limit_test.cpp` 的三个测试（`WarriorHasHigherPotionStackLimit`、`SorcererHasLowerPotionStackLimit`、`RogueHasBaseStackLimit`）与 `test/consumable_stack_test.cpp` 的前三个职业测试重复，且全部断言被撤销的加成。

```bash
git rm test/stack_limit_test.cpp
```

在 `CMake/Tests.cmake` 的 `set(tests ...)` 中删除这一行：

```cmake
  stack_limit_test
```

- [ ] **Step 5: 修正 consumable_stack_test 中的职业测试**

在 `test/consumable_stack_test.cpp` 中，删除这五个测试（原第 82–145 行区间），它们断言的加成已不存在，或被下方的新测试涵盖：

- `WarriorHasHigherPotionStackLimit`（断言 8）
- `SorcererHasLowerPotionStackLimit`（断言 3）
- `RogueHasBasePotionStackLimit`（断言 5，被新测试涵盖）
- `WarriorHasHigherScrollStackLimit`（断言 4）
- `ScrollBaseStackLimit`（断言 3，被新测试涵盖）

用两个不区分职业的测试替代，插入到 `NonStackableItemReturnsOne` 之前：

```cpp
TEST_F(ConsumableStackTest, PotionStackLimitIsClassIndependent)
{
	Item potion;
	potion._itype = ItemType::Misc;
	potion._iMiscId = IMISC_HEAL;

	for (HeroClass heroClass : { HeroClass::Warrior, HeroClass::Rogue, HeroClass::Sorcerer }) {
		Player &player = Players[0];
		player._pClass = heroClass;
		EXPECT_EQ(GetMaxStackCount(potion, player), 5)
		    << "class " << static_cast<int>(heroClass);
	}
}

TEST_F(ConsumableStackTest, ScrollStackLimitIsClassIndependent)
{
	Item scroll;
	scroll._itype = ItemType::Misc;
	scroll._iMiscId = IMISC_SCROLL;

	for (HeroClass heroClass : { HeroClass::Warrior, HeroClass::Rogue, HeroClass::Sorcerer }) {
		Player &player = Players[0];
		player._pClass = heroClass;
		EXPECT_EQ(GetMaxStackCount(scroll, player), 3)
		    << "class " << static_cast<int>(heroClass);
	}
}
```

- [ ] **Step 6: 修正 BeltStackingRespectsMaxCount**

该测试依赖法师上限为 3。将其中：

```cpp
	Player &player = Players[0];
	player._pClass = HeroClass::Sorcerer; // Potion max stack is 3
```

改为：

```cpp
	Player &player = Players[0];
	player._pClass = HeroClass::Sorcerer; // Potion max stack is 5 for every class
```

并将同一测试内的：

```cpp
	potion._iStackCount = 3;
```

改为：

```cpp
	potion._iStackCount = 5;
```

以及：

```cpp
	// Verify slot is at max capacity
	EXPECT_EQ(GetMaxStackCount(potion, player), 3);
	EXPECT_EQ(player.SpdList[0]._iStackCount, 3);
```

改为：

```cpp
	// Verify slot is at max capacity
	EXPECT_EQ(GetMaxStackCount(potion, player), 5);
	EXPECT_EQ(player.SpdList[0]._iStackCount, 5);
```

- [ ] **Step 7: 修正 FullStackingWorkflow**

该测试依赖战士上限为 8。将其中：

```cpp
	int maxStack = GetMaxStackCount(potion, warrior);
	EXPECT_EQ(maxStack, 8);
```

改为：

```cpp
	int maxStack = GetMaxStackCount(potion, warrior);
	EXPECT_EQ(maxStack, 5);
```

将循环：

```cpp
	// Step 2: Simulate stacking 7 more potions into slot 0
	for (int i = 1; i < 8; i++) {
```

改为：

```cpp
	// Step 2: Simulate stacking 4 more potions into slot 0
	for (int i = 1; i < 5; i++) {
```

将：

```cpp
	// Step 3: Verify full stack
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 8);
```

改为：

```cpp
	// Step 3: Verify full stack
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 5);
```

将：

```cpp
	// Step 4: Use one potion (decrement stack)
	EXPECT_GT(warrior.SpdList[0]._iStackCount, 1);
	warrior.SpdList[0]._iStackCount--;
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 7);
```

改为：

```cpp
	// Step 4: Use one potion (decrement stack)
	EXPECT_GT(warrior.SpdList[0]._iStackCount, 1);
	warrior.SpdList[0]._iStackCount--;
	EXPECT_EQ(warrior.SpdList[0]._iStackCount, 4);
```

- [ ] **Step 8: 删除两个占位测试**

`WarriorPassiveDescription` 与 `SorcererPassiveDescription` 的唯一断言是 `EXPECT_TRUE(true)`，且描述的正是被撤销的职业被动。从 `test/consumable_stack_test.cpp` 中整块删除：

```cpp
TEST_F(ConsumableStackTest, WarriorPassiveDescription)
{
	// 战士应该有"物品携带"被动
	// 这是占位符 - 实际实现取决于技能系统
	EXPECT_TRUE(true);
}

TEST_F(ConsumableStackTest, SorcererPassiveDescription)
{
	// 法师应该有"轻装出行"被动
	// 这是占位符 - 实际实现取决于技能系统
	EXPECT_TRUE(true);
}
```

- [ ] **Step 9: 构建并运行测试**

Run:
```bash
ninja -C build consumable_stack_test inv_test items_test \
  && ./build/consumable_stack_test && ./build/inv_test && ./build/items_test
```

Expected: 三个测试全部 PASS。

- [ ] **Step 10: 验证职业分支已清除**

Run:
```bash
sed -n '/^int GetMaxStackCount/,/^}/p' Source/items.cpp | grep -c "HeroClass" || echo "0 HeroClass branches"
```

Expected: `0 HeroClass branches`

- [ ] **Step 11: 提交**

```bash
git add Source/items.cpp Source/items.h CMake/Tests.cmake \
        test/stack_limit_test.cpp test/consumable_stack_test.cpp
git commit -m "revert(items): remove class-differentiated stack limits

Unified to 5 for potions and 3 for scrolls. Classified Depth by charter
rule 6 (resources usable in combat) and it fails depth red line 11: a
sorcerer carrying two fewer mana potions has no alternative strategy, only
an extra trip to town. The sorcerer is also the class most dependent on
mana potions, so the differentiation cut along the axis it was weakest.

stack_limit_test duplicated the first three tests of consumable_stack_test
and existed only to assert the removed bonuses; deleted. Two placeholder
tests asserting EXPECT_TRUE(true) for the class passives also deleted."
```

---

## Task 6: 恢复 floatingInfoBox 选项

**Files:**
- Modify: `Source/items.cpp:1629`–`1638`

分类判定：不触及任何平衡规则 → Base。违反底座层红线 8：移除了玩家已有的选项。

- [ ] **Step 1: 确认硬编码存在**

Run:
```bash
grep -n "AddInfoBoxString(str, true)\|AddInfoBoxString(std::move(str), true)" Source/items.cpp
```

Expected:
```
1632:	AddInfoBoxString(str, true);
1637:	AddInfoBoxString(std::move(str), true);
```

- [ ] **Step 2: 确认选项仍然存在于 Options 系统**

Run:
```bash
grep -n "floatingInfoBox" Source/options.h Source/options.cpp
```

Expected: 在 `options.h` 中有 `OptionEntryBoolean floatingInfoBox;` 声明，在 `options.cpp` 中有其构造与注册。

- [ ] **Step 3: 恢复读取选项**

在 `Source/items.cpp` 中，将：

```cpp
void AddItemInfoBoxString(const std::string_view str)
{
	AddInfoBoxString(str, true);
}

void AddItemInfoBoxString(std::string &&str)
{
	AddInfoBoxString(std::move(str), true);
}
```

替换为：

```cpp
void AddItemInfoBoxString(const std::string_view str)
{
	const bool floatingInfoBoxEnabled = *GetOptions().Gameplay.floatingInfoBox;
	AddInfoBoxString(str, floatingInfoBoxEnabled);
}

void AddItemInfoBoxString(std::string &&str)
{
	const bool floatingInfoBoxEnabled = *GetOptions().Gameplay.floatingInfoBox;
	AddInfoBoxString(std::move(str), floatingInfoBoxEnabled);
}
```

- [ ] **Step 4: 构建并验证**

Run:
```bash
ninja -C build items_test inventory_ui_test && ./build/items_test && ./build/inventory_ui_test
```

Expected: 两个测试 PASS。

- [ ] **Step 5: 验证选项已被读取**

Run:
```bash
grep -c "GetOptions().Gameplay.floatingInfoBox" Source/items.cpp
```

Expected: `2`

- [ ] **Step 6: 提交**

```bash
git add Source/items.cpp
git commit -m "revert(items): restore floatingInfoBox option check

AddItemInfoBoxString had been changed to pass true unconditionally,
removing a setting the player already had. Base red line 8: the base layer
may add things, it may not take away choices."
```

---

## Task 7: 撤销光照压制

**Files:**
- Modify: `Source/lighting.cpp`
- Modify: `Source/lighting.h`
- Modify: `Source/msg.cpp`
- Modify: `Source/loadsave.cpp`
- Modify: `Source/player.cpp`
- Modify: `Source/monster.cpp`
- Modify: `Source/monster.h`
- Delete: `test/light_suppression_test.cpp`
- Delete: `test/monster_activation_test.cpp`
- Delete: `test/combat_integration_test.cpp`
- Modify: `CMake/Tests.cmake`

分类判定：触及判定树第 4 条（光照半径、视野范围）→ Depth。当前无条件生效于 Base 层，违反 B3。

同时不满足三条深度层红线：无反制手段（DP2 / 红线 10）；产生的是单方面视野惩罚而非预算取舍（DP3 / 红线 11）；对近远程职业影响未分别评估（红线 14）——地狱层视野从 10 降到 6、地穴降到 5，近战玩家本就要贴到 1 格几乎无感，而法师与游侠的核心优势被砍掉。

- [ ] **Step 1: 记录调用点，确认范围**

Run:
```bash
grep -rn "GetEffectiveLightRadius\|GetLightSuppressionMultiplier\|GetMonsterActivationRadius" Source/
```

Expected: 共 10 处——`lighting.cpp` 3 处（1 调用 + 2 定义）、`lighting.h` 2 处声明、`msg.cpp` 1 处、`loadsave.cpp` 1 处、`player.cpp` 2 处、`monster.cpp` 2 处（定义内含 1 处调用）、`monster.h` 1 处声明。

- [ ] **Step 2: 回退 lighting.cpp 的调用点**

在 `Source/lighting.cpp` 的 `ToggleLighting()` 中，将：

```cpp
			DoLighting(player.position.tile, GetEffectiveLightRadius(player, currlevel), {});
```

改为：

```cpp
			DoLighting(player.position.tile, player._pLightRad, {});
```

- [ ] **Step 3: 删除 lighting.cpp 中的两个函数**

删除文件末尾 `} // namespace devilution` 之前的整块：

```cpp
float GetLightSuppressionMultiplier(int dungeonLevel)
{
	// Cathedral (1-4): no suppression
	if (dungeonLevel >= 1 && dungeonLevel <= 4) {
		return 1.0f;
	}
	// Catacombs (5-8): 90% (slightly darker)
	if (dungeonLevel >= 5 && dungeonLevel <= 8) {
		return 0.9f;
	}
	// Caves (9-12): 80%
	if (dungeonLevel >= 9 && dungeonLevel <= 12) {
		return 0.8f;
	}
	// Hell (13-16): 60%
	if (dungeonLevel >= 13 && dungeonLevel <= 16) {
		return 0.6f;
	}
	// Crypt (21-24): 50%
	if (dungeonLevel >= 21 && dungeonLevel <= 24) {
		return 0.5f;
	}
	// Default: no suppression
	return 1.0f;
}

int GetEffectiveLightRadius(const Player &player, int dungeonLevel)
{
	float multiplier = GetLightSuppressionMultiplier(dungeonLevel);
	int baseRadius = player._pLightRad;

	// Equipment bonus is not suppressed
	int equipmentBonus = baseRadius - 10; // Base is 10
	int suppressedBase = static_cast<int>(10 * multiplier);

	return suppressedBase + equipmentBonus;
}
```

- [ ] **Step 4: 删除 lighting.h 中的声明与前向声明**

删除：

```cpp
/**
 * @brief Get the light suppression multiplier for a dungeon level
 * @param dungeonLevel The dungeon level (0-24)
 * @return Light multiplier (0.0 to 1.0)
 */
float GetLightSuppressionMultiplier(int dungeonLevel);

/**
 * @brief Get the effective light radius for a player considering suppression
 * @param player The player
 * @param dungeonLevel The dungeon level
 * @return Effective light radius
 */
int GetEffectiveLightRadius(const Player &player, int dungeonLevel);

```

以及 `namespace devilution {` 之后的前向声明（上游 `lighting.h` 中不存在，为这两个函数而加）：

```cpp
struct Player;

```

- [ ] **Step 5: 回退 msg.cpp 的调用点**

在 `Source/msg.cpp` 中，将：

```cpp
			ActivateVision(player.position.tile, GetEffectiveLightRadius(player, currlevel), player.getId());
```

改为：

```cpp
			ActivateVision(player.position.tile, player._pLightRad, player.getId());
```

- [ ] **Step 6: 回退 loadsave.cpp 的调用点**

在 `Source/loadsave.cpp` 中，将：

```cpp
	DoUnVision(myPlayer.position.tile, GetEffectiveLightRadius(myPlayer, currlevel)); // fix for vision staying on the level
```

改为：

```cpp
	DoUnVision(myPlayer.position.tile, myPlayer._pLightRad); // fix for vision staying on the level
```

- [ ] **Step 7: 回退 player.cpp 的两处调用点**

在 `Source/player.cpp` 的 `InitPlayer` 中，将：

```cpp
		if (&player == MyPlayer) {
			int effectiveLight = GetEffectiveLightRadius(player, currlevel);
			player.lightId = AddLight(player.position.tile, effectiveLight);
			ChangeLightXY(player.lightId, player.position.tile); // fix for a bug where old light is still visible at the entrance after reentering level
		}
		ActivateVision(player.position.tile, GetEffectiveLightRadius(player, currlevel), player.getId());
```

改为：

```cpp
		if (&player == MyPlayer) {
			player.lightId = AddLight(player.position.tile, player._pLightRad);
			ChangeLightXY(player.lightId, player.position.tile); // fix for a bug where old light is still visible at the entrance after reentering level
		}
		ActivateVision(player.position.tile, player._pLightRad, player.getId());
```

- [ ] **Step 8: 删除 GetMonsterActivationRadius**

在 `Source/monster.cpp` 中，删除文件末尾 `} // namespace devilution` 之前的：

```cpp
int GetMonsterActivationRadius(const Player &player, int dungeonLevel)
{
	int playerVision = GetEffectiveLightRadius(player, dungeonLevel);
	return playerVision + 2;
}

```

在 `Source/monster.h` 中，删除：

```cpp
/**
 * @brief Get the monster activation radius based on player vision
 * @param player The player
 * @param dungeonLevel The dungeon level
 * @return Monster activation radius (player vision + 2)
 */
int GetMonsterActivationRadius(const Player &player, int dungeonLevel);

```

- [ ] **Step 9: 删除三个测试**

```bash
git rm test/light_suppression_test.cpp \
       test/monster_activation_test.cpp \
       test/combat_integration_test.cpp
```

在 `CMake/Tests.cmake` 的 `set(tests ...)` 中删除这三行：

```cmake
  light_suppression_test
  monster_activation_test
  combat_integration_test
```

`combat_integration_test` 一并删除的理由：它整篇只测这两个被删函数，不是独立的集成测试。

- [ ] **Step 10: 构建全部并验证**

Run:
```bash
ninja -C build 2>&1 | tail -3
```

Expected: 构建成功，无 `FAILED`。

- [ ] **Step 11: 验证符号已清除且与上游一致**

Run:
```bash
grep -rn "GetEffectiveLightRadius\|GetLightSuppressionMultiplier\|GetMonsterActivationRadius" Source/ test/ || echo "0 matches"
echo "--- 与上游对比 ---"
git diff origin/master...HEAD --numstat -- Source/lighting.cpp Source/lighting.h Source/msg.cpp Source/loadsave.cpp
```

Expected: 第一条输出 `0 matches`；`lighting.cpp`、`lighting.h`、`msg.cpp`、`loadsave.cpp` 不再出现在 numstat 输出中（已完全回到上游状态）。

- [ ] **Step 12: 运行全部测试**

Run:
```bash
cd build && ctest --output-on-failure 2>&1 | tail -10
```

Expected: 全部通过。

- [ ] **Step 13: 提交**

```bash
git add Source/lighting.cpp Source/lighting.h Source/msg.cpp Source/loadsave.cpp \
        Source/player.cpp Source/monster.cpp Source/monster.h CMake/Tests.cmake \
        test/light_suppression_test.cpp test/monster_activation_test.cpp \
        test/combat_integration_test.cpp
git commit -m "revert(light): remove dungeon light suppression

Classified Depth by charter rule 4 (light radius, vision range) but was
active unconditionally in the Base layer, violating pillar B3. Keeping it
would make the base layer's only external promise -- balance identical to
vanilla -- unverifiable.

It also fails three depth red lines. No counterplay existed (red line 10),
so the darkness was an unactionable unknown rather than a decision. It
produced a one-sided vision penalty rather than a budget tradeoff (red line
11). And its melee/ranged impact was never assessed (red line 14): vision
dropped from 10 to 6 in Hell and 5 in the Crypt, which barely affects a
melee player who must close to one tile anyway but removes the core
advantage of the sorcerer and the rogue.

GetMonsterActivationRadius is deleted with it: it had zero production
callers, so the intended symmetry (monsters notice you before you see them)
never existed -- only the penalty did.

Re-introduction is tracked in the charter's backlog and will need a
rewrite, so nothing is saved by keeping this code."
```

---

## Task 8: 删除任务奖励死代码

**Files:**
- Modify: `Source/stores.cpp`
- Modify: `Source/stores.h`
- Delete: `test/quest_reward_test.cpp`
- Modify: `CMake/Tests.cmake`

三个函数仅有测试调用者，零生产调用者。按宪章第 4 节的「已实施」定义，它们从未实施。

- [ ] **Step 1: 确认零生产调用者**

Run:
```bash
for fn in DoesAdriaOfferSpellChoice DoesPepinGiveRegenerationPotion DoesGriswoldOfferCustomWeapon; do
  echo "--- $fn"
  grep -rn "$fn" Source/ test/
done
```

Expected: 每个函数只有三处——`stores.h` 声明、`stores.cpp` 定义、`test/quest_reward_test.cpp` 调用。`Source/` 内无其他调用。

- [ ] **Step 2: 删除 stores.cpp 中的三个函数**

删除文件末尾 `} // namespace devilution` 之前的整块：

```cpp
bool DoesAdriaOfferSpellChoice()
{
	return Quests[Q_MUSHROOM]._qactive == QUEST_DONE;
}

bool DoesPepinGiveRegenerationPotion()
{
	return Quests[Q_PWATER]._qactive == QUEST_DONE;
}

bool DoesGriswoldOfferCustomWeapon()
{
	return Quests[Q_ANVIL]._qactive == QUEST_DONE;
}

```

- [ ] **Step 3: 删除 stores.h 中的三个声明**

删除：

```cpp
/**
 * @brief Check if Adria offers spell choice after mushroom quest
 * @return True if mushroom quest is completed
 */
bool DoesAdriaOfferSpellChoice();

/**
 * @brief Check if Pepin gives regeneration potion after poison water quest
 * @return True if poison water quest is completed
 */
bool DoesPepinGiveRegenerationPotion();

/**
 * @brief Check if Griswold offers custom weapon after anvil quest
 * @return True if anvil quest is completed
 */
bool DoesGriswoldOfferCustomWeapon();

```

- [ ] **Step 4: 删除测试**

```bash
git rm test/quest_reward_test.cpp
```

在 `CMake/Tests.cmake` 的 `set(tests ...)` 中删除这一行：

```cmake
  quest_reward_test
```

- [ ] **Step 5: 构建并运行相关测试**

Run:
```bash
ninja -C build stores_test quests_test && ./build/stores_test && ./build/quests_test
```

Expected: 两个测试 PASS。

- [ ] **Step 6: 验证符号已清除**

Run:
```bash
grep -rn "DoesAdriaOfferSpellChoice\|DoesPepinGiveRegenerationPotion\|DoesGriswoldOfferCustomWeapon" Source/ test/ || echo "0 matches"
```

Expected: `0 matches`

- [ ] **Step 7: 提交**

```bash
git add Source/stores.cpp Source/stores.h CMake/Tests.cmake test/quest_reward_test.cpp
git commit -m "chore(stores): remove quest reward skeleton functions

All three had zero production callers -- only quest_reward_test called
them. By the charter's definition of 'implemented' they never were: the
tests were testing dead code, which is what produced the false sense of
completion.

The underlying designs (Adria's spell choice, Pepin's regeneration potion,
Griswold's custom weapon) change balance and so belong in the Depth layer,
and each needs real store UI interaction design. Deleting the skeleton;
the concepts are re-derived when the work is actually specced."
```

---

## Task 9: 删除注册表入口点

**Files:**
- Modify: `Source/monster.cpp`
- Modify: `Source/tables/monstdat.h`
- Modify: `Source/tables/misdat.cpp`
- Modify: `Source/tables/misdat.h`
- Modify: `test/ai_registry_test.cpp`
- Modify: `test/missile_registry_test.cpp`
- Modify: `test/lua_integration_test.cpp`

分类判定：Infra（玩家可感知行为零变化）。三个入口点均不通过 I2——仅有测试调用者，未暴露给 Lua（Lua 侧的 `SetAI` 只能设置已有 AI 类型，不能注册新类型），且「怪物行为深化」目前只是待立项条目而非规格，所以 I2(b) 也不成立。I3 同样不通过。

**保留**已在生产使用的派发 refactor：`AiProc` 数组化（生产调用点 `Source/monster.cpp:4351`）、`static_assert`、以及 `misdat.cpp` 的 `g_addFnRegistry` / `g_processFnRegistry`（被生产代码 `ParseMissileAddFn` / `ParseMissileProcessFn` 使用）。

- [ ] **Step 1: 确认三个入口点仅有测试调用者**

Run:
```bash
for fn in RegisterAiFunction RegisterMissileAddFn RegisterMissileProcessFn; do
  echo "--- $fn"
  grep -rn "$fn" Source/ test/
done
```

Expected: 每个函数在 `Source/` 内只有声明与定义两处，其余全部在 `test/` 内。

- [ ] **Step 2: 确认保留部分确有生产消费者**

Run:
```bash
echo "--- AiProc 生产调用点"; grep -rn "AiProc\[" Source/
echo "--- 注册表映射的生产消费者"; grep -rn "g_addFnRegistry\|g_processFnRegistry" Source/tables/misdat.cpp | grep -vE "RegisterMissile"
```

Expected: `AiProc[` 在 `Source/monster.cpp:4351` 有生产调用；两个映射被 `ParseMissileAddFn` / `ParseMissileProcessFn` 与其初始化函数使用。

- [ ] **Step 3: 删除 monster.cpp 中的 RegisterAiFunction 与 FallbackAiImpl**

`FallbackAiImpl` 的唯一使用者是 `RegisterAiFunction`，必须一并删除，否则产生未使用函数警告。

在 `Source/monster.cpp` 中，将：

```cpp
static_assert(static_cast<int>(MonsterAIID::BoneDemon) < static_cast<int>(AiProc.size()),
    "AiProc must be large enough to hold all built-in AI types");

namespace {

void FallbackAiImpl(Monster &monster)
{
	if (monster.activeForTicks == 0)
		return;
	if (monster.goal == MonsterGoal::Normal) {
		monster.mode = MonsterMode::Stand;
	}
}

} // namespace

void RegisterAiFunction(MonsterAIID id, AiFunction fn)
{
	size_t index = static_cast<size_t>(static_cast<int8_t>(id));
	if (index >= AiProc.size())
		return;
	AiProc[index] = fn != nullptr ? fn : &FallbackAiImpl;
}

namespace {

bool IsRelativeMoveOK(const Monster &monster, Point position, Direction mdir)
```

替换为：

```cpp
static_assert(static_cast<int>(MonsterAIID::BoneDemon) < static_cast<int>(AiProc.size()),
    "AiProc must be large enough to hold all built-in AI types");

namespace {

bool IsRelativeMoveOK(const Monster &monster, Point position, Direction mdir)
```

- [ ] **Step 4: 删除 monstdat.h 中的声明并修正注释**

在 `Source/tables/monstdat.h` 中，将：

```cpp
// Runtime AI registration (for mod extensions)
struct Monster;
using AiFunction = void (*)(Monster &monster);
extern std::array<AiFunction, 128> AiProc;
void RegisterAiFunction(MonsterAIID id, AiFunction fn);
```

替换为：

```cpp
// AI dispatch table. Exposed for tests; there is no registration entry point.
struct Monster;
using AiFunction = void (*)(Monster &monster);
extern std::array<AiFunction, 128> AiProc;
```

- [ ] **Step 5: 删除 misdat.cpp 中的两个入口点**

在 `Source/tables/misdat.cpp` 中，删除：

```cpp
void RegisterMissileAddFn(std::string_view name, MissileData::AddFn fn)
{
	g_addFnRegistry[std::string(name)] = fn;
}

void RegisterMissileProcessFn(std::string_view name, MissileData::ProcessFn fn)
{
	g_processFnRegistry[std::string(name)] = fn;
}

```

- [ ] **Step 6: 删除 misdat.h 中的两个声明**

在 `Source/tables/misdat.h` 中，删除：

```cpp
void RegisterMissileAddFn(std::string_view name, MissileData::AddFn fn);
void RegisterMissileProcessFn(std::string_view name, MissileData::ProcessFn fn);
```

保留其后的两行——它们是生产代码，被 TSV 解析使用：

```cpp
tl::expected<MissileData::AddFn, std::string> ParseMissileAddFn(std::string_view value);
tl::expected<MissileData::ProcessFn, std::string> ParseMissileProcessFn(std::string_view value);
```

- [ ] **Step 7: 重写 ai_registry_test.cpp**

保留唯一测试派发表的测试，删除三个测试注册入口点的测试及其辅助函数。整个文件替换为：

```cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "monster.h"
#include "tables/monstdat.h"

using namespace devilution;

TEST(AiRegistryTest, ExistingTypesHaveValidFunctionPointers)
{
	for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++) {
		auto ai = static_cast<MonsterAIID>(i);
		if (ai == MonsterAIID::FireMan)
			continue;
		size_t idx = static_cast<size_t>(i);
		ASSERT_LT(idx, AiProc.size());
		EXPECT_NE(AiProc[idx], nullptr)
		    << "AI type " << i << " has null function pointer";
	}
}
```

- [ ] **Step 8: 删除 missile_registry_test.cpp 中的两个注册测试**

从 `test/missile_registry_test.cpp` 中删除这两个测试：

```cpp
TEST(MissileRegistryTest, RegisterCustomAddFn)
{
	RegisterMissileAddFn("TestCustomAdd", TestCustomAddFn);

	auto result = ParseMissileAddFn("TestCustomAdd");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, reinterpret_cast<MissileData::AddFn>(TestCustomAddFn));
}

TEST(MissileRegistryTest, RegisterCustomProcessFn)
{
	RegisterMissileProcessFn("TestCustomProcess", TestCustomProcessFn);

	auto result = ParseMissileProcessFn("TestCustomProcess");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, reinterpret_cast<MissileData::ProcessFn>(TestCustomProcessFn));
}

```

同时删除它们的辅助函数，否则产生未使用函数警告：

```cpp
// Custom missile function for testing registry
void TestCustomAddFn(Missile &, AddMissileParameter &)
{
	// no-op for test
}

void TestCustomProcessFn(Missile &)
{
	// no-op for test
}

```

保留 `AllBuiltinAddFnNamesResolve`、`AllBuiltinProcessFnNamesResolve`、`EmptyStringReturnsNullptr`、`UnknownNameReturnsError` 四个测试——它们测的是生产代码 `ParseMissile*Fn`。

- [ ] **Step 9: 删除 lua_integration_test.cpp 中的两个注册测试**

从 `test/lua_integration_test.cpp` 中删除这两个测试：

```cpp
TEST(LuaIntegrationTest, AiRegistrationAndDispatchWorkTogether)
{
	g_aiCalled = 0;
	constexpr MonsterAIID customAi = static_cast<MonsterAIID>(55);
	RegisterAiFunction(customAi, IntegrationTestAi);

	Monster monster {};
	monster.ai = customAi;
	AiProc[static_cast<size_t>(customAi)](monster);
	EXPECT_EQ(g_aiCalled, 1);

	AiProc[static_cast<size_t>(customAi)](monster);
	EXPECT_EQ(g_aiCalled, 2);
}

TEST(LuaIntegrationTest, MissileAddFnRegistrationAndParse)
{
	RegisterMissileAddFn("IntegrationTestMissile", IntegrationTestAddFn);
	auto result = ParseMissileAddFn("IntegrationTestMissile");
	ASSERT_TRUE(result.has_value());
	EXPECT_NE(*result, nullptr);
}

```

同时删除匿名命名空间中已无使用者的辅助函数与计数器：

```cpp
int g_aiCalled = 0;
void IntegrationTestAi(Monster &)
{
	g_aiCalled++;
}

void IntegrationTestAddFn(Missile &, AddMissileParameter &) {}

```

删除后该匿名命名空间为空，一并删除：

```cpp
namespace {

}

```

保留 `QuestScriptFieldAndCheckQuestsCoexist` 与 `AiProcSizeFitsAllBuiltinTypes` 两个测试。删除后 `test/lua_integration_test.cpp` 应为：

```cpp
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "monster.h"
#include "quests.h"
#include "tables/misdat.h"
#include "tables/monstdat.h"

using namespace devilution;

TEST(LuaIntegrationTest, QuestScriptFieldAndCheckQuestsCoexist)
{
	LoadQuestData();
	ASSERT_GT(QuestsData.size(), 0u);

	Quests[0]._qactive = QUEST_ACTIVE;
	QuestsData[0].scriptName = "";

	EXPECT_NO_THROW({ CheckQuests(); });
	EXPECT_EQ(QuestsData[0].scriptName, "");
}

TEST(LuaIntegrationTest, AiProcSizeFitsAllBuiltinTypes)
{
	EXPECT_GE(AiProc.size(), 64u);
	for (int i = 0; i <= static_cast<int>(MonsterAIID::BoneDemon); i++) {
		if (static_cast<MonsterAIID>(i) == MonsterAIID::FireMan) continue;
		EXPECT_NE(AiProc[static_cast<size_t>(i)], nullptr);
	}
	EXPECT_EQ(AiProc[static_cast<size_t>(MonsterAIID::FireMan)], nullptr);
}
```

- [ ] **Step 10: 构建并运行受影响测试**

Run:
```bash
ninja -C build ai_registry_test missile_registry_test lua_integration_test missiles_test \
  && ./build/ai_registry_test && ./build/missile_registry_test \
  && ./build/lua_integration_test && ./build/missiles_test
```

Expected: 四个测试全部 PASS。

- [ ] **Step 11: 验证入口点已清除、派发保留**

Run:
```bash
echo "--- 入口点应为 0"
grep -rn "RegisterAiFunction\|RegisterMissileAddFn\|RegisterMissileProcessFn" Source/ test/ || echo "0 matches"
echo "--- 派发应保留"
grep -c "AiProc\[" Source/monster.cpp
grep -c "ParseMissileAddFn\|ParseMissileProcessFn" Source/tables/misdat.cpp
```

Expected: 第一条 `0 matches`；第二条 ≥ 1；第三条 ≥ 2。

- [ ] **Step 12: 提交**

```bash
git add Source/monster.cpp Source/tables/monstdat.h Source/tables/misdat.cpp \
        Source/tables/misdat.h test/ai_registry_test.cpp \
        test/missile_registry_test.cpp test/lua_integration_test.cpp
git commit -m "chore: remove unused registry entry points

RegisterAiFunction, RegisterMissileAddFn and RegisterMissileProcessFn had
only test callers and were never exposed to Lua -- the Lua side only has
SetAI, which selects an existing AI type rather than registering a new one.
They fail Infra red line I2 (tests are not consumers) and I3 (monster
behaviour deepening is a backlog item, not a spec).

The dispatch refactor is kept because it is genuinely in production use:
AiProc array dispatch at monster.cpp:4351, the static_assert guarding its
size, and the misdat name registries consumed by ParseMissileAddFn and
ParseMissileProcessFn during TSV parsing.

FallbackAiImpl is removed with RegisterAiFunction, its only user.

When monster behaviour deepening is actually specced, that spec decides the
right entry point shape -- most likely Lua exposure rather than a C++
registration function. Keeping a guessed shape is what produced
world_state."
```

---

## Task 10: 删除 room_decoration 占位测试

**Files:**
- Delete: `test/room_decoration_test.cpp`
- Modify: `CMake/Tests.cmake`

- [ ] **Step 1: 确认它是占位测试**

Run:
```bash
grep -n "EXPECT\|ASSERT" test/room_decoration_test.cpp
```

Expected: 唯一断言为 `EXPECT_TRUE(true);`

- [ ] **Step 2: 删除**

```bash
git rm test/room_decoration_test.cpp
```

在 `CMake/Tests.cmake` 的 `set(tests ...)` 中删除这一行：

```cmake
  room_decoration_test
```

- [ ] **Step 3: 验证 Tests.cmake 列表已清理干净**

Run:
```bash
grep -nE "world_state_test|gold_drop_test|stack_limit_test|light_suppression_test|monster_activation_test|combat_integration_test|quest_reward_test|room_decoration_test" CMake/Tests.cmake || echo "0 matches"
```

Expected: `0 matches`

- [ ] **Step 4: 构建**

Run:
```bash
ninja -C build 2>&1 | tail -3
```

Expected: 构建成功。

- [ ] **Step 5: 提交**

```bash
git add CMake/Tests.cmake test/room_decoration_test.cpp
git commit -m "chore(test): remove room_decoration placeholder test

Its only assertion was EXPECT_TRUE(true), violating charter prohibition 6.
The room decoration design it was placed for has been deleted as a wish
list with no actionable content."
```

---

## Task 11: 全量验证

- [ ] **Step 1: 全量构建**

Run:
```bash
ninja -C build 2>&1 | tail -5
```

Expected: 构建成功，无 `FAILED`、无 warning 涉及本次改动的文件。

- [ ] **Step 2: 全量测试**

Run:
```bash
cd build && ctest --output-on-failure 2>&1 | tail -20
```

Expected: 全部通过，`100% tests passed`。

- [ ] **Step 3: 逐条走宪章第 10 节验证清单**

Run:
```bash
cd /home/peng/workspace/DevilutionX

echo "=== 行尾合规 ==="
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  n=$(wc -l < "$f"); c=$(grep -c $'\r' "$f")
  [ "$n" = "$c" ] && echo "OK   $f" || echo "FAIL $f ($c/$n)"
done

echo "=== 行尾噪声清除 ==="
for f in Source/diablo.cpp Source/inv.cpp Source/qol/stash.cpp Source/lua/modules/monsters.cpp; do
  t=$(git diff origin/master...HEAD --numstat -- "$f" | awk '{print $1+$2}')
  printf "%-40s %s\n" "$f" "${t:-0}"
done

echo "=== 支柱单一真相来源 ==="
grep -rl "稀缺是美德\|环境是角色\|不安感是核心体验" docs/superpowers/specs/ docs/superpowers/plans/ || echo "OK 仅宪章"

echo "=== 目录整洁 ==="
ls docs/superpowers/specs/

echo "=== 金币撤销 ==="
grep -c "rndv \* 5 / 4" Source/items.cpp || echo "OK 0 matches"

echo "=== 职业堆叠加成撤销 ==="
sed -n '/^int GetMaxStackCount/,/^}/p' Source/items.cpp | grep -c "HeroClass" || echo "OK 0 HeroClass branches"

echo "=== 选项恢复 ==="
grep -c "GetOptions().Gameplay.floatingInfoBox" Source/items.cpp

echo "=== 光照压制撤销 ==="
grep -rn "GetLightSuppressionMultiplier\|GetEffectiveLightRadius" Source/ test/ || echo "OK 0 matches"

echo "=== 死代码清零 ==="
grep -rn "GetMonsterActivationRadius\|DoesAdriaOfferSpellChoice\|DoesPepinGiveRegenerationPotion\|DoesGriswoldOfferCustomWeapon\|RegisterAiFunction\|RegisterMissileAddFn\|RegisterMissileProcessFn" Source/ test/ || echo "OK 0 matches"

echo "=== 占位测试清零 ==="
grep -rn "EXPECT_TRUE(true)" test/ || echo "OK 0 matches"

echo "=== 「已实施」标注为真：抽查四篇保留文档的核心符号有非测试调用者 ==="
for sym in FloatingInfoString BuildSpellTooltip AddItemInfoBoxString; do
  n=$(grep -rl "$sym" Source/ 2>/dev/null | wc -l)
  [ "$n" -gt 0 ] && echo "OK   $sym 有 $n 个生产文件引用" || echo "FAIL $sym 无生产引用"
done

echo "=== 数值出处抽查：宪章事实基础的三条 ==="
grep -q "^maxMag	50" assets/txtdata/classes/warrior/attributes.tsv && echo "OK   warrior maxMag = 50" || echo "FAIL warrior maxMag"
grep -n "_pLightRad = 10" Source/player.cpp | head -1
awk -F'\t' '$1=="ManaShield"{print "OK   ManaShield minIntelligence = "$10}' assets/txtdata/spells/spelldat.tsv
```

Expected:
- 行尾合规：四行 `OK`
- 行尾噪声：四个文件均 < 400
- 支柱：`OK 仅宪章`
- 目录：仅 5 个 `.md`，无 `better-d1`
- 金币、光照、死代码、占位测试：均 `OK 0 matches`
- 职业堆叠：`OK 0 HeroClass branches`
- 选项恢复：`2`

- [ ] **Step 4: 验证 Base 层承诺**

Run:
```bash
echo "=== 与上游完全一致的文件（应含光照相关全部）==="
for f in Source/lighting.cpp Source/lighting.h Source/msg.cpp Source/loadsave.cpp; do
  t=$(git diff origin/master...HEAD --numstat -- "$f" | awk '{print $1+$2}')
  [ -z "$t" ] && echo "OK   $f 与上游一致" || echo "DIFF $f 仍有 $t 行改动"
done
echo "=== 背包格数应与上游一致 ==="
grep -n "InventoryGridCells = " Source/player.h
```

Expected: 四个文件均 `与上游一致`；`InventoryGridCells = 40`。

唯一残留的平衡偏差是腰带堆叠容量，已记录于宪章待立项清单优先级 2。

- [ ] **Step 5: 更新宪章状态**

在 `docs/superpowers/specs/2026-07-27-better-d1-design-charter.md` 第 9 节的「本次范围」标题下，将标题行：

```markdown
### 本次范围
```

改为：

```markdown
### 本次范围（已于 2026-07-27 执行完毕，见 plans/2026-07-27-charter-cleanup.md）
```

- [ ] **Step 6: 提交**

```bash
git add docs/superpowers/specs/2026-07-27-better-d1-design-charter.md
git commit -m "docs: mark charter cleanup scope as executed"
```

---

## 自审

**规格覆盖**（对照宪章第 9 节「本次范围」七项）：

| 宪章条目 | 对应任务 |
|---|---|
| 1. 修复测试构建 | Task 1 |
| 2. 行尾回退 | Task 2 |
| 3. 宪章写入并提交 | 已于 `08ced091a` / `99308351d` 完成 |
| 4. 文档重组 + 4 篇补标注 | Task 3 |
| 5. 代码撤销四项 | Task 4（金币）、Task 5（职业堆叠）、Task 6（选项）、Task 7（光照） |
| 6. 删除死代码 | Task 7（`GetMonsterActivationRadius`）、Task 8（3 个任务奖励函数）、Task 9（3 个注册表入口点） |
| 7. 处理受影响测试 | Task 4、5、7、8、9、10 内各自处理；Task 11 全量验证 |

宪章第 10 节验证清单 14 项全部落在 Task 11 Step 3–4。

**占位符扫描**：无 TBD/TODO；每个代码步骤均给出完整的前后代码；每个命令步骤均给出预期输出；无「参照 Task N」式引用。

**类型一致性**：
- `GetMaxStackCount(const Item &, const Player &)` 签名在 Task 5 Step 2（定义）与 Step 3（声明）一致，与 `Source/inv.cpp:502`、`:1350` 两处生产调用点及 16 处测试调用点兼容
- `AiFunction`、`AiProc` 在 Task 9 Step 4 保留，Step 7 与 Step 9 的测试代码均引用它们
- `ParseMissileAddFn` / `ParseMissileProcessFn` 在 Task 9 Step 6 明确保留，Step 8 保留的四个测试引用它们
- `player._pLightRad` 在 Task 7 Step 2、5、6、7 的四处回退中写法一致
