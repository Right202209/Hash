import QtQuick
import QtQuick.Controls.Basic

// AppCheckBox themes the option checkboxes (password and UUID tool): the
// checked indicator fills with the deep ice accent and the shared border
// colors, so the options read as part of the same toolbox as the chips.
CheckBox {
    id: control

    spacing: Theme.space2

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: Theme.radiusSm
        color: control.checked ? Theme.iceDeep
                               : control.down ? Theme.canvas
                                              : control.hovered ? Theme.raised : Theme.inset
        border.width: 1
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

        Label {
            anchors.centerIn: parent
            text: "✓"
            visible: control.checked
            color: Theme.text
            font.pixelSize: Theme.fontSmall
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: Theme.radiusSm + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.focusRing
            visible: control.visualFocus
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
