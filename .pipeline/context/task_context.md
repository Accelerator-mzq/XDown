# Task Context

更新时间: 2026-03-08 17:15:09
来源: `D:\ClaudeProject\XDown\.smartroute\task.md`

## Objective
实现 XDown v2.0 的核心功能，包括：

1. **多线程分块下载（HTTP/HTTPS）**：
   - 实现服务器 Range 支持检测
   - 实现动态分块策略（8 线程）
   - 实现并发下载和进度聚合
   - 实现自适应线程数调整机制
   - 实现断点续传和文件合并

2. **P2P 下载协议支持（BitTorrent & Magnet）**：
   - 集成 libtorrent 库
   - 实现种子文件解析
   - 实现磁力链接解析
   - 实现 Tracker 连接与 DHT 支持
   - 实现上传与做种管理

3. **磁盘与文件管理优化**：
   - 实现磁盘预分配（Windows SetFileValidData）
   - 实现完整性校验（ETag、MD5、Piece Hash）
   - 实现临时文件管理

4. **任务管理与并发控制**：
   - 实现任务创建、启动、暂停、删除
   - 实现并发控制（HTTP 5 个，BT 3 个）
   - 实现批量操作（Start All、Pause All）

5. **数据持久化**：
   - 实现 SQLite 数据库管理
   - 实现进度节流（每 2 秒批量写入）
   - 实现重启恢复

---

## Strict Rules
### 代码规范

1. **语言标准**：严格使用 C++17，禁止使用 C++20/23 特性。
2. **注释规范**：所有代码注释必须使用简体中文，函数头必须使用 `@brief/@param/@return/@note` 格式。
3. **命名规范**：
   - 类名：大驼峰（PascalCase），如 `TaskManager`
   - 函数名：小驼峰（camelCase），如 `createTask()`
   - 成员变量：`m_` 前缀 + 小驼峰，如 `m_taskList`
   - 常量：全大写 + 下划线，如 `MAX_THREAD_COUNT`

### Qt 规范

1. **信号槽**：跨线程通信必须使用 `Qt::QueuedConnection`。
2. **对象所有权**：构造函数必须使用 `explicit` 并传递 `parent` 指针。
3. **线程隔离**：UI 线程与下载线程、数据库线程严格物理隔离。
4. **QML 规范**：QML 内部禁止进行网络请求或数据库操作，所有业务逻辑必须在 C++ 层实现。

### 测试规范

1. **单元测试**：所有标记 `[需单元测试]` 的接口必须编写单元测试。
2. **覆盖率目标**：核心模块单元测试覆盖率 ≥ 85%。
3. **测试框架**：使用 Qt Test 框架。
4. **测试命名**：测试类名为 `<ClassName>Test`，测试方法名为 `test<MethodName>`。

### 构建规范

1. **CMake 配置**：必须包含 `windeployqt` 的 `POST_BUILD` 钩子（见 CLAUDE.md）。
2. **依赖管理**：使用 vcpkg 管理 libtorrent 依赖。
3. **编译器**：首选 MSVC 2022 (v143)，备选 MinGW-w64 (GCC 11.2+)。

---

## Target Files
- ### 核心模块（优先级 P0）
- #### 任务管理
- `src/core/task/TaskManager.h`
- `src/core/task/TaskManager.cpp`
- `src/core/task/DownloadTask.h`
- #### HTTP 下载引擎
- `src/core/download/IDownloadEngine.h`
- `src/core/download/HTTPDownloadEngine.h`
- `src/core/download/HTTPDownloadEngine.cpp`
- `src/core/download/ChunkDownloader.h`
- `src/core/download/ChunkDownloader.cpp`
- `src/core/download/ChunkInfo.h`
- `src/core/download/AdaptiveThreadController.h`
- `src/core/download/AdaptiveThreadController.cpp`
- #### BT 下载引擎
- `src/core/download/BTDownloadEngine.h`
- `src/core/download/BTDownloadEngine.cpp`
- `src/core/download/SeedingController.h`
- `src/core/download/SeedingController.cpp`
- #### 文件管理
- `src/core/file/FileManager.h`
- `src/core/file/FileManager.cpp`
- #### 数据库管理
- `src/core/database/DatabaseManager.h`
- `src/core/database/DatabaseManager.cpp`
- ### ViewModel 层（优先级 P1）
- `src/gui/viewmodel/TaskListViewModel.h`
- `src/gui/viewmodel/TaskListViewModel.cpp`
- `src/gui/viewmodel/GlobalStatsViewModel.h`
- `src/gui/viewmodel/GlobalStatsViewModel.cpp`
- `src/gui/viewmodel/SettingsViewModel.h`
- `src/gui/viewmodel/SettingsViewModel.cpp`
- ### QML 界面（优先级 P2）
- `resources/qml/MainWindow.qml`
- `resources/qml/TaskListView.qml`
- `resources/qml/TaskCard.qml`
- `resources/qml/NewTaskDialog.qml`
- `resources/qml/SettingsView.qml`
- ### 单元测试（优先级 P0）
- `tests/unit/TaskManagerTest.cpp`
- `tests/unit/HTTPDownloadEngineTest.cpp`
- `tests/unit/ChunkDownloaderTest.cpp`
- `tests/unit/AdaptiveThreadControllerTest.cpp`
- `tests/unit/BTDownloadEngineTest.cpp`
- `tests/unit/FileManagerTest.cpp`
- `tests/unit/DatabaseManagerTest.cpp`
- ### 集成测试（优先级 P1）
- `tests/integration/TaskCreationFlowTest.cpp`
- `tests/integration/HttpDownloadFlowTest.cpp`
- `tests/integration/BtDownloadFlowTest.cpp`
- `tests/integration/ProgressUpdateFlowTest.cpp`
- ### 系统测试（优先级 P1）
- `tests/system/Run-Test.ps1`（已存在，需更新）
- `tests/system/test_http_download.cpp`
- `tests/system/test_bt_download.cpp`
- `tests/system/test_multi_thread.cpp`
