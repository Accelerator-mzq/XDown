/**
 * @file Main.qml
 * @brief 主界面
 * @author XDown
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 900
    height: 600
    minimumWidth: 700
    minimumHeight: 500
    visible: true
    title: "XDown - 纯粹下载工具"

    // 颜色定义
    property color primaryColor: "#2196F3"
    property color bgColor: "#F5F5F5"
    property color cardBgColor: "#FFFFFF"
    property color textColor: "#333333"
    property color subTextColor: "#888888"

    // 当前选中标签
    property int currentTab: 0  // 0 = 下载中, 1 = 已完成

    // 过滤器函数
    function filterTasks(model, status) {
        // 过滤任务列表
        return true
    }

    // 对话框
    AddTaskDialog {
        id: addTaskDialog
        onAccepted: {
            var error = downloadEngine.addNewTask(url, savePath)
            if (error) {
                errorDialog.text = error
                errorDialog.open()
            }
        }
    }

    // 错误提示对话框
    MessageDialog {
        id: errorDialog
        title: "提示"
        buttons: MessageDialog.Ok
    }

    // 删除确认对话框
    MessageDialog {
        id: deleteConfirmDialog
        title: "确认删除"
        text: "是否同时删除本地文件？"
        buttons: MessageDialog.Yes | MessageDialog.No | MessageDialog.Cancel

        property string taskIdToDelete: ""

        onButtonClicked: function(button, role) {
            if (button === MessageDialog.Yes) {
                downloadEngine.deleteTask(taskIdToDelete, true)
            } else if (button === MessageDialog.No) {
                downloadEngine.deleteTask(taskIdToDelete, false)
            }
            taskIdToDelete = ""
        }
    }

    // 退出确认对话框
    MessageDialog {
        id: quitConfirmDialog
        title: "确认退出"
        text: "确定要退出 XDown 吗？"
        buttons: MessageDialog.Yes | MessageDialog.No

        onButtonClicked: function(button, role) {
            if (button === MessageDialog.Yes) {
                Qt.quit()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        // 顶部标题栏
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: primaryColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                // 标题
                Text {
                    text: "XDown"
                    font.pixelSize: 18
                    font.bold: true
                    color: "white"
                }

                Item {
                    Layout.fillWidth: true
                }

                // 新建任务按钮
                Button {
                    text: "+ 新建任务"
                    onClicked: {
                        addTaskDialog.open()
                    }
                }
            }
        }

        // 主体区域
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // 侧边栏
            Rectangle {
                width: 160
                Layout.fillHeight: true
                color: cardBgColor

                Column {
                    anchors.fill: parent
                    anchors.topMargin: 10

                    // 下载中
                    Rectangle {
                        width: parent.width
                        height: 44
                        color: currentTab === 0 ? "#E3F2FD" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "下载中"
                            color: currentTab === 0 ? primaryColor : textColor
                            font.pixelSize: 14
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentTab = 0
                        }
                    }

                    // 已完成
                    Rectangle {
                        width: parent.width
                        height: 44
                        color: currentTab === 1 ? "#E3F2FD" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "已完成"
                            color: currentTab === 1 ? primaryColor : textColor
                            font.pixelSize: 14
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentTab = 1
                        }
                    }
                }
            }

            // 任务列表
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: bgColor

                ListView {
                    id: taskListView
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true

                    model: taskModel
                    delegate: TaskDelegate {
                        onPauseClicked: downloadEngine.pauseTask(id)
                        onResumeClicked: downloadEngine.resumeTask(id)
                        onDeleteClicked: {
                            deleteConfirmDialog.taskIdToDelete = id
                            deleteConfirmDialog.open()
                        }
                    }

                    // 空状态提示
                    Text {
                        visible: taskListView.count === 0
                        anchors.centerIn: parent
                        text: currentTab === 0 ? "暂无下载任务" : "暂无已完成任务"
                        color: subTextColor
                        font.pixelSize: 16
                    }
                }
            }
        }
    }

    // 窗口控制按钮 (Windows)
    footer: Rectangle {
        height: 30
        color: "#E0E0E0"

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10

            Button {
                text: "—"
                width: 40
                height: 30
                onClicked: root.showMinimized()
            }

            Button {
                text: "□"
                width: 40
                height: 30
                onClicked: root.showMaximized()
            }

            Button {
                text: "✕"
                width: 40
                height: 30
                onClicked: quitConfirmDialog.open()
            }
        }
    }
}
