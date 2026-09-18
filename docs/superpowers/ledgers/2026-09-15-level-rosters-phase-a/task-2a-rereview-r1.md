# Task 2a 复审 R1（修复轮次 1 裁决）

基线 `13a974a93` → Head `d62cc26ab`（diff 已读；未跑 git 命令改动工作区）
聚焦复跑：`./build/level_roster_test --gtest_filter='LevelRosterTest.*'` → `[ PASSED ] 17 tests.`
（二进制 mtime 05:25 晚于 `level_roster.cpp` 05:10 / `level_roster_test.cpp` 05:24，为当前源码构建，未重建）

## 问题裁决

- **F1【重要｜R15】caps 单一真相源 + effective=min(available,cap)：ADDRESSED**
  - helper 真的导出：`Source/tables/level_roster.h:62`（`uint8_t BehaviorClassCapForLevel(uint8_t, BehaviorClass)`），实现 `Source/tables/level_roster.cpp:133-143`。
  - **真的被校验器使用**（非空挂）：`level_roster.cpp:210-213` 取 cap 并按 `effective = cap==0 ? available : min(available, cap)` 判定，错误文案含 `caps`（`:213`）。
  - 层段边界与 `Source/monster.cpp:3513-3514` 逐字一致：`capKite = currlevel >= 9 && <= 12`（仅 `RangedKite`）、`capSameClass = currlevel >= 13 && <= 16`（任意类别）；`cap` 值 2 与 `:3531` 的 `count >= 2` 一致。
  - 用例齐备且**非假阳性**（已用 `monstdat.tsv` 独立核算裸池，证明拒绝确由 cap 而非池太小造成）：
    - `RangedKite=3`@L10 拒（`test/level_roster_test.cpp:118-131`）；实算 L10 `RangedKite` 裸池 = **7**。
    - `RangedKite=2`@L10 通过（`:133-141`）。
    - `Melee=3`@L14 拒（`:143-155`）；实算 L14 `Melee` 裸池 = **8**。
    - 两条 entry（`MT_BACID`@L10、`MT_VTEXLRD`@L14）本身可用性/unique-base 均合法（`monstdat.tsv` min/max = 10-12 / 12-14；L10/L14 的 unique base 集合不含二者），故用例不会走别的拒绝路径蒙对。
  - 未接线到 `monster.cpp` 采样循环，属 Task 3，符合简报边界。

- **F2【重要｜R16】按 level stable_sort：NOT ADDRESSED（生产修复到位，规定用例缺失）**
  - 生产侧修复正确：`level_roster.cpp:235-237` 在两个文件都加载完成、`ValidateLevelRoster` 之前 `std::stable_sort`（仅按 `level` 比较，层内保序）。
  - 但要求的用例（"交错表 L1、L2、L1 下 **`GetLevelRoster(1)`** 返回全部 L1 成员"）**未实现**：`test/level_roster_test.cpp:157-179` 的 `LoadedRosterKeepsAllMembersOfAnInterleavedLevel` 既不调用 `LoadLevelRoster()` 也不调用 `GetLevelRoster()`，而是在用例内**重抄了一遍** `stable_sort` + 两个 `find_if` 区间扫描，然后断言自己抄的结果。删掉 `level_roster.cpp:235-237` 该用例照样通过 ⇒ 自证式（占位）用例，未覆盖回归点，触碰 CLAUDE.md 禁令"不用占位测试"。
  - 客观约束（记录以便裁量）：`Entries` 是 `.cpp` 内部静态存储，2a 无真实 TSV，测试确实无法注入交错表去驱动 `GetLevelRoster`。报告 line 132 还说明：为绕过 drift check E，用例注释被特意改写成不含 `GetLevelRoster` 字样 —— 恰是该覆盖缺口的来源。
  - 关闭建议（二选一）：① 在 Task 2b 用一张**真实交错行序**的 TSV 驱动 `LoadLevelRoster()` + `GetLevelRoster(1)` 断言全取；② 2a 内导出一个测试用注入/校验入口（如 `SortRosterByLevel(std::span<LevelRosterEntry>)` 或允许喂 entries 的 accessor），使断言打在生产代码上。

- **F3【重要｜R17】存在性检查前置于 gbIsSpawn：ADDRESSED**
  - 类型存在性/越界检查已移到 `gbIsSpawn` 分支之前、两模式共用：`level_roster.cpp:150-153`（分支在 `:185`）；旧位置（分支内 retail 段）已删除。
  - spawn 只放宽三项：可用性 + unique 白名单（`:193-198`）与 floors 可满足性（`:199-215`），均在 `:190` 的 `return` 之后 —— 与规格 §4.4.4 一致。
  - 用例 `ValidationRejectsAnOutOfRangeMonsterIdInSpawnMode`（`test:181-195`）：`MT_INVALID`@L1 + `gbIsSpawn=true` 必须拒，断言含 `unknown monster id`。`MT_INVALID = -1`（`monstdat.h:304`）确实在 `magic_enum` range 内（`monstdat.h:393-395` `min = MT_INVALID`），`static_cast<size_t>(-1)` 回绕 ⇒ `>= MonstersData.size()` 命中，判定成立。用例前后正确复位 `gbIsSpawn`。

- **F4【重要｜R18】entries ∪ params 每层至少 1 core（两模式）：ADDRESSED**
  - `level_roster.cpp:168-183`：levels 由 `entries` 与 `params` 双向收集 → `sort` + `unique` 去重 → 逐层 `any_of(role==Core)`，位置在 `gbIsSpawn` 分支**之前** ⇒ 两模式都强制；错误文案 `"level N has no core roster members"`（去掉了原先的 `(spawn mode)` 后缀，两模式共用文案合理）。
  - 用例：`ValidationRejectsAParamsLevelWithNoCoreInRetailMode`（`test:197-206`，有 params 无 entries）、`ValidationRejectsAnEntryLevelWithNoCoreAndNoParamsRow`（`test:208-217`，有 tail 行无 params 行）；旧 spawn 用例 `test:104-116` 仍在且仍过。

- **F5【轻微｜R19】范围校验 + 拒绝哨兵：ADDRESSED（"两模式通用"与 spawn 语义一致）**
  - `level_roster.cpp:155-166`：`maxImage <= 0` 拒（文案含 `max_image`）、`tailDraw < 0` 拒（含 `tail_draw`）、`cls == BehaviorClass::Count` 拒（含 `Count`）。
  - 一致性核实：spawn 放宽的仍严格限于**可用性 / unique 白名单 / floors 可满足性**三项（`:190` 之后才是这三段）。F5 三项属结构性校验、且**没有**放宽存在性或范围检查 ⇒ 与"spawn 只放宽三项"不冲突，不构成 NOT ADDRESSED 条件。
  - 用例三条：`test:219-226`、`:228-235`、`:237-251`。`Count` 用例的必要性成立——`ParseClassFloors`（`level_roster.cpp:68`）的 `magic_enum::enum_cast` 会把字符串 `"Count"` 当合法值收下（`monstdat.h:385` `Count` 是枚举末项）。

- **F6【轻微】注释准确性 + 视图有效期契约：ADDRESSED**
  - ① `test/level_roster_test.cpp:43-45` 已改为 `availability=Never`（并说明层窗口拒绝路径由 `MT_WSKELAX`@L3 覆盖）；与 `assets/txtdata/monsters/monstdat.tsv:112`（`MT_DIABLO ... Never ... 26 26`）核对一致。`test:90-91` 的 spawn 用例注释同步更正。
  - ② `Source/tables/level_roster.h:42-46` 补上视图有效期契约：每次 `LoadLevelRoster()` 都 clear/重建 `Entries`/`Params`，此前返回的 `span`/指针立即失效，禁止跨重载持有。

## 修复 Diff 中的新增破坏

- **Low ｜`Source/tables/level_roster.cpp:213`**：面向用户的 `app_fatal` 文案中混入中文——`"... candidates exist (N raw, 被 caps 截断 to 2)"`，同一句其余为英文，与本仓库校验错误文案（含本文件其他 6 条）全英文的惯例不一致。建议改为 `"truncated by caps to"` 之类，同时保留测试断言的 `caps` 关键词。
- **Low ｜`test/level_roster_test.cpp:157-179`**：新增用例为自证式重实现（见 F2），是本轮修复引入的**无效覆盖**，且使 drift check E 对 `GetLevelRoster` 的"有测试调用者"判定被注释改写规避（报告 line 132 自述）。属 F2 未关闭项的一部分，非独立缺陷。
- 其余无：`std::min`/`std::sort`/`std::unique` 所需 `<algorithm>` 已在 `level_roster.cpp:8`；`test` 侧新增 `<algorithm>`/`<span>`（`test:3-4`）到位；三个改动文件均保持 **CRLF**（禁令 7 无违反）。

## 范围外观察（非阻塞）

1. **caps 的 L16 语义与规格自身不自洽**：`BehaviorClassCapForLevel` 对 L16 返回 2，忠实复刻 `monster.cpp:3514` 的 `<= 16` **字面量**；但 `monster.cpp:3442-3447` 在 `currlevel == 16` 时硬编码类型并**提前 return**，cap 在 L16 实际不可达（`monster.cpp:3508-3509` 注释亦如此说明）。规格 §4.4.2（line 154，控制者本轮修订）写 `L13-16`，而同一份规格 §5 的 F5 行（line 223）写"已解决：拆为 `13-15` + L16 特例"。实现与最新 §4.4.2 及代码字面量一致，故不判为缺陷；请控制者裁决要否收敛这两处规格表述。
2. `LoadLevelRoster()` 只对 `Entries` 排序，`Params` 未排序也未查重：同一 `level` 出现多行 params 时 `GetLevelRosterParams` 静默取第一条（`level_roster.cpp:256-260`）。本轮问题清单未涉及，建议 2b 写真实 TSV 时补一条"params 每层唯一"校验。
3. `cap == 0` 兼作"无上限"哨兵（`level_roster.cpp:142`、`:211`）：语义已在注释说明，但若未来真需要 `cap = 0`（禁某类别）会歧义。Task 3 复用时留意。
4. 报告数字中只核实了"17 用例全过"（本地聚焦复跑一致）；`721/0/100%` 与 `drift 5/5` 按指示未复跑，视为未核实陈述。

## 裁决

**修复轮次：仍有未关闭问题**

- 未关闭：**F2（R16）** —— `stable_sort` 生产修复正确，但要求的 `GetLevelRoster(1)` 交错表用例未实现，替代用例为自证式重抄（删掉生产排序仍会通过），无回归防护。
- 已关闭：F1、F3、F4、F5、F6。
- 本轮修复未引入 Critical/Important 级新破坏；两条 Low（错误文案中英混排、占位用例）。
