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
        description: qsTr("计算文本的 MD5、SHA 系列等摘要 · 按 UTF-8 编码、LF 换行计算")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    // The engines encode with QString::toUtf8, so the counter has to match that
    // and not the UTF-16 code-unit count a plain .length would give.
    function utf8Length(text) {
        let bytes = 0
        for (let i = 0; i < text.length; ++i) {
            const code = text.charCodeAt(i)
            if (code < 0x80) {
                bytes += 1
            } else if (code < 0x800) {
                bytes += 2
            } else if (code >= 0xD800 && code <= 0xDBFF && i + 1 < text.length) {
                bytes += 4
                ++i
            } else {
                bytes += 3
            }
        }
        return bytes
    }

    function compute() {
        const outcome = TextDigest.hashText(input.text, algorithmChips.selectedNames())
        if (!outcome.ok) {
            root.results = []
            input.hasError = true
            Toast.show(outcome.error, "error")
            return
        }
        input.hasError = false
        root.results = outcome.digests
    }

    contentItem: Flickable {
        id: scroller

        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 2 * Theme.space3
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            id: contentColumn

            x: Theme.pagePadding
            y: Theme.space3
            width: scroller.width - 2 * Theme.pagePadding
            spacing: Theme.space3

            AlgorithmChips {
                id: algorithmChips

                Layout.fillWidth: true
                defaults: ["sha256", "md5"]
            }

            AppCard {
                Layout.fillWidth: true
                title: qsTr("输入文本")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space2

                    AppTextArea {
                        id: input

                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.listRowHeight * 3.5
                        placeholderText: qsTr("输入要计算摘要的文本")
                        wrapMode: TextArea.Wrap
                        selectByMouse: true
                        onTextChanged: hasError = false
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("%1 字符 · %2 字节（UTF-8）").arg(input.text.length).arg(root.utf8Length(input.text))
                        color: Theme.muted
                        font.pixelSize: Theme.fontCaption
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppButton {
                    text: qsTr("计算")
                    variant: "primary"
                    enabled: algorithmChips.selectedNames().length > 0
                    onClicked: root.compute()
                }
                AppButton {
                    text: qsTr("清空")
                    variant: "ghost"
                    enabled: input.text !== ""
                    onClicked: {
                        input.text = ""
                        root.results = []
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
            }

            AppCard {
                Layout.fillWidth: true
                visible: root.results.length > 0
                title: qsTr("结果 · %1 个算法").arg(root.results.length)

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space1

                    Repeater {
                        model: root.results

                        delegate: AppKeyValueRow {
                            required property var modelData

                            Layout.fillWidth: true
                            label: modelData.label
                            value: modelData.value
                            onCopyRequested: text => Clipboard.copy(text) ? Toast.show(qsTr("摘要已复制"), "ok")
                                                                          : Toast.show(qsTr("内容过大，无法复制"), "error")
                        }
                    }
                }
            }

            AppEmptyState {
                Layout.fillWidth: true
                visible: root.results.length === 0
                glyph: "≡"
                title: qsTr("还没有结果")
                hint: qsTr("输入文本后点击「计算」")
            }

            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 0
            }
        }
    }
}
