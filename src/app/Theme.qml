pragma Singleton
import QtQuick

// Theme is the single source of truth for every custom surface color, radius,
// size and animation duration in the toolbox. Pages must reference these
// tokens (never hard-coded hex values) so all tools share one look; the same
// colors are mirrored onto the ApplicationWindow palette (Main.qml) and the
// application palette (main.cpp) for the Basic style and platform frames.
QtObject {
    // Surfaces, darkest to lightest: window canvas, sidebar/cards, hover,
    // input wells. line is the shared hairline.
    readonly property color canvas: "#0F1115"
    readonly property color surface: "#151922"
    readonly property color raised: "#1C212C"
    readonly property color inset: "#222836"
    readonly property color line: "#2A3140"
    readonly property color text: "#E6E9EF"
    readonly property color muted: "#8A93A3"
    // ice is the accent (historic name kept so every page keeps compiling);
    // iceDeep is the selection/highlight fill used by the mirrored palette
    // (main.cpp Highlight) and the active sidebar row.
    readonly property color ice: "#7AA2FF"
    readonly property color iceDeep: "#2E4780"
    readonly property color ok: "#5FD38D"
    readonly property color danger: "#F2777A"

    // One radius for small elements (fields, chips, rows) and one for cards.
    readonly property int radius: 8
    readonly property int cardRadius: 12

    // Shell metrics.
    readonly property int sidebarWidth: 224
    readonly property int pagePadding: 20

    // Standard micro-animation for hover/press color changes.
    readonly property int fadeMs: 120

    // The QML font grouped property exposes a single family; Qt substitutes
    // automatically when the name is unavailable (e.g. non-Windows CI).
    readonly property string monoFamily: Qt.platform.os === "windows" ? "Cascadia Mono"
                                                                      : "monospace"

    function sizeText(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        }
        var value = bytes
        var units = ["KB", "MB", "GB", "TB"]
        var unit = -1
        while (value >= 1024 && unit < 3) {
            value /= 1024
            unit++
        }
        return value.toFixed(1) + " " + units[unit]
    }

    function statusText(status) {
        switch (status) {
        case 0:
            return qsTr("待计算")
        case 1:
            return qsTr("计算中")
        case 2:
            return qsTr("已完成")
        case 3:
            return qsTr("错误")
        case 4:
            return qsTr("已取消")
        }
        return ""
    }

    // statusColor maps a queue status to the shared accent colors so every
    // page renders the same state in the same hue.
    function statusColor(status) {
        switch (status) {
        case 2:
            return ok
        case 3:
            return danger
        case 1:
            return ice
        case 4:
            return muted
        }
        return line
    }
}
