import QtQuick

// AppKbd renders a keyboard key cap: inset fill, hairline border and the
// monospace caption size. Used by the sidebar search hint, the status bar
// shortcuts and the command palette footer.
Rectangle {
    id: root

    property alias text: label.text

    implicitWidth: Math.max(18, label.implicitWidth + 2 * Theme.space1)
    implicitHeight: 18
    radius: Theme.radiusSm
    color: Theme.inset
    border.width: 1
    border.color: Theme.line

    Text {
        id: label

        anchors.centerIn: parent
        color: Theme.muted
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontCaption
    }
}
