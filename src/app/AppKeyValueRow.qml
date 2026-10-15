import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// AppKeyValueRow is the shared "label + monospace value + copy" row used by
// the text-hash, timestamp, radix and color pages. The value elides in the
// middle (root cause of long hashes is the tail) and the copy button is
// disabled while there is nothing to copy. wrapValue switches to a wrapped
// monospace block for values that must stay fully visible (digest summaries).
RowLayout {
    id: root

    property string label: ""
    property string value: ""
    property bool wrapValue: false

    signal copyRequested(string text)

    Layout.fillWidth: true
    spacing: Theme.space2

    Label {
        Layout.preferredWidth: Theme.kvLabelWidth
        text: root.label
        color: Theme.ice
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontSmall
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    Label {
        Layout.fillWidth: true
        text: root.value
        visible: !root.wrapValue
        color: Theme.text
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontMono
        elide: Text.ElideMiddle
        verticalAlignment: Text.AlignVCenter
    }

    Label {
        Layout.fillWidth: true
        text: root.value
        visible: root.wrapValue
        color: Theme.text
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontMono
        wrapMode: Text.WrapAnywhere
        maximumLineCount: 4
        elide: Text.ElideRight
    }

    AppToolButton {
        text: "⧉"
        tip: qsTr("复制")
        enabled: root.value.length > 0
        onClicked: root.copyRequested(root.value)
    }
}
