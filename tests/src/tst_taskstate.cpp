/**
 * @file tst_taskstate.cpp
 * @brief TaskState 枚举单元测试
 * @author XDown
 * @date 2026-03-08
 *
 * 测试 TaskState 和 TaskType 枚举的转换函数：
 * - taskStateToString / stringToTaskState
 * - taskTypeToString / stringToTaskType
 */

#include <QCoreApplication>
#include <QTest>
#include <QDebug>
#include "core/task/TaskState.h"

/**
 * @class TaskStateTest
 * @brief TaskState 枚举测试类
 */
class TaskStateTest : public QObject {
    Q_OBJECT

private slots:
    /******************************************************
     * @brief 测试任务状态转字符串
     ******************************************************/
    void testTaskStateToString() {
        // 测试所有已知状态
        QString result;

        result = taskStateToString(TaskState::Waiting);
        QCOMPARE(result, QString("Waiting"));

        result = taskStateToString(TaskState::Downloading);
        QCOMPARE(result, QString("Downloading"));

        result = taskStateToString(TaskState::Paused);
        QCOMPARE(result, QString("Paused"));

        result = taskStateToString(TaskState::Finished);
        QCOMPARE(result, QString("Finished"));

        result = taskStateToString(TaskState::Error);
        QCOMPARE(result, QString("Error"));

        result = taskStateToString(TaskState::Trashed);
        QCOMPARE(result, QString("Trashed"));

        result = taskStateToString(TaskState::Seeding);
        QCOMPARE(result, QString("Seeding"));

        // 测试未知状态
        result = taskStateToString(static_cast<TaskState>(100));
        QCOMPARE(result, QString("Unknown"));
    }

    /******************************************************
     * @brief 测试字符串转任务状态
     ******************************************************/
    void testStringToTaskState() {
        TaskState result;

        result = stringToTaskState("Waiting");
        QVERIFY(result == TaskState::Waiting);

        result = stringToTaskState("Downloading");
        QVERIFY(result == TaskState::Downloading);

        result = stringToTaskState("Paused");
        QVERIFY(result == TaskState::Paused);

        result = stringToTaskState("Finished");
        QVERIFY(result == TaskState::Finished);

        result = stringToTaskState("Error");
        QVERIFY(result == TaskState::Error);

        result = stringToTaskState("Trashed");
        QVERIFY(result == TaskState::Trashed);

        result = stringToTaskState("Seeding");
        QVERIFY(result == TaskState::Seeding);

        // 测试未知状态
        result = stringToTaskState("UnknownState");
        QVERIFY(result == TaskState::Waiting);  // 默认值

        result = stringToTaskState("");
        QVERIFY(result == TaskState::Waiting);  // 默认值
    }

    /******************************************************
     * @brief 测试任务类型转字符串
     ******************************************************/
    void testTaskTypeToString() {
        QString result;

        result = taskTypeToString(TaskType::HTTP);
        QCOMPARE(result, QString("HTTP"));

        result = taskTypeToString(TaskType::BT);
        QCOMPARE(result, QString("BT"));

        result = taskTypeToString(TaskType::Magnet);
        QCOMPARE(result, QString("Magnet"));

        // 测试未知类型
        result = taskTypeToString(static_cast<TaskType>(100));
        QCOMPARE(result, QString("Unknown"));
    }

    /******************************************************
     * @brief 测试字符串转任务类型
     ******************************************************/
    void testStringToTaskType() {
        TaskType result;

        result = stringToTaskType("HTTP");
        QVERIFY(result == TaskType::HTTP);

        result = stringToTaskType("BT");
        QVERIFY(result == TaskType::BT);

        result = stringToTaskType("Magnet");
        QVERIFY(result == TaskType::Magnet);

        // 测试未知类型
        result = stringToTaskType("UnknownType");
        QVERIFY(result == TaskType::HTTP);  // 默认值

        result = stringToTaskType("");
        QVERIFY(result == TaskType::HTTP);  // 默认值
    }

    /******************************************************
     * @brief 测试双向转换一致性
     ******************************************************/
    void testRoundTrip() {
        // 测试 TaskState 双向转换
        for (int i = 0; i <= 6; ++i) {
            TaskState original = static_cast<TaskState>(i);
            QString str = taskStateToString(original);
            TaskState converted = stringToTaskState(str);
            QVERIFY(converted == original);
        }

        // 测试 TaskType 双向转换
        for (int i = 0; i <= 2; ++i) {
            TaskType original = static_cast<TaskType>(i);
            QString str = taskTypeToString(original);
            TaskType converted = stringToTaskType(str);
            QVERIFY(converted == original);
        }
    }
};

/******************************************************
 * @brief 主函数
 ******************************************************/
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    TaskStateTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_taskstate.moc"
