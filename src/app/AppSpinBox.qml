import QtQuick
import QtQuick.Controls.Basic

// AppSpinBox themes the numeric steppers (password length/count, UUID
// count): inset field with the shared focus border and compact glyph
// buttons on either side of the value.
SpinBox {
    id: control

    leftPadding: 30
    rightPadding: 30
    topPadding: 5
    bottomPadding: 5

    background: Rectangle {
        implicitWidth: 110
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: Theme.inset
        border.width: 1
        border.color: control.activeFocus ? Theme.ice : control.hovered ? Theme.muted : Theme.line
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

    contentItem: TextInput {
        text: control.editText
        font: control.font
        color: Theme.text
        selectionColor: Theme.iceDeep
        selectedTextColor: Theme.text
        selectByMouse: true
        horizontalAlignment: TextInput.AlignHCenter
        verticalAlignment: TextInput.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
    }

    up.indicator: Rectangle {
        x: parent.width - width
        y: (parent.height - height) / 2
        implicitWidth: 24
        implicitHeight: 24
        radius: Theme.radiusSm
        color: control.up.pressed ? Theme.inset : control.up.hovered ? Theme.raised : "transparent"

        Label {
            anchors.centerIn: parent
            text: "▲"
            color: control.up.enabled
                   && (control.up.hovered || control.up.pressed) ? Theme.text : Theme.muted
            font.pixelSize: Theme.fontCaption
        }
    }

    down.indicator: Rectangle {
        x: 0
        y: (parent.height - height) / 2
        implicitWidth: 24
        implicitHeight: 24
        radius: Theme.radiusSm
        color: control.down.pressed ? Theme.inset : control.down.hovered ? Theme.raised : "transparent"

        Label {
            anchors.centerIn: parent
            text: "▼"
            color: control.down.enabled
                   && (control.down.hovered || control.down.pressed) ? Theme.text : Theme.muted
            font.pixelSize: Theme.fontCaption
        }
    }
}
