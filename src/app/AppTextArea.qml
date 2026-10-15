import QtQuick
import QtQuick.Controls.Basic

// AppTextArea themes multi-line text input and output: inset field with the
// shared focus border and an optional danger border for invalid input, so
// hash/JSON/codec text areas match the other tools regardless of how the
// platform resolves the palette.
TextArea {
    id: control

    property bool hasError: false

    color: Theme.text
    placeholderTextColor: Theme.muted
    selectionColor: Theme.iceDeep
    selectedTextColor: Theme.text
    selectByMouse: true
    leftPadding: Theme.space2
    rightPadding: Theme.space2
    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
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
    }
}
