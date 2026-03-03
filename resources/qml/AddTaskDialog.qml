/**
 * @file AddTaskDialog.qml
 * @brief 新建任务对话框
 * @author XDown
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: root
    title: "新建下载任务"
    width: 500
    height: 220
    modal: true

    // 信号
    signal accepted(string url, string savePath)
    signal rejected()

    // 默认保存路径
    property string defaultSavePath: downloadEngine ? downloadEngine.getDefaultSavePath() : ""

    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "选择保存位置"
        fileMode: FileDialog.SaveFile
        onAccepted: {
            savePathInput.text = fileDialog.selectedFile
        }
    }

    // 文件夹选择对话框
    FolderDialog {
        id: folderDialog
        title: "选择保存文件夹"
        onAccepted: {
            savePathInput.text = folderDialog.selectedFolder
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        // URL 输入
        Text {
            text: "下载链接:"
            font.pixelSize: 14
        }

        TextField {
            id: urlInput
            Layout.fillWidth: true
            placeholderText: "请输入 HTTP/HTTPS 下载链接"
            text: ""
        }

        // 保存路径
        Text {
            text: "保存位置:"
            font.pixelSize: 14
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                id: savePathInput
                Layout.fillWidth: true
                text: defaultSavePath
            }

            Button {
                text: "浏览..."
                onClicked: {
                    // 让用户选择是保存文件还是文件夹
                    folderDialog.open()
                }
            }
        }

        // 提示
        Text {
            text: "提示: 支持 HTTP/HTTPS 协议的断点续传"
            font.pixelSize: 12
            color: "#888888"
        }

        // 按钮区域
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                text: "取消"
                onClicked: {
                    root.close()
                }
            }
            Button {
                text: "确定"
                onClicked: {
                    var url = urlInput.text.trim()
                    var path = savePathInput.text.trim()

                    if (!url) {
                        errorDialog.text = "请输入下载链接"
                        errorDialog.open()
                        return
                    }

                    if (!path) {
                        errorDialog.text = "请选择保存位置"
                        errorDialog.open()
                        return
                    }

                    accepted(url, path)
                    root.close()
                }
            }
        }
    }


    MessageDialog {
        id: errorDialog
        title: "提示"
        buttons: MessageDialog.Ok
    }

    Component.onCompleted: {
        if (downloadEngine) {
            savePathInput.text = downloadEngine.getDefaultSavePath()
        }
    }
}
