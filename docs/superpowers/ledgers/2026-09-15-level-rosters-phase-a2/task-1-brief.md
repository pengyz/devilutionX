## Task 1: 校验层范围化 + L17-24 前置基线实测

**接口：**
```cpp
// level_roster.h（保持既有两参调用可用：默认 maxLevel = 最高地下层）
std::optional<std::string> ValidateLevelRoster(
    std::span<const LevelRosterEntry> entries,
    std::span<const LevelRosterParams> params,
    uint8_t maxLevel);
```

- [ ] **步骤 1：写失败的合成用例**（`test/level_roster_test.cpp`）
  构造一张含 **L17-24** 行、但其怪在基础表里 `availability=Never` 的表，`maxLevel = 16` → 断言**校验通过**（这些层在非 HF 下不存在，不应 fatal）；再以 `maxLevel = 24` → 断言**校验失败**（HF 下必须真实可用）。
- [ ] **步骤 2：跑测试确认失败**（当前实现会因 L17-24 怪不可用而 fatal）
- [ ] **步骤 3：实现层范围化**
  - 逐层检查（存在性/可用性/floors 可满足性/`core` 非空）**只对 `level <= maxLevel`** 生效；
  - 全局检查（重复行、`max_image>0`、`tail_draw>=0`、`squad_*` 范围、哨兵拒绝、unique base 白名单）**不受层范围影响**（它们与层是否可达无关）；
  - 保留既有两参重载（默认 `maxLevel`＝当前游戏最高地下层）以免破坏既有调用与测试——**若因此产生新的导出符号，须确认不触发漂移检查 E**（必要时把默认值写成调用方显式传参）。
- [ ] **步骤 4：`LoadLevelRoster()` 传入真实层上限**（自行定位 `giNumberOfLevels`/等价常量，并在报告记录来源）
- [ ] **步骤 5：L17-24 的前置基线实测**（资产门控；无 HF 素材则 `GTEST_SKIP`）
  在 `level_roster_baseline_test.cpp` 增加一个**测量**用例：HF 下 L17-24 的 placed class mix / 远程占比 / 类型数，**无条件打印**（`[ A2BASELINE ] …`）并写入报告。**这是后续验收 8（相对改动前基线 ≤ +5pp）的基线来源**——没有它就不许改数据。
- [ ] **步骤 6：提交**（`feat(roster): validate rosters only up to the active level range`）

---

