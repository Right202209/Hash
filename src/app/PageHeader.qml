import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// PageHeader is the shared tool page header: back button, glyph, title and
// description. The page wires backRequested to its own StackView, which is
// where the attached property is guaranteed to resolve.
Item {
    id: root

    property string icon: ""
    property string title: ""
    property string description: ""

    signal backRequested()

    implicitHeight: 60

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 16
        spacing: 10

        ToolButton {
            text: qsTr("←")
            font.pixelSize: 18
            onClicked: root.backRequested()
        }
        Label {
            Layout.preferredWidth: 34
            horizontalAlignment: Text.AlignHCenter
            text: root.icon
            color: Theme.ice
            font.pixelSize: 22
            font.families: Theme.monoFamilies
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
                text: root.title
                color: Theme.text
                font.pixelSize: 17
            }
            Label {
                text: root.description
                color: Theme.muted
                font.pixelSize: 12
            }
        }
    }
}
