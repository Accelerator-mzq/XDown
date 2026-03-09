/**
 * @file HTTPDownloadEngine.cpp
 * @brief HTTP/HTTPS 多线程分块下载引擎实现
 * @author XDown
 * @date 2026-03-08
 */

#include "HTTPDownloadEngine.h"
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QNetworkRequest>
#include <QFileInfo>
#include <QCryptographicHash>

/******************************************************
 * @brief 构造函数
 * @param url 下载链接
 * @param savePath 保存路径
 * @param threadCount 线程数
 * @param parent 父对象
 ******************************************************/
HTTPDownloadEngine::HTTPDownloadEngine(const QString &url,
                                     const QString &savePath,
                                     int threadCount,
                                     QObject *parent)
    : IDownloadEngine(parent)
    , m_url(url)
    , m_savePath(savePath)
    , m_threadCount(threadCount)
    , m_totalBytes(0)
    , m_downloadedBytes(0)
    , m_supportsRange(false)
    , m_controller(nullptr)
    , m_networkManager(nullptr)
    , m_currentSpeed(0)
    , m_speedTimer(nullptr)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_isMerging(false)
{
    // 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);

    // 初始化自适应线程控制器
    m_controller = new AdaptiveThreadController(4, 16, this);

    // 连接线程数调整信号
    connect(m_controller, &AdaptiveThreadController::threadCountChanged,
            this, &HTTPDownloadEngine::onThreadCountChanged);
}

/******************************************************
 * @brief 析构函数
 ******************************************************/
HTTPDownloadEngine::~HTTPDownloadEngine()
{
    // 停止所有下载
    pause();

    // 清理分块下载器
    cleanupChunkDownloaders();
}

/******************************************************
 * @brief 启动下载
 ******************************************************/
void HTTPDownloadEngine::start()
{
    if (m_isRunning) {
        return;
    }

    m_isRunning = true;
    m_isPaused = false;

    // 首先发送 HEAD 请求获取文件大小
    detectRangeSupport();
}

/******************************************************
 * @brief 暂停下载
 * @note 保存当前进度到数据库
 ******************************************************/
void HTTPDownloadEngine::pause()
{
    if (!m_isRunning || m_isPaused) {
        return;
    }

    m_isPaused = true;

    // 暂停所有分块下载器
    for (ChunkDownloader *downloader : m_chunkDownloaders) {
        if (downloader) {
            downloader->pause();
        }
    }

    // 停止速度计算定时器
    if (m_speedTimer) {
        m_speedTimer->stop();
    }
}

/******************************************************
 * @brief 恢复下载
 * @note 从数据库读取进度并继续
 ******************************************************/
void HTTPDownloadEngine::resume()
{
    if (!m_isRunning || !m_isPaused) {
        return;
    }

    m_isPaused = false;

    // 恢复所有分块下载器
    for (ChunkDownloader *downloader : m_chunkDownloaders) {
        if (downloader && downloader->isDownloading()) {
            downloader->resume();
        }
    }

    // 重启速度计算定时器
    if (m_speedTimer) {
        m_speedTimer->start(1000);
    }
}

/******************************************************
 * @brief 获取已下载字节数
 * @return 已下载字节数
 ******************************************************/
qint64 HTTPDownloadEngine::getDownloadedBytes() const
{
    return m_downloadedBytes;
}

/******************************************************
 * @brief 获取文件总大小
 * @return 文件总大小（字节）
 ******************************************************/
qint64 HTTPDownloadEngine::getTotalBytes() const
{
    return m_totalBytes;
}

/******************************************************
 * @brief 获取当前下载速度
 * @return 下载速度（字节/秒）
 ******************************************************/
double HTTPDownloadEngine::getSpeed() const
{
    return static_cast<double>(m_currentSpeed);
}

/******************************************************
 * @brief 检查是否所有分块都已完成
 * @return 是否全部完成
 ******************************************************/
bool HTTPDownloadEngine::isAllChunksCompleted() const
{
    for (const ChunkInfo &chunk : m_chunks) {
        if (!chunk.isCompleted) {
            return false;
        }
    }
    return true;
}

/******************************************************
 * @brief 检测服务器是否支持 Range 请求
 ******************************************************/
void HTTPDownloadEngine::detectRangeSupport()
{
    QNetworkRequest request;
    request.setUrl(QUrl(m_url));
    request.setHeader(QNetworkRequest::UserAgentHeader, "XDown/2.0 (Windows)");

    // 发送 HEAD 请求
    QNetworkReply *reply = m_networkManager->head(request);

    // 连接响应信号（使用 lambda 捕获 reply）
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() {
        onHeadRequestFinished(reply);
    });
}

/******************************************************
 * @brief 处理 HEAD 请求响应
 * @param reply 网络响应
 ******************************************************/
void HTTPDownloadEngine::onHeadRequestFinished(QNetworkReply *reply)
{
    // 获取文件大小
    m_totalBytes = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();

    // 检查是否支持 Range 请求
    QString acceptRanges = reply->rawHeader("Accept-Ranges");
    m_supportsRange = (acceptRanges.toLower() == "bytes");

    // 发出 Range 支持检测完成信号
    emit rangeSupportDetected(m_supportsRange);

    // 如果不支持 Range，回退到单线程下载
    if (!m_supportsRange) {
        m_threadCount = 1;
    }

    // 计算分块策略
    if (m_totalBytes > 0) {
        m_chunks = calculateChunks(m_totalBytes, m_threadCount);

        // 启动分块下载
        startChunkDownloads();
    } else {
        // 无法获取文件大小，发送错误信号
        emit error("无法获取文件大小");
    }

    reply->deleteLater();
}

/******************************************************
 * @brief 计算分块策略
 * @param fileSize 文件总大小
 * @param threadCount 线程数
 * @return 分块列表
 ******************************************************/
QList<ChunkInfo> HTTPDownloadEngine::calculateChunks(qint64 fileSize, int threadCount)
{
    QList<ChunkInfo> chunks;

    // 每个分块的大小
    qint64 chunkSize = fileSize / threadCount;

    // 避免分块太小
    if (chunkSize < 1024 * 1024) {  // 最小 1MB
        chunkSize = fileSize;
        threadCount = 1;
    }

    for (int i = 0; i < threadCount; ++i) {
        ChunkInfo chunk;
        chunk.index = i;
        chunk.startPos = i * chunkSize;

        // 最后一个分块包含剩余所有字节
        if (i == threadCount - 1) {
            chunk.endPos = fileSize - 1;
        } else {
            chunk.endPos = (i + 1) * chunkSize - 1;
        }

        chunk.downloadedBytes = 0;
        chunk.isCompleted = false;
        chunk.isFailed = false;

        // 生成临时文件路径
        QString tempPath = m_savePath + QString(".part%1").arg(i);
        chunk.tempFilePath = tempPath;

        chunks.append(chunk);
    }

    return chunks;
}

/******************************************************
 * @brief 启动分块下载
 ******************************************************/
void HTTPDownloadEngine::startChunkDownloads()
{
    // 清理旧的分块下载器
    cleanupChunkDownloaders();

    // 创建新的分块下载器
    for (const ChunkInfo &chunkInfo : m_chunks) {
        ChunkDownloader *downloader = new ChunkDownloader(
            m_url,
            chunkInfo,
            chunkInfo.tempFilePath,
            this
        );

        // 连接信号
        connect(downloader, &ChunkDownloader::finished,
                this, &HTTPDownloadEngine::onChunkFinished);
        connect(downloader, &ChunkDownloader::error,
                this, &HTTPDownloadEngine::onChunkError);
        connect(downloader, &ChunkDownloader::progressUpdated,
                this, &HTTPDownloadEngine::onChunkProgressUpdated);
        connect(downloader, &ChunkDownloader::speedUpdated,
                this, &HTTPDownloadEngine::onChunkSpeedUpdated);

        m_chunkDownloaders.append(downloader);
    }

    // 启动所有分块下载
    for (ChunkDownloader *downloader : m_chunkDownloaders) {
        downloader->start();
    }

    // 启动速度计算定时器
    if (!m_speedTimer) {
        m_speedTimer = new QTimer(this);
        connect(m_speedTimer, &QTimer::timeout,
                this, &HTTPDownloadEngine::onSpeedTimer);
    }
    m_speedTimer->start(1000);
}

/******************************************************
 * @brief 处理分块下载完成
 * @param chunkIndex 分块索引
 ******************************************************/
void HTTPDownloadEngine::onChunkFinished(int chunkIndex)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return;
    }

    // 更新分块状态
    m_chunks[chunkIndex].isCompleted = true;

    // 发出分块完成信号
    emit chunkFinished(chunkIndex);

    // 检查是否所有分块都已完成
    if (isAllChunksCompleted() && !m_isMerging) {
        m_isMerging = true;

        // 合并分块文件
        mergeChunks();
    }
}

/******************************************************
 * @brief 处理分块下载错误
 * @param chunkIndex 分块索引
 * @param errorMsg 错误信息
 ******************************************************/
void HTTPDownloadEngine::onChunkError(int chunkIndex, const QString &errorMsg)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return;
    }

    // 更新分块状态
    m_chunks[chunkIndex].isFailed = true;

    // 发出分块错误信号
    emit chunkError(chunkIndex, errorMsg);

    // 如果有分块失败，发出错误信号
    emit error(QString("分块 %1 下载失败: %2").arg(chunkIndex).arg(errorMsg));
}

/******************************************************
 * @brief 处理分块进度更新
 * @param chunkIndex 分块索引
 * @param downloaded 已下载字节数
 ******************************************************/
void HTTPDownloadEngine::onChunkProgressUpdated(int chunkIndex, qint64 downloaded)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return;
    }

    // 更新分块的已下载字节数
    m_chunks[chunkIndex].downloadedBytes = downloaded;

    // 计算总已下载字节数
    m_downloadedBytes = 0;
    for (const ChunkInfo &chunk : m_chunks) {
        m_downloadedBytes += chunk.downloadedBytes;
    }

    // 发出进度更新信号 (新签名: id, downloaded, total, speed)
    emit progressUpdated(m_url, m_downloadedBytes, m_totalBytes, static_cast<qint64>(m_currentSpeed));
}

/******************************************************
 * @brief 处理分块速度更新
 * @param chunkIndex 分块索引
 * @param speed 速度（字节/秒）
 ******************************************************/
void HTTPDownloadEngine::onChunkSpeedUpdated(int chunkIndex, qint64 speed)
{
    // 将速度样本添加到自适应控制器
    if (m_controller) {
        m_controller->addSpeedSample(speed);
    }
}

/******************************************************
 * @brief 自适应线程数调整
 * @param newCount 新的线程数
 ******************************************************/
void HTTPDownloadEngine::onThreadCountChanged(int newCount)
{
    // 可以在这里实现动态调整线程数的逻辑
    // 目前简单更新线程数
    m_threadCount = newCount;
}

/******************************************************
 * @brief 速度计算定时器回调
 ******************************************************/
void HTTPDownloadEngine::onSpeedTimer()
{
    // 计算所有分块的总速度
    qint64 totalSpeed = 0;
    for (ChunkDownloader *downloader : m_chunkDownloaders) {
        if (downloader && downloader->isDownloading()) {
            totalSpeed += downloader->getDownloadedBytes();
        }
    }

    m_currentSpeed = totalSpeed;

    // 发出速度更新信号
    emit speedUpdated(static_cast<double>(m_currentSpeed));
}

/******************************************************
 * @brief 合并分块文件
 ******************************************************/
void HTTPDownloadEngine::mergeChunks()
{
    // 创建最终文件
    QFile finalFile(m_savePath);
    if (!finalFile.open(QIODevice::WriteOnly)) {
        emit error(QString("无法创建文件: %1").arg(m_savePath));
        return;
    }

    // 依次写入每个分块
    for (const ChunkInfo &chunk : m_chunks) {
        QFile partFile(chunk.tempFilePath);
        if (!partFile.exists()) {
            emit error(QString("分块文件不存在: %1").arg(chunk.tempFilePath));
            finalFile.close();
            return;
        }

        if (!partFile.open(QIODevice::ReadOnly)) {
            emit error(QString("无法打开分块文件: %1").arg(chunk.tempFilePath));
            finalFile.close();
            return;
        }

        // 读取并写入数据
        const int bufferSize = 65536;
        char buffer[bufferSize];
        qint64 bytesRead = 0;
        while (!partFile.atEnd()) {
            bytesRead = partFile.read(buffer, bufferSize);
            if (bytesRead > 0) {
                finalFile.write(buffer, bytesRead);
            } else {
                break;
            }
        }

        partFile.close();

        // 删除临时分块文件
        partFile.remove();
    }

    finalFile.close();

    // 停止速度计算
    if (m_speedTimer) {
        m_speedTimer->stop();
    }

    m_currentSpeed = 0;

    // 发出完成信号
    emit finished();
}

/******************************************************
 * @brief 完整性校验
 ******************************************************/
void HTTPDownloadEngine::verifyIntegrity()
{
    // 目前可以添加 MD5/SHA1 校验功能
    // 需要从服务器获取文件的校验值
}

/******************************************************
 * @brief 清理分块下载器
 ******************************************************/
void HTTPDownloadEngine::cleanupChunkDownloaders()
{
    for (ChunkDownloader *downloader : m_chunkDownloaders) {
        if (downloader) {
            downloader->deleteLater();
        }
    }
    m_chunkDownloaders.clear();
}
