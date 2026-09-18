# 阶段 B（核心小队）最终全分支评审

- 基线：`08b1db0ab`　目标：`dfbef1f1b`（HEAD）
- 范围：28 提交，31 文件，+2195/-147
- 评审方式：**只读**（未改工作区/暂存区/HEAD，未建 worktree，未重建）
- 裁决：**修复后可合并**

## 阅读轮次

| 轮 | 范围 | 结论 |
|---|---|---|
| 1 | `Source/monster.h` / `monster.cpp`（G1、G2、`PlaceGroup`、散布循环、`SquadRollCounters`） | 见 S1/S2/I1 |
| 2 | `Source/tables/level_roster.*` + `level_roster_params.tsv` + 5 个夹具表 | 干净，见 M2 |
| 3 | `test/level_roster_baseline_test.cpp`（+840）、`sampling_behavior_test.cpp`（+431）、`level_roster_test.cpp`（+149） | 见 I2/M1/M3 |
| 4 | eval cases、`tools/eval/*`、`tools/run_tests.py`、规格/计划/台账/知识 | 见 I3/M4 |
| 5 | 交叉核验：`msg.cpp` delta 载入、`loadsave.cpp:SyncPackSize`、`GroupUnity`/`FollowTheLeader`/`DirOK`/`ShrinkLeaderPacksize`/`ScavengerAi`、`monhealthbar`、`DeleteMonster`/`AddMonster` 槽位复用；实跑 4+6 个用例 | 见 S1/S2 |

实跑证据（本机、未改树）：
- `sampling_behavior_test --gtest_filter='*LeaderDeath*:*SquadMinionsUnbuffed*:*UniqueMinions*'` → 4/4 PASS
- `level_roster_baseline_test --gtest_filter='SquadPlacementTest.*-SquadFormationRate'` → 6/6 PASS，
  `[ SQUADFALLBACK ] unleashed partners 1251 max leader distance 12 bound 32`；
  `[ SHIPPEDSQUAD ] level 14 rollRate 0.088 realisation 1`（与台账 8.8% 一致）
- 未跑 `SquadFormationRate`（~290s）与全量门禁：Task 5 已在冻结树上跑过 764/0/100% + drift 5/5，本轮不重复。

---

## 优点

1. **G1 的判定改造是正解且有真实死亡路径驱动**。`M_UpdateRelations` 去掉 `hasLeashedMinions()` 门槛、改为随从侧扫描，`ReleaseMinions` 内已有 `getLeader() == &leader` 过滤 → 对无随从的怪是空循环，无行为副作用。`clearReference = !isUnique()` 把"清索引"精确限定在普通怪，unique 保留索引供 `monhealthbar` 着色。`LeaderDeathReleasesMinions` 走的是真实链路（`MonsterDeath` → 推进动画到末帧 → `M_UpdateRelations`），不是手搓状态；`UniqueLeaderDeathBehaviourUnchanged` 还先 `ASSERT_TRUE(leader.hasLeashedMinions())` 钉住"确实命中旧门槛"，这条前置断言是让回归非空转的关键。

2. **G2 的 `MinionOptions` 默认值确实保持既有行为**。三个字段默认 `true`，`PrepareUniqueMonst` 的调用点连第 5 参数都没传 → 走默认；逐字节对比 diff，unique 路径上 HP×2、`intelligence` 复制、`setLeader` 的 AI 覆写、Gargoyle 分支全部原样。`ownAi` 存/恢复放在 `if (leashed)` 内是正确的，因为 `setLeader` 是唯一覆写者：`leashed=false` 时根本不调用 `setLeader`，"不覆写"天然成立，没有漏路径。

3. **HP 断言用同种子 A/B 而非区间**，并显式论证了为什么区间不行（`MT_TSKELBW` 未加倍 [256,512] 与加倍 [512,1024] 在 512 重叠）。`SquadMinionsUnbuffed` 先 `ASSERT_NE(WSKELAX.ai, TSKELBW.ai)` 钉住"leader 与随从 AI 本应不同"的前提，`UniqueMinionsBehaviourUnchanged` 用 `mAi != baseData.ai` 筛 unique（避开 `MT_TSKELAX`/Bonehead 两边都是 `SkeletonMelee` 的恒真陷阱）。这正是 `pattern_assertions_must_be_failable.md` 形态 2 的正确修法，且落盘成了知识。

4. **形成率守卫的分子/分母语义判断正确，而且是本轮最有价值的一处纠偏**。`realised` 含 `squadLeashed && packSize>0`，而 `PlaceGroup` 只在 leashed 时写 `packSize` → 用它做分子会让任何采用 §4.3.4 回退的层恒读 0%，即"守卫因它自己要求的补救被采用而变红"。改用 leash 无关、按 roll 计的 `formed` 是对的；分母沿用循环自己的 `rolls`（不由 `squad_chance` 反推），所以 L14（chance=10）按它**实际**做的 121 次 roll 评判而非名义值。RB23 把同一纠偏推广到邻案 `ShippedSquadChanceRealisesSquadsOnEveryLevel`，避免了"在合规补救上埋雷"的守卫留在旁边。

5. **豁免被真正强制**。`if (rolls == 0)` 分支里 `EXPECT_EQ(params->squadChance, 0)` 是硬断言 —— 表里写着 30 却一次 roll 都没有（分支失联）会判红，而不是静默豁免。反证也做过（给 `squadEligible` 加 `currlevel != 11`）。同层还有 `EXPECT_LE(formed, rolls)`（前提守卫，抓过 `rolls = 2^64-1` 的下溢）与 leashed 层的 `EXPECT_EQ(leashedRealised, formed)`（让 `formed` 无法偷偷计别的东西）。

6. **回退路径的邻近性从"无守卫"变成了可失败守卫，且界的取法诚实**。unleashed 随从 `leaderRelation==None`、无反向指针，放置后无法重新配对 —— 所以在放置那一刻记录 `partnersPlaced`/`maxPartnerLeaderDistance` 是唯一可行解（RB19 的裁决正确，且明确约束为"只诊断/测量，不影响放置"）。界 32 的推导写明了：代码只能证到 101，而 101 **不可失败**（散布区 [16,96) → 全图 Chebyshev ≤80），故用两侧实测（出厂 max 17 / `nullptr` 66）取中。这是"承认无法证紧界"的正确写法，而不是假装 4 是界。本机实测 max 12，余量充足。

7. **`SquadsAreAbsentWhenTheTableDisablesThem` 的 A/B 加了防空转闩**：`EXPECT_GT(uniquePackMinions, 0)`。没有它，一个把所有 leashed 随从都判成 unique 的判别器 bug 会让 A/B 全绿而零测量。`gotcha_unique_boss_packs_are_also_leashed.md` 把这个坑落盘了。

8. **`PickCorePartnerTypeIndex` 返回类型索引而非 `_monster_id` 的理由成立**：把"roster 行存在"与"该层已注册"两个失败合并成一个**可达**分支，消掉了一条出厂表永远走不到、因而永远无法被测试的分支。`StaticVector` 容量保护、`typeIndex >= LevelMonsterTypeCount` 的"未注册"判定、`candidates.empty()` 返回 `LevelMonsterTypeCount`（与 `GetMonsterTypeIndex` 同一"未找到"约定）都是干净的。

9. **leader 放置校验确实能防别名**。`before = ActiveMonsterCount` 与 `PlaceGroup` 的语义匹配：`PlaceGroup` 在 try1 重试时会先把已放置的怪 `ActiveMonsterCount--` 撤回，最终 `ActiveMonsterCount` 只在成功放置后增加。所以 `AMC == before` 唯一含义就是"一只都没放下"，此时 `Monsters[AMC-1]` 必然是**上一轮迭代**的无关怪 —— 校验有效。注释还诚实标注"今日不可达"，并用 `EXPECT_EQ(leaderFailures, 0)` 做**跳线**（若哪天真开始失败，是断言变红而不是死分支悄悄变活）。索引一致性也成立：`PlaceGroup` 写 `Monsters[AMC]`，squad 代码读 `Monsters[ActiveMonsters[AMC-1]]`，而 `InitLevelMonsters` 的 `std::iota` + 建关期无删除保证了恒等映射。

10. **单一真相源被两边共用**：`IsCoreRosterMember`/测试侧 `IsCoreMemberOfLevel` 都查 `GetLevelRoster()`；`SquadFormationRate`/`SquadRateIsMeasuredPerLevel` 都读 `GetSquadRollStats()`（生产计数器）而非重算。`PlaceGroup` 从匿名命名空间移出为导出符号使测试能直驱，且移动是**纯搬迁 + 三处 opts 分支**，无夹带改动。

11. **`squad_leashed` 解析器拒绝未知拼写**（不静默默认），并有两条 `EXPECT_EXIT` 死亡测试分别钉 `yes` 与 `2`，匹配的是解析器**自己**的消息而非通用"字段无效"。`ParamsCarrySquadColumns` 用两行值全不同的夹具（L1 `40/3/true`、L2 `0/0/false`）驱动真实 loader，并同时复查 `maxImage`/`tailDraw` 未被移位 —— 列序错位会被抓到。

12. **验收 8/9c 本轮都重测了，且发生过一次正确的跨验收取舍**：L14 抬 Melee floor 虽过验收 8 却把 9c 从 6/6 打到 2/6 → 自行回退，改为只动 `squad_chance`（30→10），未动 ceiling（守 R4/宪章）。这是本轮最值得肯定的纪律。

13. **工具层与文档配套到位**：`retail_or_hf_required` 把"缺零售素材"表达成**门控**而不是下调 `passed_min` 或放宽断言（明确写进 yaml 注释）；`run_tests.py --test` 先构建目标并在目标不存在时明确失败；存档"内容模式变化 vs 文件格式变化"的区分写进了 `decision_save_format_policy.md`；`pattern_assertions_must_be_failable.md` 把本会话 4 次复发的缺陷模式系统化了。

---

## 问题

### 严重（必须修复）

**S1. G1 的索引清理漏掉 `LeaderRelation::Separated` —— 悬挂索引在普通小队上仍然可达，且这是 G1 自己承诺要关掉的缺陷类**

`ReleaseMinions`（`Source/monster.cpp:1418-1428`）的过滤条件是：

```cpp
if (minion.leaderRelation == LeaderRelation::Leashed && minion.getLeader() == &leader)
```

只覆盖 `Leashed`。但 `GroupUnity`（`:1635-1668`）会在 leader 与随从之间视线被挡时把随从改成 `Separated`：

```cpp
} else if (monster.leaderRelation == LeaderRelation::Leashed) {
    leader.packSize--;
    monster.leaderRelation = LeaderRelation::Separated;
}
```

`Separated` 随从**保留 `leader` 索引**。此时若普通 leader 死亡：

1. `M_UpdateRelations` → `ReleaseMinions(monster, clearReference=true)` → 该随从 `relation != Leashed`，**被跳过**，`leader` 索引不清、`relation` 仍为 `Separated`；
2. leader `isInvalid = true` → 下一 tick `DeleteMonsterList()` → `DeleteMonster` 只做 `ActiveMonsterCount--` + `std::swap`，**槽位（`monsterId`）被换到 `ActiveMonsterCount` 之后待复用**；
3. 后续 `AddMonster` / `SpawnMonster`（`:4077` `Monsters[ActiveMonsters[ActiveMonsterCount++]]`）复用该槽位；
4. 该随从下一 tick 进 `GroupUnity`：入口只挡 `None`，**`Separated` 直接通过** → `auto &leader = *monster.getLeader();` 解到**被复用的无关怪**；若视线通且距离 <4 则 `leader.packSize++` 并把自己改回 `Leashed` → 此后 `DirOK`（`:4662`）把该随从的移动锁死在**那只陌生怪**的 4 格内，`FollowTheLeader` 还会同步它的 `position.last`/`activeForTicks`。

这正是规格 §4.3.1 与 `ReleaseMinions` 注释所声称要消除的场景（"keeping its index would leave the minion pointing at an unrelated live monster"），只是换成了 `Separated` 状态。它在阶段 B **新变得可达**：此前普通怪不可能当 leader，`Separated` 的普通随从只存在于 unique 队下（unique 路径按规格刻意保留索引，属既有语义）；核心小队接入后，普通 leader + 普通随从成为常态，而"引开 leader >4 格 → 脱队"恰好是附录 D 预测 5 的正常玩法路径。

台账把预测 5 记为"需人工试玩、未覆盖"，但记的是**归队行为**未被驱动；没人注意到**悬挂索引在该状态下依然存活**。所以这不是"已知延期项"，是评审新发现。

- 后果：玩家可见（随从被拴到无关怪身上、移动异常），且 `packSize` 被错误 `++` 会污染那只怪的 `DirOK` 判定（`mcount == monster.packSize`）。非内存越界（索引恒在 `Monsters[]` 范围内），但语义别名。
- 最小修法：`ReleaseMinions` 的过滤放宽到 `leaderRelation != None`，但**只在 `clearReference` 为真（非 unique）时**处理 `Separated` 分支，且对 `Separated` 只清索引 + 置 `None`、**不要**走 `setLeader(nullptr)` 之外的额外动作 —— 必须保证 unique 路径逐字节不变（unique 传 `clearReference=false`，其 `Separated` 随从须保持现状）。注意 `ShrinkLeaderPacksize` 对 `Separated` 的死者不减 `packSize` 是**正确**的（分离时已减过），修 `ReleaseMinions` 不要动它。
- 必须补一条可失败用例：普通 leader + 随从 → 手工置 `relation = Separated`（`GroupUnity` 是匿名符号，直接置状态即可，这是状态机的合法中间态）→ 走 `RunEngineDeath` → 断言 `leader == NoLeader` 且 `relation == None`；同时补 unique 版回归断言"`Separated` 随从的索引仍保留"，锁住不变性。

**S2. 小队随从被 `monhealthbar` 染成蓝色 —— 一处未被量化也未被守卫的玩家可见行为变化，且语义与事实相反**

`Source/qol/monhealthbar.cpp:147-152`：

```cpp
if (monster.isUnique())      style |= UiFlags::ColorWhitegold;
else if (monster.leader != Monster::NoLeader) style |= UiFlags::ColorBlue;
else                          style |= UiFlags::ColorWhite;
```

判据是 `leader != NoLeader`，而**不是**"被 buff 过"。`setLeader` 内的注释写明这个索引之所以被保留就是为了"buffed minions are drawn with a distinct colour"，即蓝色在既有语义里等于**HP×2 的强化随从**。

小队随从走 `setLeader` → `leader` 索引被写入 → **一律显示蓝名**；但它们按 G2 明确传 `tough=false`，**没有任何强化**。于是从阶段 B 起，蓝名不再意味着"更硬"，玩家（尤其本项目定位的 D1 老玩家）读到的是错误信号。

- 这条与红线 9/11 直接相关：小队"只买构成不买数值"是本特性的核心承诺，而 UI 恰好在宣告相反的事。
- 台账只在 RB8 提到 `monhealthbar`，讨论的是**悬挂索引风险**，不是着色语义；规格 §4.3、验收 5-9c、eval 描述里都没有这一项。属"对玩家可见行为的改动未被量化与守卫"。
- 处置建议（任一，需作者裁决其一）：(a) 把着色判据改成真实的"被强化"判据（例如 leader `isUnique()`，或显式标记），使蓝名恢复"强化随从"含义；(b) 明确接受"蓝名 = 有 leader"这一新语义，写进规格并说明为何不误导。**不建议**默认现状 —— 因为现状是未记录的语义漂移。无论选哪条都应补一条针对 `GetBorderColor`/着色分支的可失败用例或至少写入规格与台账。

### 重要（建议修复）

**I1. `ObserveSquads` 的 `minionsOutsideLeash` 是恒真断言（残余乐观偏差，形态 1/2）**

`test/level_roster_baseline_test.cpp:311-320` 用 `dx > 4 || dy > 4` 判越界，并在注释里解释了为何不用 `< 4`（钳制中心是 leader 的邻格）。但顺着同一推理往下算：`PlaceGroup` 拒绝 `abs(xp-x1) >= 4` → 通过的候选满足 `abs(xp-x1) <= 3`；`x1 = leader.x + delta`，`|delta| <= 1` → `dx <= 4`。**`dx > 4` 在构造上不可能成立**，`minionsOutsideLeash` 恒为 0，`SquadFormsAroundACoreLeader` 里那条 `EXPECT_EQ(obs.minionsOutsideLeash, 0u)` 因此是装饰而非守卫 —— 指不出"什么改动会让它红"（把 `&leader` 改 `nullptr` 会先在 `leader->packSize = placed` 上空指针崩溃，不是这条变红）。

实际风险低（邻近性本就由构造保证，且 leashed 路径另有 `leadersWithMinions > 0` 等守卫兜底），但这正是本会话反复出现的"看起来在守、实际不守"。建议二选一：把界改成**可失败的** 4（即断言 `dx <= 4` 的紧界会因钳制逻辑被改动而红），或删掉该计数并在注释里说明"该性质由构造保证、由 `maxPartnerLeaderDistance` 统一守"，别留一条无区分力的断言充数。

**I2. leashed 小队的"随从数 = `squad_size`"没有任何断言**

`[ SQUADRATE ]` 打印 per-squad 1.98593，`[ SHIPPEDSQUAD ]` 打印 minions/realised ≈ 1.97-2.0，但**只打印不断言**（`pattern_assertions_must_be_failable.md` 形态 4：测量没有断言）。把 `PlaceGroup(partnerIndex, rosterParams->squadSize, ...)` 的第 2 参数写成常量 1，全部 10 条用例仍会绿：`formed`（≥1 只即计）、`realised`（`packSize>0`）、形成率 floor、邻近性界、异类型、AI、packSize 非 0 —— 无一条能区分 1 只与 2 只。即"`squad_size` 这一旋钮是否真的生效"当前无守卫。

建议在 `ShippedSquadChanceRealisesSquadsOnEveryLevel` 或 `SquadRateIsMeasuredPerLevel` 加一条弱但可失败的断言，例如 `minions >= realised * 1.5`（出厂实测 1.98，把 `squadSize` 改 1 会掉到 1.0 必红），或直接断言 `partnersPlaced` 与 `rolls * squadSize` 的比例下界。注意别写成紧界（放置折损真实存在）。

**I3. `passed_min` 硬编码计数机制在本轮又一次被推到更脆的位置（R35 延期项已到该收的时候）**

`level-rosters.yaml` 的 `passed_min: 10` + `output_contains: "[  PASSED  ] 10 tests."` 与 `sampling-anti-monopoly.yaml` 的 30 都是手抄计数，本轮两处各改了 2 次（3→8→9→10、26→28→30）。这个机制的失效模式是**静默的**（N 偏小 → 少跑也算过）。本轮它还与 `skipped_max: 1` 耦合：`level-rosters` 的 10 个用例里 7 个带 `missingRetailTrn_` 的 `GTEST_SKIP`，一旦素材探测口径变化，会是"9 skip + 1 pass"这种既不满足 `passed_min` 也超 `skipped_max` 的混乱态，而不是干净的整案跳过（整案跳过只由 `retail_or_hf_required` 保证）。

不阻塞合并（现状是正确的、且有注释提醒），但这条延期项每轮都在加成本，建议在下一轮（A2 之前）改成从 gtest 的 `--gtest_list_tests` 或 XML 输出推导期望数。

### 轻微（可选优化）

**M1. `SquadsAreAbsentWhenTheTableDisablesThem` 的注释说 A/B "only squad_chance = 0"，但夹具 `squads_off.tsv` 同时把 `squad_size` 也改成 0**（两列都变）。两个门（`squadChance > 0 && squadSize > 0`）任一为 0 都关闭小队，所以结论不受影响，但"只差一列"的说法与夹具不符 —— 而"A/B 只差一个变量"正是这些用例的立论基础，注释不该在这点上失准。改注释或把夹具的 `squad_size` 保持 2 即可。

**M2. 台账 RB14 的"`squadSize=2 < na` → 成队反而少占槽位"这条推理不精确。** `na` 有 50% 概率为 1（`currlevel == 1 || FlipCoin()`），另外 50% 才是 2-3（L2/Crypt）或 3-5。所以小队占 3 槽相对**均值** ≈2.5 是**略多**、而非"反而少"。结论（未出现槽位不足折损）由 500 seed 实测 99.89-100% 支撑，不受影响；但这条被用来解释"为何 4 格/槽位折损没出现"，写在台账里会被后续任务当事实引用。建议改成"折损未出现，依据是实测形成率，机制上小队占 3 槽与 `na` 均值相当"。

**M3. `SquadRateIsMeasuredPerLevel` 与 `ShippedSquadChanceRealisesSquadsOnEveryLevel` 重叠度较高**（都跑 15 层 ×50 seed，各约 27-30s），前者用 `squads_always` 夹具、后者用出厂表，断言集大部分重复。加上 `SquadFormationRate` 的 290s，单个 eval case 已到 ~475s / timeout 900。若下一轮还要加长用例，先考虑把这两条的层范围或 seed 数合并降重，而不是继续上调 timeout。

**M4. `Timedemo.WarriorLevel1to2` 仍在隔离（`GTEST_SKIP`）**，而本轮 L1/L2 的 `squad_chance=30` 必然改变这两层的放置序列与 RNG 流。CLAUDE.md 要求"改动存档/玩家状态后必跑 timedemo"，本轮客观上无法满足。属既有隔离（`gotcha_timedemo_isOnActiveLevel_failure.md`），不是本轮引入，但**重录夹具的必要性因本轮改动而增加**了 —— 建议在收尾呈报里把"timedemo 夹具需重录"列为已知缺口，别让它在隔离状态下越积越久。

**M5. `PlaceMonster` 对 `MT_NAKRUL` 的早退不 `return` 到 `PlaceGroup`**（`PlaceGroup` 仍 `ActiveMonsterCount++`，留下一只未初始化的怪）。既有缺陷，且 `MT_NAKRUL` 不在任何名册的 core 行（实测 0 命中）、`availability` 为 Never，本轮不可达。仅记录，勿在本轮处理。

---

## 延期项分拣

**必须合并前修**
- **S1（G1 的 `Separated` 漏洞）** —— 新发现，非既有延期项。它使 G1 的规格承诺（"清除悬挂 `leader` 索引"）在阶段 B 新引入的普通小队上不成立，且路径是正常玩法。修 + 补两条用例（普通清除 / unique 保留）。
- **S2（蓝名语义）** —— 需作者裁决 (a)/(b) 之一并落到规格；不能以现状默认通过。

**建议合并前修（成本低、都在测试层）**
- I1（恒真断言）、I2（`squad_size` 无守卫）。两条都只改测试，不动生产代码，不会触发验收 8/9c 重测。

**可留后续（同意现有分拣）**
- **RB17 / L14 三选一（+ 控制者的 D 方案）**：分析扎实、余量 0.36pp 的风险如实记录、已入规格附录 E。同意作为收尾呈报项交作者，不阻塞合并。我倾向 **D 先试、失败退 A**：改 core 名单而非 floor，代价面确实与被否的"抬 Melee floor"不同，且是唯一能在不放宽任何守卫的前提下让 L14 参与本特性的路径。**C（提 ceiling）应明确排除** —— 那是放宽红线 14 的守卫。
- **预测 5 未覆盖**：`GroupUnity`/`FollowTheLeader` 是匿名命名空间符号、唯一调用点在 `ProcessMonsters()` tick 内、仓库无驱动完整 tick 的测试 —— 声明诚实，接受"需人工试玩"。**但注意：S1 就落在这条未覆盖区里。** 修 S1 时的用例可以绕开 tick（直接置 `Separated` 后走死亡路径），所以 S1 不因预测 5 未覆盖而豁免。
- **A2（HF overlay L17-24）**：现在多了一层含义 —— L17-24 无 params 行 → `rosterParams == nullptr` → **完全没有小队**。红线 12「全层段定义」的缺口从"名册"扩大到"小队"。同意 A2 排下一轮第一优先。
- **O2（加载期校验未含 quest 预加合计）**、`passed_min` 机制（I3）、`clang-format` 14 vs 18、`drlg_l2.cpp:2072` UBSan 越界（单独开单）：均可留后续。

---

## 改进建议

1. **修 S1 时顺手把"leader 关系状态机的三态"写成一条知识**（`None`/`Leashed`/`Separated` 各自谁写、谁读、谁负责清理）。本轮已经在同一处踩了两次：G1 修 `Leashed`、漏 `Separated`；根因是没有一份"哪些消费者会解 `leader` 索引"的清单。消费者清单实际是：`GroupUnity`、`FollowTheLeader`、`DirOK`、`ShrinkLeaderPacksize`、`ScavengerAi`、`SyncPackSize`（loadsave）、`monhealthbar`、`msg.cpp` delta 载入路径（经 `M_UpdateRelations`）。把这 8 处连同各自的入口条件写下来，下次改关系语义就不必重新扫。

2. **给"玩家可见信号"建一个与验收并列的检查位**。S2 之所以漏，是因为验收 5-9c 全在量**放置构成与数值**，没有一条覆盖 UI/反馈。既然本项目的红线 9/11 是关于"玩家感知"的，建议在规格 §5 红线检查里加一行固定问句："本改动是否改变任何玩家可见的**标识/提示/着色**语义？"

3. **`SquadRollCounters` 已经是好设计，但注释密度已超过代码**（`monster.h` 里 98 行新增几乎全是注释）。这些注释的信息价值很高（尤其 `formed` vs `realised` 那段），但放在头文件里会被每个 include 者读到。建议把"为什么 `formed` 与 leash 无关"这类**决策**移到 `docs/knowledge/`（并从注释指向它），头文件只留字段语义。

4. **反证记录建议进仓**。本轮做了至少 6 次"改坏→红→恢复→绿"，证据散在 `task-*-report.md`（该目录被 gitignore）。这些反证是守卫可失败性的**唯一凭据**，随工作区一起消失很可惜。建议每条守卫在**代码注释里**一句话写明"什么改动会让它红"（部分已做，如 `leaderFailures` 跳线、界 32 的两侧实测），把它变成随代码走的资产。

---

## 总体评估

阶段 B 的工程质量明显高于典型水平：两道前置修复都对准了真实缺陷而非表面症状；`MinionOptions` 的默认值设计让 unique 路径逐字节不变这一承诺**确实成立**（逐行核对 + 同种子 A/B 实跑）；形成率守卫的分子/分母语义经过一次正确纠偏，避免了"守卫因自己要求的补救被采用而变红"这类自相矛盾；对无法证紧界的地方（回退邻近性界 32）选择了"两侧实测 + 声明非紧界"而不是假装严谨。跨验收取舍（L14 抬 floor 过了 8 却打断 9c → 自行回退）体现了真实纪律。

两处必须修：**S1** 是 G1 承诺的缺陷类在 `Separated` 状态下的残余，因阶段 B 引入普通 leader 而**新变得可达**，且落在"引开 leader → 脱队"这条正常玩法路径上，后果玩家可见；**S2** 是小队随从被染成蓝名，而蓝名在既有代码里的含义是"被强化的随从"，与本特性"只买构成不买数值"的核心承诺相反 —— 一处未记录、未量化、未守卫的玩家可见语义漂移。两条都不是设计错误，是覆盖缺口，修复成本都不高（S1 一处过滤条件 + 两条用例；S2 需作者在两个方案间裁决）。

另建议同轮修掉 I1（一条恒真断言）与 I2（`squad_size` 旋钮无守卫）—— 都只动测试，且都属本会话反复强调的"看起来在守、实际不守"的残余。

计划/规格本身没有发现需要返工的问题；与计划的偏离只有一处且合理（L14 `squad_chance` 30→10，为守验收 8 同时不破 9c，已按 R4 只改数据、未动 ceiling、已入附录 E 并列为呈报项）。

**是否可合并？** 修复后可合并

**理由：** 主体实现、测试可失败性与文档配套都达到了合并标准，但 S1（G1 的悬挂索引在 `Separated` 状态下仍可达，因本阶段引入普通 leader 而新变得可达）与 S2（未记录的蓝名语义漂移，与"不买数值"的承诺相反）都是玩家可见的缺口，须在合并前分别修复与裁决。
