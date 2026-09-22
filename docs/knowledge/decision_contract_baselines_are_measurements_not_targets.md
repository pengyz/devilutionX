---
title: 契约判据的基线是"实测"而不是"目标"——不可为了过关抬高它
type: decision
tags: [testing, contracts, baselines, roster, tuning]
date: 2026-09-18
---

# 情形
做机制调参（例如把某层段的成队概率/规模提高）后，某个**契约级**守卫变红：

```
test/level_roster_baseline_test.cpp:733: Failure
Expected: (share) <= (kVanillaShareBaseline[level]), actual: 0.188055 vs 0.186861
level 4 ranged share is above vanilla
```

# 正确判断
该断言的口径是"**名册不得把某一层的远程占比推到 vanilla 之上**"（契约 §4.4，R4 反演）✓。
其中 `kVanillaShareBaseline` 是**同种子 vanilla 的实测值** ✓ —— 它是**基准**，**不是可调目标** ✗。

⇒ **不许**通过抬高 `kVanillaShareBaseline` 让测试变绿 ✗✗（那等于悄悄放宽契约 ✓）。
⇒ 正确做法：**回调调参** ✓，让机制重新落在契约之内 ✓。

# 本案的解法（可复用）
1. 用日志里的逐层实测行定位**具体哪一层**越顶（本案 **L4**，仅 +0.12pp ✓）；
2. 回查规格对**该层段**的原文要求 —— 本案规格写的是"**L1-8 维持现状 30/2**" ✓，而我当时改成了 **25** ✗
   ⇒ **修法就是回归规格值** ✓（一步到位、且顺带修正了我对规格的偏离 ✓）；
3. 恢复后**重跑契约用例** ⇒ 绿 ✓；再**重钉**受影响的实测黄金值（成队率表 ✓）。

# 判别口诀
- **"这条基线是别人的实测，还是我们的目标？"** —— 实测 ⇒ 只能改机制，不能改基线 ✗；
- 目标/预算类（例如"最多 +5pp"）⇒ 可以重新论证调整 ✓，但必须在注释/规格里写明理由与实测值 ✓。

# 关联
- 行尾/构建/数据加载的踩坑见 `gotcha_data_changes_need_asset_rebuild.md` ✓；
- 本条的教训已同样适用于成队率黄金表 `test/fixtures/pressure/squad_rates.txt` ✓（它也是实测钉值 ✓）。
