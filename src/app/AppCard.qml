import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// AppCard is the shared L1 container: surface fill, hairline border and card
// radius, with an optional small muted title. Pages use it instead of GroupBox
// and hand-written card rectangles so every panel shares one padding and edge.
// Content is declared inside the item, below the optional title.
Control {
    id: control

    property string title: ""

    leftPadding: Theme.space4
    rightPadding: Theme.space4
    topPadding: Theme.space4
    bottomPadding: Theme.space4

    implicitWidth: 200

    contentItem: ColumnLayout {
        spacing: control.title !== "" ? Theme.space3 : 0

        Label {
            Layout.fillWidth: true
            visible: control.title !== ""
            text: control.title
            color: Theme.muted
            font.pixelSize: Theme.fontSmall
        }

        ColumnLayout {
            id: body

            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.space2
        }
    }

    default property alias cardContent: body.data

    background: Rectangle {
        radius: Theme.cardRadius
        color: Theme.surface
        border.width: 1
        border.color: Theme.line
    }
}
