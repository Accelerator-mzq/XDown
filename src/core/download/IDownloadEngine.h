/**
 * @file IDownloadEngine.h
 * @brief 下载引擎抽象接口定义
 * @author XDown
 * @date 2026-03-08
 *
 * 所有下载协议（HTTP/HTTPS/BT/Magnet）必须实现此接口。
 *
 * [需单元测试]
 */

#ifndef IDOWNLOADENGINE_H
#define IDOWNLOADENGINE_H

#include <QObject>
#include <QString>
#include "common/DownloadTask.h"

/**
 * @class IDownloadEngine
 * @brief 下载引擎抽象接口
 *
 * 所有下载协议（HTTP/HTTPS/BT/Magnet）必须实现此接口。
 * 接口定义了下载任务的基本操作：启动、暂停、恢复、查询状态。
 */
class IDownloadEngine : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit IDownloadEngine(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~IDownloadEngine() = default;

    /**
     * @brief 启动下载
     * @note 异步操作，通过信号通知进度
     * [需单元测试]
     */
    virtual void start() = 0;

    /**
     * @brief 暂停下载
     * @note 保存当前进度到数据库
     * [需单元测试]
     */
    virtual void pause() = 0;

    /**
     * @brief 恢复下载
     * @note 从数据库读取进度并继续
     * [需单元测试]
     */
    virtual void resume() = 0;

    /**
     * @brief 获取已下载字节数
     * @return 已下载字节数
     * [需单元测试]
     */
    virtual qint64 getDownloadedBytes() const = 0;

    /**
     * @brief 获取文件总大小
     * @return 文件总大小（字节）
     * [需单元测试]
     */
    virtual qint64 getTotalBytes() const = 0;

    /**
     * @brief 获取当前下载速度
     * @return 下载速度（字节/秒）
     * [需单元测试]
     */
    virtual double getSpeed() const = 0;

signals:
    /**
     * @brief 进度更新信号
     * @param id 任务ID
     * @param downloaded 已下载字节数
     * @param total 文件总大小
     * @param speed 当前速度
     */
    void progressUpdated(const QString& id, qint64 downloaded, qint64 total, qint64 speed);

    /**
     * @brief 速度更新信号
     * @param speed 当前速度（字节/秒）
     */
    void speedUpdated(double speed);

    /**
     * @brief 下载完成信号
     */
    void finished();

    /**
     * @brief 错误信号
     * @param errorMsg 错误信息
     */
    void error(const QString &errorMsg);

    /**
     * @brief 状态变化信号
     * @param status 新状态
     */
    void statusChanged(TaskStatus status);
};

#endif // IDOWNLOADENGINE_H
