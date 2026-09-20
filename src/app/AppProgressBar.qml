import QtQuick
import QtQuick.Controls.Basic

// AppProgressBar themes the batch-progress bar with the shared track and
// ice accent so busy feedback matches the toolbox look.
ProgressBar {
    id: control

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 6
        radius: 3
        color: Theme.inset
    }

    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 6

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 3
            color: Theme.ice
        }
    }
}
