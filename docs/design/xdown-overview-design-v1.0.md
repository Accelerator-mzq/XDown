## 1. 目录结构设计 (Directory Structure)

为了保持项目清晰，v1.0 的文件将按照 MVVM 架构进行物理分层：
```
XDown/
├── src/
│   ├── common/
│   │   └── DownloadTask.h        // 核心数据结构 (新增: 错误信息、速度字段)
│   ├── core/
│   │   ├── DBManager.h/.cpp      // 数据库交互层 (新增: 线程安全、速度更新)
│   │   ├── HttpDownloader.h/.cpp // 单线程下载器 (新增: 64KB缓冲区、断点续传检测、重定向)
│   │   └── DownloadEngine.h/.cpp // 下载调度中心 (新增: 等待队列、节流写入、重复校验)
│   ├── gui/
│   │   └── TaskListModel.h/.cpp  // UI数据绑定层 (新增: 速度显示)
│   └── main.cpp                  // 程序入口
└── resources/
    └── qml/
        ├── Main.qml              // 主界面布局
        ├── TaskDelegate.qml      // 列表项组件
        └── AddTaskDialog.qml     // 新建任务对话框
```

## 2. 核心数据结构 (`src/common/DownloadTask.h`)

定义任务在内存中的基本形态。

**新增字段说明**：
- `errorMessage`: 错误信息 (当 status 为 Error 时使用)
- `createdAt`: 创建时间戳
- `updatedAt`: 更新时间戳
- `speedBytesPerSec`: 当前下载速度 (字节/秒)

``` cpp
// 枚举定义
enum class TaskStatus { Waiting = 0, Downloading, Paused, Finished, Error };

// 实体类
struct DownloadTask {
    QString id;                    // 唯一标识符 (UUID)
    QString url;                   // 下载链接
    QString localPath;              // 本地保存绝对路径
    QString fileName;              // 文件名
    int64 totalBytes;             // 文件总大小
    int64 downloadedBytes;        // 已下载大小
    TaskStatus status;             // 当前状态

    // ========== 新增字段 ==========
    QString errorMessage;          // 错误信息
    int64 createdAt;              // 创建时间戳
    int64 updatedAt;              // 更新时间戳
    int64 speedBytesPerSec;       // 当前下载速度

    // 辅助方法
    double progress() const;                      // 计算进度 0.0-1.0
    static QString formatSize(int64 bytes);       // 格式化文件大小
    static QString formatSpeed(int64 bytesPerSec); // 格式化速度
};
```

---

## 3. 数据库模块 (`src/core/DBManager.h/.cpp`)

负责 SQLite 的初始化及任务的增删改查。

**新增功能**：
- 线程安全设计 (使用 QMutex)
- 速度字段更新
- 删除任务功能
- 重复任务检查

**数据库表结构**：
```sql
CREATE TABLE IF NOT EXISTS downloads (
    id TEXT PRIMARY KEY,
    url TEXT NOT NULL,
    local_path TEXT NOT NULL,
    file_name TEXT NOT NULL,
    total_size INTEGER DEFAULT 0,
    downloaded_size INTEGER DEFAULT 0,
    status INTEGER DEFAULT 0,
    error_message TEXT DEFAULT '',
    created_at INTEGER DEFAULT 0,
    updated_at INTEGER DEFAULT 0,
    speed_bytes_per_sec INTEGER DEFAULT 0
)
```

**核心函数**：
```cpp
class DBManager {
    // 初始化数据库表
    bool initDB(const QString& dbPath = "xdown_data.db");

    // 插入新任务
    bool insertTask(const DownloadTask& task);

    // 更新任务进度
    bool updateTaskProgress(const QString& id, int64 downloaded, int64 total, TaskStatus status);

    // 更新任务状态
    bool updateTaskStatus(const QString& id, TaskStatus status, const QString& errorMessage = "");

    // 更新任务速度
    bool updateTaskSpeed(const QString& id, int64 speed);

    // 加载所有历史任务
    QList<DownloadTask> loadAllTasks();

    // 检查重复任务
    bool hasDuplicateDownloadingTask(const QString& url) const;

    // 删除任务
    bool deleteTask(const QString& id);

    // ========== 新增方法 ==========
    // 根据ID查询单个任务
    DownloadTask getTaskById(const QString& id) const;

    // 清空所有任务
    bool clearAll();
}
```

---

## 4. 核心下载引擎 (`src/core/HttpDownloader.h/.cpp`)

v1.0 实现基于 HTTP/HTTPS 的单线程断点续传下载器。

**核心特性**：
- **内存安全**：使用 64KB 固定缓冲区循环读取，避免 `readAll()` 导致的内存溢出
- **线程安全**：每个下载器运行在独立工作线程中
- **断点续传**：检测 HTTP 206 状态码判断服务器是否支持
- **重定向处理**：支持 301/302/307/308 重定向
- **速度计算**：每秒计算瞬时下载速度

**线程模型优化** (v1.0.1):
- 使用 `requestInterruption()` 实现线程优雅退出
- 使用 `QThread::msleep` 轮询替代阻塞的 `QEventLoop::exec()`
- 解决 Qt Test 框架与工作线程的兼容性问题

**核心函数**：
```cpp
class HttpDownloader : public QObject {
    // 任务信息
    DownloadTask m_task;

    // 开始/继续下载
    void start();

    // 暂停下载
    void pause();

    // 停止下载
    void stop();

signals:
    // 进度更新 (包含速度)
    void progressUpdated(const QString& id, int64 downloaded, int64 total, int64 speed);

    // 状态变化
    void statusChanged(const QString& id, TaskStatus status, const QString& errorMessage = "");

    // 下载完成
    void downloadFinished(const QString& id, const QString& localPath);

private slots:
    void onReadyRead();           // 使用缓冲区循环读取
    void onFinished();            // 检测 206/200 状态码
    void onErrorOccurred(...);    // 错误处理
    void onRedirected(...);       // 处理重定向
    void onSpeedTimer();          // 计算下载速度

private:
    // 使用 64KB 缓冲区循环读取
    static const int BUFFER_SIZE = 65536;

    // 断点续传检测
    bool checkResumeSupport();

    // 磁盘空间检查
    bool checkDiskSpace(int64 requiredBytes) const;
}
```

**关键实现细节**：

1. **断点续传检测**：
```cpp
void HttpDownloader::onFinished() {
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode == 206) {
        // 服务器支持断点续传
        m_supportResume = true;
    } else if (statusCode == 200 && m_task.downloadedBytes > 0) {
        // 服务器不支持断点续传，删除旧文件重新下载
        m_supportResume = false;
        m_task.downloadedBytes = 0;
        m_file->remove();
    }
}
```

2. **内存安全读取**：
```cpp
void HttpDownloader::onReadyRead() {
    while (m_reply->bytesAvailable() > 0) {
        QByteArray buffer = m_reply->read(BUFFER_SIZE);  // 固定缓冲区大小
        if (buffer.isEmpty()) break;

        m_file->write(buffer);
        m_task.downloadedBytes += buffer.size();

        emit progressUpdated(...);
    }
}
```

---

## 5. 调度中心 (`src/core/DownloadEngine.h/.cpp`)

管理所有 `HttpDownloader` 实例，连接底层逻辑与数据库。

**新增功能**：
- **并发控制**：最多 3 个并发下载
- **等待队列**：超过限制的任务进入等待队列
- **节流写入**：每 2 秒批量写入数据库
- **重复校验**：检查相同 URL + 下载中状态

**核心函数**：
```cpp
class DownloadEngine {
    // 活跃下载器映射
    QMap<QString, HttpDownloader*> m_activeDownloaders;

    // 等待队列
    QQueue<DownloadTask> m_waitingQueue;

    // 节流写入
    QMap<QString, QPair<int64, int64>> m_pendingUpdates;
    QTimer* m_flushTimer;  // 2秒定时器

    // 并发控制
    static const int MAX_CONCURRENT_DOWNLOADS = 3;

    // 添加新任务
    QString addNewTask(const QString& url, const QString& localPath) {
        // 1. 检查 URL 格式
        // 2. 检查重复任务
        // 3. 检查磁盘空间
        // 4. 创建任务
        // 5. 存入数据库
        // 6. 如果有可用槽位立即开始，否则加入等待队列
    }

    // 暂停任务
    void pauseTask(const QString& id);

    // 继续任务
    void resumeTask(const QString& id);

    // 删除任务
    void deleteTask(const QString& id, bool deleteFile = false);

    // 处理等待队列
    void processWaitingQueue() {
        while (m_activeDownloaders.size() < MAX_CONCURRENT_DOWNLOADS
               && !m_waitingQueue.isEmpty()) {
            // 取出等待队列中的任务开始下载
        }
    }

    // 节流写入回调
    void onFlushTimer() {
        // 批量写入待更新的任务进度
    }
}
```

---

## 6. UI 数据绑定层 (`src/gui/TaskListModel.h/.cpp`)

将 C++ 的数据列表转换为 QML 可以直接使用的数据源。

**新增角色**：
- `SpeedBytesPerSecRole`: 原始速度值
- `SpeedFormattedRole`: 格式化速度 (如 "1.5 MB/s")
- `TotalSizeFormattedRole`: 格式化总大小
- `DownloadedSizeFormattedRole`: 格式化已下载大小
- `StatusTextRole`: 状态文本

```cpp
class TaskListModel : extends QAbstractListModel {
    enum RoleNames {
        IdRole = Qt::UserRole + 1,
        // ... 其他角色
        SpeedBytesPerSecRole,
        SpeedFormattedRole,
        TotalSizeFormattedRole,
        DownloadedSizeFormattedRole,
        StatusTextRole
    };

    // QML 获取具体单元格数据
    QVariant data(const QModelIndex &index, int role) const {
        // ...
        switch (role) {
            case SpeedFormattedRole:
                return DownloadTask::formatSpeed(task.speedBytesPerSec);
            case StatusTextRole:
                // 返回状态的中文描述
        }
    }

    // 接收底层 Engine 的进度更新
    void updateTaskProgress(const QString& id, int64 downloaded, int64 total, int64 speed) {
        // 更新数据并发射 dataChanged 信号
    }
}
```
