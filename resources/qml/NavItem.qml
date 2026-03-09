/**
 * @file NavItem.qml
 * @brief 导航菜单项组件
 * @author XDown
 */

import QtQuick

Rectangle {
    property string text: ""
    property int count: 0
    property string icon: ""
    property bool active: false
    signal clicked()

    width: parent.width
    height: 42
    color: active ? "#303040" : "transparent"

    Row {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 10
        spacing: 10

        // 图标
        Text {
            text: icon
            font.pixelSize: 14
            color: active ? "#1890FF" : "#606070"
            verticalAlignment: VerticalAlignment.Center
            width: 20
        }

        // 文字
        Text {
            text: text
            font.pixelSize: 13
            color: active ? "#FFFFFF" : "#A0A0B0"
            verticalAlignment: VerticalAlignment.Center
        }

        // 计数
        Text {
            text: count > 0 ? "(" + count + ")" : ""
            font.pixelSize: 12
            color: "#606070"
            verticalAlignment: VerticalAlignment.Center
            visible: count > 0
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: if (!active) parent.color = "#252530"
        onExited: if (!active) parent.color = "transparent"
        onClicked: parent.clicked()
    }
}
