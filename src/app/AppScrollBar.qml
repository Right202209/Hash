import QtQuick
import QtQuick.Controls.Basic

// AppScrollBar is the toolbox-wide scroll indicator: a slim rounded thumb on
// an invisible track that fades in only while active, using the shared Theme
// colors so every scrolling page looks the same.
ScrollBar {
    id: control

    implicitWidth: Theme.space3
    implicitHeight: Theme.space3
    padding: 3
    visible: control.size < 1.0

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6
        radius: 3
        color: control.pressed ? Theme.ice : Theme.line
        opacity: control.active ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.fadeMs
            }
        }
        Behavior on color {
            ColorAnimation {
                duration: Theme.fadeMs
            }
        }
    }
}
