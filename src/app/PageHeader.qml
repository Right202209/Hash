import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// PageHeader is the shared tool page header: back button, glyph, title and
// description. It pops the page through the attached StackView.
Item {
    id: root

    property string icon: ""
    property string title: ""
    property string description: ""

    implicitHeight: 60

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 16
        spacing: 10

        ToolButton {
            text: qsTr("←")
            font.pixelSize: 18
            onClicked: {
                if (root.StackView.view) {
                    root.StackView.view.pop()
                }
            }
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
