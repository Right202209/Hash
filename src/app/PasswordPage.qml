import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

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

        Flow {
            Layout.fillWidth: true
            spacing: 14

            RowLayout {
                spacing: 6

                Label {
                    text: qsTr("长度")
                    color: Theme.muted
                }
                SpinBox {
                    id: lengthSpin

                    from: 4
                    to: 256
                    value: 16
                    editable: true
                }
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
                    to: 100
                    value: 5
                    editable: true
                }
            }
            CheckBox {
                id: upperCheck

                text: qsTr("大写字母")
                checked: true
            }
            CheckBox {
                id: lowerCheck

                text: qsTr("小写字母")
                checked: true
            }
            CheckBox {
                id: digitCheck

                text: qsTr("数字")
                checked: true
            }
            CheckBox {
                id: symbolCheck

                text: qsTr("符号")
                checked: true
            }
            CheckBox {
                id: safeCheck

                text: qsTr("排除易混淆字符")
                checked: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("生成")
                highlighted: true
                onClicked: root.generate()
            }
            Button {
                text: qsTr("复制全部")
                enabled: output.text !== ""
                onClicked: Clipboard.copy(output.text)
                           ? Toast.show(qsTr("已复制 %1 个密码").arg(output.text.split("\n").length))
                           : Toast.show(qsTr("内容过大，无法复制"))
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
            font.pixelSize: 14
            font.families: Theme.monoFamilies
        }
    }
}
