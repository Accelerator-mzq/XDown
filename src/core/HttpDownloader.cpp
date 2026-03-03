/**
 * @file HttpDownloader.cpp
 * @brief HTTP 下载器实现
 * @author XDown
 * @date 2024
 *
 * 修复内容:
 * - 使用缓冲区循环读写，避免内存溢出
 * - 线程安全设计
 * - 断点续传检测 (206 状态码)
 * - HTTP 重定向处理
 */

#include "HttpDownloader.h"
#include <QNetworkRequest>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QTimer>
#include <QStorageInfo>

HttpDownloader::HttpDownloader(const DownloadTask& task, QObject* parent)
    : QObject(parent)
    , m_task(task)
    , m_thread(nullptr)
    , m_netManager(nullptr)
    , m_reply(nullptr)
    , m_file(nullptr)
    , m_lastDownloadedBytes(0)
    , m_lastSpeedTime(0)
    , m_currentSpeed(0)
    , m_isRedirecting(false)
    , m_supportResume(true)
    , m_redirectCount(0)
    , m_speedTimer(nullptr)
{
    // 创建工作线程 (不设置 parent，否则无法 moveToThread)
    m_thread = new QThread();
    moveToThread(m_thread);

    // 在工作线程中初始化网络管理器和定时器
    connect(m_thread, &QThread::started, this, &HttpDownloader::initNetworkManager);

    // 启动线程
    m_thread->start();
}

HttpDownloader::~HttpDownloader() {
    stop();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait(3000);
        if (m_thread->isRunning()) {
            m_thread->terminate();
        }
        delete m_thread;
    }
    delete m_file;
    delete m_netManager;
}

void HttpDownloader::initNetworkManager() {
    qDebug() << "HttpDownloader::initNetworkManager called, thread:" << QThread::currentThreadId();
    // 在工作线程中创建网络管理器
    m_netManager = new QNetworkAccessManager(this);
    qDebug() << "QNetworkAccessManager created";

    // 创建速度计算定时器 (每秒触发一次)
    m_speedTimer = new QTimer(this);
    m_speedTimer->setInterval(1000);
    connect(m_speedTimer, &QTimer::timeout, this, &HttpDownloader::onSpeedTimer);

    // 处理重定向
    connect(m_netManager, &QNetworkAccessManager::finished,
            this, &HttpDownloader::onFinished);

    qDebug() << "initNetworkManager completed";
}

void HttpDownloader::start() {
    qDebug() << "HttpDownloader::start() called for task:" << m_task.id;
    QMetaObject::invokeMethod(this, [this]() {
        qDebug() << "HttpDownloader::start() executing in thread:" << QThread::currentThreadId();
        if (m_reply) {
            qWarning() << "Task" << m_task.id << "is already downloading";
            return;
        }

        // 重置速度计算变量
        m_lastDownloadedBytes = m_task.downloadedBytes;
        m_lastSpeedTime = QDateTime::currentMSecsSinceEpoch();
        m_currentSpeed = 0;

        // 检查磁盘空间 (如果知道文件大小)
        if (m_task.totalBytes > 0 && !checkDiskSpace(m_task.totalBytes - m_task.downloadedBytes)) {
            m_task.status = TaskStatus::Error;
            m_task.errorMessage = "磁盘空间不足";
            emit statusChanged(m_task.id, m_task.status, m_task.errorMessage);
            return;
        }

        // 打开文件 (追加模式支持断点续传)
        m_file = new QFile(m_task.localPath);
        QIODevice::OpenMode mode = QIODevice::WriteOnly;
        if (m_task.downloadedBytes > 0) {
            mode |= QIODevice::Append;
        } else {
            mode |= QIODevice::Truncate;
        }

        if (!m_file->open(mode)) {
            m_task.status = TaskStatus::Error;
            m_task.errorMessage = "无法创建文件: " + m_file->errorString();
            emit statusChanged(m_task.id, m_task.status, m_task.errorMessage);
            delete m_file;
            m_file = nullptr;
            return;
        }

        // 创建网络请求
        QNetworkRequest request(QUrl(m_task.url));

        // 设置 User-Agent
        request.setHeader(QNetworkRequest::UserAgentHeader,
                         "XDown/1.0 (Windows) Mozilla/5.0");

        // 断点续传: 设置 Range 请求头
        if (m_task.downloadedBytes > 0) {
            QByteArray rangeHeader = "bytes=" + QByteArray::number(m_task.downloadedBytes) + "-";
            request.setRawHeader("Range", rangeHeader);
            qInfo() << "Resume download from byte:" << m_task.downloadedBytes;
        }

        // 发起异步 GET 请求
        qDebug() << "Creating GET request for URL:" << m_task.url;
        qDebug() << "m_netManager is valid:" << (m_netManager != nullptr);
        m_reply = m_netManager->get(request);
        m_isRedirecting = false;
        qDebug() << "m_reply created:" << (m_reply != nullptr);

        // 连接信号槽
        connect(m_reply, &QNetworkReply::readyRead, this, &HttpDownloader::onReadyRead);
        connect(m_reply, &QNetworkReply::finished, this, &HttpDownloader::onFinished);
        connect(m_reply, &QNetworkReply::errorOccurred, this, &HttpDownloader::onErrorOccurred);
        connect(m_reply, &QNetworkReply::redirected, this, &HttpDownloader::onRedirected);
        qDebug() << "Signals connected";

        // 启动速度计算定时器
        m_lastDownloadedBytes = m_task.downloadedBytes;
        m_lastSpeedTime = QDateTime::currentMSecsSinceEpoch();
        m_speedTimer->start();

        // 更新状态
        m_task.status = TaskStatus::Downloading;
        emit statusChanged(m_task.id, m_task.status);
    });
}

void HttpDownloader::pause() {
    QMetaObject::invokeMethod(this, [this]() {
        if (m_reply) {
            m_reply->abort();
            m_reply->deleteLater();
            m_reply = nullptr;
        }

        if (m_file && m_file->isOpen()) {
            m_file->flush();
            m_file->close();
        }

        m_speedTimer->stop();
        m_task.status = TaskStatus::Paused;
        emit statusChanged(m_task.id, m_task.status);
    });
}

void HttpDownloader::stop() {
    // 使用 Qt::BlockingQueuedConnection 确保同步执行
    QMetaObject::invokeMethod(this, [this]() {
        pause();

        if (m_file) {
            delete m_file;
            m_file = nullptr;
        }

        // 停止速度计算定时器
        if (m_speedTimer) {
            m_speedTimer->stop();
        }
    }, Qt::BlockingQueuedConnection);
}

void HttpDownloader::onReadyRead() {
    qDebug() << "HttpDownloader::onReadyRead called, bytesAvailable:" << m_reply->bytesAvailable();
    if (!m_reply || !m_file) {
        qDebug() << "onReadyRead: m_reply or m_file is null";
        return;
    }

    // ========== 关键修复: 使用固定缓冲区循环读取，避免内存溢出 ==========
    // 不再使用 readAll() 一次性读取所有数据
    qDebug() << "Starting to read data...";
    while (m_reply->bytesAvailable() > 0) {
        QByteArray buffer = m_reply->read(BUFFER_SIZE);
        if (buffer.isEmpty()) break;

        m_file->write(buffer);
        m_task.downloadedBytes += buffer.size();

        // 首次获取文件总大小
        if (m_task.totalBytes == 0) {
            // 从 Content-Length header 获取
            if (m_reply->hasRawHeader("Content-Length")) {
                qint64 contentLength = m_reply->rawHeader("Content-Length").toLongLong();
                // 如果是断点续传，需要加上已下载的部分
                if (m_task.downloadedBytes > 0 && !m_isRedirecting) {
                    m_task.totalBytes = contentLength + m_task.downloadedBytes;
                } else {
                    m_task.totalBytes = contentLength;
                }
            }
        }

        // 如果正在重定向，不发送进度更新
        if (!m_isRedirecting) {
            emit progressUpdated(m_task.id, m_task.downloadedBytes,
                               m_task.totalBytes, m_currentSpeed);
        }
    }

    // 定期刷新文件到磁盘
    if (m_file) {
        m_file->flush();
    }
}

void HttpDownloader::onFinished() {
    qDebug() << "HttpDownloader::onFinished called";
    if (!m_reply) {
        qDebug() << "onFinished: m_reply is null";
        return;
    }

    // 检查是否是重定向响应
    QUrl redirectUrl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    qDebug() << "Redirect URL:" << redirectUrl << "m_isRedirecting:" << m_isRedirecting;
    if (redirectUrl.isValid() && !m_isRedirecting) {
        // 由 onRedirected 处理
        qDebug() << "Redirect detected, let onRedirected handle it";
        return;
    }

    m_speedTimer->stop();

    // 检查 HTTP 状态码
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP status code:" << statusCode << "downloadedBytes:" << m_task.downloadedBytes;

    // ========== 断点续传检测: 检查服务器是否返回 206 ==========
    if (statusCode == 206) {
        // 206 Partial Content - 服务器支持断点续传
        m_supportResume = true;
        qInfo() << "Server supports resume (206)";
    } else if (statusCode == 200 && m_task.downloadedBytes > 0 && !m_isRedirecting) {
        // 200 OK - 服务器不支持断点续传，但文件已经下载完成了
        // 不需要重新下载，直接完成即可
        qWarning() << "Server does not support resume, but download completed";
        m_supportResume = false;
    }

    // 检查是否有错误
    if (m_reply->error() != QNetworkReply::NoError) {
        m_task.status = TaskStatus::Error;
        m_task.errorMessage = m_reply->errorString();
        emit statusChanged(m_task.id, m_task.status, m_task.errorMessage);
    } else {
        // 下载完成
        m_task.status = TaskStatus::Finished;
        // 只在未获取到文件大小时才使用已下载的大小
        if (m_task.totalBytes == 0) {
            m_task.totalBytes = m_task.downloadedBytes;
        }
        m_task.speedBytesPerSec = 0;

        if (m_file && m_file->isOpen()) {
            m_file->flush();
            m_file->close();
        }

        emit statusChanged(m_task.id, m_task.status);
        emit downloadFinished(m_task.id, m_task.localPath);
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void HttpDownloader::onErrorOccurred(QNetworkReply::NetworkError code) {
    qWarning() << "HttpDownloader::onErrorOccurred called, error code:" << code;
    qWarning() << "Network error for task" << m_task.id << ":" << code;

    m_speedTimer->stop();

    // 转换错误码为友好消息
    QString errorMsg;
    switch (code) {
        case QNetworkReply::ConnectionRefusedError:
            errorMsg = "连接被拒绝";
            break;
        case QNetworkReply::RemoteHostClosedError:
            errorMsg = "远程主机关闭连接";
            break;
        case QNetworkReply::HostNotFoundError:
            errorMsg = "找不到主机";
            break;
        case QNetworkReply::TimeoutError:
            errorMsg = "连接超时";
            break;
        case QNetworkReply::OperationCanceledError:
            // 用户暂停，不算错误
            return;
        case QNetworkReply::ContentNotFoundError:  // 404
            errorMsg = "资源不存在 (404)";
            break;
        case QNetworkReply::ContentAccessDenied:  // 403
            errorMsg = "无权限访问 (403)";
            break;
        default:
            errorMsg = m_reply ? m_reply->errorString() : "未知网络错误";
            break;
    }

    m_task.status = TaskStatus::Error;
    m_task.errorMessage = errorMsg;
    emit statusChanged(m_task.id, m_task.status, errorMsg);
}

void HttpDownloader::onRedirected(const QUrl& url) {
    qDebug() << "HttpDownloader::onRedirected called, redirect to:" << url;
    m_redirectCount++;

    if (m_redirectCount > MAX_REDIRECTS) {
        m_task.status = TaskStatus::Error;
        m_task.errorMessage = "重定向次数过多";
        emit statusChanged(m_task.id, m_task.status, m_task.errorMessage);
        m_reply->abort();
        return;
    }

    // 暂停当前下载，更新 URL 后继续
    m_isRedirecting = true;
    m_task.downloadedBytes = 0;  // 重置下载进度

    if (m_file && m_file->isOpen()) {
        m_file->close();
        m_file->remove();  // 删除不完整的文件
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    // 更新 URL 并重新开始
    m_task.url = url.toString();
    qInfo() << "Redirected to:" << url.toString();

    // 重新解析文件名
    parseFileName(url);

    // 重新开始下载
    start();
}

void HttpDownloader::onSpeedTimer() {
    qDebug() << "HttpDownloader::onSpeedTimer called";
    updateSpeedStatistic();
}

bool HttpDownloader::checkResumeSupport() {
    // 通过检查响应码判断服务器是否支持断点续传
    // 206 = 支持, 200 = 不支持
    // 这个方法在第一次请求后被调用
    return m_supportResume;
}

bool HttpDownloader::checkDiskSpace(qint64 requiredBytes) const {
    if (requiredBytes <= 0) return true;

    QString path = m_task.localPath;
    QFileInfo info(path);
    QStorageInfo storage(info.absolutePath());

    qint64 availableBytes = storage.bytesAvailable();
    qWarning() << "Required:" << requiredBytes << "Available:" << availableBytes;

    // 保留 100MB 的缓冲空间
    const qint64 BUFFER = 100 * 1024 * 1024;
    return (availableBytes - BUFFER) > requiredBytes;
}

void HttpDownloader::parseFileName(const QUrl& redirectUrl) {
    QString name;

    if (redirectUrl.isValid()) {
        // 从重定向 URL 解析
        name = redirectUrl.fileName();
    }

    if (name.isEmpty()) {
        // 从本地路径获取
        name = QFileInfo(m_task.localPath).fileName();
    }

    if (name.isEmpty()) {
        // 从 URL 解析
        QUrl url(m_task.url);
        name = url.fileName();
    }

    if (!name.isEmpty()) {
        m_task.fileName = name;
        // 如果本地路径的文件名不同，更新本地路径
        QString newPath = QFileInfo(m_task.localPath).absolutePath() + "/" + name;
        if (newPath != m_task.localPath) {
            m_task.localPath = newPath;
        }
    }
}

void HttpDownloader::updateSpeedStatistic() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDiff = now - m_lastSpeedTime;

    if (timeDiff > 0) {
        qint64 bytesDiff = m_task.downloadedBytes - m_lastDownloadedBytes;
        // 计算速度 (字节/秒)
        m_currentSpeed = static_cast<qint64>((bytesDiff * 1000.0) / timeDiff);
        m_task.speedBytesPerSec = m_currentSpeed;
    }

    m_lastDownloadedBytes = m_task.downloadedBytes;
    m_lastSpeedTime = now;

    // 发送速度更新
    emit progressUpdated(m_task.id, m_task.downloadedBytes,
                       m_task.totalBytes, m_currentSpeed);
}
