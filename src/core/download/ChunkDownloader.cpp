/**
 * @file ChunkDownloader.cpp
 * @brief 单个分块下载器实现
 * @author XDown
 * @date 2026-03-08
 */

#include "ChunkDownloader.h"
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QNetworkRequest>
#include <QFileInfo>

/******************************************************
 * @brief 构造函数
 * @param url 下载链接
 * @param chunkInfo 分块信息
 * @param tempFilePath 临时文件路径
 * @param parent 父对象
 ******************************************************/
ChunkDownloader::ChunkDownloader(const QString &url,
                                 const ChunkInfo &chunkInfo,
                                 const QString &tempFilePath,
                                 QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_chunkInfo(chunkInfo)
    , m_tempFilePath(tempFilePath)
    , m_downloadedBytes(0)
    , m_networkManager(nullptr)
    , m_reply(nullptr)
    , m_file(nullptr)
    , m_retryCount(0)
    , m_isDownloading(false)
    , m_isPaused(false)
    , m_lastDownloadedBytes(0)
    , m_lastSpeedTime(0)
    , m_currentSpeed(0)
{
    // 设置已下载字节数为当前进度（用于断点续传）
    m_downloadedBytes = m_chunkInfo.downloadedBytes;
}

/******************************************************
 * @brief 析构函数
 ******************************************************/
ChunkDownloader::~ChunkDownloader()
{
    // 停止下载
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    // 关闭文件
    if (m_file) {
        if (m_file->isOpen()) {
            m_file->close();
        }
        delete m_file;
        m_file = nullptr;
    }

    // 删除网络管理器
    if (m_networkManager) {
        m_networkManager->deleteLater();
        m_networkManager = nullptr;
    }
}

/******************************************************
 * @brief 启动下载
 * @note 线程安全：必须在工作线程中调用
 ******************************************************/
void ChunkDownloader::start()
{
    if (m_isDownloading) {
        return; // 已在下载中
    }

    m_isDownloading = true;
    m_isPaused = false;
    m_retryCount = 0;

    // 初始化网络管理器（在工作线程中创建）
    initNetworkManager();

    // 打开临时文件（追加模式，支持断点续传）
    m_file = new QFile(m_tempFilePath);
    if (!m_file->open(QIODevice::Append)) {
        handleError(QString("无法打开临时文件: %1").arg(m_tempFilePath));
        return;
    }

    // 发送下载请求
    sendRequest();
}

/******************************************************
 * @brief 暂停下载
 * @note 保存当前进度到分块信息中
 ******************************************************/
void ChunkDownloader::pause()
{
    if (!m_isDownloading || m_isPaused) {
        return;
    }

    m_isPaused = true;

    // 暂停网络请求
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    // 更新分块信息中的已下载字节数
    m_chunkInfo.downloadedBytes = m_downloadedBytes;

    // 关闭文件
    if (m_file && m_file->isOpen()) {
        m_file->flush();
        m_file->close();
    }
}

/******************************************************
 * @brief 恢复下载
 * @note 从已下载位置继续下载
 ******************************************************/
void ChunkDownloader::resume()
{
    if (!m_isPaused) {
        return; // 未暂停，无需恢复
    }

    m_isPaused = false;

    // 重新打开文件
    if (m_file) {
        if (!m_file->open(QIODevice::Append)) {
            handleError(QString("无法打开临时文件: %1").arg(m_tempFilePath));
            return;
        }
    }

    // 发送下载请求（带 Range 头）
    sendRequest();
}

/******************************************************
 * @brief 获取已下载字节数
 * @return 已下载字节数
 ******************************************************/
qint64 ChunkDownloader::getDownloadedBytes() const
{
    return m_downloadedBytes;
}

/******************************************************
 * @brief 初始化网络管理器
 * @note 必须在工作线程中调用
 ******************************************************/
void ChunkDownloader::initNetworkManager()
{
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);

        // 连接错误信号
        connect(m_networkManager, &QNetworkAccessManager::finished,
                this, &ChunkDownloader::onFinished);
    }
}

/******************************************************
 * @brief 发送下载请求
 * @note 带 Range 头实现断点续传
 ******************************************************/
void ChunkDownloader::sendRequest()
{
    if (!m_networkManager) {
        handleError("网络管理器未初始化");
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(m_url));

    // 设置 Range 头，实现断点续传
    // 从已下载位置开始下载
    qint64 startPos = m_chunkInfo.startPos + m_downloadedBytes;
    QString rangeHeader = QString("bytes=%1-%2")
        .arg(startPos)
        .arg(m_chunkInfo.endPos);
    request.setRawHeader("Range", rangeHeader.toUtf8());

    // 设置 User-Agent
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "XDown/2.0 (Windows)");

    // 发送请求
    m_reply = m_networkManager->get(request);

    // 连接信号
    connect(m_reply, &QNetworkReply::readyRead,
            this, &ChunkDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &ChunkDownloader::onErrorOccurred);

    // 重置速度计算
    m_lastDownloadedBytes = m_downloadedBytes;
    m_lastSpeedTime = QDateTime::currentMSecsSinceEpoch();
}

/******************************************************
 * @brief 处理网络响应数据
 ******************************************************/
void ChunkDownloader::onReadyRead()
{
    if (!m_reply || !m_file) {
        return;
    }

    // 读取数据并写入文件
    QByteArray data = m_reply->read(BUFFER_SIZE);
    if (!data.isEmpty()) {
        qint64 written = m_file->write(data);
        m_downloadedBytes += written;

        // 更新分块信息
        m_chunkInfo.downloadedBytes = m_downloadedBytes;

        // 发出进度更新信号
        emit progressUpdated(m_chunkInfo.index, m_downloadedBytes);
    }
}

/******************************************************
 * @brief 处理下载完成
 ******************************************************/
void ChunkDownloader::onFinished()
{
    if (!m_reply) {
        return;
    }

    // 检查 HTTP 状态码
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // 206 表示部分内容下载成功（分块下载正常）
    // 200 表示服务器不支持 Range，整体下载完成
    if (statusCode == 206 || statusCode == 200) {
        // 关闭文件
        if (m_file && m_file->isOpen()) {
            m_file->flush();
            m_file->close();
        }

        // 更新分块状态
        m_chunkInfo.isCompleted = true;
        m_chunkInfo.downloadedBytes = m_downloadedBytes;

        m_isDownloading = false;

        // 发出完成信号
        emit finished(m_chunkInfo.index);
    } else if (statusCode >= 400) {
        // HTTP 错误
        QString errorMsg = QString("HTTP 错误: %1").arg(statusCode);
        handleError(errorMsg);
    }
}

/******************************************************
 * @brief 处理网络错误
 * @param code 网络错误码
 ******************************************************/
void ChunkDownloader::onErrorOccurred(QNetworkReply::NetworkError code)
{
    // 忽略用户取消错误
    if (code == QNetworkReply::OperationCanceledError) {
        return;
    }

    QString errorMsg = m_reply ? m_reply->errorString() : "未知网络错误";
    handleError(errorMsg);
}

/******************************************************
 * @brief 速度计算定时器回调
 ******************************************************/
void ChunkDownloader::onSpeedTimer()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDiff = now - m_lastSpeedTime;

    if (timeDiff > 0) {
        qint64 bytesDiff = m_downloadedBytes - m_lastDownloadedBytes;
        m_currentSpeed = (bytesDiff * 1000) / timeDiff;

        // 更新统计
        m_lastDownloadedBytes = m_downloadedBytes;
        m_lastSpeedTime = now;

        // 发出速度更新信号
        emit speedUpdated(m_chunkInfo.index, m_currentSpeed);
    }
}

/******************************************************
 * @brief 处理下载失败
 * @param errorMsg 错误信息
 ******************************************************/
void ChunkDownloader::handleError(const QString &errorMsg)
{
    m_isDownloading = false;

    // 标记分块失败
    m_chunkInfo.isFailed = true;

    // 关闭文件
    if (m_file && m_file->isOpen()) {
        m_file->close();
    }

    // 重试逻辑
    if (m_retryCount < MAX_RETRY_COUNT) {
        m_retryCount++;
        // 延迟重试
        QTimer::singleShot(1000 * m_retryCount, this, &ChunkDownloader::retryDownload);
    } else {
        // 发出错误信号
        emit error(m_chunkInfo.index, errorMsg);
    }
}

/******************************************************
 * @brief 重试下载
 ******************************************************/
void ChunkDownloader::retryDownload()
{
    if (m_isPaused) {
        return; // 如果已暂停，不重试
    }

    m_isDownloading = true;

    // 重新打开文件
    if (m_file) {
        if (!m_file->open(QIODevice::Append)) {
            handleError(QString("无法打开临时文件: %1").arg(m_tempFilePath));
            return;
        }
    }

    // 重新发送请求
    sendRequest();
}
