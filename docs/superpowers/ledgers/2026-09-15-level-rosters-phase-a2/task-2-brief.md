## Task 2: L17-24 的 core/tail 数据与逐层参数

- [ ] **步骤 1：候选池盘点**（HF 载入下的 L17-24 可用怪、`ai`→行为类别、unique base、Σimage）
- [ ] **步骤 2：写数据**（`level_rosters.tsv` 加 L17-24 core 行；`level_roster_params.tsv` 加 L17-24 行含 `max_image`/`tail_draw`/`class_floors`/`squad_chance`/`squad_size`/`squad_leashed`）
  约束：core ≤ cap（L17-24 的 caps 需按 `BehaviorClassCapForLevel` 的实际分段确认并记录）、floors 在 caps 下可满足、unique base 白名单、`Σimage` 低于 `max_image`（**口径更正 RB41**：这**不是**加载期硬校验——`ValidateLevelRoster` 只查 `max_image > 0`，预算在**采样期**生效且 **core 预加不受其约束**；越界由占比/构成守卫兜住）。
- [ ] **步骤 3：校验回归**：`level_roster_test` 全绿（含新层范围用例）；加载期不再对 L17-24 fatal（HF 载入时）。
- [ ] **步骤 4：提交**

---

