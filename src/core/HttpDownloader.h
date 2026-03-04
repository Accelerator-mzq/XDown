/**
 * @file HttpDownloader.h
 * @brief HTTP 下载器头文件
 * @author XDown
 * @date 2024
 *
 * 修复内容:
 * - 使用缓冲区循环读写，避免内存溢出
 * - 线程安全设计
 * - 断点续传检测 (206 状态码)
 * - HTTP 重定向处理
 */

#ifndef HTTPDOWNLOADER_H
#define HTTPDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>
#include <QThread>
#include <QEventLoop>
#include "common/DownloadTask.h"

/**
 * @class HttpDownloader
 * @brief HTTP/HTTPS 单线程下载器
 *
 * 设计要点:
 * - 每个下载器运行在自己的线程中
 * - 使用固定缓冲区读取数据，避免内存溢出
 * - 支持断点续传 (Range 请求)
 * - 支持 HTTP 重定向 (301, 302, 303, 307, 308)
 */
class HttpDownloader : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param task 下载任务
     * @param parent 父对象
     */
    explicit HttpDownloader(const DownloadTask& task, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HttpDownloader();

    /**
     * @brief 获取任务信息
     * @return 下载任务副本
     */
    DownloadTask getTaskInfo() const { return m_task; }

    /**
     * @brief 获取任务ID
     * @return 任务ID
     */
    QString getTaskId() const { return m_task.id; }

    /**
     * @brief 开始/继续下载
     */
    void start();

    /**
     * @brief 暂停下载
     */
    void pause();

    /**
     * @brief 停止下载并清理资源
     */
    void stop();

signals:
    /**
     * @brief 下载进度更新信号
     * @param id 任务ID
     * @param downloadedBytes 已下载字节数
     * @param totalBytes 总字节数
     * @param speedBytesPerSec 当前速度 (字节/秒)
     */
    void progressUpdated(const QString& id, qint64 downloadedBytes,
                        qint64 totalBytes, qint64 speedBytesPerSec);

    /**
     * @brief 任务状态变化信号
     * @param id 任务ID
     * @param status 新状态
     * @param errorMessage 错误信息 (可选)
     */
    void statusChanged(const QString& id, TaskStatus status,
                      const QString& errorMessage = QString());

    /**
     * @brief 下载完成信号
     * @param id 任务ID
     * @param localPath 本地文件路径
     */
    void downloadFinished(const QString& id, const QString& localPath);

private slots:
    /**
     * @brief 网络数据到达处理
     */
    void onReadyRead();

    /**
     * @brief 下载完成处理
     */
    void onFinished();

    /**
     * @brief 网络错误处理
     * @param code 网络错误码
     */
    void onErrorOccurred(QNetworkReply::NetworkError code);

    /**
     * @brief 处理 HTTP 重定向
     */
    void onRedirected(const QUrl& url);

    /**
     * @brief 计算下载速度的定时器回调
     */
    void onSpeedTimer();

private:
    /**
     * @brief 初始化网络管理器 (在线程中创建)
     */
    void initNetworkManager();

    /**
     * @brief 检查服务器是否支持断点续传
     * @return 是否支持断点续传
     */
    bool checkResumeSupport();

    /**
     * @brief 检查磁盘空间
     * @param requiredBytes 需要的字节数
     * @return 空间是否足够
     */
    bool checkDiskSpace(qint64 requiredBytes) const;

    /**
     * @brief 解析最终文件名
     * @param redirectUrl 重定向后的 URL
     */
    void parseFileName(const QUrl& redirectUrl = QUrl());

    /**
     * @brief 更新下载速度统计
     */
    void updateSpeedStatistic();

    DownloadTask m_task;                     // 下载任务
    QThread* m_thread;                       // 所属线程
    QNetworkAccessManager* m_netManager;    // 网络管理器 (必须在工作线程创建)
    QNetworkReply* m_reply;                  // 网络响应
    QFile* m_file;                           // 本地文件

    // ========== 新增字段 ==========
    qint64 m_lastDownloadedBytes;            // 上次统计时的已下载字节数
    qint64 m_lastSpeedTime;                  // 上次统计时间
    qint64 m_currentSpeed;                   // 当前速度 (字节/秒)

    bool m_isRedirecting;                    // 是否正在重定向
    bool m_supportResume;                    // 服务器是否支持断点续传
    int m_redirectCount;                    // 重定向次数 (防止无限重定向)

    QTimer* m_speedTimer;                    // 速度计算定时器

    static const int BUFFER_SIZE = 65536;    // 64KB 缓冲区
    static const int MAX_REDIRECTS = 10;     // 最大重定向次数
};

#endif // HTTPDOWNLOADER_H
