import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// CodecPage serves both Base64 and URL tools; mode picks which codec pair to
// call, keeping one shared encode/decode layout.
Page {
    id: root

    property string mode: "base64"
    readonly property bool isBase64: mode === "base64"

    header: PageHeader {
        icon: root.isBase64 ? "64" : "%"
        title: root.isBase64 ? qsTr("Base64 编解码") : qsTr("URL 编解码")
        description: root.isBase64 ? qsTr("Base64 编码与解码 · 解码容错换行与空白")
                                   : qsTr("URL 百分号编码与解码")
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

        Label {
            text: qsTr("输入")
            color: Theme.muted
        }
        TextArea {
            id: input

            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: TextArea.Wrap
            selectByMouse: true
            font.pixelSize: 13
            font.family: Theme.monoFamily
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("编码")
                highlighted: true
                onClicked: root.apply(function() {
                    return root.isBase64 ? Codec.base64Encode(input.text)
                                         : Codec.urlEncode(input.text)
                })
            }
            Button {
                text: qsTr("解码")
                onClicked: root.apply(function() {
                    return root.isBase64 ? Codec.base64Decode(input.text)
                                         : Codec.urlDecode(input.text)
                })
            }
            Button {
                text: qsTr("输入 ⇄ 输出")
                onClicked: {
                    const swap = input.text
                    input.text = output.text
                    output.text = swap
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                id: errorLabel

                text: ""
                color: Theme.danger
                visible: text !== ""
            }
        }

        Label {
            text: qsTr("输出")
            color: Theme.muted
        }
        TextArea {
            id: output

            Layout.fillWidth: true
            Layout.fillHeight: true
            readOnly: true
            wrapMode: TextArea.Wrap
            selectByMouse: true
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
