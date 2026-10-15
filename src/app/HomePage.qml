import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// HomePage is the overview shown when no tool is selected: every tool as a
// card with its category, opened with one click or with Enter/Space once a
// card has keyboard focus.
Item {
    id: root

    signal openRequested(string toolId)

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        anchors.topMargin: Theme.pagePadding
        spacing: Theme.space3

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space1

            Label {
                text: qsTr("所有工具")
                color: Theme.text
                font.pixelSize: Theme.fontDisplay
                font.bold: true
            }
            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: qsTr("按 Ctrl+K 快速搜索 · 共 %1 个工具").arg(Tools.count)
                    color: Theme.muted
                    font.pixelSize: Theme.fontSmall
                }
                RowLayout {
                    spacing: Theme.space1

                    AppKbd {
                        text: "Ctrl"
                    }
                    AppKbd {
                        text: "K"
                    }
                    Label {
                        text: qsTr("快速搜索")
                        color: Theme.muted
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }

        GridView {
            id: grid

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: Tools
            boundsBehavior: Flickable.StopAtBounds
            activeFocusOnTab: true
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 300)))
            cellHeight: 116
            ScrollBar.vertical: AppScrollBar {}

            Keys.onReturnPressed: {
                if (grid.currentItem) {
                    root.openRequested(grid.currentItem.toolId)
                }
            }
            Keys.onSpacePressed: {
                if (grid.currentItem) {
                    root.openRequested(grid.currentItem.toolId)
                }
            }

            delegate: Item {
                id: card

                required property string toolId
                required property string name
                required property string description
                required property string icon
                required property string category

                width: grid.cellWidth - Theme.space3
                height: grid.cellHeight - Theme.space3

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.cardRadius
                    color: cardHover.pressed ? Theme.inset
                                             : cardHover.containsMouse ? Theme.raised : Theme.surface
                    border.width: 1
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
                        anchors.margins: Theme.space4
                        anchors.topMargin: Theme.space3
                        spacing: Theme.space1

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space2

                            Rectangle {
                                Layout.preferredWidth: Theme.iconBlock
                                Layout.preferredHeight: Theme.iconBlock
                                radius: Theme.radius
                                color: cardHover.containsMouse ? Theme.raised : Theme.inset

                                Behavior on color {
                                    ColorAnimation {
                                        duration: Theme.fadeMs
                                    }
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: card.icon
                                    color: cardHover.containsMouse ? Theme.ice : Theme.muted
                                    font.pixelSize: Theme.fontTitle
                                    font.family: Theme.monoFamily
                                }
                            }
                            Label {
                                Layout.fillWidth: true
                                text: card.name
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: card.description
                            color: Theme.muted
                            font.pixelSize: Theme.fontSmall
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
                    onClicked: {
                        grid.currentIndex = card.index
                        root.openRequested(card.toolId)
                    }
                }

                // Keyboard focus ring on the current card.
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: Theme.cardRadius + 3
                    color: "transparent"
                    border.width: 2
                    border.color: Theme.focusRing
                    visible: card.GridView.isCurrentItem && grid.activeFocus
                }
            }
        }
    }
}