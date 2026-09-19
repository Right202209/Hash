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
        cellHeight: 96
        ScrollBar.vertical: AppScrollBar {}

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
                radius: Theme.cardRadius
                color: cardHover.pressed ? Theme.inset
                                         : cardHover.containsMouse ? Theme.raised : Theme.surface
                border.color: cardHover.containsMouse ? Theme.ice : Theme.line

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.fadeMs
                    }
                }
                Behavior on border.color {
                    ColorAnimation {
                        duration: Theme.fadeMs
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: Theme.radius
                        color: Theme.inset
                        border.color: cardHover.containsMouse ? Theme.ice : Theme.line

                        Behavior on border.color {
                            ColorAnimation {
                                duration: Theme.fadeMs
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            text: card.icon
                            color: Theme.ice
                            font.pixelSize: 18
                            font.family: Theme.monoFamily
                        }
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
