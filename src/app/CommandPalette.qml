import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// CommandPalette is the Ctrl+K quick switcher: type to filter the ToolSearch
// registry, move with the arrow keys, Enter or click to open, Esc to dismiss.
// It filters its own registry instance so the sidebar list never changes.
Popup {
    id: root

    signal toolChosen(string toolId)

    modal: true
    focus: true
    padding: 8
    width: parent ? Math.min(560, parent.width - 80) : 560
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: 72
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: Theme.cardRadius
        color: Theme.surface
        border.color: Theme.line
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(Theme.canvas.r, Theme.canvas.g, Theme.canvas.b, 0.7)
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
        spacing: 6

        AppTextField {
            id: query

            Layout.fillWidth: true
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

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 360)
            clip: true
            spacing: 2
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

                width: ListView.view.width
                height: 48
                radius: Theme.radius
                color: ListView.isCurrentItem ? Theme.iceDeep : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 12
                    spacing: 12

                    Label {
                        Layout.preferredWidth: 28
                        horizontalAlignment: Text.AlignHCenter
                        text: entry.icon
                        color: entry.ListView.isCurrentItem ? Theme.text : Theme.ice
                        font.pixelSize: 16
                        font.family: Theme.monoFamily
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: entry.name
                            color: Theme.text
                            font.pixelSize: 14
                        }
                        Label {
                            Layout.fillWidth: true
                            text: entry.description
                            color: Theme.muted
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }
                    Label {
                        text: entry.category
                        color: Theme.muted
                        font.pixelSize: 11
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

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            horizontalAlignment: Text.AlignHCenter
            visible: list.count === 0
            text: qsTr("没有匹配的工具")
            color: Theme.muted
        }

        Label {
            Layout.fillWidth: true
            leftPadding: 4
            text: qsTr("↑↓ 选择 · Enter 打开 · Esc 关闭")
            color: Theme.muted
            font.pixelSize: 11
        }
    }
}
