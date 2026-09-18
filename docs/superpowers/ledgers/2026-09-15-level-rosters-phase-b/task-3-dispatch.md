# Task 3 分发稿（小队接入；控制者预写，含已验证事实与固定检查清单）

**角色**：阶段 B / Task 3 实现者（`feature/qol-upgrades`）。
**需求**：`docs/superpowers/ledgers/2026-09-15-level-rosters-phase-b/task-3-brief.md` + 计划 `## Task 3` 段。
**BASE**：分发前用 `git log --oneline -1` 取当前 HEAD（Task 2 的 CI 修复提交之后）。
**报告**：`docs/superpowers/ledgers/2026-09-15-level-rosters-phase-b/task-3-report.md`（**第一分钟就落盘**，边做边追加——本会话已两次因未落盘丢进展）。

## 控制者已核实的符号事实（**已对着真实代码逐个核对，可直接采用**）
| 事实 | 出处 |
|---|---|
| `LevelMonsterTypes` 是 `extern CMonster LevelMonsterTypes[MaxLvlMTypes]`，用 `.type` 取类型 id | `Source/monster.h:211`、`Source/monster.cpp:112` |
| `GetMonsterTypeIndex(_monster_id)` 线性扫描，**命中返回下标、未命中返回 `LevelMonsterTypeCount`**（这个值不能当合法下标用） | `Source/monster.cpp:309-316` |
| `MT_INVALID` 是合法的 `_monster_id` 哨兵（表内有定义） | `Source/tables/monstdat.cpp:130` |
| `IsMonsterAvailable(const MonsterData&)` 可在**同一 TU** 内直接调用（core 反查/partner 筛选可用） | `Source/monster.cpp:3183` |
| `PlaceGroup(typeIndex, num, leader, leashed, MinionOptions)` 的 `MinionOptions` 三字段默认 true（= 既有行为） | Task 2 提交（`monster.h`） |
| 既有 `na` 计算与降级路径位于散布循环 `Source/monster.cpp:3831-3840` 一带 | 计划 Task 3 步骤 4 |

> 符号审计结论：计划 Task 3 代码块中的 17 个候选符号**全部存在**（不存在 RB7 那类"凭记忆写错符号"的问题）。待创建的 `IsCoreRosterMember`/`PickCorePartner` 请实现为**文件内 static 助手**，不要新增导出 API（避免漂移检查 E 噪声）。

## 分发固定清单（本会话裁决沉淀，逐条适用）
1. **素材依赖**：任何会走到 unique 路径的新用例，必须从**被测对象的真实依赖**取 TRN 名（如 `UniqueMonstersData[...]` 的 TRN 字段）再 `OpenAsset` 探测，缺失即 `GTEST_SKIP`；**不得硬编码 `genrl.trn`/`bhka.trn` 之类文件名**（CI 只有 `spawn.mpq`）。
2. **断言可失败性**：每条新守卫必须做一次反证——**改坏生产行为 → 该用例必须变红 → 恢复 → 转绿**，两次实跑都写进报告。区间型断言优先改为**相对/A-B 比较**（同 seed 下两种配置对比），不要用会重叠的绝对区间。
3. **TSV 改动**：加/改 `assets/txtdata/**.tsv` 后必须先 `ninja devilutionx_mpq` 再测量（否则量到旧表）。
4. **夹具重生成**：小队接入改变放置 RNG → 若 `timedemo` 等夹具失配，按既有流程**重生成**（不是手改数字）。
5. **禁占位**：不得出现 `DISABLED_`/跳过式占位用例；走小队路径的 `SquadMinionsKeepOwnAi` **属本任务**（不要留给以后）。
6. **eval 计数**：新增/删除用例后同步 `eval/cases/rng/sampling-anti-monopoly.yaml` 的 `passed_min` **与** `output_contains` 两处。
7. **复用既有夹具辅助**：`PrepareDeathPathPrerequisites()`、`RunEngineDeath(...)` 等（Task 1 起已在 `sampling_behavior_test.cpp`）。
8. **门禁**：`python3 tools/run_tests.py --json /tmp/ci.json`（`failed==0 && passed_pct==100 && drift_ok==true`）+ `python3 -m tools.eval.backend --smoke`（exit 0）；**推送后跟踪 CI 到终态**（R43）。
9. **报告增量**：第一分钟落盘，每步追加。
10. **绝不**分发子代理；提交信息英文 conventional commits。

## 评审重点（预置，供 Task 3 评审者使用）
① 小队路径不得超 `totalmonsters`、不得产生 `packSize==0` 的 leader；② partner 下标必须合法（见上表 `GetMonsterTypeIndex` 未命中语义）；③ `squad_leashed=false` 回退语义（仍传 leader → 相邻初置有保证，但不设 `setLeader`/`packSize`）；④ 是否改变了既有 `na` 与降级路径；⑤ 夹具是否**按流程重生成**；⑥ 新守卫是否**可失败**（要求反证证据）；⑦ 三列的解析/校验与 eval 计数；⑧ `squad_chance`/`squad_size` 的取值是否有**实测依据**而非拍脑袋。

## 控制者给定的参数起点（RB12）
`squad_chance = 30`、`squad_size = 2`、`squad_leashed = 1`（逐层先统一，Task 4 再按实测分化）。**验收看实测**：Task 3 必须报告**每层"core 组中真正成队（≥1 随从）的实现率"**，而不是只写进表的概率——4 格约束、槽位不足、`totalmonsters` 钳制都会打折。
