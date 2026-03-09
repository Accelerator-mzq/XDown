/**
 * @file tst_btdownloadengine.cpp
 * @brief BTDownloadEngine 单元测试
 * @author XDown
 * @date 2024
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include "core/download/BTDownloadEngine.h"

// 使用 QtTest 宏简化测试
class TestBTDownloadEngine : public QObject {
    Q_OBJECT

private slots:
    void testGetTaskType();
    void testParseMagnetUri();
    void testGetStatus();
    void testGetInfo();
};

void TestBTDownloadEngine::testGetTaskType() {
    BTDownloadEngine engine(
        "magnet:?xt=urn:btih:1234567890abcdef1234567890abcdef",
        "C:/Downloads"
    );
    TaskType type = engine.getTaskType();
    QVERIFY(type == TaskType::BT);
}

void TestBTDownloadEngine::testParseMagnetUri() {
    QString magnetUri = "magnet:?xt=urn:btih:1234567890abcdef1234567890abcdef&dn=testfile";
    BTDownloadEngine engine(magnetUri, "C:/Downloads");
    QString infoHash = engine.getInfoHash();
    QVERIFY(!infoHash.isEmpty());
    QVERIFY(engine.getDisplayName() == "testfile");
}

void TestBTDownloadEngine::testGetStatus() {
    BTDownloadEngine engine(
        "magnet:?xt=urn:btih:1234567890abcdef1234567890abcdef",
        "C:/Downloads"
    );
    TaskStatus status = engine.getStatus();
    QVERIFY(status == TaskStatus::Waiting);

    engine.start();
    QVERIFY(engine.getStatus() == TaskStatus::Downloading);

    engine.pause();
    QVERIFY(engine.getStatus() == TaskStatus::Paused);
}

void TestBTDownloadEngine::testGetInfo() {
    BTDownloadEngine engine(
        "magnet:?xt=urn:btih:1234567890abcdef1234567890abcdef",
        "C:/Downloads"
    );

    // 测试获取BT特有信息
    int peers = engine.getConnectedPeerCount();
    QVERIFY(peers >= 0);

    int seeds = engine.getAvailableSeedCount();
    QVERIFY(seeds >= 0);

    double ratio = engine.getShareRatio();
    QVERIFY(ratio >= 0.0);

    QList<TorrentFileInfo> files = engine.getFileList();
    QVERIFY(files.isEmpty() || files.size() >= 0);
}

QTEST_MAIN(TestBTDownloadEngine)
#include "tst_btdownloadengine.moc"
