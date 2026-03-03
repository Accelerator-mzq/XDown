/**
 * @file TaskListModel.cpp
 * @brief UI 数据绑定模型实现
 * @author XDown
 * @date 2024
 */

#include "TaskListModel.h"
#include <QDebug>

TaskListModel::TaskListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

TaskListModel::~TaskListModel() {
}

int TaskListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_tasks.count();
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_tasks.count()) {
        return QVariant();
    }

    const DownloadTask& task = m_tasks[index.row()];

    switch (role) {
        case IdRole:
            return task.id;

        case UrlRole:
            return task.url;

        case LocalPathRole:
            return task.localPath;

        case FileNameRole:
            return task.fileName;

        case TotalBytesRole:
            return task.totalBytes;

        case DownloadedBytesRole:
            return task.downloadedBytes;

        case ProgressRole:
            return task.progress();

        case StatusRole:
            return static_cast<int>(task.status);

        case ErrorMessageRole:
            return task.errorMessage;

        case SpeedBytesPerSecRole:
            return task.speedBytesPerSec;

        case SpeedFormattedRole:
            return DownloadTask::formatSpeed(task.speedBytesPerSec);

        case TotalSizeFormattedRole:
            return DownloadTask::formatSize(task.totalBytes);

        case DownloadedSizeFormattedRole:
            return DownloadTask::formatSize(task.downloadedBytes);

        case StatusTextRole:
            switch (task.status) {
                case TaskStatus::Waiting:
                    return "等待中";
                case TaskStatus::Downloading:
                    return "下载中";
                case TaskStatus::Paused:
                    return "已暂停";
                case TaskStatus::Finished:
                    return "已完成";
                case TaskStatus::Error:
                    return "错误: " + task.errorMessage;
                default:
                    return "未知";
            }

        default:
            return QVariant();
    }
}

QHash<int, QByteArray> TaskListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[UrlRole] = "url";
    roles[LocalPathRole] = "localPath";
    roles[FileNameRole] = "fileName";
    roles[TotalBytesRole] = "totalBytes";
    roles[DownloadedBytesRole] = "downloadedBytes";
    roles[ProgressRole] = "progress";
    roles[StatusRole] = "status";
    roles[ErrorMessageRole] = "errorMessage";
    roles[SpeedBytesPerSecRole] = "speedBytesPerSec";
    roles[SpeedFormattedRole] = "speedFormatted";
    roles[TotalSizeFormattedRole] = "totalSizeFormatted";
    roles[DownloadedSizeFormattedRole] = "downloadedSizeFormatted";
    roles[StatusTextRole] = "statusText";
    return roles;
}

void TaskListModel::addTask(const DownloadTask& task) {
    qDebug() << "TaskListModel::addTask called with id:" << task.id << "url:" << task.url;

    // 检查是否已存在
    if (m_idToIndex.contains(task.id)) {
        qDebug() << "Task already exists, skipping";
        return;
    }

    int row = m_tasks.count();
    qDebug() << "Inserting at row:" << row;
    beginInsertRows(QModelIndex(), row, row);

    m_tasks.append(task);
    m_idToIndex.insert(task.id, row);

    endInsertRows();

    qDebug() << "Task added successfully, total tasks:" << m_tasks.count();
    emit taskAdded(task);
}

void TaskListModel::removeTask(const QString& id) {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        return;
    }

    int row = *it;
    beginRemoveRows(QModelIndex(), row, row);

    m_tasks.removeAt(row);
    m_idToIndex.remove(id);

    // 更新索引映射
    for (int i = row; i < m_tasks.count(); ++i) {
        m_idToIndex[m_tasks[i].id] = i;
    }

    endRemoveRows();

    emit taskRemoved(id);
}

void TaskListModel::updateTaskProgress(const QString& id, qint64 downloaded,
                                       qint64 total, qint64 speed) {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        return;
    }

    int row = *it;
    DownloadTask& task = m_tasks[row];

    task.downloadedBytes = downloaded;
    task.totalBytes = total;
    task.speedBytesPerSec = speed;

    // 发射 dataChanged 信号，通知 QML 重绘
    QModelIndex index = createIndex(row, 0);
    emit dataChanged(index, index, {
        DownloadedBytesRole,
        TotalBytesRole,
        ProgressRole,
        SpeedBytesPerSecRole,
        SpeedFormattedRole,
        DownloadedSizeFormattedRole,
        TotalSizeFormattedRole
    });
}

void TaskListModel::updateTaskStatus(const QString& id, TaskStatus status,
                                     const QString& errorMessage) {
    auto it = m_idToIndex.find(id);
    if (it == m_idToIndex.end()) {
        return;
    }

    int row = *it;
    DownloadTask& task = m_tasks[row];

    task.status = status;
    if (!errorMessage.isEmpty()) {
        task.errorMessage = errorMessage;
    }

    // 发射 dataChanged 信号
    QModelIndex index = createIndex(row, 0);
    emit dataChanged(index, index, {
        StatusRole,
        StatusTextRole,
        ErrorMessageRole
    });
}

DownloadTask TaskListModel::getTask(const QString& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return m_tasks[*it];
    }
    return DownloadTask();
}

void TaskListModel::clearAll() {
    beginResetModel();
    m_tasks.clear();
    m_idToIndex.clear();
    endResetModel();
}

int TaskListModel::getTaskIndex(const QString& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return *it;
    }
    return -1;
}

void TaskListModel::loadTasks(const QList<DownloadTask>& tasks) {
    beginResetModel();

    m_tasks = tasks;
    m_idToIndex.clear();

    for (int i = 0; i < m_tasks.count(); ++i) {
        m_idToIndex.insert(m_tasks[i].id, i);
    }

    endResetModel();
}
