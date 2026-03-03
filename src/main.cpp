/**
 * @file main.cpp
 * @brief 程序入口文件
 * @author XDown
 * @date 2024
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>

#include "core/DownloadEngine.h"
#include "gui/TaskListModel.h"

int main(int argc, char *argv[]) {
    qDebug() << "XDown starting...";

    // 设置应用属性
    QCoreApplication::setOrganizationName("XDown");
    QCoreApplication::setOrganizationDomain("xdown.app");
    QCoreApplication::setApplicationName("XDown");
    QCoreApplication::setApplicationVersion("1.0.0");

    QGuiApplication app(argc, argv);
    qDebug() << "QGuiApplication created";

    // 设置 QML 风格
    QQuickStyle::setStyle("Basic");

    // 创建核心组件
    DownloadEngine engine;
    qDebug() << "DownloadEngine created";

    TaskListModel taskModel;
    qDebug() << "TaskListModel created";

    // 初始化引擎 (加载历史任务)
    engine.initialize();
    qDebug() << "Engine initialized";

    // 将引擎连接到模型
    QObject::connect(&engine, &DownloadEngine::taskAdded,
                     &taskModel, &TaskListModel::addTask);

    QObject::connect(&engine, &DownloadEngine::taskDeleted,
                     &taskModel, &TaskListModel::removeTask);

    QObject::connect(&engine, &DownloadEngine::taskProgressUpdated,
                     &taskModel, &TaskListModel::updateTaskProgress);

    QObject::connect(&engine, &DownloadEngine::taskStatusChanged,
                     &taskModel, &TaskListModel::updateTaskStatus);

    QObject::connect(&engine, &DownloadEngine::allTasksLoaded,
                     &taskModel, &TaskListModel::loadTasks);

    // 创建 QML 引擎
    QQmlApplicationEngine qmlEngine;
    qDebug() << "QML engine created";

    // 暴露 C++ 对象到 QML
    qmlEngine.rootContext()->setContextProperty("downloadEngine", &engine);
    qmlEngine.rootContext()->setContextProperty("taskModel", &taskModel);
    qDebug() << "Context properties set";

    // 加载 QML 文件
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    qDebug() << "Loading QML:" << url;

    // 处理 QML 加载错误
    QObject::connect(&qmlEngine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qCritical() << "Failed to load QML:" << url;
            QCoreApplication::exit(-1);
        } else {
            qDebug() << "QML loaded successfully";
        }
    }, Qt::QueuedConnection);

    qmlEngine.load(url);
    qDebug() << "QML load called";

    // 启动应用
    int ret = app.exec();
    qDebug() << "App exec returned:" << ret;

    // 清理资源
    qmlEngine.deleteLater();

    return ret;
}
