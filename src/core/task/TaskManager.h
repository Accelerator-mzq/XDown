/**
 * @file TaskManager.h
 * @brief 任务管理器定义
 * @author XDown
 * @date 2026-03-08
 *
 * 负责任务的创建、启动、暂停、删除，以及并发控制。
 * 单例模式，全局唯一实例。
 *
 * [需单元测试]
 */

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QString>
#include <QVariantMap>
#include <QUuid>
#include "TaskState.h"
#include "common/DownloadTask.h"

// 前向声明
class IDownloadEngine;
class HTTPDownloadEngine;
class DBManager;

/**
 * @class TaskManager
 * @brief 任务管理器
 *
 * 负责任务的创建、启动、暂停、删除，以及并发控制。
 * 单例模式，全局唯一实例。
 */
class TaskManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return TaskManager 实例指针
     */
    static TaskManager* instance();

    /**
     * @brief 销毁单例实例
     */
    static void destroy();

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
                      const QVariantMap &options = QVariantMap());

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
     * @brief 恢复任务
     * @param taskId 任务 ID
     * @return 是否成功恢复
     */
    bool resumeTask(const QString &taskId);

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
     * @param filter 筛选条件（All/Downloading/Completed/BT/Trash）
     * @return 任务列表
     * [需单元测试]
     */
    QList<DownloadTask> getTaskList(const QString &filter = "All");

    /**
     * @brief 获取单个任务
     * @param taskId 任务 ID
     * @return 任务信息（如果不存在返回空任务）
     */
    DownloadTask getTask(const QString &taskId) const;

    /**
     * @brief 获取运行中的 HTTP 任务数
     * @return HTTP 任务数
     */
    int getRunningHttpTaskCount() const;

    /**
     * @brief 获取运行中的 BT 任务数
     * @return BT 任务数
     */
    int getRunningBtTaskCount() const;

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

    /**
     * @brief 任务完成信号
     * @param taskId 任务 ID
     * @param localPath 本地文件路径
     */
    void taskFinished(const QString &taskId, const QString &localPath);

    /**
     * @brief 任务错误信号
     * @param taskId 任务 ID
     * @param errorMsg 错误信息
     */
    void taskError(const QString &taskId, const QString &errorMsg);

    /**
     * @brief 任务删除信号
     * @param taskId 任务 ID
     */
    void taskDeleted(const QString &taskId);

private slots:
    /**
     * @brief 处理引擎进度更新
     * @param taskId 任务 ID
     * @param downloaded 已下载字节
     * @param total 总字节
     */
    void onEngineProgressUpdated(const QString &taskId, qint64 downloaded, qint64 total);

    /**
     * @brief 处理引擎速度更新
     * @param taskId 任务 ID
     * @param speed 速度（字节/秒）
     */
    void onEngineSpeedUpdated(const QString &taskId, double speed);

    /**
     * @brief 处理引擎完成
     * @param taskId 任务 ID
     */
    void onEngineFinished(const QString &taskId);

    /**
     * @brief 处理引擎错误
     * @param taskId 任务 ID
     * @param errorMsg 错误信息
     */
    void onEngineError(const QString &taskId, const QString &errorMsg);

    /**
     * @brief 检查并启动等待队列中的任务
     */
    void checkAndStartWaitingTasks();

private:
    /**
     * @brief 构造函数（私有，单例模式）
     * @param parent 父对象
     */
    explicit TaskManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TaskManager();

    /**
     * @brief 解析 URL 类型
     * @param url 下载链接
     * @return 任务类型
     */
    TaskType parseUrlType(const QString &url) const;

    /**
     * @brief 检查重复任务
     * @param url 下载链接
     * @return 是否存在重复
     */
    bool checkDuplicate(const QString &url) const;

    /**
     * @brief 从 URL 解析文件名
     * @param url 下载链接
     * @return 文件名
     */
    QString parseFileNameFromUrl(const QString &url) const;

    /**
     * @brief 创建下载引擎
     * @param task 任务
     * @return 引擎指针
     */
    IDownloadEngine* createDownloadEngine(const DownloadTask &task);

    /**
     * @brief 更新任务状态
     * @param taskId 任务 ID
     * @param newState 新状态
     */
    void updateTaskState(const QString &taskId, TaskState newState);

    // 成员变量
    static TaskManager* s_instance;  // 单例实例

    QMap<QString, DownloadTask*> m_tasks;           // 任务映射表
    QMap<QString, IDownloadEngine*> m_engines;     // 引擎映射表
    QQueue<QString> m_waitingQueue;                // 等待队列

    DBManager *m_dbManager;                   // 数据库管理器

    // 并发控制
    static const int MAX_CONCURRENT_HTTP_TASKS = 3;  // 最大并发 HTTP 任务数
    static const int MAX_CONCURRENT_BT_TASKS = 2;    // 最大并发 BT 任务数
};

#endif // TASKMANAGER_H
