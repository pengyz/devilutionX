# 任务 D 分发稿：L14 core 名单重塑（在**不放宽任何守卫**前提下让 L14 用 squad_chance=30）

**背景**：L14 cores = 1 近战 + 2 远程 → 成队的 partner 有 2/3 概率是远程 → `squad_chance=30` 时 placed 远程占比 **0.615081 > ceiling 0.60876**；而"抬 Melee floor 到 2"虽过了验收 8 却把验收 9c 从 **6/6 打到 2/6**（Task 3 实测）→ 当时回退为 `squad_chance=10`（现 0.605120，余量仅 **0.36pp**，L14 只拿到 8.8% roll 率）。
**目标**：搜索 L14 的 **core 名单**（不是 floor、不是阈值），找到一个配置使 **`squad_chance=30`** 同时满足：
- 验收 8：placed 远程占比 ≤ **0.60876**（ceiling 不得修改，R4/RB17）；
- 验收 9c：L14 unique base 可达性 **6/6**（不得下降）；
- 加载期 core≤cap（L13-16 为 **2/class**）、unique base 白名单、`RosterPerSeedVariety`、`RosterQuotasSatisfied`、B1 契约（`HellCavesKiteTailBaseline` 等）全绿。
**建议方向**：使其成为 **2 近战 + 1 远程**（或其它 2/class 内组合），因为成队 partner 从**其他 core**抽取，构成变化会直接改变远程占比的期望。
**成功则采纳**（保留实测前后数据与 200-seed 数值、9c 的 `[ MEASURED ]` 证据）；**失败则维持 A（`squad_chance=10`）**并把**实测可行域**（试过哪些 core 组合、各自 8/9c 读数）写进报告与规格附录 E（控制者会落盘规格）。
**硬约束**：不改任何 ceiling/floor 阈值；不改其它层；不改 `squad_leashed`/`squad_size`；`assets/txtdata/**` 之外的出厂数据不动；CI 安全（新用例若依赖零售/HF 素材须贴真实依赖探测跳过）；每条结论附实测而非推断；推送后跟踪 CI 到终态。
