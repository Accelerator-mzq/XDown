## 1. 全局数据结构定义

**文件名：** `src/common/DownloadTask.h`

这是贯穿整个程序的标准数据载体。
```cpp
#ifndef DOWNLOADTASK_H
#define DOWNLOADTASK_H

#include <QString>
#include <QDateTime>

// 任务状态枚举
enum class TaskStatus {
    Waiting = 0,
    Downloading,
    Paused,
    Finished,
    Error
};

// 任务实体结构体
struct DownloadTask {
    QString id;                    // UUID，唯一标识
    QString url;                   // 原始下载链接
    QString localPath;             // 本地存储绝对路径 (包含文件名)
    QString fileName;              // 仅文件名
    qint64 totalBytes = 0;        // 文件总大小 (默认为0，待解析)
    qint64 downloadedBytes = 0;  // 已下载大小
    TaskStatus status = TaskStatus::Waiting;  // 默认状态

    // ========== 新增字段 ==========
    QString errorMessage;          // 错误信息 (当 status 为 Error 时使用)
    qint64 createdAt = 0;         // 创建时间戳 (秒)
    qint64 updatedAt = 0;         // 更新时间戳 (秒)
    qint64 speedBytesPerSec = 0;  // 当前下载速度 (字节/秒)

    // 默认构造函数
    DownloadTask() {
        createdAt = QDateTime::currentSecsSinceEpoch();
        updatedAt = createdAt;
    }

    // 计算下载进度百分比
    double progress() const {
        if (totalBytes > 0) {
            return static_cast<double>(downloadedBytes) / totalBytes;
        }
        return 0.0;
    }

    // 格式化文件大小
    static QString formatSize(qint64 bytes) {
        const double KB = 1024.0;
        const double MB = KB * 1024.0;
        const double GB = MB * 1024.0;

        if (bytes >= GB) {
            return QString::number(bytes / GB, 'f', 2) + " GB";
        } else if (bytes >= MB) {
            return QString::number(bytes / MB, 'f', 2) + " MB";
        } else if (bytes >= KB) {
            return QString::number(bytes / KB, 'f', 2) + " KB";
        } else {
            return QString::number(bytes) + " B";
        }
    }

    // 格式化下载速度
    static QString formatSpeed(qint64 bytesPerSec) {
        return formatSize(bytesPerSec) + "/s";
    }
};

#endif // DOWNLOADTASK_H
```

---

## 2. 数据库访问层 (Database Access Layer)

**文件名：** `src/core/DBManager.h` & `src/core/DBManager.cpp`

负责 SQLite 数据库的连接与原生 SQL 操作。

### 2.1 类声明 (`DBManager.h`)
```cpp
#include <QSqlDatabase>
#include <QList>
#include <QMutex>
#include "common/DownloadTask.h"

class DBManager : public QObject {
    Q_OBJECT
public:
    static DBManager& instance();
    bool initDB(const QString& dbPath = "xdown_data.db");

    bool insertTask(const DownloadTask& task);
    bool updateTaskProgress(const QString& id, qint64 downloadedBytes, qint64 totalBytes, TaskStatus status);
    bool updateTaskStatus(const QString& id, TaskStatus status, const QString& errorMessage = QString());
    bool updateTaskSpeed(const QString& id, qint64 speedBytesPerSec);
    QList<DownloadTask> loadAllTasks();
    bool hasDuplicateDownloadingTask(const QString& url) const;
    bool deleteTask(const QString& id);

signals:
    void taskInserted(const DownloadTask& task);
    void taskProgressUpdated(const QString& id, qint64 downloaded, qint64 total);
    void taskStatusChanged(const QString& id, TaskStatus status);

private:
    explicit DBManager(QObject* parent = nullptr);
    ~DBManager();

    DownloadTask parseQueryToTask(QSqlQuery& query) const;

    QSqlDatabase m_db;
    mutable QMutex m_mutex;  // 线程安全
    bool m_isInitialized;
};
```

### 2.2 数据库表结构
```cpp
// 建表语句 - 新增字段
QString sql = R"(
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
)";
```

---

## 3. 单线程 HTTP 下载器 (Downloader)

**文件名：** `src/core/HttpDownloader.h` & `src/core/HttpDownloader.cpp`

这是 v1.0 的心脏，负责网络 I/O 和文件 I/O。

### 3.1 核心特性

1. **内存安全**：使用 64KB 固定缓冲区循环读取
2. **线程安全**：每个下载器运行在独立工作线程
3. **断点续传**：检测 HTTP 206 状态码
4. **重定向处理**：支持 301/302/307/308
5. **速度计算**：每秒计算瞬时速度

### 3.2 类声明 (`HttpDownloader.h`)
```cpp
class HttpDownloader : public QObject {
    Q_OBJECT
public:
    explicit HttpDownloader(const DownloadTask& task, QObject* parent = nullptr);
    ~HttpDownloader();

    void start();
    void pause();
    void stop();
    DownloadTask getTaskInfo() const { return m_task; }
    QString getTaskId() const { return m_task.id; }

signals:
    void progressUpdated(const QString& id, qint64 downloadedBytes, qint64 totalBytes, qint64 speedBytesPerSec);
    void statusChanged(const QString& id, TaskStatus status, const QString& errorMessage = QString());
    void downloadFinished(const QString& id, const QString& localPath);

private slots:
    void onReadyRead();
    void onFinished();
    void onErrorOccurred(QNetworkReply::NetworkError code);
    void onRedirected(const QUrl& url);
    void onSpeedTimer();

private:
    void initNetworkManager();
    bool checkResumeSupport();
    bool checkDiskSpace(qint64 requiredBytes) const;
    void parseFileName(const QUrl& redirectUrl = QUrl());
    void updateSpeedStatistic();

    DownloadTask m_task;
    QThread* m_thread;
    QNetworkAccessManager* m_netManager;
    QNetworkReply* m_reply;
    QFile* m_file;

    qint64 m_lastDownloadedBytes;
    qint64 m_lastSpeedTime;
    qint64 m_currentSpeed;
    bool m_isRedirecting;
    bool m_supportResume;
    int m_redirectCount;
    QTimer* m_speedTimer;

    static const int BUFFER_SIZE = 65536;     // 64KB 缓冲区
    static const int MAX_REDIRECTS = 10;
};
```

### 3.3 关键实现

**3.3.1 内存安全读取**：
```cpp
void HttpDownloader::onReadyRead() {
    if (!m_reply || !m_file) return;

    // 使用固定缓冲区循环读取，避免内存溢出
    while (m_reply->bytesAvailable() > 0) {
        QByteArray buffer = m_reply->read(BUFFER_SIZE);
        if (buffer.isEmpty()) break;

        m_file->write(buffer);
        m_task.downloadedBytes += buffer.size();

        // 首次获取文件总大小
        if (m_task.totalBytes == 0 && m_reply->hasRawHeader("Content-Length")) {
            qint64 contentLength = m_reply->rawHeader("Content-Length").toLongLong();
            if (m_task.downloadedBytes > 0 && !m_isRedirecting) {
                m_task.totalBytes = contentLength + m_task.downloadedBytes;
            } else {
                m_task.totalBytes = contentLength;
            }
        }

        if (!m_isRedirecting) {
            emit progressUpdated(m_task.id, m_task.downloadedBytes,
                               m_task.totalBytes, m_currentSpeed);
        }
    }

    if (m_file) m_file->flush();
}
```

**3.3.2 断点续传检测**：
```cpp
void HttpDownloader::onFinished() {
    if (!m_reply) return;

    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode == 206) {
        // 206 Partial Content - 服务器支持断点续传
        m_supportResume = true;
    } else if (statusCode == 200 && m_task.downloadedBytes > 0 && !m_isRedirecting) {
        // 200 OK - 服务器不支持断点续传
        m_supportResume = false;
        m_task.downloadedBytes = 0;

        if (m_file && m_file->isOpen()) {
            m_file->close();
            m_file->remove();
            m_file->open(QIODevice::WriteOnly | QIODevice::Truncate);
        }
    }

    // ... 其他处理
}
```

**3.3.3 速度计算**：
```cpp
void HttpDownloader::updateSpeedStatistic() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDiff = now - m_lastSpeedTime;

    if (timeDiff > 0) {
        qint64 bytesDiff = m_task.downloadedBytes - m_lastDownloadedBytes;
        m_currentSpeed = static_cast<qint64>((bytesDiff * 1000.0) / timeDiff);
        m_task.speedBytesPerSec = m_currentSpeed;
    }

    m_lastDownloadedBytes = m_task.downloadedBytes;
    m_lastSpeedTime = now;

    emit progressUpdated(m_task.id, m_task.downloadedBytes,
                       m_task.totalBytes, m_currentSpeed);
}
```

---

## 4. 下载调度中心 (Download Engine)

**文件名：** `src/core/DownloadEngine.h` & `src/core/DownloadEngine.cpp`

管理所有的 `HttpDownloader` 实例，处理与数据库的联动。

### 4.1 核心功能

1. **并发控制**：最多 3 个并发下载
2. **等待队列**：超过限制的任务进入等待队列
3. **节流写入**：每 2 秒批量写入数据库
4. **重复校验**：检查相同 URL + 下载中状态
5. **磁盘空间检查**：下载前检查剩余空间

### 4.2 关键实现

**4.2.1 添加任务**：
```cpp
QString DownloadEngine::addNewTask(const QString& url, const QString& localPath) {
    // 1. 检查 URL 格式
    if (!isValidUrl(url)) {
        return "无效的 URL 格式";
    }

    // 2. 检查重复任务
    if (checkDuplicateTask(url)) {
        return "该 URL 正在下载中";
    }

    // 3. 检查磁盘空间
    if (!checkDiskSpace(savePath)) {
        return "磁盘空间不足";
    }

    // 4. 创建任务并加入队列或立即开始
    if (m_activeDownloaders.size() < MAX_CONCURRENT_DOWNLOADS) {
        enqueueWaitingTask(newTask);
    } else {
        m_waitingQueue.enqueue(newTask);
    }
}
```

**4.2.2 处理等待队列**：
```cpp
void DownloadEngine::processWaitingQueue() {
    while (m_activeDownloaders.size() < MAX_CONCURRENT_DOWNLOADS
           && !m_waitingQueue.isEmpty()) {
        DownloadTask task = m_waitingQueue.dequeue();
        enqueueWaitingTask(task);
    }
}
```

**4.2.3 节流写入**：
```cpp
void DownloadEngine::saveTaskProgressThrottled(const QString& id, qint64 downloaded,
                                               qint64 total, qint64 speed) {
    // 保存到待写入队列
    m_pendingUpdates.insert(id, qMakePair(downloaded, total));

    // 速度实时写入
    DBManager::instance().updateTaskSpeed(id, speed);
}

// 定时批量写入
void DownloadEngine::onFlushTimer() {
    for (auto it = m_pendingUpdates.begin(); it != m_pendingUpdates.end(); ++it) {
        const QString& id = it.key();
        qint64 downloaded = it.value().first;
        qint64 total = it.value().second;

        auto taskIt = m_tasks.find(id);
        if (taskIt != m_tasks.end()) {
            DBManager::instance().updateTaskProgress(id, downloaded, total,
                                                    taskIt->status);
        }
    }
    m_pendingUpdates.clear();
}
```

---

## 5. UI 数据绑定模型 (ViewModel)

**文件名：** `src/gui/TaskListModel.h` & `src/gui/TaskListModel.cpp`

将 C++ 的 `QList<DownloadTask>` 暴露给 QML 的 `ListView`。

### 5.1 角色定义
```cpp
enum RoleNames {
    IdRole = Qt::UserRole + 1,
    UrlRole,
    LocalPathRole,
    FileNameRole,
    TotalBytesRole,
    DownloadedBytesRole,
    ProgressRole,
    StatusRole,
    ErrorMessageRole,
    SpeedBytesPerSecRole,
    SpeedFormattedRole,         // 新增: 格式化速度 "1.5 MB/s"
    TotalSizeFormattedRole,     // 新增: 格式化总大小
    DownloadedSizeFormattedRole, // 新增: 格式化已下载
    StatusTextRole              // 新增: 状态中文文本
};
```

### 5.2 数据获取
```cpp
QVariant TaskListModel::data(const QModelIndex &index, int role) const {
    // ...

    switch (role) {
        case SpeedFormattedRole:
            return DownloadTask::formatSpeed(task.speedBytesPerSec);
        case TotalSizeFormattedRole:
            return DownloadTask::formatSize(task.totalBytes);
        case DownloadedSizeFormattedRole:
            return DownloadTask::formatSize(task.downloadedBytes);
        case StatusTextRole:
            switch (task.status) {
                case TaskStatus::Waiting: return "等待中";
                case TaskStatus::Downloading: return "下载中";
                case TaskStatus::Paused: return "已暂停";
                case TaskStatus::Finished: return "已完成";
                case TaskStatus::Error: return "错误: " + task.errorMessage;
            }
        // ...
    }
}
```
