/**
 * @file SettingsManager.h
 * @brief 设置管理器头文件
 * @author XDown
 * @date 2024
 */

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>

/**
 * @class SettingsManager
 * @brief 设置管理器
 *
 * 管理应用程序的设置，包括：
 * - 默认下载路径
 * - 最大并发数
 * - 线程数设置
 * - BT相关设置
 */
class SettingsManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return 实例指针
     */
    static SettingsManager* instance();

    /**
     * @brief 析构函数
     */
    ~SettingsManager();

    // ========== 下载设置 ==========

    /**
     * @brief 获取默认下载路径
     * @return 下载路径
     */
    Q_INVOKABLE QString getDefaultSavePath() const;

    /**
     * @brief 设置默认下载路径
     * @param path 下载路径
     */
    Q_INVOKABLE void setDefaultSavePath(const QString& path);

    /**
     * @brief 获取最大并发数
     * @return 并发数
     */
    Q_INVOKABLE int getMaxConcurrentDownloads() const;

    /**
     * @brief 设置最大并发数
     * @param count 并发数
     */
    Q_INVOKABLE void setMaxConcurrentDownloads(int count);

    /**
     * @brief 获取线程数设置
     * @return 线程数 (0 表示自动)
     */
    Q_INVOKABLE int getThreadCount() const;

    /**
     * @brief 设置线程数
     * @param count 线程数 (0 表示自动)
     */
    Q_INVOKABLE void setThreadCount(int count);

    // ========== BT 设置 ==========

    /**
     * @brief 获取下载完成后是否自动做种
     * @return 是否自动做种
     */
    Q_INVOKABLE bool getAutoSeeding() const;

    /**
     * @brief 设置下载完成后是否自动做种
     * @param autoSeed 是否自动做种
     */
    Q_INVOKABLE void setAutoSeeding(bool autoSeed);

    /**
     * @brief 获取做种时间限制（小时）
     * @return 小时数 (0 表示无限制)
     */
    Q_INVOKABLE int getSeedTimeLimit() const;

    /**
     * @brief 设置做种时间限制
     * @param hours 小时数
     */
    Q_INVOKABLE void setSeedTimeLimit(int hours);

    // ========== 回收站设置 ==========

    /**
     * @brief 获取回收站自动清空天数
     * @return 天数 (0 表示永不清空)
     */
    Q_INVOKABLE int getTrashAutoClearDays() const;

    /**
     * @brief 设置回收站自动清空天数
     * @param days 天数
     */
    Q_INVOKABLE void setTrashAutoClearDays(int days);

    // ========== 其他 ==========

    /**
     * @brief 同步保存设置到磁盘
     */
    Q_INVOKABLE void sync();

signals:
    /**
     * @brief 设置变化信号
     */
    void settingsChanged();

private:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit SettingsManager(QObject* parent = nullptr);

    static SettingsManager* s_instance;  // 单例实例
    QSettings* m_settings;               // 设置存储
};

#endif // SETTINGSMANAGER_H
