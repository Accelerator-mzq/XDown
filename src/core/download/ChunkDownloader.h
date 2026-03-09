/**
 * @file ChunkDownloader.h
 * @brief 单个分块下载器定义
 * @author XDown
 * @date 2026-03-08
 *
 * 负责下载文件的一个分块，运行在独立线程中。
 *
 * [需单元测试]
 */

#ifndef CHUNKDOWNLOADER_H
#define CHUNKDOWNLOADER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include "ChunkInfo.h"

/**
 * @class ChunkDownloader
 * @brief 单个分块下载器
 *
 * 负责下载文件的一个分块，运行在独立线程中。
 * 支持断点续传，通过 Range 请求实现分块下载。
 */
class ChunkDownloader : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param url 下载链接
     * @param chunkInfo 分块信息（起始位置、结束位置）
     * @param tempFilePath 临时文件路径
     * @param parent 父对象
     */
    explicit ChunkDownloader(const QString &url,
                            const ChunkInfo &chunkInfo,
                            const QString &tempFilePath,
                            QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ChunkDownloader();

    /**
     * @brief 启动下载
     * [需单元测试]
     */
    void start();

    /**
     * @brief 暂停下载
     * [需单元测试]
     */
    void pause();

    /**
     * @brief 恢复下载
     * @note 从已下载位置继续下载
     */
    void resume();

    /**
     * @brief 获取已下载字节数
     * @return 已下载字节数
     * [需单元测试]
     */
    qint64 getDownloadedBytes() const;

    /**
     * @brief 获取分块信息
     * @return 分块信息副本
     */
    ChunkInfo getChunkInfo() const { return m_chunkInfo; }

    /**
     * @brief 检查下载是否正在进行
     * @return 是否正在下载
     */
    bool isDownloading() const { return m_isDownloading; }

signals:
    /**
     * @brief 进度更新信号
     * @param chunkIndex 分块索引
     * @param downloaded 已下载字节数
     */
    void progressUpdated(int chunkIndex, qint64 downloaded);

    /**
     * @brief 下载完成信号
     * @param chunkIndex 分块索引
     */
    void finished(int chunkIndex);

    /**
     * @brief 错误信号
     * @param chunkIndex 分块索引
     * @param errorMsg 错误信息
     */
    void error(int chunkIndex, const QString &errorMsg);

    /**
     * @brief 速度更新信号
     * @param chunkIndex 分块索引
     * @param speed 速度（字节/秒）
     */
    void speedUpdated(int chunkIndex, qint64 speed);

private slots:
    /**
     * @brief 处理网络响应数据
     */
    void onReadyRead();

    /**
     * @brief 处理下载完成
     */
    void onFinished();

    /**
     * @brief 处理网络错误
     * @param code 网络错误码
     */
    void onErrorOccurred(QNetworkReply::NetworkError code);

    /**
     * @brief 速度计算定时器回调
     */
    void onSpeedTimer();

private:
    /**
     * @brief 初始化网络管理器
     */
    void initNetworkManager();

    /**
     * @brief 发送下载请求
     */
    void sendRequest();

    /**
     * @brief 处理下载失败
     * @param errorMsg 错误信息
     */
    void handleError(const QString &errorMsg);

    /**
     * @brief 重试下载
     */
    void retryDownload();

    // 成员变量
    QString m_url;                          // 下载链接
    ChunkInfo m_chunkInfo;                  // 分块信息
    QString m_tempFilePath;                 // 临时文件路径
    qint64 m_downloadedBytes;              // 已下载字节数
    QNetworkAccessManager *m_networkManager; // 网络管理器
    QNetworkReply *m_reply;                 // 网络响应
    QFile *m_file;                          // 临时文件
    int m_retryCount;                       // 重试次数
    bool m_isDownloading;                   // 是否正在下载
    bool m_isPaused;                        // 是否已暂停

    // 速度计算
    qint64 m_lastDownloadedBytes;           // 上次统计时的已下载字节数
    qint64 m_lastSpeedTime;                 // 上次统计时间
    qint64 m_currentSpeed;                  // 当前速度（字节/秒）

    static const int MAX_RETRY_COUNT = 3;   // 最大重试次数
    static const int BUFFER_SIZE = 65536;    // 64KB 缓冲区
};

#endif // CHUNKDOWNLOADER_H
