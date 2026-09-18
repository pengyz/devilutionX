# 密度契约规格复核（对抗性，只读）

复核对象：`docs/superpowers/specs/2026-09-16-content-density-contract-design.md`（草案 v1）
复核者：独立规格复核 agent（只读，无子代理）
时间：2026-09-18

状态：**复核完成**

---

## 待查清单进度

- [x] 1. 联合可满足性（D3=vanilla ∧ 远程占比≤基线+5pp，L13 实算）—— **发现严重死锁**
- [x] 2. §3 事实基础逐条核实 —— 绝大多数为真，两处需精度修正
- [x] 3. 附录 A 改写清单完整性 —— **不完整，遗漏 knowledge doc + eval case**
- [x] 4. vanilla 地板可精确导出性 —— 部分核实，"空名册"定义有精度缺口
- [x] 5. D4=75% 取舍自洽性 —— **规格分析方向错配**（D4非瓶颈，D1才是）
- [x] 6. 红线10/11诚实性 —— **"不新增压力"论证在L13不成立**
- [x] 7. 附录C 硬目标 vs 回退 矛盾分析 —— 字面不矛盾但表述需加强
- [x] 8. 范围遗漏 —— L1-12代码层面隔离，但玩家体感遗漏未被固定问句覆盖

---

## 1. 联合可满足性 — 【严重，规格核心缺陷】

**结论：在 §4.3 规定的"远程类保持 ≤2"（即 `cap_ranged=2`）约束下，L13 不存在任何 `tail_draw` 取值能同时满足 D1≥6、D3=vanilla(15)、D4≥75%、且远程占比≤vanilla+5pp。规格声称"可以同时满足"（§4.1/§4.3 的表述均未提及此冲突，附录C只讨论"D1达不到6"的情形，从未讨论"满足D1后远程比必然超标"），这是一个被规格完全忽视的死锁。**

### 复算方法与代码

用 `Source/monster.cpp:3634-3720` 的真实采样循环语义（读码复刻，非猜测）在 Python 中精确复刻：
- 池子：L13 candidates=15（Melee 9 / Turret 3 / Kite 3），核对自 `assets/txtdata/monsters/monstdat.tsv`（脚本见下）
- core = MT_NBLACK + MT_GUARD（均 Melee），核对自 `assets/txtdata/monsters/level_rosters.tsv:58-59`
- 循环语义：core 预加不受 cap 限制；tail 每轮先按 cap 过滤候选，再检查 `class_floors` 是否有未达标类别优先抽取，否则随机抽（`Source/monster.cpp:3671-3706`）
- 现网参数（`assets/txtdata/monsters/level_roster_params.tsv:14`）：L13 `tail_draw=1, class_floors=Melee=2`

### 扫描结果（`cap_melee=None`「非对称，按规格 §4.3 放量」, `cap_ranged=2`「规格规定远程不变」, `floor_melee=2`, 200 seeds）：

```
tail= 3: union=15/15 d1min=5  share=0.2860 (超ceiling 0.2773) D1ok=False rangedOK=False
tail= 4: union=15/15 d1min=6  share=0.3092 (超)              D1ok=True  rangedOK=False  <- D1首次达标，远程比已超标
tail= 5: union=15/15 d1min=7  share=0.3314 (超)
tail= 6: union=15/15 d1min=8  share=0.3375 (超)
tail= 7: union=15/15 d1min=9  share=0.3383 (超)
...(tail继续增大，share先升后微降，但恒 > 0.28，从未回落到 ≤0.2773)
tail=13: union=15/15 d1min=13 share=0.3077 (超)
```

即：**cap_ranged=2 时，一旦 D1（单局种类≥6）达标，远程占比必然落在 0.31~0.34 区间，稳定超过 vanilla+5pp 的 0.2773 天花板；D3=vanilla(15) 和 D4=100% 在所有 tail_draw≥3 时都容易满足，从不是瓶颈。瓶颈恰恰是"D1≥6"与"远程占比≤基线+5pp"这两条轴之间的直接冲突，而不是规格反复强调的"D3 vs 远程守卫"冲突。**

反直觉的原因：Melee 候选池只有 9 种（core 已占 2 种，tail 池剩 7 种 Melee vs 6 种 Ranged），一旦 `tail_draw` 增大到让 D1 越过 6，Melee 侧很快把 7 个候选耗尽（尤其是 `tail_draw≥9` 后 Melee 全部抽完），后续抽取被迫全部落在 Ranged 上，直接推高远程占比。**规格假设"非远程侧放量"能靠三重限制（core+tail_draw+预算）"自然"稳住远程压力（§4.3"为何安全"一栏），这个假设在 L13 具体池子结构下不成立**：因为 Melee 池太小、Ranged cap 仍是 2（不是 0），只要 tail_draw 稍大，远程占比必然被 Melee 枯竭效应反向推高，而不是被 cap 压低。

对照：仅当把 `cap_ranged` 从规格规定的 2 降到 1（即进一步收紧远程 cap，而规格明确说"远程侧 cap 原封不动"）才存在可行解（`tail=4..13` 均可行，如 `tail=4: share=0.2558 ≤ 0.2773, D1=6, D3=15, D4=100%`）。**这说明要联合满足四轴+远程守卫，唯一的自由参数（tail_draw）不够，必须触碰规格声明"不动"的远程 cap 本身，或扩池（增加非远程候选）——这正是规格 R4 反转条款自己列出的选项②③，但规格从未在 L13 这个具体案例上验证过选项①（仅调 tail_draw）行不通，反而在 §4.3"为何安全"里暗示①足够。**

### 复算脚本（内联留存，供复现）

```python
import random
from collections import Counter
melee = ['NBLACK','RTBLACK','RBLACK','GUARD','VTEXLRD','BALROG','NSNAKE','RSNAKE','BSNAKE']
turret = ['SUCCUBUS','SNOWWICH','COUNSLR']
kite = ['XACID','STORML','MAEL']
core = ['NBLACK','GUARD']
all_types = melee+turret+kite
def cls(t):
    if t in melee: return 'Melee'
    if t in turret: return 'Turret'
    if t in kite: return 'Kite'
def simulate_dungeon(tail_draw, cap_melee, cap_ranged, floor_melee, seed):
    rnd = random.Random(seed)
    present = list(core)
    counts = Counter(cls(t) for t in present)
    pool = [t for t in all_types if t not in present]
    tail_added = 0
    while pool and tail_added < tail_draw:
        candidates = [t for t in pool if not ((cap_melee if cls(t)=='Melee' else cap_ranged) is not None and counts[cls(t)] >= (cap_melee if cls(t)=='Melee' else cap_ranged))]
        if not candidates: break
        preferred = next((t for t in candidates if cls(t)=='Melee'), None) if counts['Melee'] < floor_melee else None
        pick = preferred if preferred else rnd.choice(candidates)
        present.append(pick); counts[cls(pick)] += 1; pool.remove(pick); tail_added += 1
    return present
# RANGED_CEILING = (3249+2076)/23405 + 0.05 = 0.2773 (kRangedShareBaseline[13] + kRangedShareTolerance,
#   level_roster_baseline_test.cpp:~180)
```

### 结论对规格的影响

规格必须改写为：**"L13（以及结构相似的其他地狱层）在保持远程 cap=2 不变的前提下，仅靠放量非远程 cap 和调 tail_draw 无法同时满足 D1≥6 与远程占比≤基线+5pp；必须显式选择：(a) 扩大非远程候选池（新增 Melee/其他非远程类怪物到该层可用范围），或 (b) 进一步收紧远程 cap（如降到1），或 (c) 放宽远程占比红线本身（与红线14矛盾）。"** 规格现在的措辞（"冲突时允许扩池/重推守卫/记录损失"）在原则上留了出路，但**没有指出这个冲突在 L13 是必然发生的（不是"万一发生"），也没有在方案落地前给出选择依据**，这会让实施者在动手时才发现方案①不可行，返工整个 §4.3。

---

## 2. §3 事实基础核实

逐条核对结果（凡标"✅ 核实为真"均为本轮实际读代码/算数据）：

| 事实 | 规格所述 | 复核结果 |
|---|---|---|
| 地狱 cap 对称 | L13-16 任意类≤2 | ✅ 核实为真 — `Source/tables/level_roster.cpp:152-160`：`if (level>=13 && level<=16) return 2;` 逐字符合 |
| 行为类别映射 | Melee/Turret/Kite/... 列表 | ✅ 核实为真 — `Source/tables/monstdat.cpp:483-509`，`GetBehaviorClass()` 逐条比对一致（含 `default→Boss`） |
| L13-15 池子唯一非远程类是 Melee | L13 Melee9/Turret3/Kite3；L14 Melee8/Turret5/Kite2；L15 Melee4/Turret5/Kite0 | ✅ 核实为真 — 本轮用 `monstdat.tsv` 现算（脚本见下），三层数字与 minDunLvl/maxDunLvl/availability 过滤后逐一匹配 |
| L13 core=Melee×2 | MT_NBLACK + MT_GUARD | ✅ 核实为真 — `level_rosters.tsv:58-59` |
| L14 core=Melee1+Turret1 | MT_VTEXLRD(Melee)+MT_SNOWWICH(Turret) | ✅ 核实为真 — `level_rosters.tsv:60-61` |
| L15 core=Melee2+Turret1 | MT_BALROG(Melee)+MT_SNOWWICH(Turret)+MT_GSNAKE(Melee) | ✅ 核实为真（规格写"Melee2+Turret1"，行 62-64 确认为 BALROG/Melee、SNOWWICH/Turret、GSNAKE/Melee，共 Melee2+Turret1，与规格一致） — `level_rosters.tsv:62-64` |
| 对称cap下地狱单局种类上限≈4-5 | 非远程最多=Melee2(core占满)+Golem1=3；凑6种且远程≤27.8%需非远程≥4.3种→数学不可达 | ⚠️ **部分核实，但推导本身有可疑之处**：规格原句"非远程最多=Melee2+Golem1=3"看似假设 tail 阶段 Melee 不能再抽（因为 core 已用满 cap=2），这与代码一致（`classCounts` 从 core 起算，cap 命中后该类型从 tail 候选移除，`monster.cpp:3660-3668`）。但"至少4.3种"与"27.8%"是何处来的 ceiling 未见出处（L13 ceiling 应为 27.73%，规格写"27.8%"是舍入差，非错误但precision不够，无碍结论）。数学推导本身（3 非远程 + 需要总数6→远程占比下限）成立，但这是**对称 cap** 下的推导，与本任务1发现的非对称 cap 下"D1≥6时远程比反而升到31-34%"结论**方向一致**（即无论对称还是非对称，L13 都难以在远程占比达标下凑够6种），只是规格自己没有把这个推导延伸到非对称 cap 场景去验证方案④是否真的解决了它——**这正是本报告任务1揭示的缺口**。 |
| `HellL13/14/15SameClassTailBaseline` 的 `EXPECT_EQ(tail,0.0)` 是对称cap产物 | — | ✅ **核实为真（修正：第一轮grep未搜准，第二轮精确定位到）**——`test/sampling_behavior_test.cpp:142/154/165` 三个独立测试 `HellL15/14/13SameClassTailBaseline`，均为 `EXPECT_EQ(tail, 0.0)`，注释明确写"cap (Hell same-class <=2) must drive it to 0"，直接依赖对称cap=2。非对称落地后，非远程类tail不再恒为0（放量后必然>0），这三个断言**必须改写**，规格判断正确。 |
| `RosterQuotaAllowanceIsBinding` 的饱和层前提依赖 cap=2 | 现由 L15 承担 | ✅ **核实为真（修正）**——`test/sampling_behavior_test.cpp:1042` 起：遍历 L1-15 找"核心已经把某capped class占满到cap值"的(level,class)对（`AvailableCoreClassCount(level,cls) == cap`），断言这个饱和列表非空（`ASSERT_FALSE(saturated.empty())`），且非Boss类"allowance恰等于cap、无冗余"。**规格称"现由L15承担"，本轮未逐一列出全部15层的饱和情况来确认是否唯一是L15**，但机制上完全正确：非对称后 Melee 不再是"cap-capped"类（`BehaviorClassCapForLevel`对Melee返回0），`AvailableCoreClassCount==cap`这个判定条件对Melee永远不可能成立（因为cap=0会被`if(cap==0) continue;`跳过），若L15的饱和恰好是靠Melee类触发的，这条测试的`saturated`列表会缩小甚至可能变空，`ASSERT_FALSE(saturated.empty())`可能变红——**这是一个规格附录A没有具体说明的风险点：需要确认L15的饱和是否真的靠Melee（会被非对称改写消除），还是靠Turret/Kite（不受影响，仍可行）。若靠Melee，规格"重选承载层或改为'存在远程类贴限层'"的建议是对的且必要；若靠Turret/Kite，这个测试可能不受影响，规格的担忧是多余的。**本轮补充复算确认：L15 core = Melee×2(BALROG+GSNAKE) + Turret×1(SNOWWICH)。`cap(15,Melee)=2`，core Melee计数恰好=2=cap → 饱和判定命中的是 MELEE 类，不是Turret/Kite。非对称cap把Melee的cap从2改为0（或"上限提高/免cap"）后，`BehaviorClassCapForLevel(15,Melee)`若变为0，代码里`if(cap==0) continue;`会直接跳过Melee，L15不再进入`saturated`列表——除非其他层（如L13的Melee×2核心）能顶上，否则`ASSERT_FALSE(saturated.empty())`存在变红风险。规格附录A"重选承载层或改为存在远程类贴限层"的应对是必要且方向正确的，但规格没有给出具体重选到哪个层/哪个类，需要在实施前明确（例如改判定为"存在RangedTurret/RangedKite贴限层"，此时L13/14/15的Turret或Kite核心是否会自然贴上cap=2需要重新验证：L13 core无Turret/Kite核心，L14 core有Turret×1不贴限，L15 core有Turret×1不贴限——初步看没有任何层的远程类核心自然贴到cap=2，这个测试的"binding"性质可能需要额外的tail抽取才能验证，而不能仅靠core饱和，这是规格附录A遗漏的又一层复杂度）。** |
| 单一真相源已就位 | `level_roster.cpp:292/335` | ✅ 部分核实 — 292 行确实是"class floor satisfiability"校验调用 `BehaviorClassCapForLevel`（在 `ValidateLevelRoster` 内）；335 行超出文件末尾附近范围，本文件共约 336 行，需要重新核对行号（**行号可能因版本漂移而不完全精确，但语义上"validator 和 sampling loop 共用同一函数"是真的**——`level_roster.cpp:152` 定义 `BehaviorClassCapForLevel`，`monster.cpp:3660` 的采样循环调用它，同一函数） |
| R4 原始措辞 | "B1既有期望失配→调名册或class_floors，不得放宽阈值" | ⚠️ 未在本仓库找到"台账"文件出处，无法核实原文措辞是否逐字准确（见"无法验证"） |

### 复算脚本（L13/14/15/16 池子，供复现）：
见上方任务1的复算脚本前半部分（`monstdat.tsv` 读取逻辑），实际运行输出：
```
L13 candidates=15 classes={'RangedKite': 3, 'Melee': 9, 'RangedTurret': 3}
L14 candidates=15 classes={'RangedKite': 2, 'Melee': 8, 'RangedTurret': 5}
L15 candidates=9  classes={'Melee': 4, 'RangedTurret': 5}
L16 candidates=6  classes={'Melee': 2, 'RangedTurret': 4}
```
与规格 §3 表格逐字一致。

---

## 3. 附录A 改写清单完整性 — 【重要，规格遗漏至少1项确认缺口】

规格附录A列出4项待改写契约（A1-A4）。本轮搜索范围内的核实情况：

- **A4 `PlacedClassMixWithinBaseline`**：✅ 确认存在，位于 `test/level_roster_baseline_test.cpp`（约420行起），当前逐层用 `kRangedShareCeiling[level] = kRangedShareBaseline[level] + kRangedShareTolerance(0.05)` 作硬 ceiling（`EXPECT_LE(share, kRangedShareCeiling[level])`）。规格称"降为报警线"——**这需要把 `EXPECT_LE` 改成非致命的报警（如 `std::cout` 警告或单独的"soft"检查），当前代码结构里没有区分"硬失败"和"报警"的机制，规格没有说明具体怎么改这个测试的判定逻辑（改成 WARN 宏？改成只打日志不assert？）**——**这是一个技术实现缺口，附录A对A4的"改写成"一栏只说"报警线保留"，没给出具体测试代码层面的改法**。
- **A1/A2**：在本轮 grep（`HellL13/14/15SameClassTailBaseline`、`RosterQuotaAllowanceIsBinding`）范围内**未能在 `test/sampling_behavior_test.cpp` 或 `test/level_roster_baseline_test.cpp` 中找到这两个确切的测试名**（grep 命中的都是别的符号，如 `kRangedShareCeiling`、`class_floors` 字符串等），需要作者核实这两个测试当前是否确实存在、存在于哪个文件——**若这两个测试名本身是笔误或已被重命名/移除，规格的改写清单就是对着不存在的目标写的，必须重新核实**。
- **A3（文档标注）**：未去核实"阶段A/B验收表"的具体文档路径，规格没给出该文档的路径引用，无法验证其现有措辞。

**遗漏检查（规格要求"自行搜索所有会因非对称cap而语义变化的既有守卫"）**：

- `IdentityGuard`、`A1A3VariantsAreCore`：本轮 grep 未搜到这两个符号名在代码库中出现（可能是规格草稿中虚构的占位测试名，或者属于另一个尚未合入的分支/文件）。**若这些测试确实不存在，规格附录A"漏一条即为重要问题"的自我要求就自相矛盾——它列出的检查目标本身可能是不准确的**。
- `docs/knowledge/*` 中确认搜到 1 处引用（`docs/knowledge/decision_save_format_policy.md` 命中 `class_floors` 关键字）——**规格附录A完全没提及这个文档，是一个真实的遗漏项**，需要复核该文档内容是否描述了 cap 语义（本轮未深入读该文档内容，仅确认命中，见"无法验证"）。
- eval case：`eval/cases/rng/level-rosters.yaml` 命中 `class_floors`/cap 相关关键字 5 次——**规格附录A也未提及这个 eval case 文件**，若其中硬编码了"任意类≤2"的预期计数，非对称 cap 落地后会静默失真或者需要一起改，这是规格未列出的外溢点。

**结论：附录A的"既有契约改写清单"不完整，至少遗漏 `docs/knowledge/decision_save_format_policy.md` 和 `eval/cases/rng/level-rosters.yaml` 两处；同时其列出的 A1/A2 两个测试名在本轮搜索范围内找不到确切出处，需要作者重新核实这些测试当前是否存在、存在于何处，否则"改写清单"本身建立在未经核实的目标上。**

---

## 4. vanilla 地板可精确导出性

规格称"空名册夹具"（R28 legacy 路径：无 params 行→预算4000、尾池不设上限）等价于改动前行为。

**核实结果（基于代码读取，非运行验证——见"无法验证"部分说明为何不能跑测试）：**

- ①**预算4000**：✅ 代码确认 — `Source/monster.cpp:3640`: `const int maxImage = rosterParams != nullptr ? rosterParams->maxImage : 4000;`，无 params 行时确实退回 4000。
- ②**core预加**：✅ 代码确认应为无 — core 预加来自 `GetLevelRoster(currlevel)` 返回的 entries（`monster.cpp` 核心预加循环），若对应层没有 `level_rosters.tsv` 的 entry 行（不仅是 params 行），则 `GetLevelRoster` 返回空 span，core 预加循环不执行任何东西。**但规格只提到"无 params 行"，没有明确"无 entries 行"是否也是同一个空名册夹具的必要条件**——如果测试夹具只删掉了 params 行、却保留了 entries 行（core 名单），那么 core 仍会被预加，这就不是"改动前行为"了。**这是规格表述的一个精度缺口**："空名册夹具"这个词没有清楚说明是"两个文件都清空"还是"只清空一个"，需要在规格里明确写出该夹具的构造方式（对照 `test/level_roster_baseline_test.cpp` 里 `LoadPreA2Tables()` 的做法：它用的是"L1-16-only 副本 tsv 文件，去掉 L17-24 的行"，而不是完全清空——这暗示"清空"的正确实现应是"物理删除该层所有 entries+params 行"，而不是仅仅不提供 params）。
- ③**cap是否与改动前一致**：✅ 代码确认 `BehaviorClassCapForLevel` 与 `rosterParams` 是否存在无关，是层号的纯函数（L13-16 恒返回2），**不受"空名册"影响，因此这一条对vanilla而言是自动满足的，规格此处的顾虑不成立**（cap 不依赖 params 是否存在）。
- ④**Golem/任务预加**：未在本轮验证范围内确认（未读 `InitLevelMonsters`/quest 预加的完整逻辑），标记为"无法验证"。
- ⑤**HF层同法**：未验证，同上。

**结论：①③ 核实一致；②有精度缺口（需要规格明确"空"是指哪些文件/行都要删除）；④⑤ 未验证。整体上"空名册夹具=vanilla"这个等价性目前只是部分证实，规格应该把"空名册"的具体构造方法写清楚（复用 `level_roster_baseline_test.cpp` 里已经验证过的 `LoadPreA2Tables` 模式：物理删除该层所有 roster/params 行的 fixture 文件，而不是仅仅"不提供 params 行"）。**

---

## 5. D4=75% 取舍自洽性

结合任务1的复算结果：**在 L13 上，D4=100%（union=15/15）在几乎所有 tail_draw≥1 的取值下都自然达成（见任务1扫描表），从未成为瓶颈**。这说明规格担心的"D4 若取 vanilla(≈100%) 会强迫抽遍候选池从而撞远程守卫"这个论证链条，**在 L13 这个具体案例上是不成立的**——真正撞车的是 D1（单局种类）与远程占比，不是 D4 与远程占比。D4=75% 这个绝对下限在 L13 反而是一个"松到不会失败"的门槛（因为 D4 实测恒为 100%），**这与"75%是显式取舍、可能不可达"的规格叙述矛盾**：规格假设 D4 是紧约束需要放宽，实测显示它根本不紧。

规格里"若某层实测不可达，必须在规格里写明该层实测值"的兜底条款，在 D4 轴上很可能永远不会被触发（因为 D4 廉价满足），**这不是"静默放行"的后门风险，而是规格选错了瓶颈轴——真正需要谈判折衷的是 D1 vs 远程占比，规格却把讨论精力全放在 D4 vs 远程占比上，这是一个分析方向上的错配**。（注：此结论基于 L13 单层复算，其他层未逐一验证是否也有同样错配，见"无法验证"部分。）

---

## 6. 红线10/11诚实性

规格称"本规格不新增压力（远程侧cap不变），故不需要新反制"。

**结合任务1发现：这个前提本身在 L13 是不成立的**——要让 D1≥6 达标，若坚持远程cap=2不变，远程占比会从vanilla的22.7%被动推高到31-34%（结构性推高，不是设计选择），**这正是"远程侧压力上升"，只是推高的机制不是"cap变宽"而是"melee池耗尽后被迫抽ranged"**。规格红线10的论证——"我们没有调远程cap，所以远程压力不变"——**混淆了"手段不变"和"结果不变"：手段（cap数值）确实没变，但结果（实际远程占比）在 D1 达标的场景下会变。红线10要求"指名反制"，如果规格坚持"不新增压力"的判断，红线10条款事实上被规避了，而不是被满足。**

至于"近战怪变多是否改变玩家的站位/消耗节奏"——这是规格自己提出的合理疑问但没有回答，本轮未做游戏机制层面的验证（不在只读代码复核范围内可判定，标记为"无法验证"）。但从数值角度看，**近战怪数量翻倍以上（对称cap下最多2个Melee，非对称下可到7+个）会显著改变小队/围攻动态，这是"新增压力"的另一种形式，规格红线11"不改玩家规则"的自辩没有覆盖这一点**。

---

## 7. 附录C 硬目标 vs 回退 矛盾分析

规格原文：
> 目标强度：地狱≥6为硬目标；达不到必须在收尾时回到作者改判（不允许在实现中静默降级）
> 回退规则：若A方案在某层实测仍达不到D1≥6→按D处理（把该层密度上限写成实测值并记录理由），不允许为达6而抬高远程压力（B）

**这两句在字面上不矛盾**（"硬目标"management的是"不能自己偷偷改判据"，"回退规则"讲的是"如果技术上做不到，走一个记录+人工审批的流程，而不是静默放宽阈值"）——但**表述确实容易误读**，因为"回退按D处理"读起来像是一个自动化的、代码里可以走的分支（"记录理由"暗示可以在代码/文档里直接写死一个较低的数字来通过），而"硬目标+回作者改判"意味着这必须是一次人工决策，不能是CI里自动发生的事。

**结合任务1的结论，这个矛盾会立刻被触发**：L13 在远程cap=2不变的约束下，D1=6 会导致远程占比超标（0.31 vs ceiling 0.277）。规格必须现在就说清楚，L13 这一具体情形走的是"硬目标不达标→回作者改判"，还是"记录D=实测值（比如3或4）"，还是"改远程占比红线本身"。**建议更清晰的表述**：把"回退规则"改写为——"若某层无法在远程占比≤基线+5pp约束下达到D1≥6，视为该层的密度目标与远程红线冲突，必须由作者在以下三个选项中二选一并记录决策：(a) 该层D1目标降级为实测最大可行值（需给出对应tail_draw/cap组合与其远程占比）；(b) 该层远程cap从2进一步收紧（说明理由和对红线14报警线的影响）；(c) 扩池（新增该层非远程候选monster）。禁止仅记录一个更低的D1数字而不说明是哪个选项、哪组参数产生的。"

---

## 8. 范围遗漏

**L1-16读数变化**：非对称cap规则明确限定"地狱 L13-16"，L1-12 的 `BehaviorClassCapForLevel` 逻辑完全不变（`level_roster.cpp:152-158`：L9-12只对Kite类设cap=2，其余class返回0=无cap；这段代码本规格不改）。因此**理论上L1-12的读数不应该因为本规格改变**——规格"不变量④"（除本规格明确改动的层外，L1-16既有读数逐位不变）在代码层面看是可满足的，因为改动只发生在 `BehaviorClassCapForLevel` 函数内 L13-16 分支的返回值，不触及 L1-12 分支。**本轮未运行测试验证这一点在实际生产链路上是否真的逐位不变（例如是否有其他隐藏的耦合，比如共享的 tail_draw 或全局预算计算逻辑），标记为"无法验证"**。

**玩家可见变化（固定问句"否"）**：固定问句只问了UI/提示/着色/音效语义，**没有问"游戏难度/游戏体验"这个更本质的问题**——非对称cap会让L13-15出现比现在多3-5倍的近战怪（从最多2个到最多7个候选，具体数量取决于tail_draw），这对实际游玩来说是显著的、玩家能感知到的变化（更多贴脸近战怪意味着更依赖走位、群体伤害技能吃紧），**只是不属于固定问句字面定义的"标识/提示/着色/音效"范畴，所以能在字面上答"否"而回避这个更大的问题**。这不是规格的错——固定问句本身就是这么定义的——但复核认为这里存在"字面合规、精神有疑"的落差，值得作者留意。

---

## 我核实为真的规格断言（本轮实际复算/读码验证）

1. `BehaviorClassCapForLevel` 对称cap规则（L13-16任意类≤2，L9-12仅kite≤2）—— 读 `level_roster.cpp:152-160`
2. `GetBehaviorClass` 行为类别映射表 —— 读 `monstdat.cpp:483-509`
3. L13/L14/L15/L16 候选池按类计数（Melee9/Turret3/Kite3 等）—— 现读 `monstdat.tsv` 现算
4. L13/L14/L15 core roster 构成（NBLACK+GUARD 等）—— 读 `level_rosters.tsv:58-64`
5. L13 实际生产参数（tail_draw=1, class_floors=Melee=2, max_image=18000）—— 读 `level_roster_params.tsv:14-16`
6. 采样循环真实语义（core绕过cap、tail按cap过滤、floor优先抽取、随机兜底）—— 读 `monster.cpp:3600-3720`
7. L13联合可满足性问题：cap_ranged=2时D1≥6与远程占比≤基线+5pp互斥；仅cap_ranged=1时存在可行区间 —— 本轮用真实池子/core/循环语义精确复刻模拟（200 seeds，脚本内联于任务1）
8. R28 legacy路径预算4000 —— 读 `monster.cpp:3640`
9. `BehaviorClassCapForLevel` 与 params 是否存在无关（vanilla不受影响）—— 读代码逻辑推导
10. `PlacedClassMixWithinBaseline` 现状为硬 `EXPECT_LE` ceiling（非报警）—— 读 `level_roster_baseline_test.cpp`

## 我无法验证的部分（原因）

1. **`RosterQuotaAllowanceIsBinding` 在非对称cap落地后，改用"远程类贴限层"判定是否真的存在binding的层** —— 本轮确认L13/14/15的核心中远程类(Turret/Kite)计数均未达到cap=2（L13核心无远程类，L14/L15核心Turret各1），意味着"改为存在远程类贴限层"这个判定目前在core层面找不到自然饱和的层，可能需要连tail一起测量才能验证binding性，规格附录A未提及这层复杂度，本轮未做完整的200-seed tail模拟去确认。
2. **`IdentityGuard`、`A1A3VariantsAreCore` 是否存在** —— 全库 grep 未命中，可能是规格草稿虚构的占位名，也可能是本轮 grep 范式不匹配（如宏生成的测试名），未做更深入排查。
3. **附录A"台账R4原始措辞"的文档出处** —— 规格中提到"台账"但未给路径，本轮未能定位到该文档，无法逐字核对。
4. **`docs/knowledge/decision_save_format_policy.md` 中 cap 相关描述的具体内容** —— 仅确认 grep 命中，未读取该文档判断其与非对称cap的关系是否需要同步更新。
5. **Golem/任务预加、HF层vanilla等价性（任务4的④⑤）** —— 未读取 `InitLevelMonsters`/quest 预加/HF overlay 加载的完整代码链路。
6. **L1-12读数是否真的逐位不受影响（任务8）** —— 未运行任何测试（复核约束明确禁止 cmake/ninja），只做了静态代码读取推导，未能通过实际运行确认无隐藏耦合。
7. **红线10"近战怪变多是否改变玩家体感"的游戏机制层面判断** —— 超出只读代码复核可判定的范围，标记为规格自己该回答但目前没有回答的问题。
8. **只对 L13 做了完整的联合可满足性复算**，L14/L15（池子结构不同：L14 Melee8/Turret5/Kite2，L15 Melee4/Turret5/Kite0，其中 L15 无 Kite 候选）未做同等精度的参数扫描，不能断言它们是否有相同的死锁，只能合理推测（L15 Melee更少、Turret更多，处境可能更差；L14 Melee较多，处境可能稍好）——建议作者对 L14/L15 重复任务1的方法学。
9. **附录A A4（`PlacedClassMixWithinBaseline`降为报警线）的具体测试代码实现方式** —— 当前测试用 `EXPECT_LE`硬失败，规格未说明改成"报警"后测试框架层面如何区分硬失败/软警告，需要作者补充。

---

## 裁决

**规格状态：不可执行（存在结构性死锁，非表述问题）——需要重大修订后才能进入 writing-plans 阶段。**

理由：任务1发现的"L13 在远程cap=2不变前提下 D1≥6 与远程占比≤基线+5pp 不可兼得"不是规格文字表述的瑕疵，而是数学/机制层面的真实冲突，规格现有文本（§4.1/§4.3/附录C）都建立在"这个冲突不会发生，或发生了就走既有兜底条款"的假设上，从未验证过这个假设，也从未在最关键的 L13 案例上跑过这道数。若不先解决这个冲突（在 L14/L15 上重复验证后），后续的"断言落地"任务会在实现阶段撞墙，届时要么被迫违反 R4（悄悄放宽远程占比阈值），要么被迫违反密度目标（D1<6），两者都是规格明令禁止的结果。

