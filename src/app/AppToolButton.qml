import QtQuick
import QtQuick.Controls.Basic

// AppToolButton themes the small glyph buttons (back arrow, row removal,
// digest copy): transparent on the page surface with a raised hover tile,
// independent of the ambient palette. tip supplies both the tooltip and the
// accessible name, so icon-only buttons always have a text alternative.
ToolButton {
    id: control

    property string tip: ""

    // Icon-only buttons must stay reachable with a pointer, so the hit target
    // never shrinks below the control height even for a narrow glyph.
    implicitHeight: Theme.controlHeight
    leftPadding: Theme.space2
    rightPadding: Theme.space2
    topPadding: Theme.space1
    bottomPadding: Theme.space1

    ToolTip.visible: control.hovered && control.tip !== ""
    ToolTip.delay: 600
    ToolTip.text: control.tip

    Accessible.name: control.tip

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Label {
        text: control.text
        font: control.font
        font.pixelSize: control.font.pixelSize > 0 ? control.font.pixelSize : Theme.fontBody
        color: control.enabled ? (control.hovered || control.pressed ? Theme.text : Theme.muted)
                               : Theme.muted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: control.pressed ? Theme.inset : control.hovered ? Theme.raised : "transparent"
        Behavior on color {
            ColorAnimation {
                duration: Theme.fadeMs
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: Theme.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.focusRing
            visible: control.visualFocus
        }
    }
}
