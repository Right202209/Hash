import QtQuick
import QtQuick.Controls.Basic

// AppRadioButton themes the unit switches (timestamp page): a ring that
// fills with the deep ice accent and an ice dot when selected.
RadioButton {
    id: control

    spacing: 8

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        radius: width / 2
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        color: control.checked ? Theme.iceDeep
                               : control.down ? Theme.canvas
                                              : control.hovered ? Theme.raised : Theme.inset
        border.color: control.checked ? Theme.ice
                                      : control.down || control.hovered ? Theme.muted : Theme.line
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

        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            radius: 4
            color: Theme.ice
            visible: control.checked
        }
    }

    contentItem: Label {
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.muted
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
