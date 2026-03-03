# XDown (v1.0 MVP) Developer Guide

## 项目概述
XDown 是一个基于 C++17、Qt 6 (QML) 和 CMake 构建的现代化、高性能本地下载工具。采用前后端分离的 MVVM 架构。

## 构建与运行命令 (Build & Run Commands)
- 配置 CMake: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="E:/Qt/6.5.3/mingw_64"`
- 编译项目: `cmake --build build --config Debug -j 8`
- 运行程序 (Windows): `.\build\Debug\appXDown.exe`
- 运行测试: `cd build && ctest -C Debug --output-on-failure`
- 清理构建: `rm -rf build` (或在 Windows PowerShell 中使用 `Remove-Item -Recurse -Force build`)

## 架构与代码规范 (Architecture & Code Style)
1. **技术栈**: C++17, Qt 6.5+, QML, SQLite.
2. **MVVM 架构隔离**:
   - **View (QML)**: 仅负责界面展示和动画，严禁在 QML 中编写复杂业务逻辑或网络/文件 IO。
   - **ViewModel (C++)**: 继承自 `QObject` 或 `QAbstractListModel`，使用 `Q_PROPERTY` 和 `Q_INVOKABLE` 暴露数据和方法给 QML。
   - **Model/Core (C++)**: 纯 C++ 业务逻辑（下载引擎、SQLite 操作），通过 Signals/Slots 与 ViewModel 通信，严禁包含任何 UI/QML 依赖。
3. **线程模型**:
   - 主线程（GUI 线程）绝对不允许执行耗时的网络 IO 或大文件读写。
   - 下载任务和 SQLite 数据库操作必须在独立的 Worker Thread 或使用异步 API (`QNetworkAccessManager`) 执行。
4. **C++ 规范**:
   - 优先使用现代 C++ 特性 (智能指针 `std::unique_ptr`/`std::shared_ptr`, `auto`, Lambda 表达式)。
   - 命名规范：类名 `PascalCase`，成员变量 `m_camelCase`，局部变量和函数名 `camelCase`。

## 自主除错准则 (Auto-Heeling Policy) - 进阶版
当用户要求你开发新功能或修改代码时，你必须严格遵循以下闭环，只有跑通所有流程才能向用户汇报：

1. **编译验证阶段**：
   - 执行 `cmake --build build`。如果报错，自主阅读日志，修复 C++/CMake 错误并重试。
   - 如果遇到编译失败（Exit Code != 0），**不要直接向用户求助**。你必须自己阅读 stderr 中的报错信息，分析 C++/Qt 的语法错误或 CMake 链接错误，自主修改代码并重新编译。
   - 允许你最高进行 5 次自主尝试修复。只有在连续 5 次修复依然失败的情况下，再向用户汇报当前卡在了哪里。

2. **运行时验证阶段（极其重要）**：
   - 编译成功后，**绝对不能**直接结束任务。你必须执行编译出的程序进行运行时 QML 解析测试。
   - **执行命令**：在 Windows PowerShell 中运行 `.\build\Desktop_Qt_6_5_3_MinGW_64_bit-Debug\appXDown.exe`（请根据实际路径调整）。
   - **异常捕获**：如果程序在启动后瞬间闪退（退出代码 Exit Code != 0，如 -1），说明发生了 QML 加载失败（如 `is not a type`）或 C++ 核心转储（段错误）。你必须立刻读取终端报错信息，分析是否漏写了 QML `import`（如 `QtQuick.Controls`, `QtQuick.Dialogs`）或空指针，自动修改代码并返回第 1 步重试。

3. **GUI 阻塞处理说明**：
   - 如果执行 `appXDown.exe` 后，终端被挂起阻塞超过 3 秒没有报错信息，说明程序 UI 已经成功启动！此时你不需要继续等待，可以认为测试通过，请告诉用户："程序已成功编译并启动，请在你的屏幕上查看界面。"


## 核心文件映射指引

- 数据实体: `src/common/DownloadTask.h`
- 数据库层: `src/core/DBManager.h/.cpp`
- 下载核心: `src/core/HttpDownloader.h/.cpp`
- 列表模型: `src/gui/TaskListModel.h/.cpp`
- QML 界面: `resources/qml/Main.qml`, `TaskDelegate.qml`

## 单元测试规范 (Unit Testing Standard)
为了保证底层逻辑的健壮性，本项目强制推行单元测试：
1. **测试框架**: 统一使用 Qt Test (`Qt6::Test`)。
2. **目录隔离**: 所有测试代码必须放在项目根目录的 `tests/` 文件夹下，不得污染 `src/` 目录。
3. **CMake 集成**:
   - 主 `CMakeLists.txt` 中必须包含 `enable_testing()` 和 `add_subdirectory(tests)`。
   - `tests/CMakeLists.txt` 中需要使用 `qt_add_executable` 创建独立的测试二进制文件，并使用 `add_test()` 注册。
4. **验证方式**: 编写完测试后，必须通过 `cd build && ctest -C Debug -V` 命令运行并确保 100% Pass。
5. **测试代码规范**: 如果有对应的单元测试用例.md文件，需要按照文件中测试点对应生成测试代码，结果输出需要和单元测试用例.md文件中的编号一一对应。

## 代码注释与文档规范 (Code Commenting & Documentation Standards) - 强制执行
为了保证代码的可读性和团队协作，你生成的**所有代码**必须严格遵守以下注释规范：

1. **语言强制**：所有的代码注释、变量解释、内部逻辑说明，必须 100% 使用**简体中文**。
2. **业务逻辑注释**：在复杂的业务逻辑块（如计算速度、状态流转、指针操作）上方，必须加上中文单行注释 `//` 解释"为什么这么做"以及"这是在干什么"。
3. **函数头规范**：每一个在 `.h` 或 `.cpp` 中声明或定义的方法/函数，其上方必须带有以下固定格式的 Doxygen 风格函数头注释：
	```cpp
	/******************************************************
	 * @brief [中文一句话简述函数的作用]
	 * * [如有必要，这里写一段详细的中文说明，解释内部的核心逻辑或注意事项]
	 * * @param [参数名] [中文说明参数含义]
	 * * @param [参数名] [中文说明参数含义]
	 * @return [返回值类型] [中文说明返回值代表什么意义。如果是 void 可省略]
	 * @note [可选：特殊的注意事项，如线程安全说明、锁的要求等]
	 ****************************************************** /
	```
