/**
 * @file DownloadEngine.cpp
 * @brief 下载调度中心实现
 * @author XDown
 * @date 2024
 *
 * 修复内容:
 * - 速度计算和显示
 * - 数据库写入节流 (Throttle)
 * - 重复任务校验
 * - 等待队列管理 (并发数限制)
 * - 磁盘空间检查
 */

#include "DownloadEngine.h"
#include "HttpDownloader.h"
#include "DBManager.h"
#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>

DownloadEngine::DownloadEngine(QObject* parent)
    : QObject(parent)
    , m_flushTimer(nullptr)
{
    // 初始化数据库节流写入定时器
    m_flushTimer = new QTimer(this);
    m_flushTimer->setInterval(FLUSH_INTERVAL);
    connect(m_flushTimer, &QTimer::timeout, this, &DownloadEngine::onFlushTimer);
    m_flushTimer->start();
}

DownloadEngine::~DownloadEngine() {
    m_flushTimer->stop();

    // 停止所有活跃下载
    for (auto* downloader : m_activeDownloaders.values()) {
        downloader->stop();
        downloader->deleteLater();
    }
    m_activeDownloaders.clear();
}

void DownloadEngine::initialize() {
    // 初始化数据库
    if (!DBManager::instance().initDB()) {
        qCritical() << "Failed to initialize database";
        emit taskAddFailed("数据库初始化失败");
        return;
    }

    // 加载历史任务
    QList<DownloadTask> tasks = DBManager::instance().loadAllTasks();

    for (auto& task : tasks) {
        m_tasks.insert(task.id, task);

        // 未完成的任务置为暂停状态
        if (task.status == TaskStatus::Downloading) {
            task.status = TaskStatus::Paused;
            DBManager::instance().updateTaskStatus(task.id, TaskStatus::Paused);
            m_tasks[task.id].status = TaskStatus::Paused;
        }
    }

    emit allTasksLoaded(tasks);
    qInfo() << "DownloadEngine initialized with" << tasks.size() << "tasks";
}

QString DownloadEngine::addNewTask(const QString& url, const QString& localPath) {
    // 1. 检查 URL 格式
    if (!isValidUrl(url)) {
        QString error = "无效的 URL 格式，仅支持 http:// 和 https:// 协议";
        emit taskAddFailed(error);
        return error;
    }

    // 2. 检查重复任务
    if (checkDuplicateTask(url)) {
        QString error = "该 URL 正在下载中，请勿重复添加";
        emit taskAddFailed(error);
        return error;
    }

    // 3. 解析文件名
    QString fileName = parseFileNameFromUrl(url);
    if (fileName.isEmpty()) {
        fileName = "download_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // 4. 构建完整保存路径
    QString savePath = localPath;
    if (QFileInfo(localPath).isDir()) {
        savePath = localPath + "/" + fileName;
    }

    // 5. 检查磁盘空间
    if (!checkDiskSpace(savePath)) {
        QString error = "磁盘空间不足";
        emit taskAddFailed(error);
        return error;
    }

    // 6. 创建任务实体
    DownloadTask newTask;
    newTask.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newTask.url = url;
    newTask.localPath = savePath;
    newTask.fileName = fileName;
    newTask.status = TaskStatus::Waiting;
    newTask.createdAt = QDateTime::currentSecsSinceEpoch();
    newTask.updatedAt = newTask.createdAt;

    // 7. 存入数据库
    DBManager::instance().insertTask(newTask);

    // 8. 加入等待队列或立即开始
    m_tasks.insert(newTask.id, newTask);

    if (m_activeDownloaders.size() < MAX_CONCURRENT_DOWNLOADS) {
        // 可以立即开始
        enqueueWaitingTask(newTask);
    } else {
        // 加入等待队列
        m_waitingQueue.enqueue(newTask);
    }

    emit taskAdded(newTask);
    return QString();
}

void DownloadEngine::pauseTask(const QString& id) {
    auto* downloader = m_activeDownloaders.value(id);
    if (downloader) {
        downloader->pause();
        m_activeDownloaders.remove(id);

        // 更新数据库
        DBManager::instance().updateTaskStatus(id, TaskStatus::Paused);

        // 处理等待队列
        processWaitingQueue();
    }
}

void DownloadEngine::resumeTask(const QString& id) {
    auto taskIt = m_tasks.find(id);
    if (taskIt == m_tasks.end()) return;

    DownloadTask& task = *taskIt;

    if (task.status == TaskStatus::Paused || task.status == TaskStatus::Waiting) {
        // 如果有可用槽位，立即开始
        if (m_activeDownloaders.size() < MAX_CONCURRENT_DOWNLOADS) {
            enqueueWaitingTask(task);
        } else {
            // 加入等待队列
            task.status = TaskStatus::Waiting;
            m_waitingQueue.enqueue(task);
            emit taskStatusChanged(id, TaskStatus::Waiting);
        }
    }
}

void DownloadEngine::deleteTask(const QString& id, bool deleteFile) {
    // 停止下载
    auto* downloader = m_activeDownloaders.value(id);
    if (downloader) {
        downloader->stop();
        downloader->deleteLater();
        m_activeDownloaders.remove(id);
    }

    // 从等待队列中移除
    QQueue<DownloadTask> newQueue;
    while (!m_waitingQueue.isEmpty()) {
        DownloadTask task = m_waitingQueue.dequeue();
        if (task.id != id) {
            newQueue.enqueue(task);
        }
    }
    m_waitingQueue = newQueue;

    // 删除本地文件
    if (deleteFile) {
        auto taskIt = m_tasks.find(id);
        if (taskIt != m_tasks.end()) {
            QString filePath = taskIt->localPath;
            if (QFileInfo::exists(filePath)) {
                QFile::remove(filePath);
            }
        }
    }

    // 从数据库删除
    DBManager::instance().deleteTask(id);

    // 从内存移除
    m_tasks.remove(id);
    m_pendingUpdates.remove(id);

    emit taskDeleted(id);
}

QList<DownloadTask> DownloadEngine::getAllTasks() const {
    return m_tasks.values();
}

QString DownloadEngine::getDefaultSavePath() const {
    // 使用系统默认下载目录
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    if (defaultPath.isEmpty()) {
        // 回退到用户主目录
        defaultPath = QDir::homePath() + "/Downloads";
    }

    // 确保目录存在
    QDir dir(defaultPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return defaultPath;
}

void DownloadEngine::onDownloaderProgress(const QString& id, qint64 downloaded,
                                          qint64 total, qint64 speed) {
    // 更新内存中的任务数据
    auto it = m_tasks.find(id);
    if (it != m_tasks.end()) {
        it->downloadedBytes = downloaded;
        it->totalBytes = total;
        it->speedBytesPerSec = speed;
    }

    // 发送给 UI 更新
    emit taskProgressUpdated(id, downloaded, total, speed);

    // 节流写入数据库
    saveTaskProgressThrottled(id, downloaded, total, speed);
}

void DownloadEngine::onDownloaderStatusChanged(const QString& id, TaskStatus status,
                                                const QString& errorMessage) {
    auto it = m_tasks.find(id);
    if (it != m_tasks.end()) {
        it->status = status;
        if (!errorMessage.isEmpty()) {
            it->errorMessage = errorMessage;
        }
    }

    // 立即写入数据库
    DBManager::instance().updateTaskStatus(id, status, errorMessage);

    emit taskStatusChanged(id, status, errorMessage);

    // 如果下载完成或出错，释放槽位
    if (status == TaskStatus::Finished || status == TaskStatus::Error) {
        m_activeDownloaders.remove(id);
        processWaitingQueue();
    }
}

void DownloadEngine::onDownloadFinished(const QString& id, const QString& localPath) {
    qInfo() << "Download finished:" << id << "->" << localPath;

    // 更新内存数据
    auto it = m_tasks.find(id);
    if (it != m_tasks.end()) {
        it->status = TaskStatus::Finished;
        it->speedBytesPerSec = 0;
    }

    // 移除活跃下载器
    auto* downloader = m_activeDownloaders.take(id);
    if (downloader) {
        downloader->deleteLater();
    }

    // 处理等待队列
    processWaitingQueue();
}

void DownloadEngine::onFlushTimer() {
    // 批量写入待更新的任务进度到数据库
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

bool DownloadEngine::checkDiskSpace(const QString& path, qint64 requiredBytes) const {
    QFileInfo info(path);
    QStorageInfo storage(info.absolutePath());

    qint64 available = storage.bytesAvailable();

    // 保留 100MB 缓冲
    const qint64 BUFFER = 100 * 1024 * 1024;

    if (requiredBytes > 0) {
        return (available - BUFFER) > requiredBytes;
    }

    // 默认检查至少 1GB 可用
    return (available - BUFFER) > (1024 * 1024 * 1024);
}

bool DownloadEngine::checkDuplicateTask(const QString& url) const {
    // 检查下载中的任务
    if (DBManager::instance().hasDuplicateDownloadingTask(url)) {
        return true;
    }

    // 检查内存中的活跃任务
    for (const auto& task : m_tasks.values()) {
        if (task.url == url && task.status == TaskStatus::Downloading) {
            return true;
        }
    }

    return false;
}

bool DownloadEngine::isValidUrl(const QString& url) const {
    QUrl qurl(url);
    return qurl.isValid() && (qurl.scheme() == "http" || qurl.scheme() == "https");
}

QString DownloadEngine::parseFileNameFromUrl(const QString& url) const {
    QUrl qurl(url);
    QString fileName = qurl.fileName();

    // 移除查询参数
    int queryIndex = fileName.indexOf('?');
    if (queryIndex > 0) {
        fileName = fileName.left(queryIndex);
    }

    // 如果文件名为空，尝试从 Content-Disposition 获取
    // (这需要在 HttpDownloader 中处理)

    return fileName;
}

void DownloadEngine::processWaitingQueue() {
    // 如果有可用槽位且等待队列不为空，启动下一个任务
    while (m_activeDownloaders.size() < MAX_CONCURRENT_DOWNLOADS
           && !m_waitingQueue.isEmpty()) {
        DownloadTask task = m_waitingQueue.dequeue();
        enqueueWaitingTask(task);
    }
}

void DownloadEngine::enqueueWaitingTask(const DownloadTask& task) {
    // 创建下载器
    auto* downloader = new HttpDownloader(task, this);
    m_activeDownloaders.insert(task.id, downloader);

    // 连接信号槽
    connect(downloader, &HttpDownloader::progressUpdated,
            this, &DownloadEngine::onDownloaderProgress);
    connect(downloader, &HttpDownloader::statusChanged,
            this, &DownloadEngine::onDownloaderStatusChanged);
    connect(downloader, &HttpDownloader::downloadFinished,
            this, &DownloadEngine::onDownloadFinished);

    // 更新内存状态
    auto it = m_tasks.find(task.id);
    if (it != m_tasks.end()) {
        it->status = TaskStatus::Downloading;
    }

    // 开始下载
    downloader->start();

    // 通知 UI
    emit taskStatusChanged(task.id, TaskStatus::Downloading);
}

void DownloadEngine::saveTaskProgressThrottled(const QString& id, qint64 downloaded,
                                               qint64 total, qint64 speed) {
    // 只保存到待写入队列，由定时器批量写入
    m_pendingUpdates.insert(id, qMakePair(downloaded, total));

    // 同时更新速度 (这个可以实时写入，不影响性能)
    DBManager::instance().updateTaskSpeed(id, speed);
}
