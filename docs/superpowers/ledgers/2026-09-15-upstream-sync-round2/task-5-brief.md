## Task 5: 解决 `Source/controls/plrctrls.cpp`

**文件：**
- 修改：`Source/controls/plrctrls.cpp`（1 处冲突，双方各加一个 include）

**接口：**
- 依赖输入：任务 2 的冲突状态
- 对外产出：两个 include 同时存在

- [ ] **步骤 1：采用双方**

把冲突块替换为（保持文件既有的 include 排序风格，`cursor_defs.hpp` 与 `levels/gendung.h` 都在此保留）：

```cpp
#include "cursor_defs.hpp"
#include "levels/gendung.h"
```

- [ ] **步骤 2：确认无残留标记**

```bash
grep -nE "^(<<<<<<<|>>>>>>>|=======)$" Source/controls/plrctrls.cpp   # 预期：无输出
grep -n "cursor_defs.hpp\|levels/gendung.h" Source/controls/plrctrls.cpp
```

- [ ] **步骤 3：提交（与其余冲突解决合并提交，见任务 6 步骤 3）**

---

