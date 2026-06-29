# DevilutionX 引擎基础设施 — TDD 改造计划

## TL;DR

> **Quick Summary**: 将 DevilutionX 的 5 个硬编码系统改为运行时可扩展架构，使 Mod 作者可以通过 Lua 脚本和数据文件注册新 AI、新导弹行为、新任务、创建怪物和施放法术——零 C++ 修改。全程 TDD，GoogleTest 先行。
>
> **Deliverables**:
> - AiProc 运行时注册接口 + MonsterAIID::Custom
> - ParseMissileAddFn/ProcessFn 查表替代 if-else
> - Lua World API 模块（SpawnMissile, DamageTarget, GetMonstersInRange）
> - Lua Monster API 扩展（CreateMonster, SetMonsterAI）
> - Quest 脚本化支持（script_name 字段 + Lua 回调）
>
> **Estimated Effort**: Medium (4 天)
> **Parallel Execution**: YES — 5 waves
> **Critical Path**: Task 1 → Task 2 → Task 3 → Task 5 → F1-F4

---

## Context

### Original Request
扩展 DevilutionX 为 D2-Like 的 Mod 引擎平台，用 DDD 思路重构：把硬编码逻辑解耦到数据层和脚本层。前期先打地基——5 个基础设施改造项，全部 TDD。

### Interview Summary
**Key Discussions**:
- **保留 D1 核心体验**: 不改魔法书系统、不模仿 D2 技能树；纵向加深装备 BD、精英怪、新地下城
- **三层架构**: Engine (C++ 渲染/碰撞/网络) / Data (TSV) / Script (Lua) — 脚本层不碰渲染和热路径
- **已有基础优于预期**: MissileData 已用函数指针、AiProc 是数组、DataFile 已成熟；不应推翻重来
- **TDD 强制**: 每项改动必须先写 RED 测试 → GREEN 实现 → REFACTOR

**Research Findings**:
- AiProc: `void (*AiProc[])(Monster&)` 在 `monster.cpp:3091`，41 条目，dispatch 在 `monster.cpp:4323`
- MissileData: `addFn`/`processFn` 函数指针在 `misdat.h:168-172`，Parse 在 `misdat.cpp:182-310`，TSV 加载在 `misdat.cpp:372-400`
- Lua events: 仅 10 个 hooks (`lua_event.hpp`)，全部 observer 模式 void 返回
- Lua modules: Player（read+少量 write）, Items（full）, Monsters（只读+数据注入）
- Tests: `test/missiles_test.cpp`, `test/quests_test.cpp` 等已有；GoogleTest + GMock

### Metis Review
> Subagent 不可用 — 已通过深度代码审读完成自检。覆盖了边界条件、回退兼容、性能影响。

---

## Work Objectives

### Core Objective
建立 DevilutionX 的运行时可扩展基础设施，使怪物 AI、导弹行为、任务逻辑支持外部注册和 Lua 脚本控制——全程不打破现有游戏行为。

### Concrete Deliverables
- `Source/monster.cpp`: AiProc 从固定数组改为 `std::array<std::function<>>` + `RegisterAiFunction()`
- `Source/tables/misdat.cpp`: ParseMissile*Fn 从 if-else 链改为 `std::unordered_map<string, fn>`
- `Source/lua/modules/world.cpp` (新): SpawnMissile, DamageTarget, GetMonstersInRange API
- `Source/lua/modules/monsters.cpp` (改): CreateMonster, SetMonsterAI API
- `Source/quests.h/cpp`: QuestData 加 script_name; CheckQuests 支持 Lua 回调+返回值
- `test/ai_registry_test.cpp`, `test/missile_registry_test.cpp`, `test/lua_world_api_test.cpp` 等新测试

### Definition of Done
- [ ] `bun test` (或 `ctest --test-dir build`) 全部新增/修改测试 PASS
- [ ] 现有测试无回归（`ctest` 全绿）
- [ ] Hello-world Lua 脚本能注册 Custom AI、创建怪物、造成伤害、触发 quest 回调
- [ ] 不支持 subagent 的环境下手动验证 Lua 集成

### Must Have
- TDD: 每个实现先写测试
- 向后兼容: 已有 AI/quest 行为完全不变
- 运行时安全: Lua 异常不崩溃 C++ 端
- 可读性: `RegisterAiFunction("zombie_custom", MyCustomAi)` 形式的清晰 API

### Must NOT Have (Guardrails)
- 不修改渲染管线、网络同步、碰撞检测
- 不删除现有 AI 函数或枚举值
- 不在热路径中引入 Lua 调用（AI tick 每帧数百次 → 保持 C++）
- 不添加第三方 Lua 依赖（Sol2 已够）
- 不产生无法通过自动化测试验证的功能

---

## Verification Strategy (MANDATORY)

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed.

### Test Decision
- **Infrastructure exists**: YES (GoogleTest, 30+ targets)
- **Automated tests**: TDD
- **Framework**: GoogleTest + GMock (already in build/)
- **Test pattern**: RED (写测试 → 失败) → GREEN (最小实现) → REFACTOR

### QA Policy
Every task MUST include agent-executed QA scenarios.
Evidence saved to `.omo/evidence/task-{N}-{scenario-slug}.{ext}`.

- **CLI/build**: Use Bash — `ctest --test-dir build -R <test_name>` to run specific tests
- **Lua verification**: Use Bash — `build/devilutionx --lua-eval "..."` to test Lua API in isolation

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately — independent foundations):
├── Task 1: AiProc runtime registration (monster.cpp/h, monstdat.h) [deep]
└── Task 2: MissileFn map registry (misdat.cpp) [quick]

Wave 2 (After Wave 1 — Lua API, depends on task 1+2):
├── Task 3: Lua World API — spawn/damage/query (new modules/world.cpp) [deep]
└── Task 4: Lua Monster API — create/set AI (extend modules/monsters.cpp) [deep]

Wave 3 (After Wave 2 — Quest script, depends on task 3+4):
└── Task 5: Quest script support (quests.h/cpp, lua_event.cpp) [deep]

Wave 4 (After Wave 3 — Integration verification):
└── Task 6: End-to-end Lua integration test (new test, verifies all 5 APIs together) [deep]

Wave FINAL:
├── Task F1: Plan compliance audit (oracle)
├── Task F2: Code quality review (unspecified-high)
├── Task F3: Real QA execution (unspecified-high)
└── Task F4: Scope fidelity check (deep)
→ Present results → Get explicit user okay
```

**Critical Path**: Task 1 → Task 3 → Task 5 → Task 6 → F1-F4
**Parallel Speedup**: Wave 1 可并行 (Task 1 ∥ Task 2), Wave 2 可并行 (Task 3 ∥ Task 4)

---

## TODOs

- [ ] 1. AiProc 运行时注册 + MonsterAIID::Custom

  **What to do**:
  - [ ] **RED**: 写 `test/ai_registry_test.cpp`，验证：
    1. 默认所有现有 AI 类型可正常 dispatch（Smoke test: 取 Zombie AI，调 `AiProc[id](monster)` 不崩溃）
    2. `RegisterAiFunction` 注册新自定义函数后，`AiProc[customId]` 调用的确是注册的函数
    3. 重复注册同一 ID 应覆盖旧函数
    4. 注册 `nullptr` 后调用不崩溃（monster 走 FallbackAi）
  - [ ] **GREEN**: 实现 `RegisterAiFunction(MonsterAIID, std::function<void(Monster&)>)`：
    - `AiProc` 从 `void (*[])(Monster&)` 改为 `std::array<std::function<void(Monster&)>, 128>`
    - 保留 0-54 的现有枚举值初始化（保持 `AiProc[] = { &ZombieAi, ... }`）
    - `MonsterAIID` 枚举最后加 `Custom = 55`，`AiProc` 尺寸扩到 128
    - `AiProc[customId]` 默认为 `FallbackAi`（一个简单的 wander+attack 默认 AI）
  - [ ] **REFACTOR**: 将 `monster.cpp:4323` 的 `AiProc[static_cast<int8_t>(monster.ai)](monster)` 改为 `AiProc.at(static_cast<size_t>(monster.ai))(monster)` 做越界保护
  - [ ] 在 `monstdat.h` 中暴露 `RegisterAiFunction` 声明

  **Must NOT do**:
  - 不修改现有 41 个 AI 函数的内部逻辑
  - 不改变 `MonsterAIID` 枚举现有值 0-54 的顺序
  - 不在每帧 AI tick 中加锁或内存分配

  **Recommended Agent Profile**:
  - **Category**: `deep` — logic-heavy refactoring with backward compat and crash safety
  - **Skills**: None specific

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Task 2)
  - **Blocks**: Task 3, Task 4, Task 5, Task 6
  - **Blocked By**: None (can start immediately)

  **References**:
  - `Source/monster.cpp:3091-3132` — AiProc 数组定义（当前所有条目）
  - `Source/monster.cpp:4323` — AiProc 调度行（改这里做 at() 边界检查）
  - `Source/monster.cpp:4257-4337` — ProcessMonsters() 主循环（理解调用上下文）
  - `Source/tables/monstdat.h:22-64` — MonsterAIID 枚举定义（加 Custom = 55）
  - `Source/tables/monstdat.h:98-158` — MonsterData 结构体（ai 字段类型可能需要保持 int8_t）
  - `test/missiles_test.cpp:1-60` — 现有测试风格参考

  **Acceptance Criteria**:
  - [ ] `ctest --test-dir build -R AiRegistry` → 4 tests PASS
  - [ ] 现有测试无回归：`ctest --test-dir build` → 所有通过
  - [ ] `RegisterAiFunction(MonsterAIID::Custom, myFn)` 编译并调用成功

  **QA Scenarios (MANDATORY)**:

  ```
  Scenario: Default AI dispatch works for all existing types
    Tool: Bash (ctest)
    Preconditions: Build complete, no register calls yet
    Steps:
      1. ctest --test-dir build -R "AiRegistry.ExistingTypesDispatch"
      2. Verify test instantiates a Monster with each original AI type and calls AiProc[id]
      3. Assert each call does NOT crash (AI functions assume dungeon exists, provide test fixture)
    Expected Result: 4/4 tests pass, no segfault
    Failure Indicators: Any test failure, especially ZombieAi/SkeletonAi nullptr deref
    Evidence: .omo/evidence/task-1-existing-dispatch.txt

  Scenario: Register and call custom AI function
    Tool: Bash (ctest)
    Preconditions: Build complete
    Steps:
      1. In test: define a custom function that sets monster._mgoal = 99
      2. RegisterAiFunction(MonsterAIID(55), customFn)
      3. Create monster with ai = MonsterAIID(55)
      4. Call AiProc[55](monster)
      5. Assert monster._mgoal == 99
    Expected Result: Custom function invoked, goal set to 99
    Evidence: .omo/evidence/task-1-custom-ai.txt

  Scenario: Overwrite registered function
    Tool: Bash (ctest)
    Preconditions: First registration done
    Steps:
      1. RegisterAiFunction(MonsterAIID(55), fn1) sets goal=99
      2. RegisterAiFunction(MonsterAIID(55), fn2) sets goal=88
      3. Call AiProc[55](monster)
      4. Assert mgoal == 88
    Expected Result: Second registration overwrites first
    Evidence: .omo/evidence/task-1-overwrite.txt

  Scenario: Null/fallback AI doesn't crash
    Tool: Bash (ctest)
    Preconditions: Build complete
    Steps:
      1. RegisterAiFunction(MonsterAIID(56), nullptr)
      2. Create monster with ai = MonsterAIID(56)
      3. Call AiProc[56](monster)
      4. Assert no crash, monster enters some idle state
    Expected Result: No segfault, monster.idle is true or similar
    Evidence: .omo/evidence/task-1-null-ai.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named per scenario above
  - [ ] Terminal ctest output for each test run

  **Commit**: YES (groups with Wave 1)
  - Message: `refactor(monster): make AiProc runtime-registrable via RegisterAiFunction`
  - Files: `Source/monster.cpp`, `Source/monster.h`, `Source/tables/monstdat.h`, `test/ai_registry_test.cpp`
  - Pre-commit: `ctest --test-dir build -R AiRegistry`

- [ ] 2. ParseMissileAddFn/ProcessFn if-else → map 查表

  **What to do**:
  - [ ] **RED**: 写 `test/missile_registry_test.cpp`，验证：
    1. 现有的全部 AddFn/ProcessFn 名字仍然被正确解析（`ParseMissileAddFn("AddFirebolt")` → 非 null）
    2. `RegisterMissileAddFn("MyCustomAdd", myFn)` 注册后，`ParseMissileAddFn("MyCustomAdd")` 返回 myFn
    3. 未注册的名字返回 `tl::make_unexpected(...)`（向后兼容：空名字仍返回 nullptr）
  - [ ] **GREEN**: 实现 `RegisterMissileAddFn` / `RegisterMissileProcessFn`：
    - 在 `misdat.cpp` 顶部加 `static std::unordered_map<std::string, MissileData::AddFn> g_addFnRegistry`
    - `ParseMissileAddFn` 改为先查表，命中则返回；否则遍历现有 if-else（保留对内置函数的兼容）
    - 同方式处理 `ParseMissileProcessFn`
    - 或者激进做法：初始化时把内置函数全部注册到 map，`Parse*Fn` 变成纯 map lookup
  - [ ] **REFACTOR**: 考虑在 `LoadMisdat()` 开头调 `InitDefaultMissileRegistries()` 把内置函数全部 pre-populate

  **Must NOT do**:
  - 不删除现有 if-else 链中的任何条目（可以保留为 fallback）
  - 不改变 `MissileData` 结构体或 `addFn`/`processFn` 的类型
  - 不修改 TSV 数据文件格式

  **Recommended Agent Profile**:
  - **Category**: `quick` — isolated function refactoring, small scope
  - **Skills**: None specific

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Task 1)
  - **Blocks**: Task 3, Task 6
  - **Blocked By**: None (can start immediately)

  **References**:
  - `Source/tables/misdat.cpp:182-258` — ParseMissileAddFn（~80 行 if-else）
  - `Source/tables/misdat.cpp:262-340` — ParseMissileProcessFn（~80 行 if-else）
  - `Source/tables/misdat.cpp:372-400` — LoadMisdat（看 reader.read 的调用方式）
  - `Source/tables/misdat.h:167-200` — MissileData 结构体，AddFn/ProcessFn typedef
  - `test/missiles_test.cpp` — 现有测试风格参考

  **Acceptance Criteria**:
  - [ ] `ctest --test-dir build -R MissileRegistry` → 3 tests PASS
  - [ ] 所有现有 missile 名称仍可解析（`ParseMissileAddFn("AddFirebolt")` 非 null）
  - [ ] 自定义函数注册后可被 Parse 解析
  - [ ] 现有 missile 测试无回归

  **QA Scenarios (MANDATORY)**:

  ```
  Scenario: All built-in AddFn names still resolve
    Tool: Bash (ctest)
    Preconditions: Build complete
    Steps:
      1. Create a parameterized test iterating over {"AddFirebolt", "AddFireball", "AddArrow", ... all ~50 names from misdat.tsv}
      2. For each: auto result = ParseMissileAddFn(name)
      3. ASSERT_TRUE(result.has_value()) << name
      4. ASSERT_NE(*result, nullptr)
    Expected Result: All ~50 names return non-null function pointers
    Failure Indicators: Any name fails to resolve (regression)
    Evidence: .omo/evidence/task-2-builtin-names.txt

  Scenario: Register and resolve custom AddFn
    Tool: Bash (ctest)
    Preconditions: Built-in names pass
    Steps:
      1. Define: void MyCustomAdd(Missile &m, AddMissileParameter &p) { m._midam = 999; }
      2. RegisterMissileAddFn("MyCustomAdd", MyCustomAdd)
      3. auto result = ParseMissileAddFn("MyCustomAdd")
      4. ASSERT_TRUE(result.has_value())
      5. Create missile, call (*result)(missile, param), assert missile._midam == 999
    Expected Result: Custom function registered and callable via Parse
    Evidence: .omo/evidence/task-2-custom-addfn.txt

  Scenario: Unknown name returns error
    Tool: Bash (ctest)
    Preconditions: Built-in names pass
    Steps:
      1. auto result = ParseMissileAddFn("NonExistentFnName")
      2. ASSERT_FALSE(result.has_value())
      3. Empty string: auto result2 = ParseMissileAddFn("")
      4. ASSERT_TRUE(result2.has_value()) — empty should be nullptr (backward compat)
      5. ASSERT_EQ(*result2, nullptr)
    Expected Result: Unknown returns error, empty returns nullptr
    Evidence: .omo/evidence/task-2-unknown-name.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named per scenario above

  **Commit**: YES
  - Message: `refactor(missile): replace ParseMissileAddFn/ProcessFn if-else with map registry`
  - Files: `Source/tables/misdat.cpp`, `Source/tables/misdat.h`, `test/missile_registry_test.cpp`
  - Pre-commit: `ctest --test-dir build -R MissileRegistry`

- [ ] 3. Lua World API — SpawnMissile, DamageTarget, GetMonstersInRange

  **What to do**:
  - [ ] **RED**: 写 `test/lua_world_api_test.cpp`，验证：
    1. Lua 脚本调用 `World.SpawnMissile("Firebolt", x, y, dx, dy)` 成功生成 missile 对象
    2. `World.DamageTarget(monsterId, damage, "Fire")` 对目标造成正确伤害
    3. `World.GetMonstersInRange(x, y, radius)` 返回范围内的怪物 ID 列表
    4. 非法参数不崩溃：missile 名不存在 → lua_error；坐标越界 → lua_error
  - [ ] **GREEN**: 创建 `Source/lua/modules/world.cpp` + `world.hpp`：
    - `SpawnMissile(missile_name, x, y, direction)` — 调用现有 `AddMissile()` 再包装成 Lua userdata
    - `DamageTarget(target, damage, damage_type)` — 对 Player 或 Monster 造成伤害
    - `GetMonstersInRange(x, y, radius)` — 遍历 `ActiveMonsters[]` 返回 Lua table
    - 在 `lua_global.cpp` 的 module 注册表中加入 `World` 模块
  - [ ] **REFACTOR**: 确保 `DamageTarget` 复用了 `Monster::takeDamage` / `Player::takeDamage` 的现有伤害管线

  **Must NOT do**:
  - 不在 `SpawnMissile` 中创建新的 missile 类型定义（只 spawn 已有 MissileID）
  - 不在 Lua 回调中直接操作 C++ 原始指针（全部通过 userdata 封装）
  - 不修改 `AddMissile` 的函数签名

  **Recommended Agent Profile**:
  - **Category**: `deep` — new module creation with Lua/C++ interop, memory safety
  - **Skills**: None specific

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Task 4)
  - **Blocks**: Task 5, Task 6
  - **Blocked By**: Task 1, Task 2 (needs AiProc struct + missile registry for type lookup)

  **References**:
  - `Source/missiles.cpp:284` — `AddMissile()` 签名和参数
  - `Source/missiles.cpp:4235-4238` — ProcessMissiles 如何用 `missileData.processFn` dispatch
  - `Source/lua/modules/player.cpp:49-61` — `addItem()` 的 Lua binding 模式（参考）
  - `Source/lua/modules/items.cpp:19-93` — `InitItemUserType` 的 sol::usertype 模式
  - `Source/lua/modules/monsters.cpp:32-45` — `InitMonsterUserType` 模式
  - `Source/lua/lua_global.cpp` — 找到现有 module 注册代码添加 World module
  - `Source/monster.cpp:4257` — `ActiveMonsters[]` 数组结构

  **Acceptance Criteria**:
  - [ ] `ctest --test-dir build -R LuaWorld` → 4 tests PASS
  - [ ] Lua `World.SpawnMissile("Firebolt", 10, 10, 1, 0)` 返回非空 missile 对象
  - [ ] Lua `World.DamageTarget(monster, 50, "Fire")` 后 monster HP 减少 50
  - [ ] Lua `World.GetMonstersInRange(10, 10, 5)` 返回正确的 table
  - [ ] 非法参数触发 lua_error 但不 crash 进程

  **QA Scenarios (MANDATORY)**:

  ```
  Scenario: Spawn missile from Lua
    Tool: Bash (build + ctest)
    Preconditions: Game world initialized (Players[0] exists, dungeon generated)
    Steps:
      1. Lua script: local m = World.SpawnMissile("Firebolt", 10, 10, 1, 0)
      2. Assert m is not nil
      3. Assert m._mitype == MissileID.Firebolt
    Expected Result: Missile spawned at correct position with correct type
    Failure Indicators: nil return, wrong missile type, crash
    Evidence: .omo/evidence/task-3-spawn-missile.txt

  Scenario: Damage target from Lua
    Tool: Bash (build + ctest)
    Preconditions: A monster exists at known position with known HP
    Steps:
      1. Create monster with HP=100 at (10,10)
      2. Lua: World.DamageTarget(monster, 30, "Fire")
      3. Assert monster.hitPoints == 70
      4. Lua: World.DamageTarget(monster, 500, "Physical")
      5. Assert monster is dead or HP at minimum
    Expected Result: Correct damage applied, overkill handled
    Evidence: .omo/evidence/task-3-damage-target.txt

  Scenario: Get monsters in range
    Tool: Bash (build + ctest)
    Preconditions: 3 monsters at (10,10), (12,12), (20,20), player at (10,10)
    Steps:
      1. Lua: local mons = World.GetMonstersInRange(10, 10, 3)
      2. Assert #mons == 2 (only (10,10) and (12,12) in range)
      3. Verify monster id values match expected
    Expected Result: Correct monster count and IDs returned
    Evidence: .omo/evidence/task-3-monsters-in-range.txt

  Scenario: Invalid missile name doesn't crash
    Tool: Bash (build + ctest)
    Preconditions: World initialized
    Steps:
      1. Lua: local ok, err = pcall(function() World.SpawnMissile("NonExistent", 0, 0, 0, 0) end)
      2. Assert ok == false (lua error raised)
      3. Assert process still running (no crash)
    Expected Result: lua_error raised, process alive
    Evidence: .omo/evidence/task-3-invalid-missile.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named per scenario above

  **Commit**: YES
  - Message: `feat(lua): add World module with SpawnMissile/DamageTarget/GetMonstersInRange`
  - Files: `Source/lua/modules/world.cpp`, `Source/lua/modules/world.hpp`, `Source/lua/lua_global.cpp`, `test/lua_world_api_test.cpp`
  - Pre-commit: `ctest --test-dir build -R LuaWorld`

- [ ] 4. Lua Monster API — CreateMonster, SetMonsterAI

  **What to do**:
  - [ ] **RED**: 写 `test/lua_monster_api_test.cpp`，验证：
    1. `Monsters.CreateMonster("zombie", x, y)` 在地牢中生成怪物，返回 Monster userdata
    2. `monster:SetAI(MonsterAIID.Custom)` 改变怪物的 AI 类型
    3. 怪物数量达 MaxMonsters 上限时 `CreateMonster` 返回 nil + 错误信息
    4. 无效怪物名 → lua_error 不 crash
  - [ ] **GREEN**: 扩展 `Source/lua/modules/monsters.cpp`：
    - `CreateMonster(type_name, x, y)` — 调 `AddMonster()` 生成怪物，返回 sol::userdata
    - `Monster:setAI(aiType)` — 修改 `monster.ai` 字段（注意：改 AI 可能需要重置 monster 状态）
    - `Monster:getAI()` — 读当前 ai 字段
    - 将 `MonsterData` 枚举类型（`MonsterAIID`, `MonsterClass`, `monster_resistance` 等）注册为 Lua enum
  - [ ] **REFACTOR**: 把 `InitMonsterUserType` 中已有属性扩展为包含 `ai`, `hitPoints`, `maxHitPoints` 的读写

  **Must NOT do**:
  - 不创建不在 `MonstersData` 中的怪物类型
  - 不在 CreateMonster 中跳过现有的 spawn 验证逻辑
  - 不绕过 Multiplayer 同步（单机先不考虑，标记 TODO）

  **Recommended Agent Profile**:
  - **Category**: `deep` — extending existing Lua module with new unsafe operations
  - **Skills**: None specific

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Task 3)
  - **Blocks**: Task 5, Task 6
  - **Blocked By**: Task 1 (needs AiProc registration for SetAI to be meaningful)

  **References**:
  - `Source/lua/modules/monsters.cpp:32-45` — 当前 InitMonsterUserType，只有 position/id 可读
  - `Source/monster.cpp` — `AddMonster()` 函数签名（需要先找到它）
  - `Source/tables/monstdat.h:22-64` — MonsterAIID 枚举（注册到 Lua）
  - `Source/tables/monstdat.h:98-158` — MonsterData 结构体（验证 type_name 是否存在）
  - `Source/monster.cpp:3091-3132` — AiProc 数组（SetAI 后 dispatch 到正确函数）

  **Acceptance Criteria**:
  - [ ] `ctest --test-dir build -R LuaMonster` → 4 tests PASS
  - [ ] `Monsters.CreateMonster("zombie", 10, 10)` 返回有效 monster 对象
  - [ ] `monster:getAI()` → `MonsterAIID.Zombie`; `monster:setAI(MonsterAIID.Custom)` 后 `getAI()` → Custom
  - [ ] 满员时 CreateMonster 返回 nil
  - [ ] 现有测试无回归

  **QA Scenarios (MANDATORY)**:

  ```
  Scenario: Create monster from Lua
    Tool: Bash (build + ctest)
    Preconditions: Dungeon generated, < MaxMonsters active
    Steps:
      1. Lua: local m = Monsters.CreateMonster("zombie", 10, 10)
      2. Assert m ~= nil
      3. Assert m.position.x == 10 and m.position.y == 10
      4. Assert m.hitPoints > 0
    Expected Result: Monster spawned at correct position with valid HP
    Failure Indicators: nil return, wrong position, zero HP
    Evidence: .omo/evidence/task-4-create-monster.txt

  Scenario: Set AI type on monster
    Tool: Bash (build + ctest)
    Preconditions: Monster created
    Steps:
      1. Create monster with default AI (ZombieAi)
      2. Register custom AI: RegisterAiFunction(MonsterAIID(55), testFn) — testFn sets mgoal=99
      3. monster:setAI(55)  -- Custom = 55
      4. Trigger AI tick
      5. Assert monster goal changed to 99
    Expected Result: AI type changed and new AI function invoked on next tick
    Evidence: .omo/evidence/task-4-set-ai.txt

  Scenario: Max monsters limit
    Tool: Bash (build + ctest)
    Preconditions: Fill ActiveMonsters[] to MaxMonsters
    Steps:
      1. Spawn MaxMonsters (200) monsters
      2. Lua: local m = Monsters.CreateMonster("zombie", 0, 0)
      3. Assert m == nil
      4. Check error message is descriptive
    Expected Result: nil return with meaningful error
    Evidence: .omo/evidence/task-4-max-monsters.txt

  Scenario: Invalid monster type doesn't crash
    Tool: Bash (build + ctest)
    Preconditions: Dungeon ready
    Steps:
      1. Lua: local ok, err = pcall(function() Monsters.CreateMonster("nonexistent_type", 0, 0) end)
      2. Assert ok == false
      3. Assert process alive
    Expected Result: lua_error raised, no crash
    Evidence: .omo/evidence/task-4-invalid-type.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named per scenario above

  **Commit**: YES
  - Message: `feat(lua): add CreateMonster/SetMonsterAI to monsters Lua module`
  - Files: `Source/lua/modules/monsters.cpp`, `Source/lua/modules/monsters.hpp`, `test/lua_monster_api_test.cpp`
  - Pre-commit: `ctest --test-dir build -R LuaMonster`

- [ ] 5. Quest 脚本化 — script_name 字段 + Lua 回调

  **What to do**:
  - [ ] **RED**: 写 `test/quest_script_test.cpp`，验证：
    1. `QuestData` 有 `script_name` 字段，默认空字符串
    2. 设置 `quest.script_name = "test_quest.lua"` 后，`CheckQuests()` 调 Lua 回调
    3. Lua 回调返回 `"complete"` 时 quest 被标记为 DONE
    4. Lua 回调返回 `"active"` 时 quest 保持 ACTIVE
    5. Lua 回调抛出异常时不崩溃（被 `SafeCallResult` 捕获）
    6. `script_name` 为空时行为与现在完全一致（回归测试）
  - [ ] **GREEN**: 
    - `QuestData` 结构体加 `std::string scriptName` 字段（`quests.h:97-107`）
    - `CheckQuests()` 在遍历 quest 时检查 `scriptName` 非空 → 调 Lua
    - Lua 事件命名为 `"Quest_" + quest.scriptName`
    - Lua 函数签名：`function(quest_id, quest_state, var1, var2) → returns new_state_string`
    - 在 `lua_event.cpp` 加 `OnQuestCheck` wrapper
    - LoadQuestData 中从 tsv 读取 `script` 列
  - [ ] **REFACTOR**: 把 quests.cpp 现有的 `CheckQuests` 中的硬编码逻辑标记为 `// Legacy: to be replaced by Lua scripts in Phase 2`

  **Must NOT do**:
  - 不删除现有 CheckQuests 中的任何硬编码逻辑（仅标记注释）
  - 不修改 quest 的网络同步逻辑
  - 不在 Lua 端暴露 Quest 内部状态写权限（只读传入，返回建议状态）

  **Recommended Agent Profile**:
  - **Category**: `deep` — structural change to quest lifecycle with backward compat
  - **Skills**: None specific

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 3 (sequential — depends on Task 3 + 4 for Lua API availability in quest scripts)
  - **Blocks**: Task 6
  - **Blocked By**: Task 3, Task 4

  **References**:
  - `Source/quests.h:82-107` — Quest + QuestData 结构体
  - `Source/quests.cpp:277-341` — CheckQuests() 完整实现
  - `Source/lua/lua_event.cpp:18-53` — CallLuaEvent / CallLuaEventReturn 模板
  - `Source/lua/lua_event.cpp:77-89` — OnMonsterTakeDamage / OnPlayerTakeDamage 模式（有参数传入）
  - `Source/data/record_reader.hpp` — RecordReader 的 read 方法（加 script 列读取）
  - `test/quests_test.cpp:1-60` — 现有 quest 测试风格

  **Acceptance Criteria**:
  - [ ] `ctest --test-dir build -R QuestScript` → 6 tests PASS
  - [ ] `QuestData::scriptName` 字段存在，默认空
  - [ ] 设置 script_name 后 CheckQuests 触发 Lua 回调
  - [ ] Lua 回调返回值正确改变 quest 状态
  - [ ] script_name 为空时行为与改动前一致（回归 PASS）
  - [ ] Lua 异常不崩溃进程

  **QA Scenarios (MANDATORY)**:

  ```
  Scenario: QuestData has script_name field
    Tool: Bash (build + ctest)
    Preconditions: Build with new struct
    Steps:
      1. Instantiate QuestData; assert scriptName == ""
      2. Set scriptName = "test.lua"; assert scriptName == "test.lua"
      3. Instantiate Quest; quest._qidx = Q_SKELKING
      4. Access QuestsData[Q_SKELKING].scriptName
    Expected Result: Field exists, default empty, settable
    Evidence: .omo/evidence/task-5-script-field.txt

  Scenario: CheckQuests calls Lua callback
    Tool: Bash (build + ctest)
    Preconditions: Set up quest with script_name = "test_quest"
    Steps:
      1. Register Lua callback: events.test_quest = { trigger = function(qid, state, v1, v2) called_flag = true; return "active"; end }
      2. Call CheckQuests()
      3. Assert called_flag == true
    Expected Result: Lua callback was invoked
    Evidence: .omo/evidence/task-5-lua-called.txt

  Scenario: Lua callback returns "complete" completes quest
    Tool: Bash (build + ctest)
    Preconditions: Quest ACTIVE, script set
    Steps:
      1. Lua: return "complete"
      2. Call CheckQuests()
      3. Assert quest._qactive == QUEST_DONE
    Expected Result: Quest marked done
    Evidence: .omo/evidence/task-5-complete.txt

  Scenario: Lua callback returns "active" keeps quest active
    Tool: Bash (build + ctest)
    Preconditions: Quest ACTIVE
    Steps:
      1. Lua: return "active"
      2. Call CheckQuests()
      3. Assert quest._qactive == QUEST_ACTIVE
    Expected Result: Quest stays active
    Evidence: .omo/evidence/task-5-keep-active.txt

  Scenario: Lua error doesn't crash
    Tool: Bash (build + ctest)
    Preconditions: Quest with script
    Steps:
      1. Lua: error("intentional test error")
      2. Call CheckQuests() via pcall-like wrapper
      3. Assert process alive, quest state unchanged
    Expected Result: Error logged, game continues
    Evidence: .omo/evidence/task-5-lua-error.txt

  Scenario: No script_name - backward compat
    Tool: Bash (build + ctest)
    Preconditions: script_name == ""
    Steps:
      1. Set up quest same as vanilla Q_SKELKING
      2. Call CheckQuests()
      3. Assert quest behavior identical to pre-change (regression test)
    Expected Result: Existing quest logic unchanged
    Evidence: .omo/evidence/task-5-backward-compat.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named per scenario above

  **Commit**: YES
  - Message: `feat(quest): add Lua script callback support to CheckQuests`
  - Files: `Source/quests.h`, `Source/quests.cpp`, `Source/lua/lua_event.cpp`, `Source/lua/lua_event.hpp`, `test/quest_script_test.cpp`
  - Pre-commit: `ctest --test-dir build -R QuestScript`

- [ ] 6. 端到端 Lua 集成测试

  **What to do**:
  - [ ] **RED**: 写 `test/lua_integration_test.cpp`，验证全部 5 个 API 协同工作：
    1. 注册自定义 AI → 创建怪物并设置 AI → 验证 AI 行为
    2. 注册自定义 missile → 从 Lua spawn → 验证伤害
    3. 创建怪物 → 设置 quest script → 怪物死亡触发 quest 完成
    4. 所有 API 在单次测试中串联调用，验证无状态污染
  - [ ] **GREEN**: 写一个完整的集成测试脚本（可以不依赖真实 Lua，用 C++ 模拟 Lua 回调路径验证数据流正确性）
  - [ ] 确保 build 系统的 CMakeLists.txt 正确添加了所有新测试文件的 target

  **Must NOT do**:
  - 不引入真实的 Lua 脚本文件依赖（集成测试用 C++ 模拟 Lua 调用来验证 API 管线）
  - 不修改 game loop 时序

  **Recommended Agent Profile**:
  - **Category**: `deep` — multi-system integration testing, verifying cross-cutting concerns
  - **Skills**: None specific

  **Parallelization**:
  - **Can Run In Parallel**: NO
  - **Parallel Group**: Wave 4 (sequential — depends on all previous tasks)
  - **Blocks**: None (last implementation task)
  - **Blocked By**: Task 1, Task 2, Task 3, Task 4, Task 5

  **References**:
  - `test/missiles_test.cpp` — 集成测试风格（创建 Players/Missiles 的完整 fixture）
  - `test/quests_test.cpp` — quest 测试设置模式
  - `Source/Source/CMakeLists.txt` — 现有测试 target 定义（需要加新 target）
  - All 5 previous tasks' header files

  **Acceptance Criteria**:
  - [ ] `ctest --test-dir build -R LuaIntegration` → 3 tests PASS
  - [ ] 自定义 AI + 怪物创建 + setAI 端到端工作
  - [ ] 自定义 missile + spawn + damage 端到端工作
  - [ ] Quest 脚本 + 怪物死亡触发 quest 完成 端到端工作
  - [ ] 所有新增测试 target 在 CMakeLists.txt 中正确注册

  **QA Scenarios (MANDATORY)**:

  ```
  Scenario: Full AI pipeline: register → create → setAI → verify
    Tool: Bash (build + ctest)
    Preconditions: All previous tasks' APIs available
    Steps:
      1. RegisterAiFunction(MonsterAIID(55), customDummyAi)
      2. LoadMonsterData() to ensure "zombie" type exists
      3. CreatePlayer, init dungeon stub
      4. Create monster with Monsters.CreateMonster("zombie", 5, 5)
      5. Set monster:setAI(55) (Custom)
      6. Run one AI tick: ProcessMonsters()
      7. Assert customDummyAi was called (check side effect like goal change)
    Expected Result: Custom AI function invoked on monster tick
    Evidence: .omo/evidence/task-6-ai-pipeline.txt

  Scenario: Full missile pipeline: register → spawn → damage
    Tool: Bash (build + ctest)
    Preconditions: Missile registry + World API available
    Steps:
      1. RegisterMissileAddFn + ProcessFn for custom missile
      2. Create monster at (8,8) with HP=100
      3. Lua: World.SpawnMissile(custom_missile_id_or_name, 8, 8, 1, 0) targeting monster
      4. Run ProcessMissiles() for several ticks
      5. Assert monster HP decreased
    Expected Result: Custom missile damages monster
    Evidence: .omo/evidence/task-6-missile-pipeline.txt

  Scenario: Quest + monster death → completion
    Tool: Bash (build + ctest)
    Preconditions: Quest script + Monster API available
    Steps:
      1. Create quest with script_name = "test_quest"
      2. Register Lua: on quest check, check if custom_monster_var == 1 → return "complete"
      3. Create monster with custom AI that on death sets custom_monster_var = 1
      4. Kill monster (DamageTarget with lethal damage)
      5. Run game logic tick
      6. Assert quest._qactive == QUEST_DONE
    Expected Result: Monster death triggers quest completion via Lua
    Evidence: .omo/evidence/task-6-quest-pipeline.txt
  ```

  **Evidence to Capture**:
  - [ ] Each evidence file named per scenario above

  **Commit**: YES
  - Message: `test(lua): add end-to-end integration test for all 5 engine APIs`
  - Files: `test/lua_integration_test.cpp`, `Source/CMakeLists.txt` (add test target)
  - Pre-commit: `ctest --test-dir build -R LuaIntegration`

---

## Final Verification Wave (MANDATORY)

- [ ] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists. For each "Must NOT Have": search codebase for forbidden patterns. Check evidence files exist.

- [ ] F2. **Code Quality Review** — `unspecified-high`
  Run build + all tests. Review all changed files for type suppression, empty catches, unused imports, AI slop patterns.

- [ ] F3. **Real Manual QA** — `unspecified-high`
  Start from clean build. Execute EVERY QA scenario from EVERY task. Test cross-task integration. Test edge cases.

- [ ] F4. **Scope Fidelity Check** — `deep`
  Verify 1:1 — everything in spec was built, nothing beyond spec. Check "Must NOT do" compliance. Detect cross-task contamination.

---

## Commit Strategy

- **1**: `refactor(monster): make AiProc runtime-registrable` — monster.cpp/h, monstdat.h, test/ai_registry_test.cpp
- **2**: `refactor(missile): replace ParseMissile*Fn if-else with map lookup` — misdat.cpp, test/missile_registry_test.cpp
- **3**: `feat(lua): add world module with SpawnMissile/DamageTarget/GetMonstersInRange` — modules/world.cpp/h, test/lua_world_api_test.cpp
- **4**: `feat(lua): add CreateMonster/SetMonsterAI to monsters module` — modules/monsters.cpp/h, test/lua_monster_api_test.cpp
- **5**: `feat(quest): add Lua script callback support for CheckQuests` — quests.h/cpp, lua_event.cpp, test/quest_script_test.cpp
- **6**: `test(lua): end-to-end integration test for all 5 APIs` — test/lua_integration_test.cpp

---

## Success Criteria

### Verification Commands
```bash
# Build
cmake --build build

# Run all tests
ctest --test-dir build

# Run specific test suites
ctest --test-dir build -R "AiRegistry|MissileRegistry|LuaWorld|LuaMonster|QuestScript|LuaIntegration"

# Verify no regression
ctest --test-dir build --output-on-failure
```

### Final Checklist
- [ ] All "Must Have" present
- [ ] All "Must NOT Have" absent
- [ ] All tests pass
- [ ] Existing tests no regression
