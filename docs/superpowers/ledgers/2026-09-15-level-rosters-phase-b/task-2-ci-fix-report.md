# Task 2 CI Fix Report (阶段 B / G2 测试修复)

状态：完成。提交 `e4abc02e3`。

范围：只改 `test/sampling_behavior_test.cpp`（必要时 `eval/cases/rng/sampling-anti-monopoly.yaml` 仅加注释）。

待修项：S1（AI 断言恒真）、S2（CI 素材不安全）、I3（HP 断言区间重叠）、I4（内联前提检查注释）、轻微（424 行附近矛盾注释）。

## 0. 起点状态

- `git status`: `M test/sampling_behavior_test.cpp`（前一位修复者留下的 16 行硬编码 `genrl.trn` 探测，方向错误，本次改为通用探测）
- HEAD: b61995f4e

## 1. 事实核对（代码/数据实证）

### `InitTRNForUniqueMonster` 实际使用的字段
`Source/monster.cpp:3346-3352`：
```cpp
char filestr[64];
*BufCopy(filestr, R"(monsters\monsters\)", UniqueMonstersData[static_cast<size_t>(monster.uniqueType)].mTrnName, ".trn") = '\0';
ASSIGN_OR_RETURN(monster.uniqueMonsterTRN, LoadFileInMemWithStatus<uint8_t>(filestr));
```
→ 字段是 `UniqueMonsterData::mTrnName`（`Source/tables/monstdat.h:328`，从 `unique_monstdat.tsv` 的 `trn` 列读入，`monstdat.cpp:434`），
路径拼接为 `monsters\monsters\<mTrnName>.trn`（**不含目录、不含扩展名**，两端都是代码补的）。
→ 因此通用探测必须用**所选 unique 自己的 `mTrnName`**，硬编码 `genrl.trn` 是错的。

### 为什么 CI 报 `genrl.trn` 而当前树落到 `bhka.trn`（同一机制换文件）
`MT_NAKRUL`（unique idx12，`trn=genrl`）**不在** `assets/txtdata/monsters/monstdat.tsv` 里（`grep -c NAKRUL` = 0），
而 `MonsterData::availability` 默认值是 `MonsterAvailability::Never`（`monstdat.h:106`）→ 内联前提检查的
`availability == Never` 分支把它筛掉 → 当前树扫描落到 idx13 `MT_TSKELAX`（`trn=bhka`）。
CI 那次 tsv 里还有 NAKRUL，故落 idx12/`genrl.trn`。**两者都是零售/HF 才有的素材**，硬编码任一都不通用。

### S1 恒真的数据依据
`unique_monstdat.tsv` × `monstdat.tsv` 交叉核对 Leashed 条目（脚本实跑）：
- idx13 `MT_TSKELAX` Bonehead Keenaxe：`uAi=SkeletonMelee`，`baseAi=SkeletonMelee` → **相等**
  → `EXPECT_EQ(minion.ai, monster.ai)` 无区分力（minion 的 own AI 已等于 leader 的 AI）。
- 首个 `uAi != baseAi` 且 `availability != Never` 的 Leashed 条目：idx17 `MT_WSKELAX` Boneripper
  （`uAi=Bat`，`baseAi=SkeletonMelee`，avail=Always，`trn=br`，lvl 1-2，hp 2-4）。
→ 筛选条件加 `UniqueMonstersData[u].mAi != MonstersData[mtype].ai` 后自然选到它，断言恢复区分力。
## 2. 改动内容（只动 `test/sampling_behavior_test.cpp`）

1. **回退**前一位修复者的硬编码探测：删掉 `SamplingBaselineTest::missingUniqueTrn_` 静态标志、
   `SetUpTestSuite` 里的 `genrl.trn` 探测、以及 `UniqueMinionsBehaviourUnchanged` 顶部的对应
   `GTEST_SKIP`。`Source/` 侧零改动（`git diff --stat -- Source/` 为空，已核实）。
2. **S1**：`UniqueMinionsBehaviourUnchanged` 的扫描新增筛选
   `UniqueMonstersData[u].mAi != MonstersData[mtype].ai`，并加 `ASSERT_NE` 把该前提写成显式断言。
   选中的 unique 由 idx13 `MT_TSKELAX`（uAi==baseAi，恒真）变为 **idx17 `MT_WSKELAX` Boneripper**
   （uAi=Bat=0x0B，baseAi=SkeletonMelee=0x02，`trn=br`）。
3. **S2**：探测点搬到**选中该 unique 之后**（测试体内），路径由所选 unique 自己的 `mTrnName` 拼出：
   `StrCat(R"(monsters\monsters\)", uniqueMonsterData.mTrnName, ".trn")` → `OpenAsset` →
   缺失即 `GTEST_SKIP() << "retail/HF TRN " << trnPath << " not available"`。
   探测与使用的是**同一个 unique**（同一个 `uniqueMonsterData` 引用），不存在探一个用另一个。
   未用 `HaveHellfire()`。
4. **I3**：`SquadMinionsUnbuffed` 与 `UniqueMinionsBehaviourUnchanged` 的 HP 断言都改为
   **同种子 A/B**（首选方案，实测成立）：同一 `SetRndSeed`、同一 leader/minion 类型，仅翻转
   `opts.tough`，断言 `doubled.maxHitPoints == 2 * unbuffed.maxHitPoints`；并加
   `ASSERT_GT(..., 0)` 防止 0==0 退化。区间判据（`[256,512]` vs `[512,1024]`，512 重叠）已删除，
   连同那段"staying within the un-doubled range demonstrates..."的不成立论证。
   两处也同时断言 AI / intelligence 不被 `opts.tough` 影响。
5. **I4**：内联前提检查改名为 "Candidate PRE-FILTER only"，注释明确写出：
   ①删了 `IsMonsterAvailable` 的 `dunLvl` 检查；②`gbIsSpawn && Retail` 那支在本 fixture 是死代码
   （`SetUpTestSuite` 无条件 `gbIsSpawn = false`）；③因此**筛不掉缺素材的零售 unique**，
   素材可用性**一律**以下方 TRN 探测为准，不得从该循环推断。
6. **轻微**：那段自相矛盾的注释（"baseMaxHp < 2 * baseMinHp for MT_TSKELBW (16 < 2*8 is false... see below)"）
   随 I3 重写一并删除（`grep "16 < 2\*8\|see below"` 已无命中）。
7. 新增 `#include <optional>`（A/B lambda 用 `std::optional<MinionOptions>`）。CRLF 保持不变（`file` 已核实）。

## 3. S1 反证（改坏 → 必红 → 恢复 → 绿），本地有素材环境

三个变异体都临时改 `Source/monster.cpp` 的 `PlaceGroup`，**测完全部恢复**。

### 变异体 A：强制**非**继承（去掉 `if (!opts.inheritAi)` 条件，无条件 `minion.ai = ownAi`）
```
[ RUN      ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged
test/sampling_behavior_test.cpp:2043: Failure
Expected equality of these values:
  uniquePack.ai
    Which is: 1-byte object <02>
  uniquePack.leaderAi
    Which is: 1-byte object <0B>
default MinionOptions must still inherit the leader's AI (regression)
test/sampling_behavior_test.cpp:2045: Failure
Expected: (uniquePack.ai) != (MonstersData[uniqueMonsterData.mtype].ai), actual: 1-byte object <02> vs 1-byte object <02>
the inherited AI must actually differ from the minion's own base-type AI
[  FAILED  ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged (0 ms)
```
→ **必须变红，已变红**。0x02=SkeletonMelee（minion 自己的 base AI），0x0B=Bat（leader/Boneripper 的 uAi），
证明选中的 unique 确实 uAi≠baseAi，断言有区分力。**修复前（未加筛选条件时）同一变异体下该用例是绿的** ——
这正是 S1 所指的恒真。

### 变异体 B：去掉恢复逻辑（强制**总是**继承）
```
[ RUN      ] SamplingBaselineTest.SquadMinionsUnbuffed
test/sampling_behavior_test.cpp:1888: Failure
Expected equality of these values:
  squad.ai
    Which is: 1-byte object <02>
  MonstersData[MT_TSKELBW].ai
    Which is: 1-byte object <03>
opts.inheritAi=false must keep the minion's own AI
test/sampling_behavior_test.cpp:1889: Failure
Expected: (squad.ai) != (squad.leaderAi), actual: 1-byte object <02> vs 1-byte object <02>
the minion's AI must not have been overwritten by setLeader()
[  FAILED  ] SamplingBaselineTest.SquadMinionsUnbuffed (0 ms)
```
→ 反向变异也被抓住（`SquadMinionsUnbuffed` 侧）。

### 变异体 C：忽略 `opts.tough`（永不加倍）—— 验证 I3 的新 HP 判据真能区分
```
[ RUN      ] SamplingBaselineTest.SquadMinionsUnbuffed
test/sampling_behavior_test.cpp:1880: Failure
  toughened.maxHitPoints  Which is: 384
  2 * squad.maxHitPoints  Which is: 768
[  FAILED  ] SamplingBaselineTest.SquadMinionsUnbuffed (0 ms)
[ RUN      ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged
test/sampling_behavior_test.cpp:2035: Failure
  uniquePack.maxHitPoints    Which is: 96
  2 * unbuffed.maxHitPoints  Which is: 192
[  FAILED  ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged (0 ms)
```
→ **两条用例都变红**。旧的区间判据在 384（落在未加倍区间 [256,512]）这种取值下**抓不住**，
新的 A/B 判据抓住了。这就是 I3 首选方案实测成立的依据：同种子两路 RNG 消耗一致，等式精确成立。

### 恢复后（`git diff --stat -- Source/` 为空）
```
[ RUN      ] SamplingBaselineTest.LeaderDeathReleasesMinions
[       OK ] SamplingBaselineTest.LeaderDeathReleasesMinions (11 ms)
[ RUN      ] SamplingBaselineTest.SquadMinionsUnbuffed
[       OK ] SamplingBaselineTest.SquadMinionsUnbuffed (0 ms)
[ RUN      ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged
[       OK ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged (0 ms)
[  PASSED  ] 3 tests.
```

## 4. 本地有素材：整二进制全绿、两条用例都不跳过
```
[==========] 32 tests from 2 test suites ran. (4959 ms total)
[  PASSED  ] 32 tests.
```
（`SamplingBaselineTest` 30 + `HellfireNoParamsSamplingTest` 2 = 32；无 Skipped 行。）

## 5. 门禁（本地有素材）

```
python3 tools/run_tests.py --json /tmp/ci.json
ctest: {'passed': 749, 'failed': 0, 'skipped': 3, 'total': 749, 'passed_pct': 100, 'returncode': 0}
drift_ok: True
PASS A / PASS B / PASS C / PASS C2 / PASS E
```
→ `failed==0 && passed_pct==100 && drift_ok==true` ✅

```
python3 -m tools.eval.backend --smoke  → EXIT=0
dark-expedition 4/4 | save-load 2/2 | mechanics 8/8 | data 6/6 | render 2/2 | combat 5/5 | utility 9/9
```

## 6. 本地模拟 spawn-only（S2 验收）

做法：把 `~/.local/share/diasurgical/devilution/` 下 `DIABDAT.MPQ` / `HELLFIRE.MPQ` / `hellfire.mpq`
三个都临时改名为 `*.bak_spawnonly_test`，只留 `spawn.mpq`（`LoadGameArchives` 会让 spawn 接管
`MainMpqPriority`，故 `HaveMainData()` 仍为真、不会整套 blanket-skip）。

### 修复后（本次改动）
```
[==========] 32 tests from 2 test suites ran. (4676 ms total)
[  PASSED  ] 29 tests.
[  SKIPPED ] 3 tests, listed below:
[  SKIPPED ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged
[  SKIPPED ] HellfireNoParamsSamplingTest.LevelsWithoutParamsStillSampleTypes
[  SKIPPED ] HellfireNoParamsSamplingTest.NoParamsTailExceedsTheParameterisedCap

Test Skip Summary
  • 2 tests skipped: hf overlay required: L17-24 have no candidates under base monstdat
  • 1 test skipped: retail/HF TRN monsters\monsters\br.trn not available
```
→ 该 unique 用例 **Skipped** ✅；**零 FAILED** ✅；`egrep -i "FAILED|\.trn|Failure"` 唯一命中就是那条
skip 文案本身（`br.trn`），**没有任何 `.trn` 加载失败** ✅。
注意跳过消息里报的是 **`br.trn`**（当前选中的 unique = MT_WSKELAX/Boneripper），
而不是硬编码的 `genrl.trn` —— 通用探测确实跟着所选 unique 走。

### 对照：HEAD 版本（未修复）在同一环境
```
git stash push -- test/sampling_behavior_test.cpp   # 回到 HEAD 版本
The MPQ file(s) might be damaged. Please check the file integrity.
[  FAILED  ] SamplingBaselineTest.UniqueMinionsBehaviourUnchanged (0 ms)
 1 FAILED TEST
```
→ 证实 S2 是真问题（CI 必红），本次修复确实解决它。

### 恢复确认
三个文件已全部改回原名，`ls | grep -c bak_spawnonly` = **0**；
恢复后重跑整二进制：`[==========] 32 tests ran. [  PASSED  ] 32 tests.` ✅

## 7. `passed_min` 口径说明（eval/cases/rng/sampling-anti-monopoly.yaml）

- 本环境（有 `DIABDAT.MPQ` + `hellfire.mpq`）：`SamplingBaselineTest` **30** 个用例全过、零跳过
  → `passed_min: 30` 与 `output_contains: ["[  PASSED  ] 30 tests."]` **保持不变，无需改动**。
- spawn-only 环境：`UniqueMinionsBehaviourUnchanged` 会 `GTEST_SKIP`，因此计数为 **29**、`skipped=1`。
  该 case 的 `skipped_max: 1` 已能容纳这一条；但 `output_contains` 那句硬编码 "30 tests." 在
  spawn-only 下不会出现。
- **本次不为迁就 spawn-only 而下调 `passed_min` 或放宽任何断言**：该 case 的 `setup.mpq_required: true`
  是针对有素材环境的门禁口径，在有素材环境下必须是 30/30。CI 若以 spawn-only 跑该 eval case，
  应由 CI 侧决定是否跳过该 case，而不是把断言改弱。
- YAML 仅追加一条口径注释，不改任何期望值。

## 8. 结论

S1 / S2 / I3 / I4 / 轻微 五项**全部完成**，只改了 `test/sampling_behavior_test.cpp`
（`Source/` 侧零改动，`git diff --stat -- Source/` 为空已核实）。行尾保持 CRLF（`file` 核实）。

## 9. 提交

`e4abc02e3` `test(monster): make the unique regression non-vacuous and asset-safe`
- `test/sampling_behavior_test.cpp`（主体）
- `eval/cases/rng/sampling-anti-monopoly.yaml`（**仅追加 7 行口径注释**，期望值一字未改）

`python3 tools/check_drift.py --base origin/master` → PASS A/B/C/C2/E。

## 10. 顾虑

1. **A/B 的 RNG 对齐是隐式前提**。同种子 A/B 成立依赖两次放置消耗相同的 RNG 序列
   （leader 用 `inMap=false` 不占格、两次用互不相邻的 leader tile 40,40 / 60,60，
   使 `PlaceGroup` 第一候选格都干净、一次成功）。这一点已由变异体 C 侧面验证
   （未加倍时等式为 384 vs 768，比例恰好 2，说明两路 roll 值相同）。若将来有人改动
   `PlaceGroup` 的放置循环或 `InitMonster` 的抽取次数，等式可能失衡而报"lost RNG alignment"
   —— 断言文案已显式提示这一失败模式，不会被误读成"加倍逻辑坏了"。
2. **`UniqueMinionsBehaviourUnchanged` 的对照支路不走 `PrepareUniqueMonst`**。
   `PrepareUniqueMonst` 不接受 `MinionOptions`（这正是被回归保护的行为），所以 tough=false
   对照只能直接调 `PlaceGroup`，并手工复制 leader 的 `ai`/`intelligence` 两个字段
   （PlaceGroup 的继承只会读这两个）。这是必要的近似，已在注释里写明。
3. 所选 unique 会随 `unique_monstdat.tsv` / `monstdat.tsv` 变化而漂移（当前 MT_WSKELAX）。
   这是**有意**的设计（不硬编码），且 `ASSERT_NE` + TRN 探测都跟着所选项走，
   漂移不会让断言恒真或让 CI 误红。
4. eval case 的 `output_contains: ["[  PASSED  ] 30 tests."]` 在 spawn-only 下不成立。
   本次按要求**未**改弱它；若 CI 打算在 spawn-only 环境跑该 eval case，需要 CI 侧决策
   （跳过该 case，或给 case 增加环境维度），这超出本任务范围。
