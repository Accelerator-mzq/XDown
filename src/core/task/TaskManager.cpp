/**
 * @file TaskManager.cpp
 * @brief 任务管理器实现
 * @author XDown
 * @date 2026-03-08
 */

#include "TaskManager.h"
#include "../download/IDownloadEngine.h"
#include "../download/HTTPDownloadEngine.h"
#include "../DBManager.h"
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMutexLocker>

// 静态成员初始化
TaskManager* TaskManager::s_instance = nullptr;

/******************************************************
 * @brief 获取单例实例
 * @return TaskManager 实例指针
 ******************************************************/
TaskManager* TaskManager::instance()
{
    if (!s_instance) {
        s_instance = new TaskManager();
    }
    return s_instance;
}

/******************************************************
 * @brief 销毁单例实例
 ******************************************************/
void TaskManager::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

/******************************************************
 * @brief 构造函数（私有，单例模式）
 * @param parent 父对象
 ******************************************************/
TaskManager::TaskManager(QObject *parent)
    : QObject(parent)
    , m_dbManager(nullptr)
{
    // 初始化数据库管理器
    m_dbManager = &DBManager::instance();
}

/******************************************************
 * @brief 析构函数
 ******************************************************/
TaskManager::~TaskManager()
{
    // 停止所有任务
    pauseAll();

    // 清理所有引擎
    for (IDownloadEngine *engine : m_engines.values()) {
        if (engine) {
            engine->deleteLater();
        }
    }
    m_engines.clear();

    // 清理所有任务
    for (DownloadTask *task : m_tasks.values()) {
        delete task;
    }
    m_tasks.clear();
}

/******************************************************
 * @brief 创建新任务
 * @param url 下载链接
 * @param savePath 保存路径
 * @param options 高级选项
 * @return 任务 ID，若创建失败返回空字符串
 ******************************************************/
QString TaskManager::createTask(const QString &url,
                               const QString &savePath,
                               const QVariantMap &options)
{
    // 检查 URL 是否有效
    QUrl parsedUrl(url);
    if (!parsedUrl.isValid() || parsedUrl.scheme().isEmpty()) {
        return QString();
    }

    // 检查重复任务
    if (checkDuplicate(url)) {
        return QString();
    }

    // 解析任务类型
    TaskType taskType = parseUrlType(url);

    // 生成任务 ID
    QString taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 解析文件名
    QString fileName = parseFileNameFromUrl(url);
    if (fileName.isEmpty()) {
        fileName = "download";
    }

    // 构建完整保存路径
    QString fullSavePath = savePath;
    if (!fullSavePath.endsWith('/') && !fullSavePath.endsWith('\\')) {
        fullSavePath += "/";
    }
    fullSavePath += fileName;

    // 创建任务对象
    DownloadTask *task = new DownloadTask();
    task->id = taskId;
    task->url = url;
    task->localPath = fullSavePath;
    task->fileName = fileName;
    task->status = TaskStatus::Waiting;
    task->createdAt = QDateTime::currentSecsSinceEpoch();
    task->updatedAt = task->createdAt;

    // 保存到任务映射表
    m_tasks[taskId] = task;

    // 保存到数据库
    if (m_dbManager) {
        m_dbManager->insertTask(*task);
    }

    // 发出任务创建信号
    emit taskCreated(taskId);

    return taskId;
}

/******************************************************
 * @brief 启动任务
 * @param taskId 任务 ID
 * @return 是否成功启动
 ******************************************************/
bool TaskManager::startTask(const QString &taskId)
{
    // 查找任务
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return false;
    }

    // 检查并发限制
    TaskType taskType = parseUrlType(task->url);
    if (taskType == TaskType::HTTP) {
        if (getRunningHttpTaskCount() >= MAX_CONCURRENT_HTTP_TASKS) {
            // 进入等待队列
            m_waitingQueue.enqueue(taskId);
            return false;
        }
    } else {
        if (getRunningBtTaskCount() >= MAX_CONCURRENT_BT_TASKS) {
            m_waitingQueue.enqueue(taskId);
            return false;
        }
    }

    // 创建下载引擎
    IDownloadEngine *engine = createDownloadEngine(*task);
    if (!engine) {
        return false;
    }

    // 保存引擎映射
    m_engines[taskId] = engine;

    // 连接引擎信号（使用 lambda 桥接参数）
    // 注意：BT引擎可能使用旧的信号签名
    QObject::connect(engine, &IDownloadEngine::progressUpdated,
            this, [this, taskId](const QString& id, qint64 downloaded, qint64 total, qint64 speed) {
        Q_UNUSED(id);
        onEngineProgressUpdated(taskId, downloaded, total);
    });
    connect(engine, &IDownloadEngine::speedUpdated,
            this, [this, taskId](double speed) {
        onEngineSpeedUpdated(taskId, speed);
    });
    connect(engine, &IDownloadEngine::finished,
            this, [this, taskId]() {
        onEngineFinished(taskId);
    });
    connect(engine, &IDownloadEngine::error,
            this, [this, taskId](const QString &errorMsg) {
        onEngineError(taskId, errorMsg);
    });

    // 更新任务状态
    updateTaskState(taskId, TaskState::Downloading);

    // 启动下载
    engine->start();

    return true;
}

/******************************************************
 * @brief 暂停任务
 * @param taskId 任务 ID
 * @return 是否成功暂停
 ******************************************************/
bool TaskManager::pauseTask(const QString &taskId)
{
    // 查找引擎
    IDownloadEngine *engine = m_engines.value(taskId);
    if (!engine) {
        return false;
    }

    // 暂停下载
    engine->pause();

    // 更新任务状态
    updateTaskState(taskId, TaskState::Paused);

    // 从等待队列中移除
    m_waitingQueue.removeAll(taskId);

    return true;
}

/******************************************************
 * @brief 删除任务
 * @param taskId 任务 ID
 * @param deleteFile 是否同时删除本地文件
 * @return 是否成功删除
 ******************************************************/
bool TaskManager::deleteTask(const QString &taskId, bool deleteFile)
{
    // 查找任务
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return false;
    }

    // 停止下载引擎
    IDownloadEngine *engine = m_engines.value(taskId);
    if (engine) {
        engine->pause();
        engine->deleteLater();
        m_engines.remove(taskId);
    }

    // 删除本地文件
    if (deleteFile) {
        QFile file(task->localPath);
        if (file.exists()) {
            file.remove();
        }
    }

    // 从等待队列中移除
    m_waitingQueue.removeAll(taskId);

    // 从数据库中删除
    if (m_dbManager) {
        m_dbManager->deleteTask(taskId);
    }

    // 删除任务对象
    m_tasks.remove(taskId);
    delete task;

    // 发出任务删除信号
    emit taskDeleted(taskId);

    return true;
}

/******************************************************
 * @brief 恢复任务
 * @param taskId 任务 ID
 * @return 是否成功恢复
 ******************************************************/
bool TaskManager::resumeTask(const QString &taskId)
{
    return startTask(taskId);
}

/******************************************************
 * @brief 启动所有任务
 ******************************************************/
void TaskManager::startAll()
{
    // 启动等待队列中的所有任务
    checkAndStartWaitingTasks();
}

/******************************************************
 * @brief 暂停所有任务
 ******************************************************/
void TaskManager::pauseAll()
{
    // 暂停所有运行中的任务
    for (IDownloadEngine *engine : m_engines.values()) {
        if (engine) {
            engine->pause();
        }
    }

    // 更新所有任务状态
    for (DownloadTask *task : m_tasks.values()) {
        if (task->status == TaskStatus::Downloading) {
            updateTaskState(task->id, TaskState::Paused);
        }
    }

    // 清空等待队列
    m_waitingQueue.clear();
}

/******************************************************
 * @brief 获取任务列表
 * @param filter 筛选条件
 * @return 任务列表
 ******************************************************/
QList<DownloadTask> TaskManager::getTaskList(const QString &filter)
{
    QList<DownloadTask> taskList;

    for (DownloadTask *task : m_tasks.values()) {
        bool matches = false;

        if (filter == "All") {
            matches = true;
        } else if (filter == "Downloading") {
            matches = (task->status == TaskStatus::Downloading);
        } else if (filter == "Completed") {
            matches = (task->status == TaskStatus::Finished);
        } else if (filter == "Paused") {
            matches = (task->status == TaskStatus::Paused);
        } else if (filter == "Waiting") {
            matches = (task->status == TaskStatus::Waiting);
        }

        if (matches) {
            taskList.append(*task);
        }
    }

    return taskList;
}

/******************************************************
 * @brief 获取单个任务
 * @param taskId 任务 ID
 * @return 任务信息
 ******************************************************/
DownloadTask TaskManager::getTask(const QString &taskId) const
{
    DownloadTask *task = m_tasks.value(taskId);
    if (task) {
        return *task;
    }
    return DownloadTask();
}

/******************************************************
 * @brief 获取运行中的 HTTP 任务数
 * @return HTTP 任务数
 ******************************************************/
int TaskManager::getRunningHttpTaskCount() const
{
    int count = 0;
    for (DownloadTask *task : m_tasks.values()) {
        if (task->status == TaskStatus::Downloading) {
            TaskType type = parseUrlType(task->url);
            if (type == TaskType::HTTP) {
                count++;
            }
        }
    }
    return count;
}

/******************************************************
 * @brief 获取运行中的 BT 任务数
 * @return BT 任务数
 ******************************************************/
int TaskManager::getRunningBtTaskCount() const
{
    int count = 0;
    for (DownloadTask *task : m_tasks.values()) {
        if (task->status == TaskStatus::Downloading) {
            TaskType type = parseUrlType(task->url);
            if (type == TaskType::BT || type == TaskType::Magnet) {
                count++;
            }
        }
    }
    return count;
}

/******************************************************
 * @brief 处理引擎进度更新
 * @param taskId 任务 ID
 * @param downloaded 已下载字节
 * @param total 总字节
 ******************************************************/
void TaskManager::onEngineProgressUpdated(const QString &taskId, qint64 downloaded, qint64 total)
{
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return;
    }

    task->downloadedBytes = downloaded;
    task->totalBytes = total;
    task->updatedAt = QDateTime::currentSecsSinceEpoch();

    // 计算进度和速度
    double progress = 0.0;
    if (total > 0) {
        progress = static_cast<double>(downloaded) / total;
    }

    // 发出进度更新信号
    emit taskProgressUpdated(taskId, progress, task->speedBytesPerSec);
}

/******************************************************
 * @brief 处理引擎速度更新
 * @param taskId 任务 ID
 * @param speed 速度（字节/秒）
 ******************************************************/
void TaskManager::onEngineSpeedUpdated(const QString &taskId, double speed)
{
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return;
    }

    task->speedBytesPerSec = static_cast<qint64>(speed);
}

/******************************************************
 * @brief 处理引擎完成
 * @param taskId 任务 ID
 ******************************************************/
void TaskManager::onEngineFinished(const QString &taskId)
{
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return;
    }

    // 更新任务状态
    task->status = TaskStatus::Finished;
    task->updatedAt = QDateTime::currentSecsSinceEpoch();

    // 更新数据库
    if (m_dbManager) {
        m_dbManager->updateTaskStatus(taskId, TaskStatus::Finished);
    }

    // 发出完成信号
    emit taskFinished(taskId, task->localPath);
    emit taskStateChanged(taskId, TaskState::Finished);

    // 移除引擎
    IDownloadEngine *engine = m_engines.value(taskId);
    if (engine) {
        engine->deleteLater();
        m_engines.remove(taskId);
    }

    // 检查并启动等待队列中的任务
    checkAndStartWaitingTasks();
}

/******************************************************
 * @brief 处理引擎错误
 * @param taskId 任务 ID
 * @param errorMsg 错误信息
 ******************************************************/
void TaskManager::onEngineError(const QString &taskId, const QString &errorMsg)
{
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return;
    }

    // 更新任务状态
    task->status = TaskStatus::Error;
    task->errorMessage = errorMsg;
    task->updatedAt = QDateTime::currentSecsSinceEpoch();

    // 更新数据库
    if (m_dbManager) {
        m_dbManager->updateTaskStatus(taskId, TaskStatus::Error, errorMsg);
    }

    // 发出错误信号
    emit taskError(taskId, errorMsg);
    emit taskStateChanged(taskId, TaskState::Error);

    // 移除引擎
    IDownloadEngine *engine = m_engines.value(taskId);
    if (engine) {
        engine->deleteLater();
        m_engines.remove(taskId);
    }

    // 检查并启动等待队列中的任务
    checkAndStartWaitingTasks();
}

/******************************************************
 * @brief 检查并启动等待队列中的任务
 ******************************************************/
void TaskManager::checkAndStartWaitingTasks()
{
    // 尝试启动等待队列中的任务
    while (!m_waitingQueue.isEmpty()) {
        QString taskId = m_waitingQueue.head();

        // 检查并发限制
        TaskType taskType = parseUrlType(m_tasks.value(taskId)->url);
        bool canStart = false;

        if (taskType == TaskType::HTTP) {
            canStart = (getRunningHttpTaskCount() < MAX_CONCURRENT_HTTP_TASKS);
        } else {
            canStart = (getRunningBtTaskCount() < MAX_CONCURRENT_BT_TASKS);
        }

        if (canStart) {
            m_waitingQueue.dequeue();
            startTask(taskId);
        } else {
            break;  // 无法启动，等待下次调用
        }
    }
}

/******************************************************
 * @brief 解析 URL 类型
 * @param url 下载链接
 * @return 任务类型
 ******************************************************/
TaskType TaskManager::parseUrlType(const QString &url) const
{
    QString lowerUrl = url.toLower();

    // BT 种子文件
    if (lowerUrl.endsWith(".torrent")) {
        return TaskType::BT;
    }

    // Magnet 磁力链接
    if (lowerUrl.startsWith("magnet:")) {
        return TaskType::Magnet;
    }

    // 默认 HTTP/HTTPS
    return TaskType::HTTP;
}

/******************************************************
 * @brief 检查重复任务
 * @param url 下载链接
 * @return 是否存在重复
 ******************************************************/
bool TaskManager::checkDuplicate(const QString &url) const
{
    for (DownloadTask *task : m_tasks.values()) {
        if (task->url == url) {
            return true;
        }
    }
    return false;
}

/******************************************************
 * @brief 从 URL 解析文件名
 * @param url 下载链接
 * @return 文件名
 ******************************************************/
QString TaskManager::parseFileNameFromUrl(const QString &url) const
{
    QUrl parsedUrl(url);
    QString path = parsedUrl.path();

    // 获取路径最后一部分作为文件名
    QString fileName = path.section('/', -1);

    // 如果为空，使用默认文件名
    if (fileName.isEmpty()) {
        fileName = "download";
    }

    // 移除查询参数
    int queryIndex = fileName.indexOf('?');
    if (queryIndex > 0) {
        fileName = fileName.left(queryIndex);
    }

    // URL 解码
    fileName = QUrl::fromPercentEncoding(fileName.toUtf8());

    return fileName;
}

/******************************************************
 * @brief 创建下载引擎
 * @param task 任务
 * @return 引擎指针
 ******************************************************/
IDownloadEngine* TaskManager::createDownloadEngine(const DownloadTask &task)
{
    TaskType taskType = parseUrlType(task.url);

    switch (taskType) {
        case TaskType::HTTP: {
            // 创建 HTTP 下载引擎
            HTTPDownloadEngine *engine = new HTTPDownloadEngine(
                task.url,
                task.localPath,
                8  // 默认 8 线程
            );
            return engine;
        }
        case TaskType::BT:
        case TaskType::Magnet:
            // TODO: 创建 BT 下载引擎
            return nullptr;
        default:
            return nullptr;
    }
}

/******************************************************
 * @brief 更新任务状态
 * @param taskId 任务 ID
 * @param newState 新状态
 ******************************************************/
void TaskManager::updateTaskState(const QString &taskId, TaskState newState)
{
    DownloadTask *task = m_tasks.value(taskId);
    if (!task) {
        return;
    }

    // 转换 TaskState 到 TaskStatus
    switch (newState) {
        case TaskState::Waiting:
            task->status = TaskStatus::Waiting;
            break;
        case TaskState::Downloading:
            task->status = TaskStatus::Downloading;
            break;
        case TaskState::Paused:
            task->status = TaskStatus::Paused;
            break;
        case TaskState::Finished:
            task->status = TaskStatus::Finished;
            break;
        case TaskState::Error:
            task->status = TaskStatus::Error;
            break;
        default:
            break;
    }

    task->updatedAt = QDateTime::currentSecsSinceEpoch();

    // 保存到数据库
    if (m_dbManager) {
        m_dbManager->insertTask(*task);
    }

    // 发出状态变更信号
    emit taskStateChanged(taskId, newState);
}
