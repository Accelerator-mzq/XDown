# XDown (v1.0 MVP) Developer Guide

## 项目概述
XDown 是一个基于 C++17、Qt 6 (QML) 和 CMake 构建的现代化、高性能本地下载工具。采用前后端分离的 MVVM 架构，严格遵循 [qt-project.md](.claude/commands/qt-project.md) 中的开发规范。

## 构建与运行命令 (Build & Run Commands)
- **配置 CMake**: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64" -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1120_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1120_64/bin/g++.exe"`
- **编译项目**: `cmake --build build --config Debug -j 8`
- **运行程序 (Windows)**: `.\build\Debug\appXDown.exe`
- **运行测试**: `cd build && ctest -C Debug --output-on-failure`
- **清理构建**: `Remove-Item -Recurse -Force build` (PowerShell)

## 架构与代码规范 (Architecture & Code Style)
### 1. 核心技术栈
- **基础**: C++17, Qt 6.5+, CMake.
- **UI 层**: QML (仅负责展示与动画，严禁编写业务逻辑)。
- **逻辑层**: ViewModel (继承 QObject/QAbstractListModel) 与 Model (纯 C++ 引擎)。

### 2. 项目结构参考
本项目遵循以下目录逻辑：
- `src/core/`: 下载引擎、SQLite 数据库操作、网络协议实现。
- `src/gui/`: ViewModel 类、QML 插件注册、资源加载。
- `resources/qml/`: 所有 `.qml` 界面文件。
- `tests/src/`: 单元测试代码路径。

### 3. Qt 信号槽与线程规范 (强制执行)
- **跨线程通信**: 严禁在非 GUI 线程直接操作 UI 组件。所有跨线程数据传递必须通过信号槽机制，并显式指定 `Qt::QueuedConnection`。
- **对象所有权**: 必须明确 `QObject` 父子关系以防止内存泄漏。构造函数使用 `explicit` 并传递 `parent` 指针。
- 详细规范与代码示例参见 [qt-project.md](.claude/commands/qt-project.md)。

### 4. Qt Windows 部署自动化 (强制必读)
修改 `CMakeLists.txt` 时**必须**包含 `windeployqt` 的 `POST_BUILD` 钩子，否则会出现"程序点击无反应"或"终端挂起无报错"的幽灵成功现象。
- 详细实现代码和部署检查清单参见 [qt-project.md](.claude/commands/qt-project.md) 的"部署与发布"章节。

### 5. 代码注释与文档规范 (强制执行)
- 所有代码注释必须 100% 使用简体中文，函数头必须使用 `@brief/@param/@return/@note` 格式。
- 详细模板和逻辑注释规范参见 `/project:code`。

### 6. 测试规范
- 框架: Qt Test (`QTest`)，测试代码存放在 `tests/` 目录，须对应 `src/` 中的模块逻辑。
- 编写测试后必须执行 `cd build && ctest -C Debug -V`，输出结果须与测试用例文档编号一一对应。
- 详细自动化测试执行规范参见 `/project:test-auto`。

## 常见问题处理
- QML 类型未找到: 检查 qmldir 或 qt_add_qml_module 配置。
- 信号槽失效: 检查是否缺少 Q_OBJECT 宏，或参数是否已通过 qRegisterMetaType 注册。
- 内存泄漏: 优先使用 std::unique_ptr 或 Qt 的对象树管理。

## 自动化测试技能 (Run-Test.ps1) - 强制约束
修改底层业务逻辑后，**必须**调用 `Run-Test.ps1` 进行无头自动化测试，严禁要求用户手动点击 UI 验证。测试失败则自主修复并循环直到通过。
- 详细调用方式和反馈分析规则参见 `/project:test-auto`。

## 强制缺陷处理与测试闭环协议
用户反馈 Bug 时，**绝对不允许仅修改代码就让用户自己去验证**。必须通过 `/project:test-fix` 触发完整的修复→漏测分析→自动化测试→回归测试闭环流程。

<!-- SMARTROUTE-CONFIG-START -->
# 项目 AI 协作规则

## 模型使用策略

本项目使用 SmartRoute 双模型协作方案。

### 模型定义

- **strong（强模型）**: {{STRONG_MODEL}} — 用于需求分析、设计、Review、疑难问题诊断
- **fast（快模型）**: {{FAST_MODEL}} — 用于编码、测试执行、常规修复
- **默认**: 除明确指定使用 strong 的阶段外，一律使用 fast

### 文档路径规则

> **所有文档路径以 `smartroute.config.json` 中的 `documents` 配置为准，不要使用硬编码路径。**
> 执行任意命令前，先读取 `smartroute.config.json` 中的 `documents` 配置确定对应的文档路径。

### 各阶段模型分配

| 阶段 | 模型 | 工作层 |
|------|------|--------|
| 需求分析 | strong | Claude Code |
| 设计文档 | strong | Claude Code |
| 测试文档 | strong | Claude Code |
| 编码 | fast | Claude Code |
| 测试编码 | fast | Claude Code |
| 自动化测试验证 | fast | Claude Code |
| 自动化测试循环 | fast + strong | Python 脚本 |
| 代码 Review | strong | Claude Code |
| Review 报告修改 | fast | Claude Code |
| 上传 Git | fast | Claude Code |
| 日常提问 | fast | Claude Code |

### 阶段 1：需求分析（strong）

所有需求分析相关工作必须使用 strong 模型：
- 需求点提取与整理
- 规格说明编写（必须包含运行环境、编译器型号和版本号）
- 需求按版本迭代拆分优先级

### 阶段 2：设计文档（strong）

所有设计文档相关工作必须使用 strong 模型：
- 概要设计（架构图、模块划分、数据流）
- 详细设计（类级别、核心接口定义）
- 明确标注需要单元测试的重点功能接口

### 阶段 3：测试文档（strong）

测试文档的创建必须使用 strong 模型：
- 根据需求分析文档输出系统测试例
- 系统测试例和单元测试例严格区分
- 给出可实施的自动化测试方案

### 阶段 4：编码（fast）

所有编码工作使用 fast 模型：
- 功能代码编写、编译错误解决、代码重构

### 阶段 5：测试编码（fast）

使用 fast 模型根据详细设计文档和 src/ 模块逻辑编写单元测试代码：
- 框架: Qt Test（`QTest`）
- 测试代码存放在 `tests/src/` 目录
- 代码必须包含详细的中文注释，每个函数上方有固定格式的函数头注释
- 每个被测模块对应一个测试文件（如 `src/ModuleA.cpp` → `tests/src/test_ModuleA.cpp`）
- 重点覆盖详细设计中标注的需要单元测试的功能接口
- 每次修改后确保测试代码编译通过

通过 `/project:test-code` 触发。

### 阶段 6a：自动化测试验证（fast）

使用 fast 模型逐条验证自动化测试方案的可行性：
- 读取 `docs/test/system-test-cases.md` 和 `docs/test/automation-plan.md`
- 按测试项逐条执行，确认自动化方案能落地
- 方案实施失败时尝试修复（最多 3 轮），仍失败则标记为手工测试
- 输出报告到 `.pipeline/test-auto-report.md`

通过 `/project:test-auto` 触发。建议在 test-code 之后、test-loop 之前执行。

### 阶段 6b：自动化测试循环（Python 脚本）

此阶段通过 `/project:test-loop` 触发 Python 脚本自动执行。
脚本内部自动管理模型切换：fast 超限后自动升级 strong，strong 给方案后自动降回 fast。

当通过 `--bug-report` 触发漏测反思时，流程为：
1. 漏测反思 → 更新系统测试用例文档
2. **自动生成/更新对应的自动化测试方案**（`docs/test/automation-plan.md`）
3. 回归测试验证

### 阶段 7-9：Git / Review / 修改

- 上传 Git → fast
- 代码 Review → strong，输出报告到 docs/review/
- Review 修改 → fast

## 升级/降级协议（Claude Code 内手动场景）

### 从 fast 升级到 strong 的触发条件
1. 连续 3 轮未能解决同一问题
2. 问题定位无进展
3. 需要分析漏测原因或更新测试文档

### 从 strong 降级到 fast 的触发条件
1. strong 已确定问题原因并给出修改方案
2. 进入代码修改或回归测试阶段

## 模型状态报告规则

在执行操作时，第一行报告当前状态：

格式：`🤖 [当前模型: xxx] [当前阶段: xxx] [任务: xxx]`

建议升级时：`⚠️ [建议切换模型: fast → strong] [原因: xxx]`

切换完成后：`✅ [模型已切换: strong] [继续任务: xxx]`

## 文件约定

| 文件/目录 | 用途 |
|-----------|------|
| `smartroute.config.json` | **统一配置入口（模型/Key/文档路径只改这里）** |
| `documents.requirements_dir` | 需求分析文档目录 |
| `documents.overview_design` | 概要设计文档 |
| `documents.detailed_design` | 详细设计文档 |
| `documents.system_test_cases` | 系统测试用例 |
| `documents.unit_test_cases` | 单元测试用例 |
| `documents.automation_plan` | 自动化测试方案 |
| `documents.review_dir` | Review 报告目录 |
| `documents.system_test_code_dir` | 系统测试代码目录 |
| `documents.unit_test_code_dir` | 单元测试代码目录（Qt Test） |
| `.pipeline/` | 自动化脚本和日志 |

## 项目构建信息

<!-- SMARTROUTE-CONFIG-START -->
- 编译命令: {{cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64" -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1120_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1120_64/bin/g++.exe" && cmake --build build --config Debug -j 8}}
- 系统测试命令: {.\Run-Test.ps1 -Module net -Param "https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf"}
- 单元测试命令: {cd build && ctest -C Debug -V}
<!-- SMARTROUTE-CONFIG-END -->

- 运行程序 (Windows): [.\build\Debug\appXDown.exe]
- 清理构建: [Remove-Item -Recurse -Force build`](PowerShell)

<!-- SMARTROUTE-CONFIG-END -->