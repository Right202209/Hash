import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Password page: length and charset options feed the system-strong random
// generator; the result card shows the batch and its count.
Page {
    id: root

    property string errorText: ""

    readonly property int resultCount: output.text === "" ? 0 : output.text.split("\n").length

    // charsetHint joins the selected character sets into the generation chart
    // ("大写·小写·数字·符号") shown under the options.
    readonly property string charsetHint: {
        const parts = []
        if (upperCheck.checked) {
            parts.push(qsTr("大写"))
        }
        if (lowerCheck.checked) {
            parts.push(qsTr("小写"))
        }
        if (digitCheck.checked) {
            parts.push(qsTr("数字"))
        }
        if (symbolCheck.checked) {
            parts.push(qsTr("符号"))
        }
        if (parts.length === 0) {
            return qsTr("请至少选择一种字符集")
        }
        if (safeCheck.checked) {
            parts.push(qsTr("排除易混淆"))
        }
        return parts.join("·")
    }

    header: PageHeader {
        icon: "⚿"
        title: qsTr("密码生成")
        description: qsTr("按长度与字符集批量生成随机密码 · 使用系统安全随机源")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function generate() {
        const outcome = Password.generate(lengthSpin.value, countSpin.value, upperCheck.checked,
                                          lowerCheck.checked, digitCheck.checked,
                                          symbolCheck.checked, safeCheck.checked)
        if (outcome.ok) {
            root.errorText = ""
            output.text = outcome.list.join("\n")
            Toast.show(qsTr("已生成 %1 个密码").arg(outcome.list.length), "ok")
        } else {
            root.errorText = outcome.error
            output.text = ""
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.space3

        AppCard {
            Layout.fillWidth: true
            title: qsTr("生成选项")

            Flow {
                Layout.fillWidth: true
                spacing: Theme.space4

                RowLayout {
                    spacing: Theme.space2

                    Label {
                        text: qsTr("长度")
                        color: Theme.muted
                        font.pixelSize: Theme.fontSmall
                    }
                    AppSpinBox {
                        id: lengthSpin

                        from: 4
                        to: 256
                        value: 16
                        editable: true
                    }
                }
                RowLayout {
                    spacing: Theme.space2

                    Label {
                        text: qsTr("数量")
                        color: Theme.muted
                        font.pixelSize: Theme.fontSmall
                    }
                    AppSpinBox {
                        id: countSpin

                        from: 1
                        to: 100
                        value: 5
                        editable: true
                    }
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: Theme.space4

                AppCheckBox {
                    id: upperCheck

                    text: qsTr("大写字母")
                    checked: true
                }
                AppCheckBox {
                    id: lowerCheck

                    text: qsTr("小写字母")
                    checked: true
                }
                AppCheckBox {
                    id: digitCheck

                    text: qsTr("数字")
                    checked: true
                }
                AppCheckBox {
                    id: symbolCheck

                    text: qsTr("符号")
                    checked: true
                }
                AppCheckBox {
                    id: safeCheck

                    text: qsTr("排除易混淆字符")
                    checked: true
                }
            }

            Label {
                Layout.fillWidth: true
                text: root.charsetHint
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space2

            AppButton {
                text: qsTr("生成")
                variant: "primary"
                onClicked: root.generate()
            }
            AppButton {
                text: qsTr("清空")
                variant: "ghost"
                enabled: output.text !== ""
                onClicked: {
                    output.text = ""
                    root.errorText = ""
                }
            }
            Item {
                Layout.fillWidth: true
            }
            AppButton {
                text: qsTr("复制全部")
                variant: "ghost"
                enabled: output.text !== ""
                onClicked: Clipboard.copy(output.text) ? Toast.show(
                                                             qsTr("已复制 %1 个密码").arg(
                                                                 root.resultCount), "ok")
                                                       : Toast.show(qsTr("内容过大，无法复制"),
                                                                    "error")
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
            Layout.fillHeight: true
            title: qsTr("结果")

            Label {
                Layout.fillWidth: true
                visible: root.resultCount > 0
                text: qsTr("共 %1 条").arg(root.resultCount)
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }

            AppTextArea {
                id: output

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: Theme.listRowHeight * 4
                readOnly: true
                wrapMode: TextArea.NoWrap
                selectByMouse: true
                placeholderText: qsTr("生成的密码会显示在这里")
                font.pixelSize: Theme.fontMono
                font.family: Theme.monoFamily
            }
        }
    }
}
