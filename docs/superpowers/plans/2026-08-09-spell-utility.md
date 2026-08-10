# 法术实用性实施计划：Etherealize 删除

**日期**：2026-08-09
**状态**：草案（随规格 `2026-08-09-spell-utility-design.md`）
**取代**：无

---

## 0. 文档关系

本计划执行 `specs/2026-08-09-spell-utility-design.md`（已批准）。范围：**仅删除 Etherealize**。Rage 回宪章重新裁决，Golem 保留。

---

## 1. 执行顺序

```
Wave 0（立即）:
├── Task 1: 删除 Etherealize 数据行（spelldat base+HF）[quick]
├── Task 2: 移除 SpellFlag::Etherealize 与代码引用（missiles/monster/player/spell_book）[deep]

Wave 1:
├── Task 3: spelldat_test 更新（移出 Etherealize）[quick]
└── Task 4: 保留项核验（SfxID/枚举/TSV/防御性解析）[quick]

Wave FINAL:
├── Task 5: 全量门禁（测试+漂移+timedemo）[quick]
└── Task 6: eval cases + 独立复核 [oracle]
```

**关键路径**：Task 1 → Task 2 → Task 3 → Task 5 → Task 6

---

## 2. 文件改动清单

| 文件 | 改动 | 归属 |
|---|---|---|
| `assets/txtdata/spells/spelldat.tsv` | 删 Etherealize 行（base+HF） | Task 1 |
| `Source/player.h` | 移除 `SpellFlag::Etherealize` 位 | Task 2 |
| `Source/missiles.cpp` | 移除 :386/:929/:1093/:1226 分支 | Task 2 |
| `Source/monster.cpp` | 移除 :1176 Etherealize 检查 | Task 2 |
| `Source/player.cpp` | 移除 :717 Etherealize 检查 | Task 2 |
| `Source/panels/spell_book.cpp` | :50 移出 Etherealize，槽位填 `SpellID::Invalid` | Task 2 |
| `test/spelldat_test.cpp` | `AllLearnableSpellsHaveDescriptions` 移出 Etherealize | Task 3 |
| `eval/cases/` | 新增/更新 case | Task 6 |

**保留不动**（防御性，规格 §3.2）：`spelldat.cpp`/`misdat.cpp`/`spell_tooltip.cpp`/`lua`/`translation_dummy` 解析、`spell_icons.cpp:55` 槽位、`SfxID::SpellEtherealize`、missiles/spelldesc/effects TSV、`misdat.h:70`。

**行尾约束（禁令 7）**：改动文件保持 CRLF；spelldat_test 按文件既有行尾。

---

## 3. Task 1：删除 Etherealize 数据行

**目标**：从 spelldat.tsv（base + HF）删除 Etherealize 行。

**步骤**：删除 `assets/txtdata/spells/spelldat.tsv:23` 与 `mods/hf/txtdata/spells/spelldat.tsv` 对应行（`sed -i '/^Etherealize\t/d'`）。

**注意**：名称键控加载器（Dark Expedition 已建）自动处理——删除行后枚举值解析到 `MakeUnloadedSpellData` 默认行，不错位。

**验收**（规格 §6 #1）：grep `^Etherealize` in spelldat.tsv（base+HF）→ 无匹配。

---

## 4. Task 2：移除 SpellFlag::Etherealize 与代码引用

**目标**：移除运行时状态标志与全部生产引用。

**步骤**：
1. `player.h`：从 `SpellFlag` 枚举移除 Etherealize 位
2. `missiles.cpp:386/:929/:1093/:1226`：移除分支（:929 是 spell→missile 映射的 `return false` case，:386/:1093 是防箭检查，:1226 是状态清除）
3. `monster.cpp:1176`：移除 `HasAnyOf(player._pSpellFlags, SpellFlag::Etherealize)` 条件
4. `player.cpp:717`：移除 Etherealize 防伤检查
5. `spell_book.cpp:50`：从 `SpellPages` 列表移除 Etherealize，槽位填 `SpellID::Invalid`

**注意**：
- 保留 `MissileID::Etherealize` 枚举与相关 TSV（弹道 ID 保留，防御性）
- `SfxID::SpellEtherealize` 保留（Telekinesis/Warp 用作 castSound）
- 编译后确认无 `SpellFlag::Etherealize` 残留引用

**验收**（规格 §6 #2）：grep `SpellFlag::Etherealize` in Source/ → 无生产引用。

---

## 5. Task 3：spelldat_test 更新

**目标**：`AllLearnableSpellsHaveDescriptions` 移出 Etherealize。

**步骤**：`test/spelldat_test.cpp:55` 从学习列表移除 `SpellID::Etherealize`（Rage 在 :54 保留）。

**注意**：删除数据行后 Etherealize 描述为空 → 测试必失败，须先更新。Dark Expedition 的 `CutContentSpellsHaveEmptyDescriptions` 已覆盖 stub，可考虑把 Etherealize 加入该测试（描述应为空）。

**验收**（规格 §6 #6）：spelldat_test 通过。

---

## 6. Task 4：保留项核验

**目标**：确认防御性保留项全部在位。

**步骤**：
1. `SpellID::Etherealize`/`MissileID::Etherealize` 枚举保留
2. `spell_icons.cpp:55` 槽位保留（C 数组）
3. `SfxID::SpellEtherealize`（sound_effect_enums.h:194）保留
4. spelldesc/missiles/effects TSV 行保留
5. `spelldat.cpp`/`misdat.cpp`/`spell_tooltip.cpp`/`lua`/`translation_dummy` 解析保留

**验收**：grep 确认各保留项存在。

---

## 7. Wave FINAL 门禁

### Task 5：全量门禁
- `python3 tools/run_tests.py --json /tmp/ci.json`：全部测试 + 漂移
- 通过标准：`ctest.passed_pct==100 && failed==0 && drift.drift_ok==true`；`pack_test`+`writehero_test`+`timedemo` 全过（存档兼容）

### Task 6：eval cases + 独立复核
- 新增/更新 case：spell-data（确认 Etherealize 删除后 spelldat_test 通过）
- Oracle 复核落地 diff 对照规格红线

---

## 8. 提交策略

| 提交 | 内容 |
|---|---|
| 1 | Task 1+2 数据行 + 代码引用删除 |
| 2 | Task 3 spelldat_test 更新 |
| 3 | Task 4-6 保留核验 + eval + 门禁 |

---

## 9. 状态

**草案**。待规格评审通过后执行。

**已知风险**：
- Task 2 的 missiles/monster/player 分支移除需编译验证无残留
- Task 3 的 spelldat_test 更新与 Dark Expedition 的 stub 测试（`CutContentSpellsHaveEmptyDescriptions`）交互——Etherealize 可并入该测试
- Rage 不在本计划（回宪章），Golem 保留
