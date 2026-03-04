# XDown (v1.0 MVP) Developer Guide

## 项目概述
XDown 是一个基于 C++17、Qt 6 (QML) 和 CMake 构建的现代化、高性能本地下载工具。采用前后端分离的 MVVM 架构，严格遵循 [qt-project.md](./qt-project.md) 中的开发规范。

## 构建与运行命令 (Build & Run Commands)
- **配置 CMake**: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64"`
- **编译项目**: `cmake --build build --config Debug -j 8`
- **运行程序 (Windows)**: `.\build\Debug\appXDown.exe`
- **运行测试**: `cd build && ctest -C Debug --output-on-failure`
- **清理构建**: `Remove-Item -Recurse -Force build` (PowerShell)

## 架构与代码规范 (Architecture & Code Style)
### 1. 核心技术栈
- **基础**: C++17, Qt 6.5+, CMake.
- **UI 层**: QML (仅负责展示与动画，严禁编写业务逻辑)。
- **逻辑层**: ViewModel (继承 QObject/QAbstractListModel) 与 Model (纯 C++ 引擎)。

### 2. Qt 信号槽与线程规范 (强制执行)
- **跨线程通信**: 严禁在非 GUI 线程直接操作 UI 组件。所有跨线程的数据传递必须通过信号槽机制，并显式指定连接类型为 `Qt::QueuedConnection`。
- **对象所有权**: 必须明确 `QObject` 的父子关系以防止内存泄漏。在构造函数中使用 `explicit` 关键字并传递 `parent` 指针。

### 3. 项目结构参考
本项目遵循以下目录逻辑：
- `src/core/`: 下载引擎、SQLite 数据库操作、网络协议实现。
- `src/gui/`: ViewModel 类、QML 插件注册、资源加载。
- `resources/qml/`: 所有 `.qml` 界面文件。

## Qt Windows 部署自动化 (Deployment Strategy) - 强制必读
由于 Windows 系统的 DLL 加载机制，为杜绝“程序点击无反应”或“终端挂起无报错”的幽灵成功现象，**修改 `CMakeLists.txt` 时必须包含以下部署钩子**：

```cmake
# 必须在可执行文件目标定义后添加
if(WIN32)
    add_custom_command(TARGET appXDown POST_BUILD
        COMMAND windeployqt $<TARGET_FILE:appXDown>
        COMMENT "执行 windeployqt 自动拷贝 Qt 运行时库，确保 Windows 环境可运行..."
    )
endif()
```
## 测试规范 (Testing Standards)
	1.框架: 使用 Qt Test 框架。
	2.位置: 测试代码存放在 tests/ 目录，且须对应 src/ 中的模块逻辑。
	3.验证: 编写完测试后，必须执行 cd build && ctest -C Debug -V。输出结果必须与“单元测试用例.md”中的编号一一对应。

##代码注释与文档规范 (Code Commenting) - 强制执行
	1.语言限制: 所有的代码注释、变量解释、逻辑说明，必须 100% 使用简体中文。
	2.函数头规范:
``` cpp
/******************************************************
 * @brief [中文一句话简述函数的作用]
 * @param [参数名] [中文说明参数含义]
 * @return [返回值说明]
 * @note [特别注意：例如此函数是否线程安全，或是否涉及 Qt::QueuedConnection]
 ******************************************************/
```
	3.逻辑注释: 在复杂的指针操作、SQLite 事务或下载状态流转处，必须加上 // 中文单行注释解释原因。
## 常见问题处理
- QML 类型未找到: 检查 qmldir 或 qt_add_qml_module 配置。
- 信号槽失效: 检查是否缺少 Q_OBJECT 宏，或参数是否已通过 qRegisterMetaType 注册。
- 内存泄漏: 优先使用 std::unique_ptr 或 Qt 的对象树管理。
```
### 优化说明：
1.  **明确了 `qt-project.md` 的地位**：在概述中直接将其作为引用的规范文件。
2.  **强化了部署逻辑**：直接将 `windeployqt` 的 CMake 代码块植入 `CLAUDE.md`，这样 Claude 在帮你生成或修改构建脚本时，不会漏掉这个关键步骤。
3.  **细化了信号槽要求**：加入了对 `Qt::QueuedConnection` 和 `Q_OBJECT` 宏的检查要求，这是 Qt 开发中最常见的错误来源。
4.  **整合了 Doxygen 注释规范**：将你原有的中文注释要求与 Qt 风格的函数说明结合。

现在，Claude Code 在读取此文件后，会非常清楚在 Windows 下开发 XDown 时哪些操作是“禁区”，哪些是“必须”。
```

## 自动化测试技能 (Testing Skill) - 强制约束
作为 AI 开发助手，你拥有一个名为 `Run-Test.ps1` 的专属测试技能。当你修改了底层业务逻辑（如网络、数据库、文件 IO）后，**你必须**调用此脚本进行无头自动化测试，严禁要求用户手动点击 UI 验证。

**技能调用方式 (在 PowerShell 中)：**
`.\Run-Test.ps1 -Module <模块名> -Param "<参数>"`
例如测试下载：`.\Run-Test.ps1 -Module net -Param "https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf"`

**技能反馈分析规则：**
1. 脚本运行结束后，仔细阅读终端输出的日志。
2. 如果日志末尾包含 `[TEST_RESULT: SUCCESS]`，说明测试通过，你可以向用户汇报成果。
3. 如果日志末尾包含 `[TEST_RESULT: FAILED]`，代表你写的代码有 Bug 或遇到了重定向/超时等问题。你必须向上翻阅终端里的 `qDebug` 错误信息，自主修改 C++ 代码，重新编译 (`cmake --build build`)，并**再次调用该技能**，不断循环，直到看到 SUCCESS 为止。

## 强制缺陷处理与测试闭环协议 (Defect Handling & Test-Driven Protocol)
当用户手动测试发现了 Bug 并反馈给你时，**你绝对不允许仅仅修改代码就让用户自己去验证**。你必须严格执行以下完整的 QA 闭环流程，并在最终回复中按此结构输出报告：

### 1. 缺陷修复 (Bug Fix)
分析用户反馈，修改底层 C++ 或 QML 代码解决该问题。

### 2. 测试覆盖率审查 (Coverage Analysis) - 双轨制
你必须主动进行以下两层审查，并在回复中分别汇报：
- **A. 系统级覆盖审查 (读取 docs/XDown_SystemTestCases_1.0.md)**：
  检查该 Bug 的用户场景是否在系统测试用例文档中。如果属于漏测缺陷，你必须主动修改该 Markdown 文件，补充新的 ST 用例。
- **B. 单元级覆盖审查 (直接读取 tests/ 目录下的 C++ 测试源码)**：
  你必须去查阅 `tests/` 目录下相关的 `tst_*.cpp` 文件（例如报了数据库 Bug 就去查 `tst_dbmanager.cpp`）。
  分析并回答：为什么之前的单元测试没有拦截到这个底层的逻辑/边界错误？如果是缺失了特定的断言 (Assert) 或边界条件 Mock，你必须直接在对应的 C++ 测试文件中补齐这个 Unit Test Case。

### 3. 强制自动化测试实现 (Automated Test Implementation)
对于该缺陷，你必须给出一个自动化测试方案，并尝试落实到代码中：
- 优先在 `tests/` 目录下编写对应的 C++ QtTest 单元测试，或补充到无头命令行测试模式中。
- 编写完成后，你必须自己调用 `Run-Test.ps1` 或 `ctest` 跑通该测试，证明 Bug 已被修复。
- **豁免条款**：如果该 Bug 属于纯粹的视觉 UI 问题（如颜色不对、弹窗位置偏移、QML 渲染错位），无法通过无头模式和 C++ 断言完成自动化测试，你必须在回复中明确说明：“**无法完成自动化测试，原因：[具体的技术限制]**”，并提供详细的手动验证步骤。