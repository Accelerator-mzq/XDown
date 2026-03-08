# XDown v2.0 详细设计文档 (Detailed Design Document)

**版本号**：v2.0
**文档状态**：Draft
**编写日期**：2026-03-08
**基于文档**：xdown-srs-v2.0.md, xdown-overview-design-v2.0.md

---

## 1. 类设计 (Class Design)

### 1.1 核心类图

```
┌─────────────────────────────────────────────────────────────┐
│                      IDownloadEngine                         │
│  ┌────────────────────────────────────────────────────┐    │
│  │ + start(): void                                     │    │
│  │ + pause(): void                                     │    │
│  │ + resume(): void                                    │    │
│  │ + getDownloadedBytes(): qint64                      │    │
│  │ + getTotalBytes(): qint64                           │    │
│  │ + getSpeed(): double                                │    │
│  │ signals:                                            │    │
│  │   - progressUpdated(qint64, qint64)                 │    │
│  │   - speedUpdated(double)                            │    │
│  │   - finished()                                      │    │
│  │   - error(QString)                                  │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            △
                            │
            ┌───────────────┴───────────────┐
            │                               │
┌───────────┴──────────┐       ┌───────────┴──────────┐
│ HTTPDownloadEngine   │       │  BTDownloadEngine    │
│  ┌────────────────┐  │       │  ┌────────────────┐  │
│  │ - chunks       │  │       │  │ - session      │  │
│  │ - threadPool   │  │       │  │ - torrentHandle│  │
│  │ - controller   │  │       │  │ - seedingCtrl  │  │
│  └────────────────┘  │       │  └────────────────┘  │
└──────────────────────┘       └──────────────────────┘
```

---

## 2. 接口定义 (Interface Definitions)

### 2.1 IDownloadEngine（下载引擎接口）

**文件路径**：`src/core/download/IDownloadEngine.h`

```cpp
/**
 * @brief 下载引擎抽象接口
 *
 * 所有下载协议（HTTP/HTTPS/BT/Magnet）必须实现此接口。
 *
 * [需单元测试]
 */
class IDownloadEngine : public QObject {
    Q_OBJECT

public:
    explicit IDownloadEngine(QObject *parent = nullptr);
    virtual ~IDownloadEngine() = default;

    /**
     * @brief 启动下载
     * @note 异步操作，通过信号通知进度
     * [需单元测试]
     */
    virtual void start() = 0;

    /**
     * @brief 暂停下载
     * @note 保存当前进度到数据库
     * [需单元测试]
     */
    virtual void pause() = 0;

    /**
     * @brief 恢复下载
     * @note 从数据库读取进度并继续
     * [需单元测试]
     */
    virtual void resume() = 0;

    /**
     * @brief 获取已下载字节数
     * @return 已下载字节数
     * [需单元测试]
     */
    virtual qint64 getDownloadedBytes() const = 0;

    /**
     * @brief 获取文件总大小
     * @return 文件总大小（字节）
     * [需单元测试]
     */
    virtual qint64 getTotalBytes() const = 0;

    /**
     * @brief 获取当前下载速度
     * @return 下载速度（字节/秒）
     * [需单元测试]
     */
    virtual double getSpeed() const = 0;

signals:
    /**
     * @brief 进度更新信号
     * @param downloaded 已下载字节数
     * @param total 文件总大小
     */
    void progressUpdated(qint64 downloaded, qint64 total);

    /**
     * @brief 速度更新信号
     * @param speed 当前速度（字节/秒）
     */
    void speedUpdated(double speed);

    /**
     * @brief 下载完成信号
     */
    void finished();

    /**
     * @brief 错误信号
     * @param errorMsg 错误信息
     */
    void error(const QString &errorMsg);
};
```

---

### 2.2 TaskManager（任务管理器）

**文件路径**：`src/core/task/TaskManager.h`

```cpp
/**
 * @brief 任务管理器
 *
 * 负责任务的创建、启动、暂停、删除，以及并发控制。
 * 单例模式，全局唯一实例。
 *
 * [需单元测试]
 */
class TaskManager : public QObject {
    Q_OBJECT

public:
    static TaskManager* instance();

    /**
     * @brief 创建新任务
     * @param url 下载链接（HTTP/HTTPS/Magnet/Torrent）
     * @param savePath 保存路径
     * @param options 高级选项（线程数、分块大小等）
     * @return 任务 ID，若创建失败返回空字符串
     * [需单元测试]
     */
    QString createTask(const QString &url,
                       const QString &savePath,
                       const QVariantMap &options);

    /**
     * @brief 启动任务
     * @param taskId 任务 ID
     * @return 是否成功启动
     * @note 若超过并发限制，任务进入等待队列
     * [需单元测试]
     */
    bool startTask(const QString &taskId);

    /**
     * @brief 暂停任务
     * @param taskId 任务 ID
     * @return 是否成功暂停
     * [需单元测试]
     */
    bool pauseTask(const QString &taskId);

    /**
     * @brief 删除任务
     * @param taskId 任务 ID
     * @param deleteFile 是否同时删除本地文件
     * @return 是否成功删除
     * [需单元测试]
     */
    bool deleteTask(const QString &taskId, bool deleteFile);

    /**
     * @brief 启动所有任务
     * @note 受并发限制约束
     * [需单元测试]
     */
    void startAll();

    /**
     * @brief 暂停所有任务
     * [需单元测试]
     */
    void pauseAll();

    /**
     * @brief 获取任务列表
     * @param filter 筛选条件（Downloading/Completed/BT/Trash）
     * @return 任务列表
     * [需单元测试]
     */
    QList<DownloadTask> getTaskList(const QString &filter = "All");

signals:
    /**
     * @brief 任务创建信号
     * @param taskId 任务 ID
     */
    void taskCreated(const QString &taskId);

    /**
     * @brief 任务状态变更信号
     * @param taskId 任务 ID
     * @param newState 新状态
     */
    void taskStateChanged(const QString &taskId, TaskState newState);

    /**
     * @brief 任务进度更新信号
     * @param taskId 任务 ID
     * @param progress 进度（0.0-1.0）
     * @param speed 速度（字节/秒）
     */
    void taskProgressUpdated(const QString &taskId, double progress, double speed);

private:
    explicit TaskManager(QObject *parent = nullptr);
    ~TaskManager();

    // 并发控制
    void checkAndStartWaitingTasks();
    int getRunningHttpTaskCount() const;
    int getRunningBtTaskCount() const;

    // 成员变量
    QMap<QString, DownloadTask*> m_tasks;  // 任务映射表
    QQueue<QString> m_waitingQueue;        // 等待队列
    DatabaseManager *m_dbManager;          // 数据库管理器
    QMutex m_mutex;                        // 线程安全锁
};
```

---

### 2.3 HTTPDownloadEngine（HTTP 下载引擎）

**文件路径**：`src/core/download/HTTPDownloadEngine.h`

```cpp
/**
 * @brief HTTP/HTTPS 多线程分块下载引擎
 *
 * 支持：
 * - Range 请求分块下载
 * - 自适应线程数调整
 * - 断点续传
 * - 文件合并
 *
 * [需单元测试]
 */
class HTTPDownloadEngine : public IDownloadEngine {
    Q_OBJECT

public:
    explicit HTTPDownloadEngine(const QString &url,
                                 const QString &savePath,
                                 int threadCount = 8,
                                 QObject *parent = nullptr);
    ~HTTPDownloadEngine() override;

    // 实现 IDownloadEngine 接口
    void start() override;
    void pause() override;
    void resume() override;
    qint64 getDownloadedBytes() const override;
    qint64 getTotalBytes() const override;
    double getSpeed() const override;

private slots:
    /**
     * @brief 处理 HEAD 请求响应
     * @param reply 网络响应
     * [需单元测试]
     */
    void onHeadRequestFinished(QNetworkReply *reply);

    /**
     * @brief 处理分块下载完成
     * @param chunkIndex 分块索引
     * [需单元测试]
     */
    void onChunkFinished(int chunkIndex);

    /**
     * @brief 处理分块下载错误
     * @param chunkIndex 分块索引
     * @param errorMsg 错误信息
     * [需单元测试]
     */
    void onChunkError(int chunkIndex, const QString &errorMsg);

private:
    /**
     * @brief 检测服务器是否支持 Range 请求
     * [需单元测试]
     */
    void detectRangeSupport();

    /**
     * @brief 计算分块策略
     * @param fileSize 文件总大小
     * @param threadCount 线程数
     * @return 分块列表
     * [需单元测试]
     */
    QList<ChunkInfo> calculateChunks(qint64 fileSize, int threadCount);

    /**
     * @brief 启动分块下载
     * [需单元测试]
     */
    void startChunkDownloads();

    /**
     * @brief 合并分块文件
     * [需单元测试]
     */
    void mergeChunks();

    /**
     * @brief 完整性校验
     * [需单元测试]
     */
    void verifyIntegrity();

    // 成员变量
    QString m_url;                          // 下载链接
    QString m_savePath;                     // 保存路径
    int m_threadCount;                      // 线程数
    qint64 m_totalBytes;                    // 文件总大小
    qint64 m_downloadedBytes;               // 已下载字节数
    bool m_supportsRange;                   // 是否支持 Range 请求
    QList<ChunkDownloader*> m_chunkDownloaders;  // 分块下载器列表
    AdaptiveThreadController *m_controller;      // 自适应线程控制器
    QNetworkAccessManager *m_networkManager;     // 网络管理器
};
```

---

### 2.4 ChunkDownloader（分块下载器）

**文件路径**：`src/core/download/ChunkDownloader.h`

```cpp
/**
 * @brief 单个分块下载器
 *
 * 负责下载文件的一个分块，运行在独立线程中。
 *
 * [需单元测试]
 */
class ChunkDownloader : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param url 下载链接
     * @param chunkInfo 分块信息（起始位置、结束位置）
     * @param tempFilePath 临时文件路径
     * @param parent 父对象
     */
    explicit ChunkDownloader(const QString &url,
                              const ChunkInfo &chunkInfo,
                              const QString &tempFilePath,
                              QObject *parent = nullptr);
    ~ChunkDownloader();

    /**
     * @brief 启动下载
     * [需单元测试]
     */
    void start();

    /**
     * @brief 暂停下载
     * [需单元测试]
     */
    void pause();

    /**
     * @brief 获取已下载字节数
     * @return 已下载字节数
     * [需单元测试]
     */
    qint64 getDownloadedBytes() const;

signals:
    /**
     * @brief 进度更新信号
     * @param chunkIndex 分块索引
     * @param downloaded 已下载字节数
     */
    void progressUpdated(int chunkIndex, qint64 downloaded);

    /**
     * @brief 下载完成信号
     * @param chunkIndex 分块索引
     */
    void finished(int chunkIndex);

    /**
     * @brief 错误信号
     * @param chunkIndex 分块索引
     * @param errorMsg 错误信息
     */
    void error(int chunkIndex, const QString &errorMsg);

private slots:
    /**
     * @brief 处理网络响应数据
     */
    void onReadyRead();

    /**
     * @brief 处理下载完成
     */
    void onFinished();

private:
    QString m_url;                      // 下载链接
    ChunkInfo m_chunkInfo;              // 分块信息
    QString m_tempFilePath;             // 临时文件路径
    qint64 m_downloadedBytes;           // 已下载字节数
    QNetworkAccessManager *m_networkManager;  // 网络管理器
    QNetworkReply *m_reply;             // 网络响应
    QFile *m_file;                      // 临时文件
    int m_retryCount;                   // 重试次数
};
```

---


## 5. 数据库表设计补充说明

### 5.1 数据库索引策略

为了优化查询性能，需要在以下字段上创建索引：
- `tasks.task_state`: 用于按状态筛选任务
- `tasks.task_type`: 用于按类型筛选任务
- `tasks.create_time`: 用于按创建时间排序
- `chunks.task_id`: 用于快速查询任务的所有分块
- `trash.delete_time`: 用于自动清理过期任务

### 5.2 数据库事务处理

所有涉及多表操作的场景必须使用事务：
1. 创建任务时：同时插入 `tasks` 表和 `chunks` 表
2. 删除任务时：同时删除 `tasks` 表、`chunks` 表和 `trash` 表的记录
3. 批量更新进度时：使用事务批量提交

---

## 6. 需求覆盖矩阵 (Requirements Traceability Matrix)

| 需求编号 | 需求描述 | 对应类 | 对应方法 | 测试用例 |
|---------|---------|--------|---------|---------|
| 3.1.1 | HTTP/HTTPS 协议解析 | TaskManager | `createTask()` | TaskManagerTest::testCreateHttpTask |
| 3.1.1 | Magnet 磁力链接解析 | BTDownloadEngine | `parseMagnetUri()` | BTDownloadEngineTest::testParseMagnet |
| 3.1.1 | Torrent 种子文件解析 | BTDownloadEngine | `parseTorrentFile()` | BTDownloadEngineTest::testParseTorrent |
| 3.1.3 | 重复性校验 | TaskManager | `createTask()` | TaskManagerTest::testDuplicateCheck |
| 3.2.1 | 服务器能力检测 | HTTPDownloadEngine | `detectRangeSupport()` | HTTPDownloadEngineTest::testRangeDetection |
| 3.2.1 | 动态分块策略 | HTTPDownloadEngine | `calculateChunks()` | HTTPDownloadEngineTest::testChunkCalculation |
| 3.2.1 | 并发下载 | ChunkDownloader | `start()` | ChunkDownloaderTest::testConcurrentDownload |
| 3.2.1 | 断点续传 | ChunkDownloader | `resume()` | ChunkDownloaderTest::testResume |
| 3.2.1 | 文件合并 | FileManager | `mergeChunks()` | FileManagerTest::testMergeChunks |
| 3.2.1 | 自适应线程数调整 | AdaptiveThreadController | `adjustThreads()` | AdaptiveThreadControllerTest::testAdjustment |
| 3.2.2 | Tracker 连接 | BTDownloadEngine | `addTorrent()` | BTDownloadEngineTest::testTrackerConnection |
| 3.2.2 | DHT 支持 | BTDownloadEngine | `initSession()` | BTDownloadEngineTest::testDHTSupport |
| 3.2.2 | 上传与做种 | SeedingController | `startSeeding()` | SeedingControllerTest::testSeeding |
| 3.2.3 | 磁盘预分配 | FileManager | `preallocateDiskSpace()` | FileManagerTest::testPreallocate |
| 3.2.3 | 完整性校验 | FileManager | `verifyFileIntegrity()` | FileManagerTest::testIntegrityCheck |
| 3.3.2 | 并发控制 | TaskManager | `startTask()` | TaskManagerTest::testConcurrencyLimit |
| 3.4.1 | 数据绑定 | TaskListViewModel | `data()` | TaskListViewModelTest::testDataBinding |
| 3.5.1 | 数据持久化 | DatabaseManager | `saveTask()` | DatabaseManagerTest::testSaveTask |
| 3.5.2 | 进度节流 | DatabaseManager | `updateProgress()` | DatabaseManagerTest::testProgressThrottling |

---

## 7. 单元测试覆盖清单 (Unit Test Coverage Checklist)

### 7.1 核心模块测试

| 模块 | 测试类 | 测试用例数 | 覆盖率目标 |
|------|--------|-----------|-----------|
| TaskManager | TaskManagerTest | 15 | 90% |
| HTTPDownloadEngine | HTTPDownloadEngineTest | 20 | 85% |
| ChunkDownloader | ChunkDownloaderTest | 10 | 85% |
| AdaptiveThreadController | AdaptiveThreadControllerTest | 8 | 80% |
| BTDownloadEngine | BTDownloadEngineTest | 15 | 80% |
| FileManager | FileManagerTest | 12 | 90% |
| DatabaseManager | DatabaseManagerTest | 10 | 90% |

### 7.2 关键接口测试标记

文档中所有标记 `[需单元测试]` 的接口必须编写单元测试。

---

## 8. 下一阶段

详细设计已完成，下一步请执行：

```
/project:test-plan
```

进入**阶段4：测试计划**，编写测试用例和测试脚本。

---

**文档结束**
