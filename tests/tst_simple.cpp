/**
 * @file tst_simple.cpp
 * @brief 简单测试用于调试
 */

#include <QCoreApplication>
#include <QTest>
#include <QDebug>

class SimpleTest : public QObject {
    Q_OBJECT

private slots:
    void testSimple() {
        qDebug() << "Simple test running...";
        QVERIFY(true);
    }
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    SimpleTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_simple.moc"
