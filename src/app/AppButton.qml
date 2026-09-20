import QtQuick
import QtQuick.Controls.Basic

// AppButton is the toolbox-wide push button: shared fills, radius and fade
// animations instead of the Basic style's palette-derived look, which does
// not reliably pick up the dark palette on every platform. highlighted
// buttons use the deep ice accent, matching the chips' checked state.
Button {
    id: control

    leftPadding: 12
    rightPadding: 12
    topPadding: 5
    bottomPadding: 5

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.muted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitHeight: 32
        radius: Theme.radius
        color: !control.enabled ? Theme.canvas
                                : control.highlighted ? (control.down ? Theme.canvas : Theme.iceDeep)
                                                      : control.down ? Theme.inset
                                                                     : control.hovered ? Theme.raised
                                                                                       : Theme.surface
        border.color: !control.enabled ? Theme.line
                                       : control.highlighted ? Theme.ice
                                                             : control.down || control.hovered ? Theme.muted
                                                                                               : Theme.line
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
    }
}
