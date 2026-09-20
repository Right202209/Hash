import QtQuick
import QtQuick.Controls.Basic

// AppToolButton themes the small glyph buttons (back arrow, row removal,
// digest copy): transparent on the page surface with a raised hover tile,
// independent of the ambient palette.
ToolButton {
    id: control

    leftPadding: 8
    rightPadding: 8
    topPadding: 4
    bottomPadding: 4

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.enabled ? (control.hovered || control.pressed ? Theme.text : Theme.muted)
                               : Theme.muted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitHeight: 30
        radius: Theme.radius
        color: control.pressed ? Theme.inset : control.hovered ? Theme.raised : "transparent"
        Behavior on color {
            ColorAnimation {
                duration: Theme.fadeMs
            }
        }
    }
}
