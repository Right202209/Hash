import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

    property var outcome: null

    header: PageHeader {
        icon: "◐"
        title: qsTr("颜色转换")
        description: qsTr("HEX、RGB 与 HSL 颜色格式互相转换 · 支持 Alpha 通道")
    }

    function convert(text) {
        const result = Color.convert(text)
        root.outcome = result.ok ? result : null
        errorLabel.text = text.trim() === "" || result.ok ? "" : result.error
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: valueInput

                Layout.preferredWidth: 260
                placeholderText: qsTr("#FF8000 或 rgb(255, 128, 0)")
                selectByMouse: true
                font.families: Theme.monoFamilies
                onTextChanged: root.convert(text)
            }
            Rectangle {
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                radius: 6
                color: root.outcome ? root.outcome.hex : "transparent"
                border.color: Theme.line
            }
            Label {
                id: errorLabel

                Layout.fillWidth: true
                text: ""
                color: Theme.danger
                visible: text !== ""
                wrapMode: Text.WrapAnywhere
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: root.outcome ? [{
                        "label": "HEX",
                        "value": root.outcome.hex
                    }, {
                        "label": "RGB",
                        "value": root.outcome.rgba
                    }, {
                        "label": "HSL",
                        "value": root.outcome.hsl
                    }] : []

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.preferredWidth: 50
                        text: modelData.label
                        color: Theme.ice
                        font.families: Theme.monoFamilies
                    }
                    Label {
                        Layout.fillWidth: true
                        text: modelData.value
                        color: Theme.text
                        font.pixelSize: 14
                        font.families: Theme.monoFamilies
                    }
                    ToolButton {
                        text: qsTr("⧉")
                        font.pixelSize: 12
                        onClicked: Clipboard.copy(modelData.value)
                                   ? Toast.show(qsTr("已复制"))
                                   : Toast.show(qsTr("内容过大，无法复制"))
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
