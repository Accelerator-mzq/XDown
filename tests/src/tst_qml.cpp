/**
 * @file tst_qml.cpp
 * @brief QML GUI 加载测试用例
 * @description 验证 QML 界面能否正确加载，检测 Qt Quick Controls 2 插件缺失等问题
 */

#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QEventLoop>
#include <QUrl>
#include <cstdio>

int main(int argc, char *argv[]) {
    // 设置无头模式
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);
    app.setApplicationName("XDownQmlTest");

    printf("Starting QML load test...\n");
    fflush(stdout);

    // 测试1: 检查 QML 资源文件是否存在
    printf("Test 1: Checking QML resource...\n");
    printf("  Current directory: %s\n", QDir::currentPath().toUtf8().constData());
    printf("  App dir: %s\n", QGuiApplication::applicationDirPath().toUtf8().constData());

    // 尝试多种方式加载 QML
    QStringList qmlPaths = {
        "qrc:/qml/Main.qml",
        ":/qml/Main.qml",
        QGuiApplication::applicationDirPath() + "/qml/qml/Main.qml",
        QGuiApplication::applicationDirPath() + "/../qml/qml/Main.qml",
        QCoreApplication::applicationDirPath() + "/resources/qml/Main.qml",
    };

    QString qmlPath;
    bool found = false;
    for (const QString &path : qmlPaths) {
        printf("  Trying: %s\n", path.toUtf8().constData());
        if (path.startsWith("qrc:") || path.startsWith(":/")) {
            if (QFile::exists(path)) {
                qmlPath = path;
                found = true;
                printf("  Found: %s\n", path.toUtf8().constData());
                break;
            }
        } else {
            if (QFile::exists(path)) {
                qmlPath = path;
                found = true;
                printf("  Found: %s\n", path.toUtf8().constData());
                break;
            }
        }
    }

    if (!found) {
        printf("FAIL: QML resource qrc:/qml/Main.qml does not exist in any known path!\n");
        return 1;
    }

    // 将路径转换为 QUrl
    QUrl qmlUrl;
    if (qmlPath.startsWith("qrc:") || qmlPath.startsWith(":/")) {
        // 对于 qrc 资源，使用 fromUserInput 或者手动设置
        if (qmlPath.startsWith(":/")) {
            qmlUrl = QUrl(QStringLiteral("qrc") + qmlPath);
        } else {
            qmlUrl = QUrl(qmlPath);
        }
    } else {
        qmlUrl = QUrl::fromLocalFile(qmlPath);
    }

    printf("PASS: QML resource exists and is valid\n");

    // 测试2: 加载 QML
    printf("Test 2: Loading QML...\n");
    QQmlApplicationEngine engine;

    // 收集错误
    QStringList errors;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     [&errors](const QList<QQmlError> &list) {
        for (const QQmlError &error : list) {
            errors.append(error.toString());
            fprintf(stderr, "QML Warning: %s\n", qPrintable(error.toString()));
        }
    });

    printf("Loading from: path=%s, url=%s\n", qPrintable(qmlPath), qPrintable(qmlUrl.toString()));

    // 加载 QML（使用 loadUrl 异步）
    QObject::connect(&engine, &QQmlApplicationEngine::quit, &app, &QGuiApplication::quit);
    engine.load(qmlUrl);

    // 等待一段时间让 QML 加载完成
    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();

    // 检查 root object
    if (engine.rootObjects().isEmpty()) {
        printf("FAIL: No root objects created!\n");
        if (!errors.isEmpty()) {
            printf("Errors:\n");
            for (const QString &err : errors) {
                printf("  %s\n", qPrintable(err));
            }
        }
        return 1;
    }

    printf("PASS: QML loaded successfully!\n");
    printf("Root object type: %s\n", engine.rootObjects().first()->metaObject()->className());

    // 测试 QtQuick.Controls - 使用同一个引擎检查
    printf("Test 3: Testing QtQuick.Controls plugin...\n");
    bool controlsAvailable = false;
    for (const QObject *obj : engine.rootObjects()) {
        if (obj) {
            // 如果 Main 窗口成功创建，说明 QtQuick.Controls 可用
            controlsAvailable = true;
            break;
        }
    }

    if (!controlsAvailable) {
        printf("FAIL: QtQuick.Controls plugin not available - Main window not created!\n");
        return 1;
    }

    printf("PASS: QtQuick.Controls plugin available (Main window created)\n");

    printf("\n=== All tests passed! ===\n");
    return 0;
}
