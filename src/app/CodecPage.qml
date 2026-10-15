import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// CodecPage serves both Base64 and URL tools; mode picks which codec pair to
// call, keeping one shared encode/decode layout. Input and output are two
// cards with the swap button centered between them.
Page {
    id: root

    property string mode: "base64"
    property string errorText: ""

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
            root.errorText = ""
            Toast.show(qsTr("已生成输出"), "ok")
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
            title: qsTr("输入")

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.space2

                AppTextArea {
                    id: input

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: Theme.listRowHeight * 2
                    placeholderText: qsTr("粘贴要编码或解码的文本")
                    wrapMode: TextArea.Wrap
                    selectByMouse: true
                    font.pixelSize: Theme.fontMono
                    font.family: Theme.monoFamily
                    onTextChanged: root.errorText = ""
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("%1 字符").arg(input.text.length)
                    color: Theme.muted
                    font.pixelSize: Theme.fontCaption
                }
            }
        }

        // The swap reads as a transformation, not a separate action, so it sits
        // centered on its own row between the two panes.
        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }
            AppButton {
                text: qsTr("输入 ⇄ 输出")
                variant: "ghost"
                onClicked: {
                    const swap = input.text
                    input.text = output.text
                    output.text = swap
                }
            }
            Item {
                Layout.fillWidth: true
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
                placeholderText: qsTr("输出会显示在这里")
                wrapMode: TextArea.Wrap
                selectByMouse: true
                font.pixelSize: Theme.fontMono
                font.family: Theme.monoFamily
            }
        }

        AppBanner {
            Layout.fillWidth: true
            kind: "danger"
            title: root.errorText
            visible: root.errorText !== ""
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppButton {
                text: qsTr("编码")
                variant: "primary"
                enabled: input.text !== ""
                onClicked: root.apply(function() {
                    return root.isBase64 ? Codec.base64Encode(input.text) : Codec.urlEncode(input.text)
                })
            }
            AppButton {
                text: qsTr("解码")
                enabled: input.text !== ""
                onClicked: root.apply(function() {
                    return root.isBase64 ? Codec.base64Decode(input.text) : Codec.urlDecode(input.text)
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
    }
}
