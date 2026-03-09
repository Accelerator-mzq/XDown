/**
 * @file BTDownloadEngine.h
 * @brief BT 下载引擎头文件
 * @author XDown
 * @date 2024
 *
 * 实现 BitTorrent 和 Magnet 协议下载:
 * - 种子文件解析
 * - 磁力链接解析
 * - Tracker 连接与 DHT 支持
 * - Peer 连接与数据传输
 * - 上传与做种管理
 */

#ifndef BTDOWNLOADENGINE_H
#define BTDOWNLOADENGINE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
#include "IDownloadEngine.h"
#include "common/DownloadTask.h"
#include "core/task/TaskState.h"

/**
 * @struct TorrentFileInfo
 * @brief BT 任务中的文件信息
 */
struct TorrentFileInfo {
    QString fileName;      // 文件名
    qint64 fileSize;     // 文件大小
    int fileIndex;       // 文件索引
    int priority;        // 下载优先级 (0=跳过, 1=低, 4=高)
    QString savePath;    // 保存路径
};

/**
 * @class BTDownloadEngine
 * @brief BT 下载引擎
 *
 * 实现 BitTorrent 和 Magnet 协议下载
 * 注意：当前版本需要集成 libtorrent 库后才能正常工作
 */
class BTDownloadEngine : public IDownloadEngine {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param torrentPath 种子文件路径或磁力链接
     * @param savePath 保存目录
     * @param parent 父对象
     */
    explicit BTDownloadEngine(const QString& torrentPath,
                            const QString& savePath,
                            QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~BTDownloadEngine() override;

    // ========== IDownloadEngine 接口实现 ==========

    /**
     * @brief 启动下载
     */
    void start() override;

    /**
     * @brief 暂停下载
     */
    void pause() override;

    /**
     * @brief 恢复下载
     */
    void resume() override;

    /**
     * @brief 获取已下载字节数
     * @return 已下载字节数
     */
    qint64 getDownloadedBytes() const override;

    /**
     * @brief 获取总字节数
     * @return 总字节数
     */
    qint64 getTotalBytes() const override;

    /**
     * @brief 获取当前下载速度
     * @return 速度 (字节/秒)
     */
    double getSpeed() const override;

    /**
     * @brief 获取当前上传速度
     * @return 上传速度 (字节/秒)
     */
    double getUploadSpeed() const;

    /**
     * @brief 获取任务状态
     * @return 任务状态
     */
    TaskStatus getStatus() const;

    /**
     * @brief 获取任务类型
     * @return 任务类型
     */
    TaskType getTaskType() const { return TaskType::BT; }

    // ========== BT 特有接口 ==========

    /**
     * @brief 获取已连接的 Peer 数量
     * @return Peer 数量
     */
    int getConnectedPeerCount() const;

    /**
     * @brief 获取可用的 Seed 数量
     * @return Seed 数量
     */
    int getAvailableSeedCount() const;

    /**
     * @brief 获取做种时间（秒）
     * @return 做种时间
     */
    qint64 getSeedingTime() const;

    /**
     * @brief 获取分享率（上传/下载）
     * @return 分享率
     */
    double getShareRatio() const;

    /**
     * @brief 获取文件列表
     * @return 文件信息列表
     */
    QList<TorrentFileInfo> getFileList() const;

    /**
     * @brief 获取 info hash
     * @return info hash 字符串
     */
    QString getInfoHash() const;

    /**
     * @brief 获取显示名称
     * @return 显示名称
     */
    QString getDisplayName() const;

    /**
     * @brief 停止做种
     */
    void stopSeeding();

    /**
     * @brief 设置做种参数
     * @param maxUploadSpeed 最大上传速度 (0=无限制)
     * @param maxConnections 最大连接数
     */
    void setSeedingOptions(int maxUploadSpeed, int maxConnections);

signals:
    /**
     * @brief Peer 数量变化信号
     * @param peers Peer 数量
     * @param seeds Seed 数量
     */
    void peersUpdated(int peers, int seeds);

    /**
     * @brief 元数据下载完成信号
     */
    void metadataReceived();

    /**
     * @brief 做种状态变化信号
     * @param isSeeding 是否在做种
     */
    void seedingStatusChanged(bool isSeeding);

    /**
     * @brief 分享率更新信号
     * @param ratio 分享率
     */
    void shareRatioUpdated(double ratio);

private slots:
    /**
     * @brief 处理状态更新
     */
    void onStatusUpdated();

private:
    /**
     * @brief 解析种子文件
     * @param torrentPath 种子路径
     * @return 是否成功
     */
    bool parseTorrentFile(const QString& torrentPath);

    /**
     * @brief 解析磁力链接
     * @param magnetUri 磁力链接
     * @return 是否成功
     */
    bool parseMagnetUri(const QString& magnetUri);

    /**
     * @brief 验证种子文件安全性
     * @return 是否安全
     */
    bool validateTorrent() const;

    // ========== 成员变量 ==========
    QString m_torrentPath;                     // 种子路径/磁力链接
    QString m_savePath;                        // 保存路径
    QString m_infoHash;                       // info hash
    QString m_displayName;                     // 显示名称
    qint64 m_totalBytes;                      // 总大小
    qint64 m_downloadedBytes;                 // 已下载
    qint64 m_uploadedBytes;                    // 已上传
    double m_downloadSpeed;                    // 下载速度
    double m_uploadSpeed;                      // 上传速度
    int m_connectedPeers;                     // 已连接 Peer
    int m_availableSeeds;                     // 可用 Seed
    qint64 m_seedingTime;                    // 做种时间
    TaskStatus m_status;                      // 任务状态
    bool m_isSeeding;                         // 是否在做种
    bool m_isMetadataReady;                   // 元数据是否就绪
    QTimer* m_statusTimer;                    // 状态更新定时器
    QList<TorrentFileInfo> m_fileList;        // 文件列表

    // libtorrent 相关 (未来集成)
    // lt::session* m_session;
    // lt::torrent_handle m_torrentHandle;
};

#endif // BTDOWNLOADENGINE_H
