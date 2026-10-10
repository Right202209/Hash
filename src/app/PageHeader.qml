import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// PageHeader is the shared tool page header: glyph, title and description.
// The sidebar shell switches tools directly, so the back button is hidden by
// default; showBack restores it for a page hosted in a StackView, which wires
// backRequested to that view.
Item {
    id: root

    property string icon: ""
    property string title: ""
    property string description: ""
    property bool showBack: false

    signal backRequested()

    implicitHeight: 68

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.line
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.showBack ? 10 : Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: 12

        AppToolButton {
            visible: root.showBack
            text: qsTr("←")
            font.pixelSize: 18
            onClicked: root.backRequested()
        }
        Rectangle {
            Layout.preferredWidth: 38
            Layout.preferredHeight: 38
            radius: Theme.radius
            color: Theme.inset
            border.color: Theme.line

            Label {
                anchors.centerIn: parent
                text: root.icon
                color: Theme.ice
                font.pixelSize: 18
                font.family: Theme.monoFamily
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                text: root.title
                color: Theme.text
                font.pixelSize: 18
                font.bold: true
            }
            Label {
                text: root.description
                color: Theme.muted
                font.pixelSize: 12
            }
        }
    }
}
