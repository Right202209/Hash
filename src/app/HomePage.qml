import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Item {
    id: root

    signal openRequested(string toolId)

    Label {
        anchors.centerIn: parent
        visible: grid.count === 0
        text: qsTr("没有匹配的工具")
        color: Theme.muted
    }

    GridView {
        id: grid

        anchors.fill: parent
        anchors.margins: 18
        clip: true
        model: Tools
        boundsBehavior: Flickable.StopAtBounds
        cellWidth: Math.max(240, Math.floor(width / Math.max(1, Math.floor(width / 320))))
        cellHeight: 92
        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            id: card

            width: grid.cellWidth - 12
            height: grid.cellHeight - 12

            required property string toolId
            required property string name
            required property string description
            required property string icon

            Rectangle {
                anchors.fill: parent
                radius: 6
                color: cardHover.containsMouse ? Theme.raised : Theme.surface
                border.color: cardHover.containsMouse ? Theme.ice : Theme.line

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Label {
                        Layout.preferredWidth: 36
                        horizontalAlignment: Text.AlignHCenter
                        text: card.icon
                        color: Theme.ice
                        font.pixelSize: 24
                        font.families: Theme.monoFamilies
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: card.name
                            color: Theme.text
                            font.pixelSize: 15
                        }
                        Label {
                            Layout.fillWidth: true
                            text: card.description
                            color: Theme.muted
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }
                }

                MouseArea {
                    id: cardHover

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.openRequested(card.toolId)
                }
            }
        }
    }
}
