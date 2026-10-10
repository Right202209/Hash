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
        anchors.margins: 12
        anchors.rightMargin: 13
        spacing: 12

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
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    radius: Theme.radius
                    color: Theme.iceDeep

                    Label {
                        anchors.centerIn: parent
                        text: "#"
                        color: Theme.ice
                        font.pixelSize: 18
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
                        font.pixelSize: 16
                        font.bold: true
                    }
                    Label {
                        text: qsTr("工具箱 · 概览")
                        color: Theme.muted
                        font.pixelSize: 11
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            radius: Theme.radius
            color: searchHover.containsMouse ? Theme.raised : Theme.inset
            border.color: searchHover.containsMouse ? Theme.ice : Theme.line

            Behavior on border.color {
                ColorAnimation {
                    duration: Theme.fadeMs
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    text: qsTr("搜索工具…")
                    color: Theme.muted
                    font.pixelSize: 13
                }
                Rectangle {
                    Layout.preferredWidth: kbdLabel.implicitWidth + 10
                    Layout.preferredHeight: 20
                    radius: 4
                    color: Theme.canvas
                    border.color: Theme.line

                    Label {
                        id: kbdLabel

                        anchors.centerIn: parent
                        text: "Ctrl K"
                        color: Theme.muted
                        font.pixelSize: 11
                        font.family: Theme.monoFamily
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
            spacing: 2
            model: Tools
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar {}

            section.property: "category"
            section.delegate: Label {
                required property string section

                width: ListView.view.width
                topPadding: 12
                bottomPadding: 4
                leftPadding: 8
                text: section
                color: Theme.muted
                font.pixelSize: 11
                font.letterSpacing: 1
            }

            delegate: Rectangle {
                id: row

                required property string toolId
                required property string name
                required property string icon

                readonly property bool active: toolId === root.currentToolId

                width: ListView.view.width
                height: 34
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
                    height: 16
                    radius: 2
                    color: Theme.ice
                    visible: row.active
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 8
                    spacing: 10

                    Label {
                        Layout.preferredWidth: 24
                        horizontalAlignment: Text.AlignHCenter
                        text: row.icon
                        color: row.active ? Theme.text : Theme.ice
                        font.pixelSize: 14
                        font.family: Theme.monoFamily
                    }
                    Label {
                        Layout.fillWidth: true
                        text: row.name
                        color: Theme.text
                        font.pixelSize: 14
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
