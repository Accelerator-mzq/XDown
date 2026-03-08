# XDown v2.0 开发任务工单

**任务类型**：功能开发 + 测试
**优先级**：P0
**预计工作量**：6 周
**创建时间**：2026-03-08

---

[Task Objective]

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

[Strict Rules]

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

[Target Files]

### 核心模块（优先级 P0）

#### 任务管理
- `src/core/task/TaskManager.h`
- `src/core/task/TaskManager.cpp`
- `src/core/task/DownloadTask.h`

#### HTTP 下载引擎
- `src/core/download/IDownloadEngine.h`
- `src/core/download/HTTPDownloadEngine.h`
- `src/core/download/HTTPDownloadEngine.cpp`
- `src/core/download/ChunkDownloader.h`
- `src/core/download/ChunkDownloader.cpp`
- `src/core/download/ChunkInfo.h`
- `src/core/download/AdaptiveThreadController.h`
- `src/core/download/AdaptiveThreadController.cpp`

#### BT 下载引擎
- `src/core/download/BTDownloadEngine.h`
- `src/core/download/BTDownloadEngine.cpp`
- `src/core/download/SeedingController.h`
- `src/core/download/SeedingController.cpp`

#### 文件管理
- `src/core/file/FileManager.h`
- `src/core/file/FileManager.cpp`

#### 数据库管理
- `src/core/database/DatabaseManager.h`
- `src/core/database/DatabaseManager.cpp`

### ViewModel 层（优先级 P1）

- `src/gui/viewmodel/TaskListViewModel.h`
- `src/gui/viewmodel/TaskListViewModel.cpp`
- `src/gui/viewmodel/GlobalStatsViewModel.h`
- `src/gui/viewmodel/GlobalStatsViewModel.cpp`
- `src/gui/viewmodel/SettingsViewModel.h`
- `src/gui/viewmodel/SettingsViewModel.cpp`

### QML 界面（优先级 P2）

- `resources/qml/MainWindow.qml`
- `resources/qml/TaskListView.qml`
- `resources/qml/TaskCard.qml`
- `resources/qml/NewTaskDialog.qml`
- `resources/qml/SettingsView.qml`

### 单元测试（优先级 P0）

- `tests/unit/TaskManagerTest.cpp`
- `tests/unit/HTTPDownloadEngineTest.cpp`
- `tests/unit/ChunkDownloaderTest.cpp`
- `tests/unit/AdaptiveThreadControllerTest.cpp`
- `tests/unit/BTDownloadEngineTest.cpp`
- `tests/unit/FileManagerTest.cpp`
- `tests/unit/DatabaseManagerTest.cpp`

### 集成测试（优先级 P1）

- `tests/integration/TaskCreationFlowTest.cpp`
- `tests/integration/HttpDownloadFlowTest.cpp`
- `tests/integration/BtDownloadFlowTest.cpp`
- `tests/integration/ProgressUpdateFlowTest.cpp`

### 系统测试（优先级 P1）

- `tests/system/Run-Test.ps1`（已存在，需更新）
- `tests/system/test_http_download.cpp`
- `tests/system/test_bt_download.cpp`
- `tests/system/test_multi_thread.cpp`

---

## [Implementation Strategy]

### 第一轮：核心下载引擎（Week 1-2）

**目标**：实现 HTTP 多线程下载和 BT 下载的核心功能。

**任务**：
1. 实现 `IDownloadEngine` 接口
2. 实现 `HTTPDownloadEngine`（不含自适应线程）
3. 实现 `ChunkDownloader`
4. 实现 `BTDownloadEngine`（基础功能）
5. 实现 `FileManager`（磁盘预分配、文件合并、完整性校验）
6. 编写对应的单元测试

**验收标准**：
- 单元测试通过率 100%
- 覆盖率 ≥ 85%
- 能够成功下载 HTTP 文件（多线程）
- 能够成功下载 BT 文件（基础功能）

---

### 第二轮：任务管理与数据持久化（Week 3）

**目标**：实现任务管理、并发控制和数据持久化。

**任务**：
1. 实现 `TaskManager`（任务创建、启动、暂停、删除、并发控制）
2. 实现 `DatabaseManager`（SQLite 数据库管理、进度节流）
3. 实现自适应线程数调整（`AdaptiveThreadController`）
4. 实现做种控制（`SeedingController`）
5. 编写对应的单元测试和集成测试

**验收标准**：
- 单元测试通过率 100%
- 集成测试通过率 100%
- 并发控制正常工作（HTTP 5 个，BT 3 个）
- 数据持久化正常工作（重启恢复）

---

### 第三轮：ViewModel 与 QML 界面（Week 4）

**目标**：实现 ViewModel 层和 QML 界面，完成前后端集成。

**任务**：
1. 实现 `TaskListViewModel`（数据绑定、CRUD 操作）
2. 实现 `GlobalStatsViewModel`（全局速度监控）
3. 实现 `SettingsViewModel`（设置管理）
4. 实现 QML 界面（MainWindow、TaskListView、TaskCard、NewTaskDialog、SettingsView）
5. 编写 ViewModel 单元测试

**验收标准**：
- ViewModel 单元测试通过率 100%
- QML 界面能够正常显示和交互
- 数据绑定正常工作

---

### 第四轮：系统测试与性能优化（Week 5）

**目标**：完成系统测试，优化性能。

**任务**：
1. 执行系统测试用例（33 项验收标准）
2. 性能测试（内存占用、磁盘 I/O、网络带宽利用率）
3. 性能优化（内存优化、磁盘 I/O 优化、网络优化）
4. 修复发现的缺陷

**验收标准**：
- 系统测试通过率 ≥ 95%
- 性能指标满足需求（内存 ≤ 150MB，速度提升 ≥ 5 倍）

---

### 第五轮：回归测试与发布准备（Week 6）

**目标**：回归测试，准备发布。

**任务**：
1. 回归测试（所有测试用例）
2. 修复剩余缺陷
3. 编写用户文档
4. 准备发布包（windeployqt）

**验收标准**：
- 所有测试用例通过
- 无 P0/P1 级别缺陷
- 发布包能够在 Windows 10/11 上正常运行

---

## [Dependencies]

### 第三方库

1. **Qt 6.5.3**：UI 框架、网络库、数据库库
2. **libtorrent-rasterbar 2.0+**：BT 协议实现
3. **OpenSSL 1.1.1+**：HTTPS 加密通信

### 安装方式

```bash
# 使用 vcpkg 安装 libtorrent
vcpkg install libtorrent:x64-windows

# CMake 配置
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
```

---

## [Success Criteria]

### 功能完整性

- ✅ 所有 33 项验收标准通过
- ✅ 支持 HTTP/HTTPS 多线程下载
- ✅ 支持 BT 种子和磁力链接下载
- ✅ 支持断点续传
- ✅ 支持完整性校验
- ✅ 支持自适应线程数调整
- ✅ 支持做种管理

### 质量标准

- ✅ 单元测试覆盖率 ≥ 85%
- ✅ 所有单元测试通过
- ✅ 所有集成测试通过
- ✅ 系统测试通过率 ≥ 95%
- ✅ 无 P0/P1 级别缺陷

### 性能标准

- ✅ 8 线程下载速度提升 ≥ 5 倍
- ✅ 内存占用 ≤ 150MB（10GB 文件）
- ✅ UI 无卡顿（多线程下载时）
- ✅ 磁盘 I/O 利用率 ≥ 200 MB/s（SSD）

---

## [Notes]

1. **优先级说明**：
   - P0：核心功能，必须完成
   - P1：重要功能，尽量完成
   - P2：次要功能，时间允许时完成

2. **风险提示**：
   - libtorrent 集成可能遇到编译问题，建议使用 vcpkg 预编译包
   - 多线程下载的文件合并可能耗时较长，需要优化（使用内存映射文件）
   - BT 下载的网络穿透问题，需要支持 DHT 和 PEX 协议

3. **测试数据准备**：
   - HTTP 测试文件：Ubuntu 24.04 ISO（4.7 GB）
   - BT 测试文件：Ubuntu 24.04 Torrent
   - 磁力链接：公开的开源软件磁力链接

4. **文档参考**：
   - 需求文档：`docs/requirements/xdown-srs-v2.0.md`
   - 概要设计：`docs/design/xdown-overview-design-v2.0.md`
   - 详细设计：`docs/design/xdown-detailed-design-v2.0.md`
   - 测试计划：`docs/test/xdown-test-plan-v2.0.md`
   - 系统测试用例：`docs/test/xdown-system-test-cases-v2.0.md`
   - 自动化测试方案：`docs/test/xdown-automation-plan-v2.0.md`

---

**工单结束**
