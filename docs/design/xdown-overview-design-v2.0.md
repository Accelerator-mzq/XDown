# XDown v2.0 概要设计文档 (Overview Design Document)

**版本号**：v2.0
**文档状态**：Draft
**编写日期**：2026-03-08
**基于需求文档**：xdown-srs-v2.0.md

---

## 1. 架构概述 (Architecture Overview)

### 1.1 设计原则

XDown v2.0 采用**分层架构 + MVVM 模式**，遵循以下核心设计原则：

1. **关注点分离**：UI 层（QML）、业务逻辑层（ViewModel）、核心引擎层（Model）严格分离。
2. **线程隔离**：UI 线程与下载线程、数据库线程物理隔离，通过信号槽通信。
3. **可扩展性**：支持多协议扩展（HTTP/HTTPS/BT/Magnet），未来可轻松添加 FTP/SFTP。
4. **高性能**：多线程并发下载，充分利用网络带宽和磁盘 I/O。
5. **可测试性**：核心模块提供清晰的接口，便于单元测试和集成测试。

### 1.2 整体架构图

```
┌─────────────────────────────────────────────────────────────┐
│                      Presentation Layer                      │
│                         (QML UI)                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ MainView │  │ TaskList │  │ Settings │  │  Dialog  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└───────────────────────────┬─────────────────────────────────┘
                            │ (Data Binding)
┌───────────────────────────┴─────────────────────────────────┐
│                      ViewModel Layer                         │
│                    (Qt C++ QObject)                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ TaskListVM   │  │ GlobalStatsVM│  │ SettingsVM   │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└───────────────────────────┬─────────────────────────────────┘
                            │ (Signal/Slot)
┌───────────────────────────┴─────────────────────────────────┐
│                      Business Logic Layer                    │
│                      (Core Engine)                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ TaskManager  │  │DownloadEngine│  │ DatabaseMgr  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ HTTPDownload │  │  BTDownload  │  │ FileManager  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└───────────────────────────┬─────────────────────────────────┘
                            │
┌───────────────────────────┴─────────────────────────────────┐
│                      Infrastructure Layer                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Qt Network   │  │  libtorrent  │  │  SQLite DB   │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. 模块划分 (Module Division)

### 2.1 表现层 (Presentation Layer)

**职责**：负责 UI 渲染、用户交互、动画效果。

**主要组件**：
- `MainWindow.qml`：主窗口，包含侧边栏、顶部操作栏、主内容区。
- `TaskListView.qml`：任务列表视图，展示下载任务卡片。
- `TaskCard.qml`：单个任务卡片，显示文件名、进度、速度、操作按钮。
- `NewTaskDialog.qml`：新建任务对话框，输入 URL、选择路径、配置高级选项。
- `SettingsView.qml`：设置界面，配置线程数、做种策略、完整性校验等。

**技术栈**：Qt Quick (QML)、Qt Quick Controls 2

---

### 2.2 视图模型层 (ViewModel Layer)

**职责**：连接 UI 与业务逻辑，提供数据绑定、命令处理、状态管理。

**主要类**：

#### 2.2.1 TaskListViewModel
- **职责**：管理任务列表的数据模型，提供 CRUD 操作。
- **继承**：`QAbstractListModel`
- **主要接口**：
  - `addTask(url, savePath, options)`: 添加新任务
  - `removeTask(taskId)`: 删除任务
  - `startTask(taskId)`: 启动任务
  - `pauseTask(taskId)`: 暂停任务
  - `startAll()`: 启动所有任务
  - `pauseAll()`: 暂停所有任务
- **信号**：
  - `taskAdded(taskId)`
  - `taskRemoved(taskId)`
  - `taskProgressUpdated(taskId, progress, speed)`

#### 2.2.2 GlobalStatsViewModel
- **职责**：管理全局统计信息（总下载速度、总上传速度）。
- **继承**：`QObject`
- **主要属性**：
  - `totalDownloadSpeed`: 全局下载速度（MB/s）
  - `totalUploadSpeed`: 全局上传速度（KB/s）
- **信号**：
  - `statsUpdated(downloadSpeed, uploadSpeed)`

#### 2.2.3 SettingsViewModel
- **职责**：管理用户设置，提供配置读写接口。
- **继承**：`QObject`
- **主要接口**：
  - `getThreadCount()`: 获取默认线程数
  - `setThreadCount(count)`: 设置默认线程数
  - `getSeedingStrategy()`: 获取做种策略
  - `setSeedingStrategy(strategy)`: 设置做种策略

---

### 2.3 业务逻辑层 (Business Logic Layer)

**职责**：实现核心下载逻辑、任务管理、数据持久化。

#### 2.3.1 TaskManager（任务管理器）
- **职责**：统一管理所有下载任务，协调并发控制、状态流转。
- **主要接口**：
  - `createTask(url, savePath, options)`: 创建新任务
  - `startTask(taskId)`: 启动任务（检查并发限制）
  - `pauseTask(taskId)`: 暂停任务
  - `deleteTask(taskId, deleteFile)`: 删除任务
  - `getTaskList(filter)`: 获取任务列表（按分类筛选）
- **并发控制**：
  - HTTP/HTTPS 任务：最多 5 个并发
  - BT/Magnet 任务：最多 3 个并发
- **状态机**：管理任务状态流转（Waiting → Downloading → Paused/Completed/Error）

#### 2.3.2 DownloadEngine（下载引擎）
- **职责**：抽象下载引擎接口，支持多协议扩展。
- **接口定义**：
  ```cpp
  class IDownloadEngine {
  public:
      virtual void start() = 0;
      virtual void pause() = 0;
      virtual void resume() = 0;
      virtual qint64 getDownloadedBytes() = 0;
      virtual qint64 getTotalBytes() = 0;
      virtual double getSpeed() = 0;
  signals:
      void progressUpdated(qint64 downloaded, qint64 total);
      void speedUpdated(double speed);
      void finished();
      void error(QString errorMsg);
  };
  ```

#### 2.3.3 HTTPDownloadEngine（HTTP 下载引擎）
- **职责**：实现 HTTP/HTTPS 多线程分块下载。
- **继承**：`IDownloadEngine`
- **核心功能**：
  - 服务器能力检测（Range 支持）
  - 动态分块策略
  - 多线程并发下载
  - 自适应线程数调整
  - 断点续传
  - 文件合并
- **主要类**：
  - `HTTPDownloadEngine`: 主引擎类
  - `ChunkDownloader`: 单个分块下载器
  - `ThreadPoolManager`: 线程池管理器
  - `AdaptiveThreadController`: 自适应线程数控制器

#### 2.3.4 BTDownloadEngine（BT 下载引擎）
- **职责**：实现 BitTorrent 和 Magnet 协议下载。
- **继承**：`IDownloadEngine`
- **核心功能**：
  - 种子文件解析（libtorrent::torrent_info）
  - 磁力链接解析（libtorrent::parse_magnet_uri）
  - Tracker 连接与 DHT 支持
  - Peer 连接与数据传输
  - 上传与做种管理
- **主要类**：
  - `BTDownloadEngine`: 主引擎类
  - `TorrentSession`: libtorrent 会话管理
  - `SeedingController`: 做种控制器

#### 2.3.5 DatabaseManager（数据库管理器）
- **职责**：管理 SQLite 数据库，提供任务持久化接口。
- **主要接口**：
  - `saveTask(task)`: 保存任务
  - `updateTask(task)`: 更新任务
  - `deleteTask(taskId)`: 删除任务
  - `loadAllTasks()`: 加载所有任务
  - `updateProgress(taskId, progress)`: 更新进度（节流）
- **数据库表设计**：见详细设计文档

#### 2.3.6 FileManager（文件管理器）
- **职责**：管理文件操作、磁盘预分配、完整性校验。
- **主要接口**：
  - `preallocateDiskSpace(filePath, size)`: 磁盘预分配
  - `checkDiskSpace(path, requiredSize)`: 检查磁盘空间
  - `verifyFileIntegrity(filePath, etag/md5)`: 完整性校验
  - `mergeChunks(chunkFiles, outputFile)`: 合并分块文件
  - `cleanupTempFiles(taskId)`: 清理临时文件

---

### 2.4 基础设施层 (Infrastructure Layer)

**职责**：提供底层技术支持。

- **Qt Network**：HTTP/HTTPS 网络通信（QNetworkAccessManager）
- **libtorrent**：BitTorrent 协议实现
- **SQLite**：数据持久化（Qt SQL 模块）
- **Qt Concurrent**：线程池与异步任务

---

## 3. 数据流设计 (Data Flow Design)

### 3.1 任务创建流程

```
User Input (QML)
    ↓
NewTaskDialog.qml
    ↓ (emit signal)
TaskListViewModel::addTask()
    ↓
TaskManager::createTask()
    ↓
1. 解析 URL（HTTP/Magnet/Torrent）
2. 校验重复性
3. 创建 DownloadEngine 实例（HTTP/BT）
4. 保存到数据库
    ↓
DatabaseManager::saveTask()
    ↓
TaskListViewModel::taskAdded() signal
    ↓
TaskListView.qml (update UI)
```

### 3.2 下载进度更新流程

```
DownloadEngine (Worker Thread)
    ↓
每秒计算进度和速度
    ↓ (emit signal, Qt::QueuedConnection)
TaskManager::onProgressUpdated()
    ↓
1. 更新内存中的任务状态
2. 每 2 秒批量写入数据库
    ↓
TaskListViewModel::taskProgressUpdated() signal
    ↓
TaskCard.qml (update progress bar & speed)
```

### 3.3 多线程下载流程

```
HTTPDownloadEngine::start()
    ↓
1. 发送 HEAD 请求检测 Range 支持
    ↓
2. 计算分块策略（文件大小 / 线程数）
    ↓
3. 创建 ChunkDownloader 实例（每个线程一个）
    ↓
4. 启动线程池，并发下载
    ↓
ChunkDownloader::run() (Worker Thread)
    ↓
- 发送 Range 请求
- 写入临时文件（.part1, .part2, ...）
- 定时上报进度
    ↓
5. 所有 Chunk 完成后，合并文件
    ↓
FileManager::mergeChunks()
    ↓
6. 完整性校验
    ↓
FileManager::verifyFileIntegrity()
    ↓
7. 清理临时文件
    ↓
HTTPDownloadEngine::finished() signal
```

---

## 4. 线程模型 (Threading Model)

### 4.1 线程划分

| 线程名称 | 职责 | 数量 |
|---------|------|------|
| **Main Thread** | UI 渲染、事件循环 | 1 |
| **Database Thread** | 数据库读写操作 | 1 |
| **HTTP Worker Threads** | HTTP 分块下载 | 动态（1-32 per task） |
| **BT Worker Thread** | libtorrent 事件循环 | 1 per task |
| **File I/O Thread** | 文件合并、完整性校验 | 1 |

### 4.2 线程通信

- **Main Thread ↔ Worker Threads**：通过 `Qt::QueuedConnection` 信号槽通信。
- **Worker Threads ↔ Database Thread**：通过消息队列（QQueue + QMutex）通信。
- **线程安全**：所有共享数据使用 `QMutex` 或 `QReadWriteLock` 保护。

---

## 5. 关键技术决策 (Key Technical Decisions)

### 5.1 多线程下载实现方案

**方案选择**：每个 Chunk 使用独立的 `QNetworkAccessManager` 实例。

**理由**：
- `QNetworkAccessManager` 是线程安全的（每个线程一个实例）。
- 避免共享 `QNetworkAccessManager` 导致的锁竞争。
- 简化错误处理（每个 Chunk 独立重试）。

### 5.2 文件合并策略

**方案选择**：使用内存映射文件（QFile::map）进行合并。

**理由**：
- 避免大文件的内存拷贝。
- 利用操作系统的页缓存机制，提升性能。
- 支持大文件（10GB+）合并。

### 5.3 自适应线程数调整算法

**算法**：基于速度增长率的动态调整。

**伪代码**：
```cpp
void AdaptiveThreadController::adjustThreads() {
    double currentSpeed = calculateAverageSpeed();
    double growthRate = (currentSpeed - lastSpeed) / lastSpeed;

    if (growthRate < 0.10 && threadCount < 32) {
        threadCount += 2;  // 增加线程
    } else if (growthRate < 0 && threadCount > 4) {
        threadCount -= 2;  // 减少线程
    }
    // 否则保持不变

    lastSpeed = currentSpeed;
}
```

### 5.4 libtorrent 集成方案

**方案选择**：使用 vcpkg 管理 libtorrent 依赖。

**CMake 配置**：
```cmake
find_package(LibtorrentRasterbar REQUIRED)
target_link_libraries(appXDown PRIVATE LibtorrentRasterbar::torrent-rasterbar)
```

---

## 6. 性能优化策略 (Performance Optimization)

### 6.1 内存优化

1. **分块下载缓冲区**：每个 Chunk 使用 64KB 固定缓冲区，避免内存溢出。
2. **进度更新节流**：UI 更新频率限制为 1 次/秒，数据库更新频率限制为 1 次/2 秒。
3. **对象池**：复用 `ChunkDownloader` 对象，避免频繁创建/销毁。

### 6.2 磁盘 I/O 优化

1. **磁盘预分配**：使用 `SetFileValidData`（Windows）预分配磁盘空间，防止碎片化。
2. **批量写入**：使用 64KB 缓冲区批量写入，减少系统调用次数。
3. **异步 I/O**：文件合并使用异步 I/O（Qt Concurrent），避免阻塞主线程。

### 6.3 网络优化

1. **连接复用**：HTTP/1.1 Keep-Alive，减少 TCP 握手开销。
2. **并发连接数**：根据网络带宽动态调整线程数（自适应算法）。
3. **超时重试**：网络超时后自动重试（最多 3 次，间隔 5 秒）。

---

## 7. 安全性设计 (Security Design)

### 7.1 HTTPS 证书验证

- 默认启用证书验证（`QSslConfiguration::defaultConfiguration()`）。
- 用户可在设置中选择"忽略证书错误"（显著警告）。

### 7.2 文件完整性校验

- HTTP/HTTPS：支持 ETag、Content-MD5 校验。
- BT：使用 libtorrent 内置的 Piece Hash 校验（SHA-1）。

### 7.3 恶意种子防护

- 解析种子文件时，检查文件大小是否异常（单个文件 > 1TB）。
- 若检测到异常，弹窗警告用户。

---

## 8. 可扩展性设计 (Extensibility Design)

### 8.1 协议扩展

通过 `IDownloadEngine` 接口，支持新协议扩展：

```cpp
class FTPDownloadEngine : public IDownloadEngine {
    // 实现 FTP 协议下载
};

// 注册到 DownloadEngineFactory
DownloadEngineFactory::registerEngine("ftp", FTPDownloadEngine::create);
```

### 8.2 插件系统（未来）

预留插件接口，支持第三方扩展：
- 浏览器插件集成
- 云同步插件
- 自定义协议插件

---

## 9. 测试策略 (Testing Strategy)

### 9.1 单元测试

- **测试框架**：Qt Test
- **覆盖模块**：
  - `TaskManager`：任务创建、状态流转、并发控制
  - `HTTPDownloadEngine`：分块策略、断点续传、自适应线程
  - `BTDownloadEngine`：种子解析、磁力链接解析
  - `FileManager`：磁盘预分配、完整性校验、文件合并
  - `DatabaseManager`：CRUD 操作、进度节流

### 9.2 集成测试

- **测试场景**：
  - 多任务并发下载（5 HTTP + 3 BT）
  - 网络中断恢复
  - 磁盘空间不足处理
  - 大文件下载（10GB+）

### 9.3 系统测试

- **测试工具**：`Run-Test.ps1`
- **测试用例**：见需求文档验收标准（33 项）

---

## 10. 需求覆盖矩阵 (Requirements Traceability Matrix)

| 需求编号 | 需求描述 | 对应模块 | 对应接口 |
|---------|---------|---------|---------|
| 3.1.1 | 多协议 URL 解析 | TaskManager | `createTask()` |
| 3.2.1 | 多线程分块下载 | HTTPDownloadEngine | `start()`, `ChunkDownloader` |
| 3.2.1 | 自适应线程数调整 | AdaptiveThreadController | `adjustThreads()` |
| 3.2.2 | BT 种子解析 | BTDownloadEngine | `parseTorrent()` |
| 3.2.2 | 磁力链接解析 | BTDownloadEngine | `parseMagnet()` |
| 3.2.3 | 磁盘预分配 | FileManager | `preallocateDiskSpace()` |
| 3.2.3 | 完整性校验 | FileManager | `verifyFileIntegrity()` |
| 3.3.2 | 并发控制 | TaskManager | `startTask()` |
| 3.4.1 | 数据绑定 | TaskListViewModel | `QAbstractListModel` |
| 3.5.1 | 数据持久化 | DatabaseManager | `saveTask()`, `updateTask()` |

---

## 11. 下一阶段

概要设计已完成，下一步请查看：

```
docs/design/xdown-detailed-design-v2.0.md
```

进入**详细设计**，查看类图、接口定义、数据库表结构等详细信息。

---

**文档结束**
