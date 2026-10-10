import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// HomePage is the overview shown when no tool is selected: every tool as a
// card with its category, opened with one click.
Item {
    id: root

    signal openRequested(string toolId)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding + 8
        spacing: 18

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: qsTr("所有工具")
                color: Theme.text
                font.pixelSize: 24
                font.bold: true
            }
            Label {
                text: qsTr("从左侧选择，或按 Ctrl+K 快速搜索 · 共 %1 个工具").arg(grid.count)
                color: Theme.muted
                font.pixelSize: 13
            }
        }

        GridView {
            id: grid

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: Tools
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 300)))
            cellHeight: 116
            ScrollBar.vertical: AppScrollBar {}

            delegate: Item {
                id: card

                required property string toolId
                required property string name
                required property string description
                required property string icon
                required property string category

                width: grid.cellWidth - 12
                height: grid.cellHeight - 12

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

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Rectangle {
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 36
                                radius: Theme.radius
                                color: cardHover.containsMouse ? Theme.iceDeep : Theme.inset

                                Behavior on color {
                                    ColorAnimation {
                                        duration: Theme.fadeMs
                                    }
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: card.icon
                                    color: Theme.ice
                                    font.pixelSize: 16
                                    font.family: Theme.monoFamily
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: card.name
                                color: Theme.text
                                font.pixelSize: 15
                                font.bold: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.preferredWidth: categoryLabel.implicitWidth + 12
                                Layout.preferredHeight: 20
                                radius: 4
                                color: Theme.inset

                                Label {
                                    id: categoryLabel

                                    anchors.centerIn: parent
                                    text: card.category
                                    color: Theme.muted
                                    font.pixelSize: 11
                                }
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: card.description
                            color: Theme.muted
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
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
