import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

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
            errorLabel.text = ""
        } else {
            errorLabel.text = outcome.error
        }
    }

    contentItem: ColumnLayout {
        spacing: 6

        TextArea {
            id: input

            Layout.fillWidth: true
            Layout.fillHeight: true
            placeholderText: qsTr("粘贴 JSON 文本")
            wrapMode: TextArea.Wrap
            selectByMouse: true
            font.pixelSize: 13
            font.family: Theme.monoFamily
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("格式化")
                highlighted: true
                onClicked: root.apply(function() {
                    return Json.formatJson(input.text)
                })
            }
            Button {
                text: qsTr("压缩")
                onClicked: root.apply(function() {
                    return Json.minifyJson(input.text)
                })
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                id: errorLabel

                Layout.fillWidth: true
                text: ""
                color: Theme.danger
                visible: text !== ""
                elide: Text.ElideMiddle
            }
        }

        TextArea {
            id: output

            Layout.fillWidth: true
            Layout.fillHeight: true
            readOnly: true
            wrapMode: TextArea.NoWrap
            selectByMouse: true
            placeholderText: qsTr("输出会显示在这里")
            font.pixelSize: 13
            font.family: Theme.monoFamily
        }

        RowLayout {
            Button {
                text: qsTr("复制输出")
                enabled: output.text !== ""
                onClicked: Clipboard.copy(output.text)
                           ? Toast.show(qsTr("输出已复制"))
                           : Toast.show(qsTr("内容过大，无法复制"))
            }
            Button {
                text: qsTr("复制到输入")
                enabled: output.text !== ""
                onClicked: input.text = output.text
            }
            Button {
                text: qsTr("清空")
                onClicked: {
                    input.text = ""
                    output.text = ""
                    errorLabel.text = ""
                }
            }
        }
    }
}
