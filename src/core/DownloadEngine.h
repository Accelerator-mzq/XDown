/**
 * @file DownloadEngine.h
 * @brief 下载调度中心头文件
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

#ifndef DOWNLOADENGINE_H
#define DOWNLOADENGINE_H

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QTimer>
#include <QStorageInfo>
#include "common/DownloadTask.h"

class HttpDownloader;

/**
 * @class DownloadEngine
 * @brief 下载调度中心
 *
 * 管理所有下载任务:
 * - 任务添加、删除、暂停、继续
 * - 并发下载数量限制
 * - 等待队列管理
 * - 数据库写入节流
 */
class DownloadEngine : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DownloadEngine(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~DownloadEngine();

    /**
     * @brief 添加新下载任务
     * @param url 下载链接
     * @param localPath 本地保存路径
     * @return 是否添加成功 (失败时返回错误信息)
     */
    QString addNewTask(const QString& url, const QString& localPath);

    /**
     * @brief 暂停任务
     * @param id 任务ID
     */
    void pauseTask(const QString& id);

    /**
     * @brief 继续/开始任务
     * @param id 任务ID
     */
    void resumeTask(const QString& id);

    /**
     * @brief 删除任务
     * @param id 任务ID
     * @param deleteFile 是否同时删除本地文件
     */
    void deleteTask(const QString& id, bool deleteFile = false);

    /**
     * @brief 获取所有任务
     * @return 任务列表
     */
    QList<DownloadTask> getAllTasks() const;

    /**
     * @brief 获取等待中的任务
     * @return 等待队列
     */
    QQueue<DownloadTask> getWaitingTasks() const { return m_waitingQueue; }

    /**
     * @brief 获取当前下载中的任务数
     * @return 下载中的任务数量
     */
    int getDownloadingCount() const { return m_activeDownloaders.size(); }

    /**
     * @brief 获取默认保存路径
     * @return 默认保存路径
     */
    QString getDefaultSavePath() const;

signals:
    /**
     * @brief 任务被添加的信号
     * @param task 新任务
     */
    void taskAdded(const DownloadTask& task);

    /**
     * @brief 任务进度更新信号
     * @param id 任务ID
     * @param downloaded 已下载字节
     * @param total 总字节
     * @param speed 当前速度
     */
    void taskProgressUpdated(const QString& id, qint64 downloaded,
                            qint64 total, qint64 speed);

    /**
     * @brief 任务状态变化信号
     * @param id 任务ID
     * @param status 新状态
     * @param errorMessage 错误信息
     */
    void taskStatusChanged(const QString& id, TaskStatus status,
                          const QString& errorMessage = QString());

    /**
     * @brief 添加任务失败的信号
     * @param error 错误信息
     */
    void taskAddFailed(const QString& error);

    /**
     * @brief 任务被删除的信号
     * @param id 任务ID
     */
    void taskDeleted(const QString& id);

    /**
     * @brief 所有任务加载完成的信号
     * @param tasks 任务列表
     */
    void allTasksLoaded(const QList<DownloadTask>& tasks);

public slots:
    /**
     * @brief 初始化引擎 (加载历史任务)
     */
    void initialize();

private slots:
    /**
     * @brief 下载器进度更新处理
     */
    void onDownloaderProgress(const QString& id, qint64 downloaded,
                            qint64 total, qint64 speed);

    /**
     * @brief 下载器状态变化处理
     */
    void onDownloaderStatusChanged(const QString& id, TaskStatus status,
                                  const QString& errorMessage);

    /**
     * @brief 下载完成处理
     */
    void onDownloadFinished(const QString& id, const QString& localPath);

    /**
     * @brief 数据库写入节流定时器
     */
    void onFlushTimer();

private:
    /**
     * @brief 检查磁盘空间
     * @param path 路径
     * @param requiredBytes 需要的字节数
     * @return 空间是否足够
     */
    bool checkDiskSpace(const QString& path, qint64 requiredBytes = 0) const;

    /**
     * @brief 检查重复任务
     * @param url 下载链接
     * @return 是否存在重复
     */
    bool checkDuplicateTask(const QString& url) const;

    /**
     * @brief 检查 URL 格式
     * @param url 下载链接
     * @return URL 是否有效
     */
    bool isValidUrl(const QString& url) const;

    /**
     * @brief 从 URL 解析文件名
     * @param url 下载链接
     * @return 文件名
     */
    QString parseFileNameFromUrl(const QString& url) const;

    /**
     * @brief 处理等待队列
     */
    void processWaitingQueue();

    /**
     * @brief 将任务加入等待队列
     * @param task 任务
     */
    void enqueueWaitingTask(const DownloadTask& task);

    /**
     * @brief 保存任务到数据库 (带节流)
     * @param id 任务ID
     * @param downloaded 已下载
     * @param total 总数
     * @param speed 速度
     */
    void saveTaskProgressThrottled(const QString& id, qint64 downloaded,
                                   qint64 total, qint64 speed);

    // ========== 成员变量 ==========
    QMap<QString, HttpDownloader*> m_activeDownloaders;  // 活跃下载器
    QMap<QString, DownloadTask> m_tasks;                  // 任务缓存
    QQueue<DownloadTask> m_waitingQueue;                   // 等待队列

    // 节流写入
    QMap<QString, QPair<qint64, qint64>> m_pendingUpdates; // 待写入的进度更新
    QTimer* m_flushTimer;                                  // 写入定时器 (2秒)
    static const int FLUSH_INTERVAL = 2000;                // 写入间隔 (毫秒)

    // 并发控制
    static const int MAX_CONCURRENT_DOWNLOADS = 3;         // 最大并发数
};

#endif // DOWNLOADENGINE_H
