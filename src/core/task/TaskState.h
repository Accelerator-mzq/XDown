/**
 * @file TaskState.h
 * @brief 任务状态枚举定义
 * @author XDown
 * @date 2026-03-08
 *
 * [需单元测试]
 */

#ifndef TASKSTATE_H
#define TASKSTATE_H

/**
 * @enum TaskState
 * @brief 任务状态枚举 (扩展版，支持 BT 下载)
 *
 * 与旧版 TaskStatus 兼容，但增加了更多状态：
 * - Waiting: 等待中
 * - Downloading: 下载中
 * - Paused: 已暂停
 * - Finished: 已完成
 * - Error: 错误
 * - Trashed: 已删除（回收站）
 * - Seeding: 做种中（BT 特有）
 */
enum class TaskState {
    Waiting = 0,     // 等待中
    Downloading,     // 下载中
    Paused,         // 已暂停
    Finished,       // 已完成
    Error,          // 错误
    Trashed,        // 回收站（软删除）
    Seeding         // 做种中（BT 特有）
};

/**
 * @enum TaskType
 * @brief 任务类型枚举
 */
enum class TaskType {
    HTTP = 0,       // HTTP/HTTPS 下载
    BT,             // BT 种子下载
    Magnet          // Magnet 磁力链接
};

/**
 * @brief 任务状态转字符串
 * @param state 任务状态
 * @return 状态对应的字符串
 */
inline QString taskStateToString(TaskState state) {
    switch (state) {
        case TaskState::Waiting:    return "Waiting";
        case TaskState::Downloading: return "Downloading";
        case TaskState::Paused:     return "Paused";
        case TaskState::Finished:   return "Finished";
        case TaskState::Error:      return "Error";
        case TaskState::Trashed:    return "Trashed";
        case TaskState::Seeding:    return "Seeding";
        default:                    return "Unknown";
    }
}

/**
 * @brief 字符串转任务状态
 * @param str 状态字符串
 * @return 对应的任务状态
 */
inline TaskState stringToTaskState(const QString &str) {
    if (str == "Waiting")    return TaskState::Waiting;
    if (str == "Downloading") return TaskState::Downloading;
    if (str == "Paused")     return TaskState::Paused;
    if (str == "Finished")   return TaskState::Finished;
    if (str == "Error")      return TaskState::Error;
    if (str == "Trashed")    return TaskState::Trashed;
    if (str == "Seeding")    return TaskState::Seeding;
    return TaskState::Waiting;
}

/**
 * @brief 任务类型转字符串
 * @param type 任务类型
 * @return 类型对应的字符串
 */
inline QString taskTypeToString(TaskType type) {
    switch (type) {
        case TaskType::HTTP:   return "HTTP";
        case TaskType::BT:     return "BT";
        case TaskType::Magnet: return "Magnet";
        default:               return "Unknown";
    }
}

/**
 * @brief 字符串转任务类型
 * @param str 类型字符串
 * @return 对应的任务类型
 */
inline TaskType stringToTaskType(const QString &str) {
    if (str == "HTTP")   return TaskType::HTTP;
    if (str == "BT")     return TaskType::BT;
    if (str == "Magnet") return TaskType::Magnet;
    return TaskType::HTTP;
}

#endif // TASKSTATE_H
