---
name: 无头测试 HF 的 L17-24 必须自己挂 hellfire.mpq（不能用 LoadHellfireArchives）并补齐 NEST/CRYPT 前置
description: 想无头跑 HF 层（L17-24）时的四个真实坑：LoadHellfireArchives 会因缺 hfmonk/hfmusic/hfvoice 直接 FatalExit 整个二进制；缺 NEST/CRYPT 的 .til 与 Hive/Crypt 触发器分派会在 DRLG_LPass3 SEGV；必须快照/还原进程级全局；整段必须以真实依赖探测门控以让 CI 跳过。
type: gotcha
created: 2026-09-16
sources:
  - docs/superpowers/ledgers/2026-09-15-level-rosters-phase-a2/task-1-report.md（实现者实测）
  - Source/engine/mpq_archive / Source/storm/...（MpqArchive::Open 的 priority 参数）
  - Source/levels/gendung.cpp:594（DRLG_LPass3 需要 .til 与触发器初始化）
---

**场景**：要在 GoogleTest 里无头驱动 HF 的 L17-24（Nest/Crypt）放置/采样。

**四个坑（都实测过）**：
1. **不要调 `LoadHellfireArchives()`** —— 它同时要求 `hfmonk.mpq`、`hfmusic.mpq`、`hfvoice.mpq`；本机/CI 通常缺这三个，它会 `DisplayFatalErrorAndExit()` **把整个测试二进制杀掉**（不是失败某个用例）。正确做法：自己 `MpqArchive::Open("hellfire.mpq", /*priority=*/8000)` 挂载。
2. **必须补齐关卡前置**：NEST/CRYPT 的 `.til` 路径（`nlevels\l5data\l5.til`、`l6data\l6.til` 之类）与 `InitHiveTriggers`/`InitCryptTriggers` 的分派——缺任一会在 `DRLG_LPass3`（`Source/levels/gendung.cpp:594`）**SEGV**。
3. **必须快照/还原进程级全局**（`Quests`/`Players`/`sgGameInitInfo`/`gbIsMultiplayer`/`gbIsHellfire`/`gbIsSpawn`/PrefPath + 重载 Diablo 数据），否则 HF 数据会泄漏到后续套件；**用 `--gtest_shuffle` 验证**。
4. **整段必须以真实依赖探测门控**（例如探测 `nlevels\l6data\l6.til` 是否可读），缺失即 `GTEST_SKIP` —— CI 只有 `spawn.mpq`，否则必然红。

**何时使用**：任何要覆盖 HF 内容（L17-24、HF 怪、HF 物品）的无头测试。
