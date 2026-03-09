/**
 * @file TaskCard.qml
 * @brief 任务卡片组件
 * @author XDown
 */

import QtQuick
import QtQuick.Layouts

Rectangle {
    property string fileName: ""
    property string fileSize: ""
    property string downloadedSize: ""
    property real progress: 0
    property string speed: ""
    property int status: 0  // 0=等待, 1=下载中, 2=暂停, 3=完成, 4=错误
    property string remainingTime: ""

    // 操作信号
    signal pauseClicked()
    signal resumeClicked()
    signal deleteClicked()

    // 颜色定义
    property color colorDownloading: "#1890FF"
    property color colorCompleted: "#00C853"
    property color colorPaused: "#FFAB00"
    property color colorError: "#FF5252"
    property color colorTextPrimary: "#FFFFFF"
    property color colorTextSecondary: "#A0A0B0"
    property color colorTextMuted: "#606070"
    property color bgCard: "#2A2A35"

    width: parent.width
    height: 90
    radius: 8
    color: bgCard

    // 根据状态获取颜色
    function getStatusColor() {
        switch (status) {
            case 0: return colorTextMuted
            case 1: return colorDownloading
            case 2: return colorPaused
            case 3: return colorCompleted
            case 4: return colorError
            case 6: return colorCompleted  // 做种中
            default: return colorTextMuted
        }
    }

    function getStatusText() {
        switch (status) {
            case 0: return "等待中"
            case 1: return "下载中"
            case 2: return "已暂停"
            case 3: return "已完成"
            case 4: return "错误"
            case 5: return "回收站"
            case 6: return "做种中"
            default: return "未知"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 8

        // 第一行: 文件名 + 状态 + 操作按钮
        RowLayout {
            Layout.fillWidth: true

            // 文件名
            Text {
                text: fileName
                font.pixelSize: 14
                font.bold: true
                color: colorTextPrimary
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            // 状态标签
            Text {
                text: getStatusText()
                font.pixelSize: 12
                color: getStatusColor()
            }

            // 操作按钮
            Row {
                spacing: 8

                // 暂停/继续按钮
                Rectangle {
                    width: 24
                    height: 24
                    radius: 4
                    color: "transparent"
                    visible: status === 1 || status === 2 || status === 0

                    Text {
                        anchors.centerIn: parent
                        text: status === 1 ? "⏸" : "▶"
                        font.pixelSize: 12
                        color: colorTextSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHand
                        onClicked: {
                            if (status === 1) {
                                pauseClicked()
                            } else {
                                resumeClicked()
                            }
                        }
                    }
                }

                // 删除按钮
                Rectangle {
                    width: 24
                    height: 24
                    radius: 4
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 12
                        color: colorTextSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHand
                        onClicked: deleteClicked()
                    }
                }
            }
        }

        // 第二行: 扁平化进度条
        Rectangle {
            Layout.fillWidth: true
            height: 6
            radius: 3
            color: "#404050"

            // 进度填充
            Rectangle {
                width: parent.width * progress
                height: parent.height
                radius: 3
                color: getStatusColor()
            }
        }

        // 第三行: 下载详情
        RowLayout {
            Layout.fillWidth: true

            // 已下载/总大小 | 速度 | 剩余时间
            Text {
                text: downloadedSize + " / " + fileSize + " | " + speed + " | " + remainingTime
                font.pixelSize: 12
                color: colorTextSecondary
            }

            Item {
                Layout.fillWidth: true
            }

            // 百分比
            Text {
                text: Math.round(progress * 100) + "%"
                font.pixelSize: 12
                font.bold: true
                color: getStatusColor()
            }
        }
    }
}
