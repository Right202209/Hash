import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

    header: PageHeader {
        icon: "⚿"
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
        if (!outcome.ok) {
            errorLabel.text = outcome.error
            output.text = ""
            return
        }
        errorLabel.text = ""
        output.text = outcome.list.join("\n")
    }

    contentItem: ColumnLayout {
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            CheckBox {
                id: upperCheck

                text: qsTr("大写")
            }
            CheckBox {
                id: bracesCheck

                text: qsTr("大括号")
            }
            CheckBox {
                id: hyphenCheck

                text: qsTr("包含连字符")
                checked: true
            }
            RowLayout {
                spacing: 6

                Label {
                    text: qsTr("数量")
                    color: Theme.muted
                }
                SpinBox {
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
            Button {
                text: qsTr("生成")
                highlighted: true
                onClicked: root.generate()
            }
            Label {
                id: errorLabel

                text: ""
                color: Theme.danger
                visible: text !== ""
            }
        }

        TextArea {
            id: output

            Layout.fillWidth: true
            Layout.fillHeight: true
            readOnly: true
            wrapMode: TextArea.NoWrap
            selectByMouse: true
            font.pixelSize: 13
            font.family: Theme.monoFamily
        }

        RowLayout {
            Button {
                text: qsTr("复制全部")
                enabled: output.text !== ""
                onClicked: Clipboard.copy(output.text)
                           ? Toast.show(qsTr("已复制 %1 个 UUID").arg(output.text.split("\n").length))
                           : Toast.show(qsTr("内容过大，无法复制"))
            }
        }
    }
}
