import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// CommandPalette is the Ctrl+K quick switcher: type to filter the ToolSearch
// registry, move with the arrow keys, Enter or click to open, Esc to dismiss.
// It filters its own registry instance so the sidebar list never changes.
// The panel is the L3 layer: a scrim under it plus an inner top highlight
// instead of a drop shadow. Only opacity animates; the palette never scales
// or slides.
Popup {
    id: root

    signal toolChosen(string toolId)

    modal: true
    focus: true
    padding: Theme.space2
    width: parent ? Math.min(560, parent.width - 2 * Theme.space6) : 560
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: 96
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    enter: Transition {
        OpacityAnimator {
            from: 0
            to: 1
            duration: Theme.overlayMs
        }
    }
    exit: Transition {
        OpacityAnimator {
            from: 1
            to: 0
            duration: Theme.overlayMs
        }
    }

    background: Rectangle {
        radius: Theme.cardRadius
        color: Theme.surface
        border.width: 1
        border.color: Theme.line

        // Inner top edge: a 1 px light line that reads as "above the page"
        // without a shadow.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 1
            height: 1
            color: Theme.edgeHighlight
        }
    }

    Overlay.modal: Rectangle {
        color: Theme.scrim

        Behavior on opacity {
            NumberAnimation {
                duration: Theme.overlayMs
            }
        }
    }

    onOpened: {
        query.text = ""
        ToolSearch.filter = ""
        list.currentIndex = 0
        query.forceActiveFocus()
    }

    function choose(index) {
        if (index < 0 || index >= list.count) {
            return
        }
        const item = list.itemAtIndex(index)
        if (item) {
            root.close()
            root.toolChosen(item.toolId)
        }
    }

    function move(step) {
        if (list.count > 0) {
            list.currentIndex = (list.currentIndex + step + list.count) % list.count
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.space2

        // Input area: borderless, one hairline separating it from the rows.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "transparent"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.line
            }

            TextField {
                id: query

                anchors.fill: parent
                color: Theme.text
                placeholderTextColor: Theme.muted
                selectionColor: Theme.iceDeep
                selectedTextColor: Theme.text
                selectByMouse: true
                leftPadding: Theme.space1
                rightPadding: Theme.space1
                verticalAlignment: TextInput.AlignVCenter
                font.pixelSize: Theme.fontBody
                background: null
                placeholderText: qsTr("输入工具名称或关键字，例如 sha256、json、时间戳")
                onTextChanged: {
                    ToolSearch.filter = text
                    list.currentIndex = 0
                }
                Keys.onUpPressed: root.move(-1)
                Keys.onDownPressed: root.move(1)
                Keys.onReturnPressed: root.choose(list.currentIndex)
                Keys.onEnterPressed: root.choose(list.currentIndex)
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 8 * Theme.rowHeight)
            clip: true
            spacing: Theme.space1
            model: ToolSearch
            currentIndex: 0
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar {}

            delegate: Rectangle {
                id: entry

                required property int index
                required property string toolId
                required property string name
                required property string description
                required property string icon
                required property string category

                readonly property bool current: ListView.isCurrentItem

                width: ListView.view.width
                height: Theme.rowHeight
                radius: Theme.radius
                color: entry.current ? Theme.iceDeep : "transparent"

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.fadeMs
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space1
                    anchors.rightMargin: Theme.space2
                    spacing: Theme.space3

                    // Fixed-width icon block keeps the fallback font's glyph
                    // width from shifting the row.
                    Rectangle {
                        Layout.preferredWidth: Theme.iconBlockSm
                        Layout.preferredHeight: Theme.iconBlockSm
                        radius: Theme.radiusSm
                        color: entry.current ? Theme.ice : Theme.inset

                        Label {
                            anchors.centerIn: parent
                            text: entry.icon
                            color: entry.current ? Theme.canvas : Theme.ice
                            font.pixelSize: Theme.fontSmall
                            font.family: Theme.monoFamily
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: entry.name
                            color: Theme.text
                            font.pixelSize: Theme.fontSmall
                        }
                        Label {
                            Layout.fillWidth: true
                            text: entry.description
                            color: entry.current ? Theme.mutedOnAccent : Theme.muted
                            font.pixelSize: Theme.fontCaption
                            elide: Text.ElideRight
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: categoryText.implicitWidth + 2 * Theme.space2
                        Layout.preferredHeight: 20
                        radius: Theme.radiusPill
                        color: entry.current ? Theme.iceDeep : Theme.inset
                        border.width: 1
                        border.color: Theme.line

                        Label {
                            id: categoryText

                            anchors.centerIn: parent
                            text: entry.category
                            color: entry.current ? Theme.mutedOnAccent : Theme.muted
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: list.currentIndex = entry.index
                    onClicked: root.choose(entry.index)
                }
            }
        }

        AppEmptyState {
            Layout.fillWidth: true
            Layout.topMargin: 0
            Layout.bottomMargin: 0
            visible: list.count === 0
            compact: true
            title: qsTr("没有匹配的工具")
            hint: qsTr("试试 sha256、json、时间戳")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Layout.leftMargin: Theme.space1
            spacing: Theme.space1

            AppKbd {
                text: "↑"
            }
            AppKbd {
                text: "↓"
            }
            Label {
                text: qsTr("选择")
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }
            Label {
                text: "·"
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }
            AppKbd {
                text: "Enter"
            }
            Label {
                text: qsTr("打开")
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }
            Label {
                text: "·"
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }
            AppKbd {
                text: "Esc"
            }
            Label {
                text: qsTr("关闭")
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }
        }
    }
}
