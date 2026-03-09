/**
 * @file SettingsManager.cpp
 * @brief 设置管理器实现
 * @author XDown
 * @date 2024
 */

#include "SettingsManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

SettingsManager* SettingsManager::s_instance = nullptr;

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
{
    // 使用 ini 格式的配置文件
    m_settings = new QSettings("XDown", "XDown", this);

    // 设置默认值
    if (!m_settings->contains("defaultSavePath")) {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (defaultPath.isEmpty()) {
            defaultPath = QDir::homePath() + "/Downloads";
        }
        m_settings->setValue("defaultSavePath", defaultPath);
    }

    if (!m_settings->contains("maxConcurrentDownloads")) {
        m_settings->setValue("maxConcurrentDownloads", 3);
    }

    if (!m_settings->contains("threadCount")) {
        m_settings->setValue("threadCount", 0);  // 0 表示自动
    }

    if (!m_settings->contains("autoSeeding")) {
        m_settings->setValue("autoSeeding", true);
    }

    if (!m_settings->contains("seedTimeLimit")) {
        m_settings->setValue("seedTimeLimit", 24);
    }

    if (!m_settings->contains("trashAutoClearDays")) {
        m_settings->setValue("trashAutoClearDays", 30);
    }

    qInfo() << "SettingsManager initialized";
}

SettingsManager::~SettingsManager() {
    sync();
}

SettingsManager* SettingsManager::instance() {
    if (!s_instance) {
        s_instance = new SettingsManager();
    }
    return s_instance;
}

// 下载设置
QString SettingsManager::getDefaultSavePath() const {
    return m_settings->value("defaultSavePath").toString();
}

void SettingsManager::setDefaultSavePath(const QString& path) {
    m_settings->setValue("defaultSavePath", path);
    emit settingsChanged();
}

int SettingsManager::getMaxConcurrentDownloads() const {
    return m_settings->value("maxConcurrentDownloads").toInt();
}

void SettingsManager::setMaxConcurrentDownloads(int count) {
    m_settings->setValue("maxConcurrentDownloads", count);
    emit settingsChanged();
}

int SettingsManager::getThreadCount() const {
    return m_settings->value("threadCount").toInt();
}

void SettingsManager::setThreadCount(int count) {
    m_settings->setValue("threadCount", count);
    emit settingsChanged();
}

// BT 设置
bool SettingsManager::getAutoSeeding() const {
    return m_settings->value("autoSeeding").toBool();
}

void SettingsManager::setAutoSeeding(bool autoSeed) {
    m_settings->setValue("autoSeeding", autoSeed);
    emit settingsChanged();
}

int SettingsManager::getSeedTimeLimit() const {
    return m_settings->value("seedTimeLimit").toInt();
}

void SettingsManager::setSeedTimeLimit(int hours) {
    m_settings->setValue("seedTimeLimit", hours);
    emit settingsChanged();
}

// 回收站设置
int SettingsManager::getTrashAutoClearDays() const {
    return m_settings->value("trashAutoClearDays").toInt();
}

void SettingsManager::setTrashAutoClearDays(int days) {
    m_settings->setValue("trashAutoClearDays", days);
    emit settingsChanged();
}

void SettingsManager::sync() {
    m_settings->sync();
}
