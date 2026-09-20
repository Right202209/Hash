import QtQuick
import QtQuick.Controls.Basic

// AppTextField themes single-line input (command bar, timestamp and radix
// fields): inset field, ice focus border and shared text colors.
TextField {
    id: control

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
        implicitHeight: 32
        radius: Theme.radius
        color: Theme.inset
        border.color: control.activeFocus ? Theme.ice : control.hovered ? Theme.muted : Theme.line
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.fadeMs
            }
        }
    }
}
