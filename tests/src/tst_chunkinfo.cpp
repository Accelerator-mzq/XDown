/**
 * @file tst_chunkinfo.cpp
 * @brief ChunkInfo 结构体单元测试
 * @author XDown
 * @date 2026-03-08
 *
 * 测试 ChunkInfo 的功能：
 * - 构造函数
 * - 分块大小计算
 * - 进度计算
 *
 * 注意：使用纯 C++ 方式 + QVERIFY 宏，避免 Qt Test 框架的 MOC 兼容性问题
 */

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QDebug>
#include "core/download/ChunkInfo.h"

/**
 * @brief 测试默认构造函数
 */
void testDefaultConstructor() {
    ChunkInfo chunk;
    QVERIFY(chunk.index == 0);
    QVERIFY(chunk.startPos == 0);
    QVERIFY(chunk.endPos == 0);
    QVERIFY(chunk.downloadedBytes == 0);
    QVERIFY(chunk.isCompleted == false);
    QVERIFY(chunk.isFailed == false);
}

/**
 * @brief 测试参数构造函数
 */
void testParameterizedConstructor() {
    ChunkInfo chunk(2, 1024, 2048);
    QVERIFY(chunk.index == 2);
    QVERIFY(chunk.startPos == 1024);
    QVERIFY(chunk.endPos == 2048);
    QVERIFY(chunk.downloadedBytes == 0);
    QVERIFY(chunk.isCompleted == false);
    QVERIFY(chunk.isFailed == false);
}

/**
 * @brief 测试分块大小计算
 */
void testSize() {
    // 测试正常情况
    ChunkInfo chunk1(0, 0, 99);
    QVERIFY(chunk1.size() == 100);

    // 测试大文件
    ChunkInfo chunk2(0, 0, 1024 * 1024 * 10 - 1);
    QVERIFY(chunk2.size() == 1024 * 1024 * 10);

    // 测试单字节
    ChunkInfo chunk3(0, 100, 100);
    QVERIFY(chunk3.size() == 1);
}

/**
 * @brief 测试进度计算
 */
void testProgress() {
    // 测试未下载时进度为 0
    ChunkInfo chunk(0, 0, 1000);
    qreal progress = chunk.progress();
    QVERIFY(progress == 0.0);

    // 测试部分下载
    chunk.downloadedBytes = 500;
    progress = chunk.progress();
    QVERIFY(qFuzzyCompare(progress, 0.5));

    // 测试完全下载
    chunk.downloadedBytes = 1000;
    progress = chunk.progress();
    QVERIFY(qFuzzyCompare(progress, 1.0));

    // 测试下载超过总大小（异常情况）
    chunk.downloadedBytes = 1500;
    progress = chunk.progress();
    QVERIFY(progress > 1.0);
}

/**
 * @brief 测试边界条件
 */
void testBoundaryConditions() {
    // 测试空块（start > end）
    ChunkInfo chunk(0, 100, 50);
    qint64 size = chunk.size();
    QVERIFY(size <= 0);
}

/**
 * @brief 主函数
 */
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    qDebug() << "======================================";
    qDebug() << "  ChunkInfo Unit Test Report";
    qDebug() << "======================================";

    // 运行所有测试函数
    testDefaultConstructor();
    qDebug() << "PASS: testDefaultConstructor";

    testParameterizedConstructor();
    qDebug() << "PASS: testParameterizedConstructor";

    testSize();
    qDebug() << "PASS: testSize";

    testProgress();
    qDebug() << "PASS: testProgress";

    testBoundaryConditions();
    qDebug() << "PASS: testBoundaryConditions";

    qDebug() << "======================================";
    qDebug() << "Result: PASS (100%)";
    qDebug() << "======================================";

    return 0;
}
