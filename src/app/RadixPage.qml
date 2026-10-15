import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Radix page: one input card carries the value and the two base pickers, the
// result card shows the converted value large with the decimal form underneath.
Page {
    id: root

    property var outcome: null
    property string errorText: ""

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
        if (result.ok) {
            root.errorText = ""
            root.outcome = result
        } else {
            root.errorText = result.error
            root.outcome = null
        }
    }

    function copy(text) {
        if (text === "" || text === undefined) {
            return
        }
        Clipboard.copy(text) ? Toast.show(qsTr("已复制"), "ok") : Toast.show(qsTr("内容过大，无法复制"),
                                                                          "error")
    }

    contentItem: ColumnLayout {
        spacing: Theme.space3

        AppCard {
            Layout.fillWidth: true
            title: qsTr("数值")

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppTextField {
                    id: valueInput

                    Layout.fillWidth: true
                    placeholderText: qsTr("输入数值")
                    selectByMouse: true
                    hasError: root.errorText !== ""
                    font.family: Theme.monoFamily
                    onTextChanged: root.errorText = ""
                    onAccepted: root.convert()
                }
                Label {
                    text: qsTr("从")
                    color: Theme.muted
                    font.pixelSize: Theme.fontSmall
                }
                AppComboBox {
                    id: fromCombo

                    Layout.preferredWidth: Theme.fieldWidth
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
                    font.pixelSize: Theme.fontSmall
                }
                AppComboBox {
                    id: toCombo

                    Layout.preferredWidth: Theme.fieldWidth
                    model: [2, 8, 10, 16, 32, 36]
                    currentIndex: 0
                    editable: true
                    validator: IntValidator {
                        bottom: 2
                        top: 36
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppButton {
                text: qsTr("转换")
                variant: "primary"
                enabled: valueInput.text !== ""
                onClicked: root.convert()
            }
            AppButton {
                text: qsTr("清空")
                variant: "ghost"
                enabled: valueInput.text !== ""
                onClicked: {
                    valueInput.text = ""
                    root.outcome = null
                    root.errorText = ""
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }

        AppBanner {
            Layout.fillWidth: true
            kind: "danger"
            title: root.errorText
            visible: root.errorText !== ""
        }

        AppCard {
            Layout.fillWidth: true
            title: qsTr("结果（%1 进制）").arg(toCombo.currentValue)
            visible: root.outcome !== null

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                Label {
                    Layout.fillWidth: true
                    text: root.outcome !== null ? root.outcome.value : ""
                    color: Theme.ice
                    font.pixelSize: Theme.fontTitle
                    font.family: Theme.monoFamily
                    elide: Text.ElideMiddle
                    verticalAlignment: Text.AlignVCenter
                }
                AppToolButton {
                    tip: qsTr("复制结果")
                    enabled: root.outcome !== null
                    onClicked: root.copy(root.outcome.value)
                }
            }

            AppKeyValueRow {
                Layout.fillWidth: true
                label: qsTr("十进制")
                value: root.outcome !== null ? String(root.outcome.decimal) : ""
                onCopyRequested: text => root.copy(text)
            }
        }

        AppEmptyState {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.outcome === null && root.errorText === ""
            glyph: "01"
            title: qsTr("还没有结果")
            hint: qsTr("输入数值并选择进制后点「转换」")
        }

        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }
    }
}
