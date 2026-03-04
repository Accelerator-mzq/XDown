# XDown v1.0 MVP 代码审查报告

**审查日期**: 2026-03-04  
**审查版本**: main 分支 (7 commits)  
**审查范围**: 全部源代码 vs 产品规格说明书

---

## 一、需求完成度分析

### 1.1 任务创建模块 (产品规格 §3.1)

| 需求项 | 状态 | 说明 |
|--------|------|------|
| URL 解析 (http/https) | ✅ 已实现 | `isValidUrl()` 通过 `QUrl::scheme()` 检查 |
| 路径选择 + 默认路径 | ✅ 已实现 | `getDefaultSavePath()` + QML `FolderDialog` |
| 重复性校验 | ⚠️ 部分实现 | 仅检查 `Downloading` 状态，`Waiting` 和 `Paused` 的同URL任务未拦截 |
| 文件名自动解析 | ⚠️ 部分实现 | 从 URL 解析了文件名，但未处理 `Content-Disposition` 响应头 |

### 1.2 核心下载引擎 (产品规格 §3.2)

| 需求项 | 状态 | 说明 |
|--------|------|------|
| 单线程传输 | ✅ 已实现 | `QNetworkAccessManager` + 独立工作线程 |
| 64KB 缓冲区循环读取 | ✅ 已实现 | `BUFFER_SIZE = 65536`，`onReadyRead()` 循环读写 |
| 断点续传 (Range/206) | ✅ 已实现 | `Range` 请求头 + `onFinished()` 检测 206 |
| HTTP 重定向 (301/302/307/308) | ⚠️ 有Bug | 见下方 P0-1 和 P0-2 |
| 错误处理 (404/403/超时) | ✅ 已实现 | `onErrorOccurred()` 覆盖主要错误码 |
| 磁盘空间检查 | ✅ 已实现 | `checkDiskSpace()` 保留 100MB 缓冲 |

### 1.3 任务控制与状态流转 (产品规格 §3.3)

| 需求项 | 状态 | 说明 |
|--------|------|------|
| 5种状态定义 | ✅ 已实现 | `TaskStatus` 枚举完整 |
| 并发控制 (最多3个) | ✅ 已实现 | `MAX_CONCURRENT_DOWNLOADS = 3` |
| 开始/继续 | ✅ 已实现 | `resumeTask()` |
| 暂停 | ⚠️ 有缺陷 | 功能正常但内存状态未同步 (见 P1-2) |
| 删除 + 二次确认 | ✅ 已实现 | QML `deleteConfirmDialog` |

### 1.4 任务列表展示 (产品规格 §3.4)

| 需求项 | 状态 | 说明 |
|--------|------|------|
| QAbstractListModel 绑定 | ✅ 已实现 | 完整的角色映射 |
| 文件名/速度/大小/进度条 | ✅ 已实现 | 格式化函数完备 |
| 操作按钮 | ✅ 已实现 | 按状态动态显示 |
| Tab过滤 (下载中/已完成) | ⚠️ 有缺陷 | 使用 `visible` 属性隐藏项，不改高度导致空白间距 |

### 1.5 数据持久化 (产品规格 §3.5)

| 需求项 | 状态 | 说明 |
|--------|------|------|
| 状态变化立即写入 | ✅ 已实现 | `updateTaskStatus` 直接调用 |
| 进度节流 (2秒) | ⚠️ 部分失效 | 进度节流了但速度写入仍逐次触发 (见 P1-4) |
| 重启恢复 | ✅ 已实现 | `initialize()` 加载历史并恢复状态 |

### 1.6 非功能需求 (产品规格 §4)

| 需求项 | 状态 | 说明 |
|--------|------|------|
| 线程隔离 (UI/Worker) | ⚠️ 有风险 | 线程模型存在多处安全隐患 (见 P0-3, P0-4) |
| 内存 ≤50MB | ✅ 已实现 | 64KB 缓冲区设计合理 |
| 刷新率 1-2次/秒 | ✅ 已实现 | 速度定时器 1000ms 间隔 |

---

## 二、Bug与风险清单

### 🔴 P0 — 必须在发布前修复 (会导致崩溃/数据损坏)

#### P0-1: `onFinished` 被连接两次，导致双重触发

**文件**: `HttpDownloader.cpp`  
**位置**: `initNetworkManager()` 第76行 + `start()` 第154行

```
initNetworkManager():
  connect(m_netManager, &QNetworkAccessManager::finished, this, &HttpDownloader::onFinished);

start():
  connect(m_reply, &QNetworkReply::finished, this, &HttpDownloader::onFinished);
```

`QNetworkAccessManager::finished` 和 `QNetworkReply::finished` 对同一请求都会触发。`onFinished()` 被调用两次会导致：第二次调用时 `m_reply` 已经 `deleteLater`，访问悬空指针引发崩溃；状态信号被重复发射，`DownloadEngine` 中出现逻辑混乱。

**修复**: 删除 `initNetworkManager()` 中的 `connect(m_netManager, &QNetworkAccessManager::finished, ...)` 这行。

---

#### P0-2: 重定向时信号被累积连接

**文件**: `HttpDownloader.cpp`  
**位置**: `onRedirected()` 第388行调用 `start()`

每次重定向都调用 `start()`，`start()` 内部会重新 `connect` 四个信号槽。但旧连接不会自动断开，N次重定向后 `onReadyRead` 会被调用 N+1 次，写入重复数据导致文件损坏。

**修复**: 在 `start()` 开头添加信号断开逻辑，或者将重定向处理提取为独立方法，避免重复连接：

```cpp
// 在 start() 中 connect 之前：
if (m_reply) {
    m_reply->disconnect(this);
}
```

---

#### P0-3: 析构函数跨线程删除对象

**文件**: `HttpDownloader.cpp`  
**位置**: 析构函数第60-61行

```cpp
HttpDownloader::~HttpDownloader() {
    stop();
    // ...
    delete m_file;         // ← m_file 在工作线程创建
    delete m_netManager;   // ← m_netManager 在工作线程创建
}
```

`HttpDownloader` 的析构函数在主线程（`DownloadEngine` 所在线程）执行，但 `m_file` 和 `m_netManager` 是在工作线程中创建的。跨线程 `delete` Qt 对象违反 Qt 线程安全规则，可能导致随机崩溃。

**修复**: 通过 `QMetaObject::invokeMethod` 在工作线程中清理资源，或将清理逻辑放到 `stop()` 中以 `BlockingQueuedConnection` 执行。

---

#### P0-4: 工作线程中调用 `QCoreApplication::processEvents()` 不安全

**文件**: `HttpDownloader.cpp`  
**位置**: `initNetworkManager()` 第83-86行

```cpp
while (!m_thread->isInterruptionRequested()) {
    QThread::msleep(100);
    QCoreApplication::processEvents();  // ← 处理主线程事件队列
}
```

在工作线程中调用 `QCoreApplication::processEvents()` 会处理全局事件队列，可能导致 GUI 代码在工作线程中执行。

**修复**: 删除这个循环，改为重写 `QThread::run()` 中调用 `exec()`，或者让 `initNetworkManager()` 只做初始化不做事件循环（因为 `QThread::start()` 已经有默认的 `exec()`）。实际上整个 `initNetworkManager()` 的事件循环设计都需要重构：

```cpp
void HttpDownloader::initNetworkManager() {
    m_netManager = new QNetworkAccessManager(this);
    m_speedTimer = new QTimer(this);
    m_speedTimer->setInterval(1000);
    connect(m_speedTimer, &QTimer::timeout, this, &HttpDownloader::onSpeedTimer);
    // 不需要手动事件循环，QThread::exec() 会自动运行
}
```

同时 `QThread` 应该改为子类化方式或使用默认的 `exec()` 事件循环。

---

### 🟡 P1 — 强烈建议修复 (功能缺陷/数据不一致)

#### P1-1: 下载完成/错误时信号重复处理

**文件**: `DownloadEngine.cpp`  
**位置**: `onDownloaderStatusChanged()` 和 `onDownloadFinished()`

下载完成时 `HttpDownloader` 发射两个信号：`statusChanged(Finished)` 和 `downloadFinished`。`DownloadEngine` 的两个槽函数都尝试从 `m_activeDownloaders` 中移除下载器：
- `onDownloaderStatusChanged` 在第278-281行调用 `m_activeDownloaders.remove(id)` + `processWaitingQueue()`
- `onDownloadFinished` 在第298行调用 `m_activeDownloaders.take(id)` + `processWaitingQueue()`

第二次移除操作虽然不会崩溃（QMap::take 对不存在的 key 返回 nullptr），但 `processWaitingQueue()` 会被调用两次，可能导致等待队列中的任务被提前启动两个。

**修复**: 在 `onDownloadFinished` 中删除重复的移除和队列处理逻辑，只做转发信号。

---

#### P1-2: `pauseTask` 未更新内存中的任务状态

**文件**: `DownloadEngine.cpp`  
**位置**: `pauseTask()` 第150-162行

```cpp
void DownloadEngine::pauseTask(const QString& id) {
    auto* downloader = m_activeDownloaders.value(id);
    if (downloader) {
        downloader->pause();
        m_activeDownloaders.remove(id);
        DBManager::instance().updateTaskStatus(id, TaskStatus::Paused);
        // ← 缺少: m_tasks[id].status = TaskStatus::Paused
        processWaitingQueue();
    }
}
```

数据库更新了但 `m_tasks` 内存缓存未更新。后续基于 `m_tasks` 的判断（如重复检查、`resumeTask`）可能读到过期的状态值。

**修复**: 添加 `m_tasks[id].status = TaskStatus::Paused;`

---

#### P1-3: 断点续传 200 响应处理不完整

**文件**: `HttpDownloader.cpp`  
**位置**: `onFinished()` 第282-287行

产品规格要求：若服务器返回 200（不支持续传），应删除已下载部分重新开始。当前代码只是设置了 `m_supportResume = false` 并打印警告，没有执行删除和重新下载操作。

```cpp
} else if (statusCode == 200 && m_task.downloadedBytes > 0 && !m_isRedirecting) {
    qWarning() << "Server does not support resume, but download completed";
    m_supportResume = false;
    // ← 缺少: 删除文件、重置 downloadedBytes、重新发起请求
}
```

**修复**: 检测到 200 且有 Range 请求时，应关闭文件、truncate、从头重新下载。

---

#### P1-4: 速度写入数据库未节流

**文件**: `DownloadEngine.cpp`  
**位置**: `saveTaskProgressThrottled()` 第418行

```cpp
void DownloadEngine::saveTaskProgressThrottled(const QString& id, ...) {
    m_pendingUpdates.insert(id, qMakePair(downloaded, total));  // 节流 ✓
    DBManager::instance().updateTaskSpeed(id, speed);            // 每次都写 ✗
}
```

进度节流了（2秒批量写），但 `updateTaskSpeed` 在每次进度回调（约每秒1次，但 `onReadyRead` 每次数据到达都触发 `progressUpdated`）都直接写数据库。高速下载时可能每秒写入数十次，与节流设计初衷矛盾。

**修复**: 将速度也放入 `m_pendingUpdates` 中，或者在 `onFlushTimer` 中一并写入。

---

#### P1-5: `onReadyRead` 中 Content-Length 解析时机有误

**文件**: `HttpDownloader.cpp`  
**位置**: `onReadyRead()` 第229-239行

`totalBytes` 的解析在 `while` 循环内部，但 `m_task.downloadedBytes` 已经被前面的 `+=` 增加。当断点续传时，计算 `totalBytes = contentLength + m_task.downloadedBytes` 中的 `downloadedBytes` 已包含本次读取的数据量，导致总大小偏大。

此外，Content-Length 应该在首次收到响应头时解析（如 `onMetaDataChanged` 或首次 `onReadyRead`），而非每次数据到达都尝试。

**修复**: 将 Content-Length 解析移到循环外，并在 `start()` 中记录初始 `downloadedBytes`。

---

### 🟢 P2 — 后续迭代修复 (优化/增强)

#### P2-1: QML TaskDelegate 过滤方式低效

`TaskDelegate` 使用 `visible` 属性隐藏不属于当前 Tab 的任务，但 `height` 固定为 80。隐藏的项仍占据布局空间，导致列表中出现空白区域。同时 ListView 仍为所有项创建 delegate，任务量大时性能下降。

**修复**: 设置 `height: visible ? 80 : 0`，或者更好的方案是使用 `DelegateModel`/`SortFilterProxyModel` 在模型层过滤。

---

#### P2-2: 重复任务校验范围过窄

`checkDuplicateTask` 只检查 `Downloading` 状态。`Waiting` 状态的任务也应被视为"处理中"。当前实现允许用户添加多个相同 URL 的等待任务。

---

#### P2-3: Content-Disposition 文件名解析缺失

产品规格提到"从 URL 中自动提取文件名"，代码中也有注释提及需要处理 `Content-Disposition`，但未实现。对于 CDN 下载链接（URL 中无文件名），用户看到的是 UUID 式的默认文件名。

---

#### P2-4: DBManager 数据库连接名缺失

`DBManager::initDB()` 使用 `QSqlDatabase::addDatabase("QSQLITE")`（默认连接名）。如果测试和主程序在同一进程运行，会产生连接名冲突。建议使用命名连接：

```cpp
m_db = QSqlDatabase::addDatabase("QSQLITE", "xdown_main");
```

---

#### P2-5: DBManager::m_workerThread 声明但从未使用

`DBManager.h` 中声明了 `QThread* m_workerThread`，析构函数中有 `quit/wait/delete` 逻辑，但该线程从未被创建或启动。属于死代码，应清理。

---

#### P2-6: m_eventLoop 声明但从未使用

`HttpDownloader.h` 中声明了 `QEventLoop* m_eventLoop`，但在 `.cpp` 中从未使用。

---

#### P2-7: main.cpp 中 taskStatusChanged 信号被连接两次

`main.cpp` 无头测试模式中（第117行和第133行），`taskStatusChanged` 信号被连接了两个 lambda。两个 lambda 都会被执行，第一个打印状态变化，第二个处理错误退出。虽然功能上没问题，但建议合并为一个连接。

---

## 三、验收标准对照

| # | 验收条件 | 状态 | 风险 |
|---|---------|------|------|
| 1 | 编译启动无崩溃 | ⚠️ | P0-3/P0-4 线程安全问题可能导致偶发崩溃 |
| 2 | 有效链接正常下载 | ✅ | 基本路径可用 |
| 3 | 进度/速度平滑更新 | ✅ | 1秒定时器 + dataChanged 信号 |
| 4 | 暂停/继续断点续传 | ⚠️ | P1-3: 不支持续传的服务器处理不完整 |
| 5 | 重启恢复记录 | ✅ | 数据库持久化正常 |
| 6 | 文件完整无损坏 | ⚠️ | P0-2: 重定向时可能写入重复数据 |
| 7 | 重复URL提示 | ⚠️ | P2-2: Waiting状态未纳入检查 |
| 8 | 并发超3个进入等待 | ✅ | 队列机制正常 |

---

## 四、修复优先级总览

| 优先级 | ID | 问题 | 影响 |
|--------|-----|------|------|
| **P0** | P0-1 | onFinished 双重连接 | 崩溃 (悬空指针) |
| **P0** | P0-2 | 重定向信号累积 | 文件损坏 |
| **P0** | P0-3 | 析构函数跨线程删除 | 随机崩溃 |
| **P0** | P0-4 | processEvents 不安全 | 线程安全 |
| **P1** | P1-1 | 完成信号重复处理 | 队列逻辑异常 |
| **P1** | P1-2 | pauseTask 内存未更新 | 状态不一致 |
| **P1** | P1-3 | 200响应续传处理缺失 | 规格不符 |
| **P1** | P1-4 | 速度写入未节流 | 性能浪费 |
| **P1** | P1-5 | Content-Length 解析时机 | 进度显示偏差 |
| **P2** | P2-1~7 | 优化项 | 体验/代码质量 |

---

## 五、总评

XDown v1.0 的功能框架已经搭建完成，核心下载流程（添加→下载→暂停→继续→完成）的基本路径可用，数据持久化、并发控制、UI绑定等模块实现完整度较高。

**主要风险集中在线程模型和信号连接两个方面**:
- `HttpDownloader` 的线程生命周期管理（创建、事件循环、销毁）需要重构
- 信号的双重连接和重定向时的累积连接是最紧迫的崩溃级Bug

建议修复路径：先解决4个P0问题（预计1-2天工作量），再处理P1问题，即可达到 v1.0 发布标准。
