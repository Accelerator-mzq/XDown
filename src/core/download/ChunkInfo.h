/**
 * @file ChunkInfo.h
 * @brief 分块信息结构体定义
 * @author XDown
 * @date 2026-03-08
 *
 * [需单元测试]
 */

#ifndef CHUNKINFO_H
#define CHUNKINFO_H

#include <QString>

/**
 * @struct ChunkInfo
 * @brief 分块下载信息结构体
 *
 * 用于描述文件分块下载的起始位置、结束位置和状态。
 */
struct ChunkInfo {
    int index;                  // 分块索引 (从 0 开始)
    qint64 startPos;           // 起始字节位置
    qint64 endPos;             // 结束字节位置
    qint64 downloadedBytes;    // 已下载字节数
    QString tempFilePath;      // 临时文件路径
    bool isCompleted;          // 是否已完成下载
    bool isFailed;             // 是否下载失败

    /**
     * @brief 默认构造函数
     */
    ChunkInfo()
        : index(0)
        , startPos(0)
        , endPos(0)
        , downloadedBytes(0)
        , isCompleted(false)
        , isFailed(false)
    {}

    /**
     * @brief 构造函数
     * @param idx 分块索引
     * @param start 起始位置
     * @param end 结束位置
     */
    ChunkInfo(int idx, qint64 start, qint64 end)
        : index(idx)
        , startPos(start)
        , endPos(end)
        , downloadedBytes(0)
        , isCompleted(false)
        , isFailed(false)
    {}

    /**
     * @brief 获取分块大小
     * @return 分块字节数
     */
    qint64 size() const {
        return endPos - startPos + 1;
    }

    /**
     * @brief 获取已下载的比例
     * @return 0.0 - 1.0 之间的进度值
     */
    double progress() const {
        qint64 totalSize = size();
        if (totalSize > 0) {
            return static_cast<double>(downloadedBytes) / totalSize;
        }
        return 0.0;
    }
};

#endif // CHUNKINFO_H
