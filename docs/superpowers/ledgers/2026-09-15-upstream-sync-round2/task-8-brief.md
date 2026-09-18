## Task 8: 门禁 1-2（CMake 配置 + 全目标编译）

**文件：**
- 修改：无（除非编译再暴露问题，按任务 7 的判据处理并追加提交）

**接口：**
- 依赖输入：任务 7 的编译通过
- 对外产出：`build.ok == true`

- [ ] **步骤 1：配置通过**

```bash
cmake -S. -Bbuild -G Ninja -DDEVILUTIONX_SYSTEM_BENCHMARK=OFF 2>&1 | tail -5
```

预期：退出码 0，无「找不到目标」类错误。

- [ ] **步骤 2：库与主程序编译通过**

```bash
cmake --build build --target devilutionx -j 20 2>&1 | tail -5
```

预期：链接成功，生成 `build/devilutionx`。

- [ ] **步骤 3：若上游 CMake 改动引发配置错误，按错误原文处理并追加提交**

```bash
git commit -am "fix(build): adapt to upstream CMake changes"
```

（仅在步骤 1/2 失败时执行；提交信息里贴错误原文与判据。）

---

