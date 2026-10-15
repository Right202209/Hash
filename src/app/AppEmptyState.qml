import QtQuick
import QtQuick.Layouts

// AppEmptyState is the centered placeholder shown when a list or result area
// has nothing to display: a 40 px glyph, a body-size title and a small muted
// hint, with roomy vertical padding.
ColumnLayout {
    id: root

    property string glyph: ""
    property string title: ""
    property string hint: ""
    // compact trims the vertical breathing room for use inside popups.
    property bool compact: false

    spacing: Theme.space2

    Label {
        Layout.fillWidth: true
        Layout.topMargin: root.compact ? Theme.space3 : Theme.space8
        Layout.bottomMargin: root.compact ? Theme.space3 : Theme.space8
        horizontalAlignment: Text.AlignHCenter
        text: root.glyph
        visible: root.glyph !== ""
        color: Theme.muted
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontGlyph
    }

    Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: root.title
        color: Theme.text
        font.pixelSize: Theme.fontBody
    }

    Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: root.hint
        visible: root.hint !== ""
        color: Theme.muted
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }
}
