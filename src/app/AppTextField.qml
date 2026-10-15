import QtQuick
import QtQuick.Controls.Basic

// AppTextField themes single-line input (command bar, timestamp and radix
// fields): inset field, ice focus border and shared text colors. hasError
// switches the border to danger, always shown together with the neighbouring
// error label so the state is not conveyed by color alone.
TextField {
    id: control

    property bool hasError: false

    color: Theme.text
    placeholderTextColor: Theme.muted
    selectionColor: Theme.iceDeep
    selectedTextColor: Theme.text
    selectByMouse: true
    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: Theme.inset
        border.width: 1
        border.color: control.hasError ? Theme.danger
                                       : control.activeFocus ? Theme.ice
                                                             : control.hovered ? Theme.muted : Theme.line
        Behavior on border.color {
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
