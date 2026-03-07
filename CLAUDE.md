# XDown (SmartRoute V3) Developer Guide

## 项目概述
XDown 是一个基于 C++17、Qt 6 (QML) 和 CMake 构建的现代化、高性能本地下载工具。采用前后端分离的 MVVM 架构。

## 架构与代码规范 (Architecture & Code Style)

### 1. 核心技术栈
- **基础**: C++17, Qt 6.5+, CMake.
- **UI 层**: QML (仅负责展示与动画，严禁编写业务逻辑)。
- **逻辑层**: ViewModel (继承 QObject/QAbstractListModel) 与 Model (纯 C++ 引擎)。

### 2. 项目结构
本项目的目录逻辑：
- `src/core/`: 下载引擎、SQLite 数据库操作、网络协议实现。
- `src/gui/`: ViewModel 类、QML 插件注册、资源加载。
- `resources/qml/`: 所有 `.qml` 界面文件。
- `tests/`: 单元测试与系统集成测试例。

### 3. Qt 信号槽与线程规范 (强制执行)
- **跨线程通信**: 严禁在非 GUI 线程直接操作 UI 组件。所有跨线程数据传递必须通过信号槽机制，并显式指定 `Qt::QueuedConnection`。
- **对象所有权**: 必须明确 `QObject` 父子关系以防止内存泄漏。构造函数使用 `explicit` 并传递 `parent` 指针。

### 4. Qt Windows 部署自动化 (强制必读)
修改 `CMakeLists.txt` 时**必须**包含 `windeployqt` 的 `POST_BUILD` 钩子，否则会出现"程序点击无反应"或"终端挂起无报错"的幽灵成功现象（这不会在 stdout 下打印任何异常）。
> **禁止遗漏以下代码片段**：
> ```cmake
> if(WIN32)
>     add_custom_command(TARGET appXDown POST_BUILD
>         COMMAND windeployqt $<TARGET_FILE:appXDown>
>         COMMENT "Running windeployqt to copy Qt DLLs..."
>     )
> endif()
> ```

### 5. 代码与测试规范 (强制执行)
- 所有代码注释必须使用简体中文，函数头必须使用 `@brief/@param/@return/@note` 格式。
- QML 内部禁止进行网络请求或数据库操作。
- 添加任何新底层业务逻辑后，**必须**配套编写 Qt Test 或通过 `Run-Test.ps1` 覆盖它。

## 如何开发新功能 / 解决 Bug

本项目已接入 **SmartRoute V3 自动化流水线框架**。
你在前台（本 CLI 界面）与 AI 讨论完架构、敲定设计后，无需手动去触发一遍遍的编译排错！

**标准工作流：**
在对话框中直接输入 `/project:test-loop` 或者说：
`“方案已定，请帮我写入 .smartroute/task.md 工单，并启动后台 test_loop 流水线自动完成开发。”`

> 引擎会拉起 `coder`, `test_coder`, `fixer` 等角色，在后台闭门循环执行“写代码 -> 编译 -> 测试 -> 修复”，直到终端输出 `[SUCCESS]`。

<!-- SMARTROUTE-CONFIG-START -->
# 项目 AI 协作规则（SmartRoute V3.0）

## 总体分层

- 前台指挥部（CLI / 强模型）: Assistant / PM / Architect / QA Lead / Reviewer / DevOps
- 后台兵工厂（Python Orchestrator）: Planner / Coder / Test Coder / Runtime / Fixer / Debug Expert

## 后台角色配置（可由 setup.py 注入）

- `planner`: claude-sonnet-4-5-20250929
- `coder`: MiniMax-M2.5-highspeed
- `test_coder`: MiniMax-M2.5-highspeed
- `fixer`: MiniMax-M2.5-highspeed
- `debug_expert`: claude-sonnet-4-5-20250929

## 生命周期执行顺序（对齐 5-9 步）

1. `Planner` 读取 `.smartroute/task.md`，生成 `Execution_Plan.json`
2. `Coder` 按原子步骤生成业务代码
3. `Test Coder` 生成系统/单元测试代码
4. `Runtime` 执行 `cmake/make/ctest`（本地子进程）
5. `Fixer` 处理编译/测试失败（最多重试 3 次）
6. 超限后升级 `Debug Expert`，再回落 `Coder` 执行方案

## 执行铁律

1. CLI 负责需求与决策，不做循环编译/测试。
2. Python 流水线负责执行、试错、归档。
3. 任务交接文件必须包含：
   - `[Task Objective]`
   - `[Strict Rules]`
   - `[Target Files]`
4. 执行命令：
   - `python .pipeline/test_loop.py --project-dir . --task .smartroute/task.md`

## 文件约定

| 文件/目录 | 用途 |
|-----------|------|
| `.smartroute/task.md` | CLI → Python 的任务交接书 |
| `.smartroute/Execution_Plan.json` | Planner 产出的原子执行图 |
| `.pipeline/context/` | Context Manager |
| `.pipeline/logs/` | Observability 日志与 trace |
| `.smartroute/artifacts/` | Artifact Manager 工件归档 |

## 项目构建信息

- 编译命令: cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64" && cmake --build build --config Debug -j 8
- 系统测试命令: powershell -ExecutionPolicy Bypass -File .\Run-Test.ps1 -Module net
- 单元测试命令: cd build && ctest -C Debug -V

<!-- SMARTROUTE-CONFIG-END -->