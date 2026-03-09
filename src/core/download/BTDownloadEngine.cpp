/**
 * @file BTDownloadEngine.cpp
 * @brief BT 下载引擎实现
 * @author XDown
 * @date 2024
 */

#include "BTDownloadEngine.h"
#include <QUrl>
#include <QUrlQuery>
#include <QFileInfo>

BTDownloadEngine::BTDownloadEngine(const QString& torrentPath,
                                   const QString& savePath,
                                   QObject* parent)
    : IDownloadEngine(parent)
    , m_torrentPath(torrentPath)
    , m_savePath(savePath)
    , m_totalBytes(0)
    , m_downloadedBytes(0)
    , m_uploadedBytes(0)
    , m_downloadSpeed(0)
    , m_uploadSpeed(0)
    , m_connectedPeers(0)
    , m_availableSeeds(0)
    , m_seedingTime(0)
    , m_status(TaskStatus::Waiting)
    , m_isSeeding(false)
    , m_isMetadataReady(false)
    , m_statusTimer(nullptr)
{
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &BTDownloadEngine::onStatusUpdated);

    // 解析种子或磁力链接
    if (torrentPath.toLower().startsWith("magnet:")) {
        parseMagnetUri(torrentPath);
    } else {
        parseTorrentFile(torrentPath);
    }

    qInfo() << "BTDownloadEngine created for:" << torrentPath;
}

BTDownloadEngine::~BTDownloadEngine() {
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    qInfo() << "BTDownloadEngine destroyed";
}

// ========== IDownloadEngine 接口实现 ==========

void BTDownloadEngine::start() {
    if (m_status == TaskStatus::Downloading) {
        return;
    }

    m_status = TaskStatus::Downloading;
    m_statusTimer->start(1000);  // 每秒更新状态

    // TODO: 启动 libtorrent 下载
    qInfo() << "BT download started:" << m_displayName;

    emit statusChanged(m_status);
}

void BTDownloadEngine::pause() {
    if (m_status != TaskStatus::Downloading) {
        return;
    }

    m_status = TaskStatus::Paused;
    m_statusTimer->stop();

    // TODO: 暂停 libtorrent 下载
    qInfo() << "BT download paused:" << m_displayName;

    emit statusChanged(m_status);
}

void BTDownloadEngine::resume() {
    if (m_status != TaskStatus::Paused) {
        return;
    }

    m_status = TaskStatus::Downloading;
    m_statusTimer->start(1000);

    // TODO: 恢复 libtorrent 下载
    qInfo() << "BT download resumed:" << m_displayName;

    emit statusChanged(m_status);
}

qint64 BTDownloadEngine::getDownloadedBytes() const {
    return m_downloadedBytes;
}

qint64 BTDownloadEngine::getTotalBytes() const {
    return m_totalBytes;
}

double BTDownloadEngine::getSpeed() const {
    return m_downloadSpeed;
}

double BTDownloadEngine::getUploadSpeed() const {
    return m_uploadSpeed;
}

TaskStatus BTDownloadEngine::getStatus() const {
    return m_status;
}

// ========== BT 特有接口 ==========

int BTDownloadEngine::getConnectedPeerCount() const {
    return m_connectedPeers;
}

int BTDownloadEngine::getAvailableSeedCount() const {
    return m_availableSeeds;
}

qint64 BTDownloadEngine::getSeedingTime() const {
    return m_seedingTime;
}

double BTDownloadEngine::getShareRatio() const {
    if (m_downloadedBytes == 0) {
        return 0.0;
    }
    return static_cast<double>(m_uploadedBytes) / m_downloadedBytes;
}

QList<TorrentFileInfo> BTDownloadEngine::getFileList() const {
    return m_fileList;
}

QString BTDownloadEngine::getInfoHash() const {
    return m_infoHash;
}

QString BTDownloadEngine::getDisplayName() const {
    return m_displayName;
}

void BTDownloadEngine::stopSeeding() {
    if (!m_isSeeding) {
        return;
    }

    m_isSeeding = false;
    m_seedingTime = 0;

    // TODO: 停止 libtorrent 做种
    qInfo() << "BT seeding stopped:" << m_displayName;

    emit seedingStatusChanged(false);
}

void BTDownloadEngine::setSeedingOptions(int maxUploadSpeed, int maxConnections) {
    // TODO: 设置 libtorrent 做种参数
    qInfo() << "BT seeding options set - maxUploadSpeed:" << maxUploadSpeed
            << "maxConnections:" << maxConnections;
}

// ========== 私有槽函数 ==========

void BTDownloadEngine::onStatusUpdated() {
    // TODO: 从 libtorrent 获取实际状态
    // 这里使用模拟数据

    // 更新信号
    emit peersUpdated(m_connectedPeers, m_availableSeeds);
    emit shareRatioUpdated(getShareRatio());

    // 发送进度更新
    emit progressUpdated(m_torrentPath, m_downloadedBytes, m_totalBytes,
                        static_cast<qint64>(m_downloadSpeed));

    // 检查是否下载完成
    if (m_downloadedBytes >= m_totalBytes && m_totalBytes > 0) {
        m_status = TaskStatus::Finished;
        m_statusTimer->stop();

        // 自动开始做种
        m_isSeeding = true;
        emit statusChanged(m_status);
        emit seedingStatusChanged(true);
    }
}

// ========== 私有函数 ==========

bool BTDownloadEngine::parseTorrentFile(const QString& torrentPath) {
    // TODO: 使用 libtorrent 解析种子文件
    // lt::torrent_info t(torrentPath.toStdString());

    // 模拟解析结果
    m_displayName = QFileInfo(torrentPath).fileName();
    m_displayName.replace(".torrent", "");

    // 模拟文件大小 (需要从种子文件读取)
    m_totalBytes = 1024 * 1024 * 100;  // 100 MB 模拟值

    // 模拟 info hash (需要从种子文件读取)
    m_infoHash = "mock_hash_" + QString::number(qHash(torrentPath));

    qInfo() << "Parsed torrent file:" << m_displayName
             << "size:" << m_totalBytes << "hash:" << m_infoHash;

    return validateTorrent();
}

bool BTDownloadEngine::parseMagnetUri(const QString& magnetUri) {
    // TODO: 使用 libtorrent 解析磁力链接
    // lt::parse_magnet_uri(magnetUri.toStdString());

    QUrl url(magnetUri);
    QUrlQuery query(url);

    // 从磁力链接提取显示名称
    m_displayName = query.queryItemValue("dn");
    if (m_displayName.isEmpty()) {
        // 使用 hash 作为临时名称
        QString hash = query.queryItemValue("xt");
        if (hash.startsWith("urn:btih:")) {
            m_displayName = hash.mid(9);
        } else {
            m_displayName = "Magnet Download";
        }
    }

    // 从磁力链接提取 info hash
    QString xt = query.queryItemValue("xt");
    if (xt.startsWith("urn:btih:")) {
        m_infoHash = xt.mid(9);
    }

    // 模拟文件大小 (磁力链接需要获取元数据后才知道)
    m_totalBytes = 0;
    m_isMetadataReady = false;

    qInfo() << "Parsed magnet URI:" << m_displayName << "hash:" << m_infoHash;

    // 模拟：假设需要获取元数据
    QTimer::singleShot(1000, this, [this]() {
        m_isMetadataReady = true;
        m_totalBytes = 1024 * 1024 * 500;  // 500 MB 模拟值
        emit metadataReceived();
    });

    return true;
}

bool BTDownloadEngine::validateTorrent() const {
    // TODO: 实现恶意种子检测
    // 检查文件大小是否异常 (如单个文件超过 1TB)

    // 当前简单检查
    if (m_totalBytes > 1024LL * 1024 * 1024 * 1024 * 10) {  // 10 TB
        qWarning() << "Suspicious torrent file size:" << m_totalBytes;
        return false;
    }

    return true;
}
