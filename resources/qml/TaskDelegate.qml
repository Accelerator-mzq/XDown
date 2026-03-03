/**
 * @file TaskDelegate.qml
 * @brief 任务列表项组件
 * @author XDown
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: parent.width
    height: 80
    radius: 8
    color: "white"

    // 任务状态: 0=等待, 1=下载中, 2=暂停, 3=完成, 4=错误
    property int taskStatus: model.status

    // 从 Main.qml 传入的当前 tab
    property int currentTab: 0

    // 根据状态过滤显示
    visible: {
        if (currentTab === 0) {
            // 下载中: 等待、下载中、暂停、错误（不显示已完成的）
            return taskStatus !== 3
        } else {
            // 已完成: 只显示完成的
            return taskStatus === 3
        }
    }

    signal pauseClicked(string id)
    signal resumeClicked(string id)
    signal deleteClicked(string id)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6

        // 第一行: 文件名和状态
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: model.fileName
                font.pixelSize: 14
                font.bold: true
                color: "#333333"
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            Text {
                text: model.statusText
                font.pixelSize: 12
                color: {
                    switch (taskStatus) {
                        case 1: return "#2196F3"  // 下载中蓝色
                        case 2: return "#FF9800"  // 暂停橙色
                        case 3: return "#4CAF50"  // 完成绿色
                        case 4: return "#F44336"  // 错误红色
                        default: return "#888888"
                    }
                }
            }
        }

        // 第二行: 进度条和速度
        RowLayout {
            Layout.fillWidth: true

            // 进度条
            ProgressBar {
                value: model.progress
                Layout.fillWidth: true
                height: 6

                background: Rectangle {
                    radius: 3
                    color: "#E0E0E0"
                }

                contentItem: Rectangle {
                    radius: 3
                    color: {
                        switch (taskStatus) {
                            case 1: return "#2196F3"
                            case 2: return "#FF9800"
                            case 3: return "#4CAF50"
                            case 4: return "#F44336"
                            default: return "#888888"
                        }
                    }
                }
            }

            // 速度
            Text {
                text: model.speedFormatted
                font.pixelSize: 12
                color: "#888888"
                width: 80
                horizontalAlignment: Text.AlignRight
                visible: taskStatus === 1  // 只在下载中显示速度
            }
        }

        // 第三行: 大小信息
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: model.downloadedSizeFormatted + " / " + model.totalSizeFormatted
                font.pixelSize: 12
                color: "#888888"
            }

            Item {
                Layout.fillWidth: true
            }

            // 操作按钮
            Row {
                spacing: 8

                // 暂停/继续按钮
                Button {
                    width: 60
                    height: 24
                    text: taskStatus === 1 ? "暂停" : "继续"
                    visible: taskStatus === 1 || taskStatus === 2
                    onClicked: {
                        if (taskStatus === 1) {
                            pauseClicked(model.id)
                        } else {
                            resumeClicked(model.id)
                        }
                    }
                }

                // 删除按钮
                Button {
                    width: 60
                    height: 24
                    text: "删除"
                    visible: taskStatus !== 1
                    onClicked: {
                        deleteClicked(model.id)
                    }
                }
            }
        }
    }

    // 底部边框
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: "#EEEEEE"
    }
}
