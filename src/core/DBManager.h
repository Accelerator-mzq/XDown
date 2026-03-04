/**
 * @file DBManager.h
 * @brief 数据库访问层头文件
 * @author XDown
 * @date 2024
 */

#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QList>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include "common/DownloadTask.h"

/**
 * @class DBManager
 * @brief SQLite 数据库管理器 (线程安全单例模式)
 *
 * 注意：所有数据库操作都是线程安全的，使用内部任务队列处理
 */
class DBManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return DBManager 引用
     */
    static DBManager& instance();

    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径 (默认为 xdown_data.db)
     * @return 初始化是否成功
     */
    bool initDB(const QString& dbPath = "xdown_data.db");

    /**
     * @brief 插入新任务
     * @param task 下载任务
     * @return 插入是否成功
     */
    bool insertTask(const DownloadTask& task);

    /**
     * @brief 更新任务进度
     * @param id 任务ID
     * @param downloadedBytes 已下载字节数
     * @param totalBytes 总字节数
     * @param status 任务状态
     * @return 更新是否成功
     */
    bool updateTaskProgress(const QString& id, qint64 downloadedBytes,
                           qint64 totalBytes, TaskStatus status);

    /**
     * @brief 更新任务状态
     * @param id 任务ID
     * @param status 任务状态
     * @param errorMessage 错误信息 (可选)
     * @return 更新是否成功
     */
    bool updateTaskStatus(const QString& id, TaskStatus status,
                         const QString& errorMessage = QString());

    /**
     * @brief 加载所有历史任务
     * @return 任务列表
     */
    QList<DownloadTask> loadAllTasks();

    /**
     * @brief 检查是否存在相同 URL 且处于下载中的任务
     * @param url 下载链接
     * @return 是否存在重复任务
     */
    bool hasDuplicateDownloadingTask(const QString& url) const;

    /**
     * @brief 删除任务
     * @param id 任务ID
     * @return 删除是否成功
     */
    bool deleteTask(const QString& id);

    /**
     * @brief 根据ID查询任务
     * @param id 任务ID
     * @return 查询到的任务（未找到时返回空任务）
     */
    DownloadTask getTaskById(const QString& id) const;

    /**
     * @brief 清空所有任务
     * @return 是否成功
     */
    bool clearAll();

    /**
     * @brief 更新任务速度
     * @param id 任务ID
     * @param speedBytesPerSec 当前速度
     * @return 更新是否成功
     */
    bool updateTaskSpeed(const QString& id, qint64 speedBytesPerSec);

signals:
    /**
     * @brief 任务被添加到数据库的信号
     */
    void taskInserted(const DownloadTask& task);

    /**
     * @brief 任务进度更新的信号
     */
    void taskProgressUpdated(const QString& id, qint64 downloaded, qint64 total);

    /**
     * @brief 任务状态变化的信号
     */
    void taskStatusChanged(const QString& id, TaskStatus status);

private:
    explicit DBManager(QObject* parent = nullptr);
    ~DBManager();

    // 禁用拷贝
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    /**
     * @brief 解析查询结果为任务实体
     * @param query SQL 查询对象
     * @return 下载任务
     */
    DownloadTask parseQueryToTask(QSqlQuery& query) const;

    QSqlDatabase m_db;              // 数据库连接
    mutable QMutex m_mutex;        // 互斥锁 (保护数据库操作)
    QThread* m_workerThread;       // 数据库工作线程
    bool m_isInitialized;          // 初始化标志
};

#endif // DBMANAGER_H
