import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// UUID page: a formatting-options card drives a batch generator; the result
// card shows the joined list and its count.
Page {
    id: root

    property string errorText: ""

    readonly property int resultCount: output.text === "" ? 0 : output.text.split("\n").length

    header: PageHeader {
        icon: "ID"
        title: qsTr("UUID 生成")
        description: qsTr("批量生成 UUID v4 · 可控制大小写、大括号与连字符")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function generate() {
        const outcome = Uuid.generate(countSpin.value, upperCheck.checked, bracesCheck.checked,
                                      hyphenCheck.checked)
        if (outcome.ok) {
            root.errorText = ""
            output.text = outcome.list.join("\n")
            Toast.show(qsTr("已生成 %1 个 UUID").arg(outcome.list.length), "ok")
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

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space4

                AppCheckBox {
                    id: upperCheck

                    text: qsTr("大写")
                }
                AppCheckBox {
                    id: bracesCheck

                    text: qsTr("大括号")
                }
                AppCheckBox {
                    id: hyphenCheck

                    text: qsTr("包含连字符")
                    checked: true
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
                        to: 1000
                        value: 5
                        editable: true
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
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
                                                             qsTr("已复制 %1 个 UUID").arg(
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
                placeholderText: qsTr("生成的 UUID 会显示在这里")
                font.pixelSize: Theme.fontMono
                font.family: Theme.monoFamily
            }
        }
    }
}
