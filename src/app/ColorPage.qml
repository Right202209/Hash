import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Color page: the input field pairs with a 48 px swatch lying on an 8 px
// checkerboard so the alpha channel is readable; the result card lists the
// HEX / RGB / HSL forms as copyable rows.
Page {
    id: root

    property var outcome: null
    property string errorText: ""

    header: PageHeader {
        icon: "◐"
        title: qsTr("颜色转换")
        description: qsTr("HEX、RGB 与 HSL 颜色格式互相转换 · 支持 Alpha 通道")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    // channel reads one numeric component out of the rgb()/rgba() string the
    // tool returns, so the swatch can be rebuilt with the slider's alpha.
    function channel(text, index) {
        const body = text.substring(text.indexOf("(") + 1, text.length - 1).split(",")
        return Number(body[index].trim())
    }

    function convert(text) {
        if (text.trim() === "") {
            root.outcome = null
            root.errorText = ""
            return
        }
        const result = Color.convert(text)
        if (result.ok) {
            root.outcome = result
            root.errorText = ""
        } else {
            root.outcome = null
            root.errorText = result.error
        }
    }

    function copy(text) {
        if (text === "" || text === undefined) {
            return
        }
        Clipboard.copy(text) ? Toast.show(qsTr("已复制"), "ok") : Toast.show(qsTr("内容过大，无法复制"),
                                                                          "error")
    }

    // The parsed RGB with its own alpha; transparent until a colour parses.
    // The alpha is what the checkerboard behind the swatch makes visible.
    readonly property color swatchColor: root.outcome === null ? "transparent" : Qt.rgba(
                                             root.channel(root.outcome.rgba, 0) / 255.0,
                                             root.channel(root.outcome.rgba, 1) / 255.0,
                                             root.channel(root.outcome.rgba, 2) / 255.0,
                                             (root.outcome.alpha !== undefined ? root.outcome.alpha
                                                                               : 255) / 255.0)

    contentItem: ColumnLayout {
        spacing: Theme.space3

        AppCard {
            Layout.fillWidth: true
            title: qsTr("颜色值")

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space3

                AppTextField {
                    id: valueInput

                    Layout.fillWidth: true
                    placeholderText: qsTr("#RRGGBB / #RRGGBBAA 或 rgb() · hsl()")
                    selectByMouse: true
                    hasError: root.errorText !== ""
                    font.family: Theme.monoFamily
                    onTextChanged: root.convert(text)
                }

                // 48 px preview: an 8 px checkerboard under a translucent fill
                // shows the alpha instead of blending into the card.
                Rectangle {
                    Layout.preferredWidth: Theme.swatchSize
                    Layout.preferredHeight: Theme.swatchSize
                    radius: Theme.radius
                    clip: true
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.line

                    Grid {
                        anchors.fill: parent
                        columns: Math.max(1, Math.ceil(width / Theme.space2))
                        rows: Math.max(1, Math.ceil(height / Theme.space2))

                        Repeater {
                            model: parent.columns * parent.rows

                            Rectangle {
                                width: Theme.space2
                                height: Theme.space2
                                color: (Math.floor(index / parent.parent.columns)
                                        + index % parent.parent.columns) % 2 === 0 ? Theme.raised
                                                                                  : Theme.inset
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: root.swatchColor

                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.fadeMs
                            }
                        }
                    }
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
            title: qsTr("结果")
            visible: root.outcome !== null

            // The 48 px swatch with its checkerboard, repeated so the alpha is
            // readable even after the field scrolled out of view. The board
            // sits between the colour and the rows, matching the field card.
            Rectangle {
                Layout.preferredWidth: Theme.swatchSize
                Layout.preferredHeight: Theme.swatchSize
                radius: Theme.radius
                clip: true
                color: "transparent"
                border.width: 1
                border.color: Theme.line

                Grid {
                    anchors.fill: parent
                    columns: Math.max(1, Math.ceil(width / Theme.space2))
                    rows: Math.max(1, Math.ceil(height / Theme.space2))

                    Repeater {
                        model: parent.columns * parent.rows

                        Rectangle {
                            width: Theme.space2
                            height: Theme.space2
                            color: (Math.floor(index / parent.parent.columns)
                                    + index % parent.parent.columns) % 2 === 0 ? Theme.raised
                                                                              : Theme.inset
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: root.swatchColor

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.fadeMs
                        }
                    }
                }
            }

            AppKeyValueRow {
                Layout.fillWidth: true
                label: "HEX"
                value: root.outcome !== null ? root.outcome.hex : ""
                onCopyRequested: text => root.copy(text)
            }
            AppKeyValueRow {
                Layout.fillWidth: true
                label: "RGB"
                value: root.outcome !== null ? root.outcome.rgba : ""
                onCopyRequested: text => root.copy(text)
            }
            AppKeyValueRow {
                Layout.fillWidth: true
                label: "HSL"
                value: root.outcome !== null ? root.outcome.hsl : ""
                onCopyRequested: text => root.copy(text)
            }
        }

        AppEmptyState {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.outcome === null && root.errorText === ""
            glyph: "◐"
            title: qsTr("还没有颜色")
            hint: qsTr("输入 HEX、rgb() 或 hsl() 颜色值")
        }

        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }
    }
}
