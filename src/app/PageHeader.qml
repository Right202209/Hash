import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// PageHeader is the shared tool page header: glyph, title and description.
// The rule underneath spans the content area only (inset by pagePadding), and
// the description stays on one line with the full text in a tooltip. The
// sidebar shell switches tools directly, so the back button is hidden by
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
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        height: 1
        color: Theme.line
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.showBack ? 10 : Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.space3

        AppToolButton {
            visible: root.showBack
            text: qsTr("←")
            tip: qsTr("返回")
            font.pixelSize: Theme.fontTitle
            onClicked: root.backRequested()
        }
        Rectangle {
            Layout.preferredWidth: Theme.iconBlockLg
            Layout.preferredHeight: Theme.iconBlockLg
            radius: Theme.radius
            color: Theme.inset
            border.width: 1
            border.color: Theme.line

            Label {
                anchors.centerIn: parent
                text: root.icon
                color: Theme.ice
                font.pixelSize: Theme.fontTitle
                font.family: Theme.monoFamily
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                text: root.title
                color: Theme.text
                font.pixelSize: Theme.fontTitle
                font.bold: true
            }
            Label {
                id: descriptionLabel

                Layout.fillWidth: true
                text: root.description
                color: Theme.muted
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight

                HoverHandler {
                    id: descriptionHover
                }
                ToolTip.visible: descriptionHover.hovered && descriptionLabel.truncated
                ToolTip.delay: 600
                ToolTip.text: root.description
            }
        }
    }
}
