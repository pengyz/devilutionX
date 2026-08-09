# Eval 运行日志

## 2026-08-09 首次 Windows 本机闭环（git 6375f94adf）

- 构建：66 个测试 exe 全部链接成功（build_vs18，MSVC/Ninja/vcpkg）
- smoke：35/36 PASS（render-palette FAIL）
- nightly：62/64 evaluated PASS + 2 FAIL（render-palette、render-text_render_integration）；flow-timedemo-warrior 在 MPQ 复制到 home 后 PASS
- timedemo（存档格式 + RNG 确定性）：PASS

### 过程中发现并修复的真实框架 bug（均为 Windows 特有，Linux CI 未暴露）

1. tools/eval/runner.py + run_tests.py + test_impact.py：Path.exists() 不解析 .exe 后缀 → Windows 全部误报 "binary not found"。加 _binary_path() helper
2. CMake：Windows 构建缺 gettext libintl 链接（libintl_sprintf/libintl_fprintf LNK2019）——mega 对象库 PUBLIC intl.lib（WIN32 条件）+ 所有测试目标统一链接
3. CMake：tests 组经 libdevilutionx_so（Windows DLL）链接时，DATA 符号（AiProc/dFlags/MyPlayer 等全局变量）无法经导入库解析（非 dllimport 引用）→ tests 组改为直接链接 mega 对象库 libdevilutionx
4. CMake：测试 TU 编译需 _DVL_EXPORTING（attributes.h 的 DVL_API_FOR_TEST 在 BUILD_TESTING+MSVC 下默认 dllimport，与真实对象链接不匹配）
5. CMake：test_main（OBJECT 中间层）不递归传递 mega OBJECT 的 obj → tests foreach 用自定义 target_link_dependencies
6. tools/eval/runner.py + run_tests.py：text=True 在 Windows 用 GBK 解码 stdout，GBK 字节导致 UnicodeDecodeError → stdout 丢失 → 误判 passed=0。加 encoding='utf-8', errors='replace'
7. test/palette_blending_test.cpp：TEST/MATCHER 在匿名命名空间（MSVC /OPT:REF 可能丢弃 gtest 注册器）→ 移出匿名 ns（gtest 最佳实践）
8. CMake：SDL 链 standalone 测试加 SDL_MAIN_HANDLED（修复 text_render 注册；SDL2main 入口包装与 gtest_main.dll 导入 main 交互）
9. tools/run_tests.py：TEST_TARGETS 缺 dark_expedition_drop_test、consume_scroll_test（与 CMake 注册漂移）→ 补全

### 待办

- [ ] palette_blending_test Windows 注册问题（见 known-gaps.md）
- [ ] text_render SEH 崩溃（见 known-gaps.md）
- [ ] mpq_present() 检测构建目录（P1）
