import QtQuick
import QtQuick.Controls.Basic

// AppProgressBar themes the batch-progress bar: a slim inset track with a
// rounded ice fill and no stripes or glow animation, so a long run never
// costs a repaint per frame.
ProgressBar {
    id: control

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 6
        radius: Theme.radiusSm
        color: Theme.inset
    }

    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 6

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: Theme.radiusSm
            color: Theme.ice
        }
    }
}
