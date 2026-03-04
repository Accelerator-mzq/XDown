/**
 * @file DBManager.cpp
 * @brief 数据库访问层实现
 * @author XDown
 * @date 2024
 */

#include "DBManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

DBManager& DBManager::instance() {
    static DBManager instance;
    return instance;
}

DBManager::DBManager(QObject* parent)
    : QObject(parent)
    , m_workerThread(nullptr)
    , m_isInitialized(false)
{
}

DBManager::~DBManager() {
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
    }
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBManager::initDB(const QString& dbPath) {
    QMutexLocker locker(&m_mutex);

    // 获取应用程序路径
    QString absolutePath = dbPath;
    if (QFileInfo(dbPath).isRelative()) {
        absolutePath = QCoreApplication::applicationDirPath() + "/" + dbPath;
    }

    // 确保目录存在
    QDir dir = QFileInfo(absolutePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 添加数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(absolutePath);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // 创建表结构 - 添加新字段
    QSqlQuery query(m_db);
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

    if (!query.exec(sql)) {
        qCritical() << "Failed to create table:" << query.lastError().text();
        return false;
    }

    // 创建索引以提高查询性能
    query.exec("CREATE INDEX IF NOT EXISTS idx_url ON downloads(url)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_status ON downloads(status)");

    m_isInitialized = true;
    qInfo() << "Database initialized successfully at:" << absolutePath;
    return true;
}

bool DBManager::insertTask(const DownloadTask& task) {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO downloads (id, url, local_path, file_name, total_size,
                             downloaded_size, status, error_message, created_at, updated_at, speed_bytes_per_sec)
        VALUES (:id, :url, :path, :name, :total, :down, :status, :error, :created, :updated, :speed)
    )");

    query.bindValue(":id", task.id);
    query.bindValue(":url", task.url);
    query.bindValue(":path", task.localPath);
    query.bindValue(":name", task.fileName);
    query.bindValue(":total", task.totalBytes);
    query.bindValue(":down", task.downloadedBytes);
    query.bindValue(":status", static_cast<int>(task.status));
    query.bindValue(":error", task.errorMessage);
    query.bindValue(":created", task.createdAt);
    query.bindValue(":updated", task.updatedAt);
    query.bindValue(":speed", task.speedBytesPerSec);

    bool success = query.exec();
    if (!success) {
        qWarning() << "Failed to insert task:" << query.lastError().text();
    }
    return success;
}

bool DBManager::updateTaskProgress(const QString& id, qint64 downloadedBytes,
                                   qint64 totalBytes, TaskStatus status) {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE downloads
        SET downloaded_size = :down, total_size = :total, status = :status, updated_at = :updated
        WHERE id = :id
    )");

    query.bindValue(":id", id);
    query.bindValue(":down", downloadedBytes);
    query.bindValue(":total", totalBytes);
    query.bindValue(":status", static_cast<int>(status));
    query.bindValue(":updated", QDateTime::currentSecsSinceEpoch());

    return query.exec();
}

bool DBManager::updateTaskStatus(const QString& id, TaskStatus status,
                                 const QString& errorMessage) {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    if (errorMessage.isEmpty()) {
        query.prepare(R"(
            UPDATE downloads
            SET status = :status, updated_at = :updated
            WHERE id = :id
        )");
    } else {
        query.prepare(R"(
            UPDATE downloads
            SET status = :status, error_message = :error, updated_at = :updated
            WHERE id = :id
        )");
        query.bindValue(":error", errorMessage);
    }

    query.bindValue(":id", id);
    query.bindValue(":status", static_cast<int>(status));
    query.bindValue(":updated", QDateTime::currentSecsSinceEpoch());

    bool success = query.exec();
    if (success) {
        emit taskStatusChanged(id, status);
    }
    return success;
}

bool DBManager::updateTaskSpeed(const QString& id, qint64 speedBytesPerSec) {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE downloads
        SET speed_bytes_per_sec = :speed, updated_at = :updated
        WHERE id = :id
    )");

    query.bindValue(":id", id);
    query.bindValue(":speed", speedBytesPerSec);
    query.bindValue(":updated", QDateTime::currentSecsSinceEpoch());

    return query.exec();
}

QList<DownloadTask> DBManager::loadAllTasks() {
    QMutexLocker locker(&m_mutex);

    QList<DownloadTask> tasks;
    QSqlQuery query(m_db);

    if (!query.exec("SELECT * FROM downloads ORDER BY created_at DESC")) {
        qWarning() << "Failed to load tasks:" << query.lastError().text();
        return tasks;
    }

    while (query.next()) {
        tasks.append(parseQueryToTask(query));
    }

    qInfo() << "Loaded" << tasks.size() << "tasks from database";
    return tasks;
}

bool DBManager::hasDuplicateDownloadingTask(const QString& url) const {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT COUNT(*) FROM downloads
        WHERE url = :url AND status = :status
    )");
    query.bindValue(":url", url);
    query.bindValue(":status", static_cast<int>(TaskStatus::Downloading));

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

bool DBManager::deleteTask(const QString& id) {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM downloads WHERE id = :id");
    query.bindValue(":id", id);

    bool success = query.exec();
    if (!success) {
        qWarning() << "Failed to delete task:" << query.lastError().text();
    }
    return success;
}

DownloadTask DBManager::getTaskById(const QString& id) const {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM downloads WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        return parseQueryToTask(query);
    }

    // 未找到时返回空任务
    return DownloadTask();
}

bool DBManager::clearAll() {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    bool success = query.exec("DELETE FROM downloads");
    if (!success) {
        qWarning() << "Failed to clear all tasks:" << query.lastError().text();
    }
    return success;
}

DownloadTask DBManager::parseQueryToTask(QSqlQuery& query) const {
    DownloadTask task;
    task.id = query.value("id").toString();
    task.url = query.value("url").toString();
    task.localPath = query.value("local_path").toString();
    task.fileName = query.value("file_name").toString();
    task.totalBytes = query.value("total_size").toLongLong();
    task.downloadedBytes = query.value("downloaded_size").toLongLong();
    task.status = static_cast<TaskStatus>(query.value("status").toInt());
    task.errorMessage = query.value("error_message").toString();
    task.createdAt = query.value("created_at").toLongLong();
    task.updatedAt = query.value("updated_at").toLongLong();
    task.speedBytesPerSec = query.value("speed_bytes_per_sec").toLongLong();
    return task;
}
