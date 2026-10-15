import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Sidebar is the shell's permanent navigation: the brand row returns to the
// overview, the search field opens the command palette, and every tool is
// listed under its category so switching never goes through the overview.
// It reads the unfiltered Tools registry; the palette filters ToolSearch.
Rectangle {
    id: root

    property string currentToolId: ""

    signal toolSelected(string toolId)
    signal homeRequested()
    signal searchRequested()

    implicitWidth: Theme.sidebarWidth
    color: Theme.surface

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: Theme.line
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space3
        anchors.topMargin: Theme.space3
        anchors.rightMargin: Theme.space3
        anchors.bottomMargin: Theme.space3
        spacing: Theme.space3

        // Brand row: clicking returns to the overview.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            radius: Theme.radius
            color: brandHover.containsMouse || root.currentToolId === "" ? Theme.raised
                                                                          : "transparent"

            Behavior on color {
                ColorAnimation {
                    duration: Theme.fadeMs
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.space2
                anchors.rightMargin: Theme.space2
                spacing: Theme.space2

                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    radius: Theme.radius
                    color: Theme.iceDeep

                    Label {
                        anchors.centerIn: parent
                        text: "#"
                        color: Theme.ice
                        font.pixelSize: Theme.fontTitle
                        font.bold: true
                        font.family: Theme.monoFamily
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Label {
                        text: "Hash"
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        font.bold: true
                    }
                    Label {
                        text: qsTr("工具箱 · 概览")
                        color: Theme.muted
                        font.pixelSize: Theme.fontCaption
                    }
                }
            }

            MouseArea {
                id: brandHover

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.homeRequested()
            }
        }

        // Search field: opens the command palette.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.controlHeight
            radius: Theme.radius
            color: searchHover.containsMouse ? Theme.raised : Theme.inset
            border.width: 1
            border.color: searchHover.containsMouse ? Theme.ice : Theme.line

            Behavior on border.color {
                ColorAnimation {
                    duration: Theme.fadeMs
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.space2
                anchors.rightMargin: Theme.space1
                spacing: Theme.space1

                Label {
                    text: "⌕"
                    color: searchHover.containsMouse ? Theme.ice : Theme.muted
                    font.pixelSize: Theme.fontSmall
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("搜索工具")
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
                }
            }

            MouseArea {
                id: searchHover

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.searchRequested()
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.space1
            model: Tools
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar {}

            section.property: "category"
            section.delegate: Label {
                required property string section

                width: ListView.view.width
                topPadding: Theme.space3
                bottomPadding: Theme.space1
                leftPadding: Theme.space2
                text: section
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
                font.letterSpacing: 1
            }

            delegate: Rectangle {
                id: row

                required property string toolId
                required property string name
                required property string icon

                readonly property bool active: toolId === root.currentToolId

                width: ListView.view.width
                height: Theme.rowHeight
                radius: Theme.radius
                color: active ? Theme.iceDeep : rowHover.containsMouse ? Theme.raised : "transparent"

                Behavior on color {
                    ColorAnimation {
                        duration: Theme.fadeMs
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: 18
                    radius: Theme.radiusSm
                    color: Theme.ice
                    visible: row.active
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space2
                    anchors.rightMargin: Theme.space2
                    spacing: Theme.space3

                    Label {
                        Layout.preferredWidth: Theme.space5
                        horizontalAlignment: Text.AlignHCenter
                        text: row.icon
                        color: row.active ? Theme.text : Theme.ice
                        font.pixelSize: Theme.fontSmall
                        font.family: Theme.monoFamily
                    }
                    Label {
                        Layout.fillWidth: true
                        text: row.name
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: rowHover

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toolSelected(row.toolId)
                }
            }
        }
    }
}