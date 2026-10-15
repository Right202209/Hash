import QtQuick
import QtQuick.Controls.Basic

// AppButton is the toolbox-wide push button: shared fills, radius, a keyboard
// focus ring and fade animations instead of the Basic style's palette-derived
// look, which does not reliably pick up the dark palette on every platform.
// variant selects the weight: "primary" (deep ice, one per view), "secondary"
// (default), "ghost" (secondary actions) and "danger" (clear/cancel). The
// legacy highlighted property keeps behaving as primary.
Button {
    id: control

    property string variant: "secondary"

    // highlighted is the pre-v1.2 spelling of variant === "primary"; both are
    // honoured so existing pages keep working unchanged.
    readonly property bool isPrimary: control.variant === "primary" || control.highlighted
    readonly property bool isGhost: control.variant === "ghost"
    readonly property bool isDanger: control.variant === "danger"

    leftPadding: Theme.space3
    rightPadding: Theme.space3
    topPadding: 5
    bottomPadding: 5

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: !control.enabled ? Theme.muted : control.isDanger ? Theme.danger : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: !control.enabled ? Theme.canvas
                                : control.isDanger ? (control.down ? Theme.inset : Theme.dangerSoft)
                                                   : control.isPrimary ? (control.down ? Theme.canvas : Theme.iceDeep)
                                                                      : control.isGhost ? (control.down ? Theme.inset
                                                                                                        : control.hovered ? Theme.raised
                                                                                                                          : "transparent")
                                                                                        : control.down ? Theme.inset
                                                                                                       : control.hovered ? Theme.raised
                                                                                                                         : Theme.surface
        border.width: 1
        border.color: !control.enabled ? Theme.line
                                       : control.isDanger ? Theme.danger
                                                          : control.isPrimary ? Theme.ice
                                                                              : control.isGhost ? (control.hovered ? Theme.muted : "transparent")
                                                                                                : control.down
                                                                                                  || control.hovered ? Theme.muted : Theme.line
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

        // Keyboard focus ring: drawn only for keyboard navigation, outside the
        // button so the fill stays untouched.
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
