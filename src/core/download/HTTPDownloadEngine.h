/**
 * @file HTTPDownloadEngine.h
 * @brief HTTP/HTTPS 多线程分块下载引擎定义
 * @author XDown
 * @date 2026-03-08
 *
 * 支持：
 * - Range 请求分块下载
 * - 自适应线程数调整
 * - 断点续传
 * - 文件合并
 *
 * [需单元测试]
 */

#ifndef HTTPDOWNLOADENGINE_H
#define HTTPDOWNLOADENGINE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "IDownloadEngine.h"
#include "ChunkInfo.h"
#include "ChunkDownloader.h"
#include "AdaptiveThreadController.h"

/**
 * @class HTTPDownloadEngine
 * @brief HTTP/HTTPS 多线程分块下载引擎
 *
 * 支持：
 * - Range 请求分块下载
 * - 自适应线程数调整
 * - 断点续传
 * - 文件合并
 */
class HTTPDownloadEngine : public IDownloadEngine {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param url 下载链接
     * @param savePath 保存路径
     * @param threadCount 线程数（默认 8）
     * @param parent 父对象
     */
    explicit HTTPDownloadEngine(const QString &url,
                              const QString &savePath,
                              int threadCount = 8,
                              QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HTTPDownloadEngine() override;

    // 实现 IDownloadEngine 接口
    void start() override;
    void pause() override;
    void resume() override;
    qint64 getDownloadedBytes() const override;
    qint64 getTotalBytes() const override;
    double getSpeed() const override;

    /**
     * @brief 获取下载 URL
     * @return 下载链接
     */
    QString getUrl() const { return m_url; }

    /**
     * @brief 获取保存路径
     * @return 保存路径
     */
    QString getSavePath() const { return m_savePath; }

    /**
     * @brief 检查是否所有分块都已完成
     * @return 是否全部完成
     */
    bool isAllChunksCompleted() const;

signals:
    /**
     * @brief 分块下载完成信号
     * @param chunkIndex 分块索引
     */
    void chunkFinished(int chunkIndex);

    /**
     * @brief 分块下载错误信号
     * @param chunkIndex 分块索引
     * @param errorMsg 错误信息
     */
    void chunkError(int chunkIndex, const QString &errorMsg);

    /**
     * @brief 服务器支持 Range 请求检测完成信号
     * @param supports 是否支持
     */
    void rangeSupportDetected(bool supports);

private slots:
    /**
     * @brief 处理 HEAD 请求响应
     * @param reply 网络响应
     * [需单元测试]
     */
    void onHeadRequestFinished(QNetworkReply *reply);

    /**
     * @brief 处理分块下载完成
     * @param chunkIndex 分块索引
     * [需单元测试]
     */
    void onChunkFinished(int chunkIndex);

    /**
     * @brief 处理分块下载错误
     * @param chunkIndex 分块索引
     * @param errorMsg 错误信息
     * [需单元测试]
     */
    void onChunkError(int chunkIndex, const QString &errorMsg);

    /**
     * @brief 处理分块进度更新
     * @param chunkIndex 分块索引
     * @param downloaded 已下载字节数
     */
    void onChunkProgressUpdated(int chunkIndex, qint64 downloaded);

    /**
     * @brief 处理分块速度更新
     * @param chunkIndex 分块索引
     * @param speed 速度（字节/秒）
     */
    void onChunkSpeedUpdated(int chunkIndex, qint64 speed);

    /**
     * @brief 自适应线程数调整
     * @param newCount 新的线程数
     */
    void onThreadCountChanged(int newCount);

    /**
     * @brief 速度计算定时器回调
     */
    void onSpeedTimer();

private:
    /**
     * @brief 检测服务器是否支持 Range 请求
     * [需单元测试]
     */
    void detectRangeSupport();

    /**
     * @brief 计算分块策略
     * @param fileSize 文件总大小
     * @param threadCount 线程数
     * @return 分块列表
     * [需单元测试]
     */
    QList<ChunkInfo> calculateChunks(qint64 fileSize, int threadCount);

    /**
     * @brief 启动分块下载
     * [需单元测试]
     */
    void startChunkDownloads();

    /**
     * @brief 合并分块文件
     * [需单元测试]
     */
    void mergeChunks();

    /**
     * @brief 完整性校验
     * [需单元测试]
     */
    void verifyIntegrity();

    /**
     * @brief 清理分块下载器
     */
    void cleanupChunkDownloaders();

    // 成员变量
    QString m_url;                          // 下载链接
    QString m_savePath;                     // 保存路径
    int m_threadCount;                      // 线程数
    qint64 m_totalBytes;                   // 文件总大小
    qint64 m_downloadedBytes;              // 已下载字节数
    bool m_supportsRange;                  // 是否支持 Range 请求
    QList<ChunkInfo> m_chunks;            // 分块列表
    QList<ChunkDownloader*> m_chunkDownloaders;  // 分块下载器列表
    AdaptiveThreadController *m_controller;      // 自适应线程控制器
    QNetworkAccessManager *m_networkManager;     // 网络管理器

    // 速度统计
    qint64 m_currentSpeed;                 // 当前总速度（字节/秒）
    QTimer *m_speedTimer;                  // 速度计算定时器

    // 状态
    bool m_isRunning;                      // 是否正在运行
    bool m_isPaused;                       // 是否已暂停
    bool m_isMerging;                      // 是否正在合并文件
};

#endif // HTTPDOWNLOADENGINE_H
