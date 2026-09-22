---
title: CI 是终态权威——每次 push 后都要看，而不是收尾时看一次
type: gotcha
tags: [ci, gates, tests, assets]
date: 2026-09-18
---

# 症状
本地全量门禁绿 ✓、eval smoke 绿 ✓，于是以为"每项都绿" ✓；但 `gh -R <fork> run list` 显示**连续多次 push 全部 failure** ✗✗（本案 5 次 ✗）。

# 两个各自独立的坑
1. **`gh` 默认不指向你的 fork** ✗：`gh run list` 解析到上游（本案 `diasurgical/DevilutionX` ✓），显示的是**别人的**运行（状态 `action_required` ✓）⇒ 必须显式 `gh -R <owner>/<repo> run list` ✓；
   `gh workflow run` 同理：不带 `-R` 会在**默认分支**上找该工作流 ⇒ `HTTP 404` ✓。
2. **本地绿 ≠ CI 绿** ✗：CI 只有 **spawn.mpq**（shareware ✓），本地有 retail/HF 素材 ✓ ⇒ 依赖素材的用例在 CI 走**不同分支**：
   有 `missingRetailTrn_` 守卫的用例在 CI **Skipped** ✓；**没有**该守卫的用例在 CI **照常运行**并用真实钉值断言 ⇒ **红** ✗✓（本案 2/788 ✗：`SquadRateMatchesMeasuredBaseline` 与 `EncounterHasTwoDemandTypes`）。

# 正确做法
- **每次 push 后**：`gh -R <fork> run list --limit 5` ✓；需要时 `gh -R <fork> run watch <id> --exit-status` ✓ 到终态 ✓；
- **新增依赖素材的用例**：必须与**同套件兄弟用例**采用**同一组** skip 守卫（`missingMpqAssets_` **且** `missingRetailTrn_` ✓）；
- 自检问句：`--gtest_list_tests` 之后问"**这条用例在只有 spawn.mpq 的机器上会走哪条分支？**" ✓。

# 关联
- 素材/数据加载的坑见 `gotcha_data_changes_need_asset_rebuild.md` ✓；
- 并发踩坑：后台门禁独占 `build/` 时不要另起构建（本案因此出现一次**假红** ✗ ⇒ 复跑即绿 ✓）。
