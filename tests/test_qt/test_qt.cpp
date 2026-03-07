#include <QCoreApplication>
#include <QTest>
#include <QDebug>

class TestSimple : public QObject {
    Q_OBJECT
private slots:
    void testPass() {
        QVERIFY(true);
    }
};

int main(int argc, char *argv[]) {
    qDebug() << "Test starting...";
    QCoreApplication app(argc, argv);
    qDebug() << "App created";

    TestSimple test;
    int result = QTest::qExec(&test, argc, argv);

    qDebug() << "Result:" << result;
    return result;
}

#include "test_qt.moc"
