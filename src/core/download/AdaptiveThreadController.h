/**
 * @file AdaptiveThreadController.h
 * @brief 自适应线程控制器定义
 * @author XDown
 * @date 2026-03-08
 *
 * 根据下载速度动态调整线程数，提高下载效率。
 *
 * [需单元测试]
 */

#ifndef ADAPTIVETHREADCONTROLLER_H
#define ADAPTIVETHREADCONTROLLER_H

#include <QObject>
#include <QList>
#include <QPair>

/**
 * @class AdaptiveThreadController
 * @brief 自适应线程控制器
 *
 * 根据下载速度和网络状况动态调整线程数。
 * 当速度较低时增加线程数，当速度稳定时减少线程数以节省资源。
 */
class AdaptiveThreadController : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param minThreads 最小线程数
     * @param maxThreads 最大线程数
     * @param parent 父对象
     */
    explicit AdaptiveThreadController(int minThreads = 4,
                                     int maxThreads = 16,
                                     QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AdaptiveThreadController();

    /**
     * @brief 获取推荐的线程数
     * @return 推荐线程数
     */
    int getRecommendedThreadCount();

    /**
     * @brief 记录速度样本
     * @param speed 当前速度（字节/秒）
     * @note 用于分析速度趋势
     */
    void addSpeedSample(qint64 speed);

    /**
     * @brief 重置控制器状态
     */
    void reset();

    /**
     * @brief 获取当前线程数
     * @return 当前线程数
     */
    int getCurrentThreadCount() const { return m_currentThreadCount; }

    /**
     * @brief 手动设置线程数
     * @param count 线程数
     */
    void setThreadCount(int count);

signals:
    /**
     * @brief 线程数调整信号
     * @param newCount 新的线程数
     */
    void threadCountChanged(int newCount);

private:
    /**
     * @brief 分析速度趋势并调整线程数
     */
    void analyzeAndAdjust();

    /**
     * @brief 计算平均速度
     * @return 平均速度（字节/秒）
     */
    qint64 calculateAverageSpeed() const;

    // 成员变量
    int m_minThreads;               // 最小线程数
    int m_maxThreads;               // 最大线程数
    int m_currentThreadCount;       // 当前线程数
    QList<qint64> m_speedSamples;   // 速度样本列表
    qint64 m_lastAdjustmentTime;   // 上次调整时间
    qint64 m_totalBytesDownloaded; // 总下载字节数
    qint64 m_sampleCount;          // 样本数量
};

#endif // ADAPTIVETHREADCONTROLLER_H
