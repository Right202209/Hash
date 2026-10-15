import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// JSON formatting page: the parse error is the important feedback, so it gets
// a banner under the input while a well-formed document flips the primary
// button to a success state in the shell.
Page {
    id: root

    property string errorText: ""

    header: PageHeader {
        icon: "{}"
        title: qsTr("JSON 格式化")
        description: qsTr("格式化、压缩并校验 JSON 文本 · 错误定位到行")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function apply(call) {
        const outcome = call()
        if (outcome.ok) {
            output.text = outcome.text
            root.errorText = ""
            Toast.show(qsTr("JSON 有效，已格式化"), "ok")
        } else {
            root.errorText = outcome.error
        }
    }

    contentItem: ColumnLayout {
        anchors.margins: Theme.pagePadding
        spacing: Theme.space3

        AppCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("输入 JSON")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.space2

                AppTextArea {
                    id: input

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: Theme.listRowHeight * 2
                    placeholderText: qsTr("粘贴 JSON 文本")
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    hasError: root.errorText !== ""
                    onTextChanged: root.errorText = ""
                    font.pixelSize: Theme.fontMono
                    font.family: Theme.monoFamily
                }

                AppBanner {
                    Layout.fillWidth: true
                    kind: "danger"
                    title: root.errorText
                    visible: root.errorText !== ""
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            AppButton {
                text: qsTr("格式化")
                variant: "primary"
                enabled: input.text !== ""
                onClicked: root.apply(function() {
                    return Json.formatJson(input.text)
                })
            }
            AppButton {
                text: qsTr("压缩")
                enabled: input.text !== ""
                onClicked: root.apply(function() {
                    return Json.minifyJson(input.text)
                })
            }
            Item {
                Layout.fillWidth: true
            }
            AppButton {
                text: qsTr("复制输出")
                variant: "ghost"
                enabled: output.text !== ""
                onClicked: Clipboard.copy(output.text) ? Toast.show(qsTr("输出已复制"), "ok")
                                                       : Toast.show(qsTr("内容过大，无法复制"), "error")
            }
            AppButton {
                text: qsTr("复制到输入")
                variant: "ghost"
                enabled: output.text !== ""
                onClicked: input.text = output.text
            }
            AppButton {
                text: qsTr("清空")
                variant: "ghost"
                enabled: input.text !== "" || output.text !== ""
                onClicked: {
                    input.text = ""
                    output.text = ""
                    root.errorText = ""
                }
            }
        }

        AppCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: qsTr("输出")

            AppTextArea {
                id: output

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: Theme.listRowHeight * 2
                readOnly: true
                placeholderText: qsTr("格式化后的 JSON 会显示在这里")
                wrapMode: TextArea.NoWrap
                selectByMouse: true
                font.pixelSize: Theme.fontMono
                font.family: Theme.monoFamily
            }
        }
    }
}