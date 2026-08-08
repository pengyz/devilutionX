# DevilutionX 项目知识库

团队共享的 AI 协作知识。所有工程师的 Claude / 编程助手实例可加载此目录，作为项目级共享记忆。

## 与 `docs/` 的分工

- `docs/superpowers/` 面向**人类**：设计规格、宪章、实施计划（有 7 段结构、红线检查）
- `docs/knowledge/` 面向**AI 实例**：单条独立、易召回、强调"为什么"和"何时使用"的离散知识点

互补不重复。写规格进 `docs/superpowers/`；写"另一个 AI 实例遇到这场景能少走弯路"的离散经验进 `docs/knowledge/`。

## 形态

- 1 条知识 1 文件：`<type>_<slug>.md`
- frontmatter 强制 `name` / `description` / `type` / `created` / `sources`
- 正文：事实陈述 + `**为什么：**` + `**何时使用：**`（`reference` 类可省 Why/When）
- `MEMORY.md` 是机器维护的索引（≤200 行 / 25KB），按 type 分组

## 类型

| type | 写入内容 | 触发时刻 |
|------|---------|---------|
| `architecture` | 设计决策、模块关系、为什么选 A 不选 B | 方案被确认采纳、读代码看不出"为什么"时 |
| `gotcha` | 平台坑、反直觉行为、踩雷记录（Diablo 引擎/存档/渲染/Lua 等） | 触碰某机制导致非预期结果、找到 workaround 时 |
| `pattern` | 代码约定、开发模式、本项目最佳实践 | 同类问题第二次出现、确立项目约定时 |
| `debug` | 调试方法、关键日志位置、排查路径 | 排查中发现高效诊断手段、定位到关键文件时 |
| `decision` | 已验证的取舍（含正反馈） | 用户确认非常规选择有效时 |
| `reference` | 外部资源指针、关键文件位置 | 用户提到一个外部资源及其用途时 |

## 写入时机（4 个时刻）

1. **commit 后** — 用户主动提及刚 commit 的内容
2. **用户纠正方法** — "别这样做"、"不是这个" → 多为 `pattern` / `gotcha`
3. **用户确认非显然方法有效** — "对，就这么做" → `decision`，**正反馈也写**，避免知识库只剩避雷
4. **遇到平台坑/反直觉行为** — `gotcha` / `debug`

判断标准：另一个 AI 实例遇到同样场景时，这条知识能帮它少走弯路吗？不能 → 不写。

## 不写入

- 代码本身能表达的（函数签名、文件结构、模块依赖）→ 读代码
- `git log` / `git blame` 能查到的 → 那里是权威
- `docs/superpowers/` 已有的规则 → 不重复（宪章是裁决基准）
- 仅对当前用户有用的偏好 → 写个人 memory

## 规则（针对本仓库）

- 本仓库是 DevilutionX（Diablo 反编译引擎）fork，大量逻辑是**读代码看不出为什么**的上古设计——优先记录此类
- 宪章（`docs/superpowers/specs/2026-07-27-better-d1-design-charter.md`）是裁决基准，knowledge 不与之冲突；冲突时以宪章为准并修 knowledge
- 行尾：本目录 `.md` 文件必须 LF（`.editorconfig` `[*.md]` end_of_line = lf）
