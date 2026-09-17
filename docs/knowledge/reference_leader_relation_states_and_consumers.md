---
name: leader 关系的三态与 8 个消费者（改放置/死亡/小队相关逻辑前的必查清单）
description: LeaderRelation 有 None/Leashed/Separated 三态，且 leader 索引在非 None 时可能被保留（着色/归队用）。以下 8 处消费者对三态的处理各不相同——本仓曾因只看 Leashed 而漏掉 Separated，留下"指向无关怪"的悬挂索引。
type: reference
created: 2026-09-16
sources:
  - Source/monster.h（LeaderRelation 定义、setLeader 注释说明保留索引是为了给 buffed minion 上色）
  - Source/monster.cpp（GroupUnity 在视线被挡时置 Separated 且保留索引；ReleaseMinions/ShrinkLeaderPacksize/DirOK/FollowTheLeader/ScavengerAi）
  - Source/loadsave.cpp（SyncPackSize）、Source/msg.cpp（delta 载入路径）、Source/monhealthbar.cpp（着色）
  - 阶段 B 最终评审（24ed88d8）S1/S2
---

**三态**（`LeaderRelation`）：
- `None`：无 leader 关系（`setLeader(nullptr)` 置此态，**但按设计保留 `leader` 索引**——见下）。
- `Leashed`：拴系随从（在 leader 附近、共享战斗）。
- `Separated`：**视线被挡时由 `GroupUnity` 置入**，脱队但仍**保留 `leader` 索引**，靠近后会回到 `Leashed`。

**为什么索引会被保留**：`setLeader(nullptr)` 的注释写明——为了在血量条上把被强化的随从画成蓝色，索引**故意不清**。`monhealthbar` 的着色判据历史上就是 `leader != Monster::NoLeader`。

**8 个消费者（改任何 leader 相关逻辑前，逐一确认三态处理）**：
1. `GroupUnity`（monster.cpp）：入口只挡 `None` → **`Separated` 会通过**，可能 `*getLeader()` 解引用并 `packSize++`、翻回 `Leashed`。
2. `FollowTheLeader`：同步 `position.last`/`activeForTicks`（取决于 leader 有效）。
3. `DirOK`：把随从移动锁在 leader 附近（依赖索引有效）。
4. `ShrinkLeaderPacksize`：**只在 `Leashed` 时**为 leader 减 `packSize`（`Separated` 时已在分离那一刻减过 → 不应再减）。
5. `ReleaseMinions`：释放随从（历史实现**只过滤 `Leashed`** → 漏 `Separated`，见下"已知坑"）。
6. `ScavengerAi`：清理路径之一（**不清索引**）。
7. `loadsave.cpp SyncPackSize`：存档载入时重建 pack（只处理 unique+Scavenger）。
8. `msg.cpp` 的 delta 载入路径 + `monhealthbar.cpp` 的着色：前者会对 `hitPoints==0` 的怪调 `M_UpdateRelations`；后者依赖索引做**玩家可见**的着色。

**已知坑（2026-09 阶段 B 修正）**：普通怪在阶段 B 之前**不可能当 leader**；引入普通怪小队后，`ReleaseMinions` 只清 `Leashed` 索引就成了可达缺陷——**普通 leader 死亡 + 随从当时是 `Separated`** → 索引残留 → 槽位复用后指向无关怪。修法是过滤放宽到 `!= None`，但**只在普通怪路径清索引**（unique 路径保持逐字节不变）。

**另一处连带**：小队随从（`tough=false`，**未强化**）也会被染成蓝名，因为着色判据是"有无 leader"而非"是否真的被强化"。修法：着色判据改用**真实的强化判据**（当前＝leader 为 unique），并抽成可测试谓词。

**何时使用**：改动放置/死亡/小队/AI/着色/存档载入中任何与 `leader` 相关的逻辑前；排查"随从跟着不该跟的怪""随从卡在某处""莫名蓝名"时。
