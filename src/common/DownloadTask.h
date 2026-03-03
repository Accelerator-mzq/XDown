/**
 * @file DownloadTask.h
 * @brief 下载任务数据结构定义
 * @author XDown
 * @date 2024
 */

#ifndef DOWNLOADTASK_H
#define DOWNLOADTASK_H

#include <QString>
#include <QDateTime>

/**
 * @enum TaskStatus
 * @brief 任务状态枚举
 */
enum class TaskStatus {
    Waiting = 0,    // 等待中
    Downloading,    // 下载中
    Paused,         // 已暂停
    Finished,       // 已完成
    Error           // 错误
};

/**
 * @struct DownloadTask
 * @brief 下载任务实体结构体
 */
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
    qint64 createdAt = 0;          // 创建时间戳 (秒)
    qint64 updatedAt = 0;          // 更新时间戳 (秒)
    qint64 speedBytesPerSec = 0;   // 当前下载速度 (字节/秒)

    /**
     * @brief 默认构造函数
     */
    DownloadTask() {
        createdAt = QDateTime::currentSecsSinceEpoch();
        updatedAt = createdAt;
    }

    /**
     * @brief 计算下载进度百分比
     * @return 0.0 - 1.0 之间的进度值
     */
    double progress() const {
        if (totalBytes > 0) {
            return static_cast<double>(downloadedBytes) / totalBytes;
        }
        return 0.0;
    }

    /**
     * @brief 格式化文件大小为人类可读字符串
     * @param bytes 字节数
     * @return 格式化后的字符串 (如 "1.5 MB")
     */
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

    /**
     * @brief 格式化下载速度为人类可读字符串
     * @param bytesPerSec 字节/秒
     * @return 格式化后的字符串 (如 "1.5 MB/s")
     */
    static QString formatSpeed(qint64 bytesPerSec) {
        return formatSize(bytesPerSec) + "/s";
    }
};

#endif // DOWNLOADTASK_H
