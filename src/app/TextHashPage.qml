import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

    property var results: []

    header: PageHeader {
        icon: "≡"
        title: qsTr("文本哈希")
        description: qsTr("计算文本的 MD5、SHA 系列等摘要 · 输入以 UTF-8 编码")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function compute() {
        var names = []
        for (var i = 0; i < algorithmChips.count; ++i) {
            const chip = algorithmChips.itemAt(i)
            if (chip.checked) {
                names.push(chip.name)
            }
        }
        const outcome = TextDigest.hashText(input.text, names)
        if (!outcome.ok) {
            errorLabel.text = outcome.error
            root.results = []
            return
        }
        errorLabel.text = ""
        root.results = outcome.digests
    }

    contentItem: ColumnLayout {
        spacing: 8

        Flow {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                id: algorithmChips

                model: Algorithms

                delegate: CheckBox {
                    required property string name
                    required property string label

                    text: label
                    checked: name === "sha256" || name === "md5"
                }
            }
        }

        TextArea {
            id: input

            Layout.fillWidth: true
            Layout.preferredHeight: 150
            placeholderText: qsTr("输入要计算摘要的文本")
            wrapMode: TextArea.Wrap
            selectByMouse: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("计算")
                highlighted: true
                onClicked: root.compute()
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

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                model: root.results

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.preferredWidth: 110
                        text: modelData.label
                        color: Theme.ice
                        font.pixelSize: 13
                        font.family: Theme.monoFamily
                    }
                    Label {
                        Layout.fillWidth: true
                        text: modelData.value
                        color: Theme.text
                        elide: Text.ElideMiddle
                        font.pixelSize: 13
                        font.family: Theme.monoFamily
                    }
                    ToolButton {
                        text: qsTr("⧉")
                        font.pixelSize: 12
                        onClicked: Clipboard.copy(modelData.value)
                                   ? Toast.show(qsTr("摘要已复制"))
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
