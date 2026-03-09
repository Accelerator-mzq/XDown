/**
 * @file AdaptiveThreadController.cpp
 * @brief 自适应线程控制器实现
 * @author XDown
 * @date 2026-03-08
 */

#include "AdaptiveThreadController.h"
#include <QDateTime>
#include <QtMath>

/******************************************************
 * @brief 构造函数
 * @param minThreads 最小线程数
 * @param maxThreads 最大线程数
 * @param parent 父对象
 ******************************************************/
AdaptiveThreadController::AdaptiveThreadController(int minThreads,
                                                 int maxThreads,
                                                 QObject *parent)
    : QObject(parent)
    , m_minThreads(minThreads)
    , m_maxThreads(maxThreads)
    , m_currentThreadCount(8)  // 默认 8 线程
    , m_lastAdjustmentTime(0)
    , m_totalBytesDownloaded(0)
    , m_sampleCount(0)
{
}

/******************************************************
 * @brief 析构函数
 ******************************************************/
AdaptiveThreadController::~AdaptiveThreadController()
{
}

/******************************************************
 * @brief 获取推荐的线程数
 * @return 推荐线程数
 ******************************************************/
int AdaptiveThreadController::getRecommendedThreadCount()
{
    // 每 5 秒分析一次
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastAdjustmentTime > 5000 && m_speedSamples.size() >= 3) {
        analyzeAndAdjust();
        m_lastAdjustmentTime = now;
    }

    return m_currentThreadCount;
}

/******************************************************
 * @brief 记录速度样本
 * @param speed 当前速度（字节/秒）
 ******************************************************/
void AdaptiveThreadController::addSpeedSample(qint64 speed)
{
    m_speedSamples.append(speed);
    m_sampleCount++;

    // 保留最近 10 个样本
    if (m_speedSamples.size() > 10) {
        m_speedSamples.removeFirst();
    }

    // 更新总下载字节数
    m_totalBytesDownloaded += speed;
}

/******************************************************
 * @brief 重置控制器状态
 ******************************************************/
void AdaptiveThreadController::reset()
{
    m_speedSamples.clear();
    m_lastAdjustmentTime = 0;
    m_totalBytesDownloaded = 0;
    m_sampleCount = 0;
    m_currentThreadCount = 8;  // 重置为默认 8 线程
}

/******************************************************
 * @brief 手动设置线程数
 * @param count 线程数
 ******************************************************/
void AdaptiveThreadController::setThreadCount(int count)
{
    if (count < m_minThreads) {
        count = m_minThreads;
    } else if (count > m_maxThreads) {
        count = m_maxThreads;
    }

    if (count != m_currentThreadCount) {
        m_currentThreadCount = count;
        emit threadCountChanged(count);
    }
}

/******************************************************
 * @brief 分析速度趋势并调整线程数
 ******************************************************/
void AdaptiveThreadController::analyzeAndAdjust()
{
    if (m_speedSamples.size() < 3) {
        return;
    }

    qint64 avgSpeed = calculateAverageSpeed();

    // 获取最新速度
    qint64 latestSpeed = m_speedSamples.last();

    // 计算速度变化趋势
    qreal speedTrend = 0.0;
    if (m_speedSamples.size() >= 2) {
        speedTrend = static_cast<qreal>(latestSpeed - m_speedSamples.first()) / m_speedSamples.first();
    }

    int newThreadCount = m_currentThreadCount;

    // 根据速度趋势调整线程数
    if (latestSpeed < 1024 * 1024) {  // 速度低于 1MB/s
        // 速度较低，增加线程数
        if (speedTrend < -0.1) {  // 速度持续下降
            newThreadCount = qMin(m_currentThreadCount + 2, m_maxThreads);
        }
    } else if (latestSpeed > 10 * 1024 * 1024) {  // 速度高于 10MB/s
        // 速度很高，减少线程数节省资源
        if (speedTrend > 0.1) {  // 速度持续上升
            newThreadCount = qMax(m_currentThreadCount - 2, m_minThreads);
        }
    }

    // 如果速度很稳定且线程数不是最小值，可以减少线程
    if (qAbs(speedTrend) < 0.05 && m_currentThreadCount > m_minThreads) {
        // 速度稳定，可以适当减少线程
        newThreadCount = qMax(m_currentThreadCount - 1, m_minThreads);
    }

    // 只有线程数变化时才发出信号
    if (newThreadCount != m_currentThreadCount) {
        m_currentThreadCount = newThreadCount;
        emit threadCountChanged(newThreadCount);
    }
}

/******************************************************
 * @brief 计算平均速度
 * @return 平均速度（字节/秒）
 ******************************************************/
qint64 AdaptiveThreadController::calculateAverageSpeed() const
{
    if (m_speedSamples.isEmpty()) {
        return 0;
    }

    qint64 total = 0;
    for (qint64 speed : m_speedSamples) {
        total += speed;
    }

    return total / m_speedSamples.size();
}
