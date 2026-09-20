import QtQuick
import QtQuick.Controls.Basic

// AppTextArea themes multi-line text input and output: inset field with the
// shared focus border, so hash/JSON/codec text areas match the other tools
// regardless of how the platform resolves the palette.
TextArea {
    id: control

    color: Theme.text
    placeholderTextColor: Theme.muted
    selectionColor: Theme.iceDeep
    selectedTextColor: Theme.text
    selectByMouse: true
    leftPadding: 8
    rightPadding: 8
    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
        radius: Theme.radius
        color: Theme.inset
        border.color: control.activeFocus ? Theme.ice : Theme.line
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.fadeMs
            }
        }
    }
}
