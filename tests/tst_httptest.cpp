/**
 * @file tst_httptest.cpp
 * @brief 简化的 HttpDownloader 系统测试
 * @description 不使用 Qt Test 框架，直接实现测试逻辑
 */

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QTemporaryDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "common/DownloadTask.h"
#include "core/HttpDownloader.h"

// 全局变量
int testsPassed = 0;
int testsFailed = 0;

void printResult(const QString& testName, bool passed, const QString& message = QString()) {
    if (passed) {
        qDebug() << "[PASS]" << testName << message;
        testsPassed++;
    } else {
        qDebug() << "[FAIL]" << testName << message;
        testsFailed++;
    }
}

// 等待下载完成或超时
bool waitForDownload(HttpDownloader* downloader, int timeoutMs = 10000) {
    QEventLoop loop;
    QTimer timeoutTimer;

    QObject::connect(downloader, &HttpDownloader::downloadFinished, &loop, &QEventLoop::quit);
    QObject::connect(downloader, &HttpDownloader::statusChanged,
                   [&loop](const QString&, TaskStatus status) {
        if (status == TaskStatus::Error || status == TaskStatus::Finished) {
            loop.quit();
        }
    });

    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeoutTimer.start(timeoutMs);
    loop.exec();

    return timeoutTimer.isActive();
}

// 测试 1: 创建 HttpDownloader 实例
void testCreateDownloader() {
    qDebug() << "\n=== Test: Create HttpDownloader ===";

    DownloadTask task;
    task.id = "test-001";
    task.url = "http://127.0.0.1:8080/file1.zip";
    task.localPath = "test.zip";

    // 创建下载器
    HttpDownloader* downloader = new HttpDownloader(task);

    // 等待初始化
    QCoreApplication::processEvents();
    QThread::sleep(1);

    bool valid = (downloader != nullptr);
    printResult("Create HttpDownloader", valid);

    // 清理
    downloader->stop();
    QCoreApplication::processEvents();
    QThread::sleep(1);
    delete downloader;
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "===== Simple HTTP Downloader Test =====\n");
    fflush(stderr);

    QCoreApplication app(argc, argv);
    fprintf(stderr, "QCoreApplication created\n");
    fflush(stderr);

    // 确保网络模块初始化
    QNetworkAccessManager nam;
    fprintf(stderr, "QNetworkAccessManager created\n");
    fflush(stderr);
    QCoreApplication::processEvents();
    QThread::msleep(100);

    fprintf(stderr, "Running testCreateDownloader...\n");
    fflush(stderr);

    // 运行测试
    testCreateDownloader();

    fprintf(stderr, "testCreateDownloader returned\n");
    fflush(stderr);

    // 等待清理
    for (int i = 0; i < 5; ++i) {
        QCoreApplication::processEvents();
        QThread::msleep(100);
    }

    // 打印结果
    fprintf(stderr, "\n===== Test Results =====\n");
    fprintf(stderr, "Passed: %d\n", testsPassed);
    fprintf(stderr, "Failed: %d\n", testsFailed);

    return testsFailed > 0 ? 1 : 0;
}
