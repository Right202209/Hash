import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// AppBanner is the inline result strip (run summary, comparison result): a
// soft tinted fill, a 3 px solid edge bar and a leading glyph, so the state is
// readable without relying on color alone. kind is "ok" | "danger" | "info".
Control {
    id: control

    property string kind: "info"
    property string title: ""

    readonly property color accent: control.kind === "ok" ? Theme.ok
                                                          : control.kind === "danger" ? Theme.danger
                                                                                      : Theme.ice
    readonly property color tint: control.kind === "ok" ? Theme.okSoft
                                                        : control.kind === "danger" ? Theme.dangerSoft
                                                                                    : Theme.inset
    readonly property string symbol: control.kind === "ok" ? "✓"
                                                           : control.kind === "danger" ? "✕"
                                                                                       : "ⓘ"

    leftPadding: 0
    rightPadding: Theme.space3
    topPadding: Theme.space2
    bottomPadding: Theme.space2

    implicitWidth: 200
    implicitHeight: Math.max(36, contentItem.implicitHeight + control.topPadding
                             + control.bottomPadding)

    contentItem: RowLayout {
        spacing: Theme.space2

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 3
            Layout.margins: 0
            color: control.accent
        }
        Text {
            text: control.symbol
            color: control.accent
            font.pixelSize: Theme.fontSmall
        }
        Text {
            Layout.fillWidth: true
            text: control.title
            color: Theme.text
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WrapAnywhere
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        radius: Theme.radius
        color: control.tint
        border.width: 1
        border.color: Theme.line
    }
}
