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
#include <QCommandLineParser>
#include <QTextStream>
#include <QMessageLogger>

#include "core/DownloadEngine.h"
#include "gui/TaskListModel.h"

// 全局 QTextStream 用于输出
static QTextStream out(stdout);

// 自定义消息处理器，将 qDebug 输出重定向到 stdout
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("DEBUG: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("WARNING: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("CRITICAL: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("FATAL: %1").arg(msg);
        break;
    default:
        txt = msg;
    }
    out << txt << Qt::endl;
    out.flush();
}

int main(int argc, char *argv[]) {
    // 安装自定义消息处理器，将 qDebug 输出重定向到 stdout
    qInstallMessageHandler(customMessageHandler);

    out << "XDown starting..." << Qt::endl;
    out.flush();

    // 设置应用属性
    QCoreApplication::setOrganizationName("XDown");
    QCoreApplication::setOrganizationDomain("xdown.app");
    QCoreApplication::setApplicationName("XDown");
    QCoreApplication::setApplicationVersion("1.0.0");

    QGuiApplication app(argc, argv);
    out << "QGuiApplication created" << Qt::endl;
    out.flush();

    // ========== 命令行解析器配置 ==========
    out << "开始解析命令行参数..." << Qt::endl;
    out.flush();
    QCommandLineParser parser;
    parser.setApplicationDescription("XDown - 高性能下载工具");
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("test-net", "无头网络测试模式", "url"));

    parser.process(app);
    out << "命令行解析完成" << Qt::endl;
    out.flush();

    // ========== 无头测试模式 ==========
    if (parser.isSet("test-net")) {
        out << "检测到 test-net 参数" << Qt::endl;
        out.flush();
        QString testUrl = parser.value("test-net");
        out << "=== 进入无头测试模式 ===" << Qt::endl;
        out << "测试 URL:" << testUrl << Qt::endl;
        out.flush();

        out << "创建 DownloadEngine..." << Qt::endl;
        out.flush();
        // 创建下载引擎 (不需要 QML)
        DownloadEngine engine;
        out << "调用 initialize()..." << Qt::endl;
        out.flush();
        engine.initialize();
        out << "initialize() 完成" << Qt::endl;
        out.flush();

        // 获取默认保存路径
        QString savePath = engine.getDefaultSavePath();
        // 追加文件名
        QUrl url(testUrl);
        QString fileName = url.fileName();
        if (fileName.isEmpty()) {
            fileName = "test_download.zip";
        }
        savePath += "/" + fileName;
        out << "保存路径:" << savePath << Qt::endl;
        out.flush();

        // 连接信号: 进度更新
        QObject::connect(&engine, &DownloadEngine::taskProgressUpdated,
                         [&](const QString& id, qint64 downloaded, qint64 total, qint64 speed) {
            double progress = (total > 0 ? (downloaded * 100.0 / total) : 0);
            out << "进度: " << downloaded << " / " << total
                << " (" << QString::number(progress, 'f', 1) << "%)"
                << " 速度: " << speed << " B/s" << Qt::endl;
            out.flush();
        });

        // 连接信号: 状态变化
        QObject::connect(&engine, &DownloadEngine::taskStatusChanged,
                         [&](const QString& id, TaskStatus status, const QString& errorMessage) {
            out << "状态变化: id=" << id << " status=" << (int)status << " msg=" << errorMessage << Qt::endl;
            out.flush();
        });

        // 连接信号: 下载完成
        QObject::connect(&engine, &DownloadEngine::downloadFinished,
                         [&](const QString& id, const QString& localPath) {
            out << "=== 下载成功 ===" << Qt::endl;
            out << "文件保存至:" << localPath << Qt::endl;
            out.flush();
            QCoreApplication::exit(0);
        });

        // 连接信号: 下载错误
        QObject::connect(&engine, &DownloadEngine::taskStatusChanged,
                         [&](const QString& id, TaskStatus status, const QString& errorMessage) {
            if (status == TaskStatus::Error) {
                out << "=== 下载失败 ===" << Qt::endl;
                out << "错误信息:" << errorMessage << Qt::endl;
                out.flush();
                QCoreApplication::exit(1);
            }
        });

        // 添加下载任务
        out << "调用 addNewTask..." << Qt::endl;
        out.flush();
        QString error = engine.addNewTask(testUrl, savePath);
        if (!error.isEmpty()) {
            out << "添加任务失败:" << error << Qt::endl;
            out.flush();
            return 1;
        }

        out << "任务已添加，开始下载..." << Qt::endl;
        out.flush();

        // 启动事件循环，等待下载完成
        return app.exec();
    }

    // ========== 正常 GUI 模式 ==========
    // 设置 QML 风格
    QQuickStyle::setStyle("Basic");

    // 创建核心组件
    DownloadEngine engine;
    out << "DownloadEngine created" << Qt::endl;
    out.flush();

    TaskListModel taskModel;
    out << "TaskListModel created" << Qt::endl;
    out.flush();

    // 初始化引擎 (加载历史任务)
    engine.initialize();
    out << "Engine initialized" << Qt::endl;
    out.flush();

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
    out << "QML engine created" << Qt::endl;
    out.flush();

    // 暴露 C++ 对象到 QML
    qmlEngine.rootContext()->setContextProperty("downloadEngine", &engine);
    qmlEngine.rootContext()->setContextProperty("taskModel", &taskModel);
    out << "Context properties set" << Qt::endl;
    out.flush();

    // 加载 QML 文件
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    out << "Loading QML:" << url.toString() << Qt::endl;
    out.flush();

    // 处理 QML 加载错误
    QObject::connect(&qmlEngine, &QQmlApplicationEngine::objectCreated,
                     &app, [&engine, url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            out << "Failed to load QML:" << url.toString() << Qt::endl;
            out.flush();
            QCoreApplication::exit(-1);
        } else {
            out << "QML loaded successfully" << Qt::endl;
            out.flush();
        }
    });

    qmlEngine.load(url);
    out << "QML load called" << Qt::endl;
    out.flush();

    // 启动应用
    int ret = app.exec();
    out << "App exec returned:" << ret << Qt::endl;
    out.flush();

    // 清理资源
    qmlEngine.deleteLater();

    return ret;
}
