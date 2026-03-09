/**
 * @file IDownloadEngine.cpp
 * @brief 下载引擎抽象接口实现
 * @author XDown
 * @date 2026-03-08
 */

#include "IDownloadEngine.h"

/******************************************************
 * @brief 构造函数
 * @param parent 父对象
 ******************************************************/
IDownloadEngine::IDownloadEngine(QObject *parent)
    : QObject(parent)
{
}
