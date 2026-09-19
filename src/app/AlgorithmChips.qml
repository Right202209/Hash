import QtQuick
import QtQuick.Controls.Basic
import HashTools

// AlgorithmChips renders the registered digest algorithms as toggle chips.
// Both hash pages share this component so the chips look and behave
// identically everywhere.
Flow {
    id: root

    // Registry names checked when the chips are created; toggling afterwards
    // is free.
    property var defaults: ["sha256"]

    signal selectionChanged()

    function selectedNames() {
        var names = []
        for (var i = 0; i < chips.count; ++i) {
            const chip = chips.itemAt(i)
            if (chip.checked) {
                names.push(chip.name)
            }
        }
        return names
    }

    spacing: 6

    Repeater {
        id: chips

        model: Algorithms

        delegate: AbstractButton {
            id: chip

            required property string name
            required property string label

            checkable: true
            checked: root.defaults.indexOf(chip.name) !== -1
            onToggled: root.selectionChanged()
            hoverEnabled: true
            cursor: Qt.PointingHandCursor
            leftPadding: 12
            rightPadding: 12
            topPadding: 5
            bottomPadding: 5

            contentItem: Row {
                spacing: 6

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 12
                    text: chip.checked ? "✓" : ""
                    color: Theme.ice
                    font.pixelSize: 12
                    font.family: Theme.monoFamily
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: chip.label
                    color: chip.checked ? Theme.text : Theme.muted
                }
            }

            background: Rectangle {
                radius: Theme.radius + 2
                color: chip.checked ? Theme.raised : chip.hovered ? Theme.surface : Theme.inset
                border.color: chip.checked ? Theme.ice : chip.hovered ? Theme.muted : Theme.line
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
    }
}
