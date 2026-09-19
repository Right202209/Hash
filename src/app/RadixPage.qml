import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

    property var outcome: null

    header: PageHeader {
        icon: "01"
        title: qsTr("进制转换")
        description: qsTr("在 2-36 进制之间转换整数值 · 支持负数")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function convert() {
        const result = Radix.convert(valueInput.text, fromCombo.currentValue, toCombo.currentValue)
        if (!result.ok) {
            errorLabel.text = result.error
            root.outcome = null
            return
        }
        errorLabel.text = ""
        root.outcome = result
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: valueInput

                Layout.preferredWidth: 260
                placeholderText: qsTr("输入数值")
                selectByMouse: true
                font.families: Theme.monoFamilies
                onAccepted: root.convert()
            }
            Label {
                text: qsTr("从")
                color: Theme.muted
            }
            ComboBox {
                id: fromCombo

                model: [2, 8, 10, 16, 32, 36]
                currentIndex: 3
                editable: true
                validator: IntValidator {
                    bottom: 2
                    top: 36
                }
            }
            Label {
                text: qsTr("到")
                color: Theme.muted
            }
            ComboBox {
                id: toCombo

                model: [2, 8, 10, 16, 32, 36]
                currentIndex: 0
                editable: true
                validator: IntValidator {
                    bottom: 2
                    top: 36
                }
            }
            Button {
                text: qsTr("转换")
                highlighted: true
                onClicked: root.convert()
            }
            Label {
                id: errorLabel

                text: ""
                color: Theme.danger
                visible: text !== ""
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            visible: root.outcome !== null

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    Layout.preferredWidth: 90
                    text: qsTr("结果（%1 进制）").arg(root.outcome ? toCombo.currentValue : "")
                    color: Theme.ice
                }
                Label {
                    Layout.fillWidth: true
                    text: root.outcome ? root.outcome.value : ""
                    color: Theme.text
                    font.pixelSize: 18
                    font.families: Theme.monoFamilies
                    elide: Text.ElideMiddle
                }
                ToolButton {
                    text: qsTr("⧉")
                    visible: root.outcome !== null
                    onClicked: Clipboard.copy(root.outcome.value)
                               ? Toast.show(qsTr("已复制"))
                               : Toast.show(qsTr("内容过大，无法复制"))
                }
            }
            Label {
                text: root.outcome ? qsTr("十进制：%1").arg(root.outcome.decimal) : ""
                color: Theme.muted
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
