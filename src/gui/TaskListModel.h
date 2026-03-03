/**
 * @file TaskListModel.h
 * @brief UI 数据绑定模型头文件
 * @author XDown
 * @date 2024
 */

#ifndef TASKLISTMODEL_H
#define TASKLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QHash>
#include <QByteArray>
#include "common/DownloadTask.h"

class DownloadEngine;

/**
 * @class TaskListModel
 * @brief 任务列表数据模型
 *
 * 将 C++ 的 DownloadTask 列表映射到 QML 的 ListView
 */
class TaskListModel : public QAbstractListModel {
    Q_OBJECT

public:
    // ========== QML 角色定义 ==========
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
        SpeedFormattedRole,
        TotalSizeFormattedRole,
        DownloadedSizeFormattedRole,
        StatusTextRole
    };

    explicit TaskListModel(QObject* parent = nullptr);
    ~TaskListModel() override;

    /**
     * @brief 返回模型行数
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief 获取数据
     * @param index 模型索引
     * @param role 角色
     * @return 数据
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief 设置角色名
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief 添加任务
     * @param task 任务
     */
    void addTask(const DownloadTask& task);

    /**
     * @brief 移除任务
     * @param id 任务ID
     */
    void removeTask(const QString& id);

    /**
     * @brief 更新任务进度
     * @param id 任务ID
     * @param downloaded 已下载
     * @param total 总数
     * @param speed 速度
     */
    void updateTaskProgress(const QString& id, qint64 downloaded,
                           qint64 total, qint64 speed);

    /**
     * @brief 更新任务状态
     * @param id 任务ID
     * @param status 状态
     * @param errorMessage 错误信息
     */
    void updateTaskStatus(const QString& id, TaskStatus status,
                         const QString& errorMessage = QString());

    /**
     * @brief 获取任务
     * @param id 任务ID
     * @return 任务
     */
    DownloadTask getTask(const QString& id) const;

    /**
     * @brief 清空所有任务
     */
    void clearAll();

    /**
     * @brief 获取任务索引
     * @param id 任务ID
     * @return 索引 (-1 表示未找到)
     */
    int getTaskIndex(const QString& id) const;

signals:
    /**
     * @brief 任务被添加
     */
    void taskAdded(const DownloadTask& task);

    /**
     * @brief 任务被移除
     */
    void taskRemoved(const QString& id);

public slots:
    /**
     * @brief 加载所有任务
     * @param tasks 任务列表
     */
    void loadTasks(const QList<DownloadTask>& tasks);

private:
    QList<DownloadTask> m_tasks;                    // 任务列表
    QHash<QString, int> m_idToIndex;               // ID 到索引的映射
};

#endif // TASKLISTMODEL_H
