pragma Singleton
import QtQuick

// Theme is the single source of truth for every custom surface color, radius
// and animation duration in the toolbox. Pages must reference these tokens
// (never hard-coded hex values) so all tools share one look; the control
// palette in main.cpp mirrors the same values for the Basic style.
QtObject {
    readonly property color canvas: "#081421"
    readonly property color surface: "#0E2031"
    readonly property color raised: "#142E43"
    readonly property color inset: "#1A2B38"
    readonly property color line: "#28485D"
    readonly property color text: "#E8F5FB"
    readonly property color muted: "#8EA8BB"
    readonly property color ice: "#76D8F7"
    // Deep accent used by the mirrored control palette (main.cpp Highlight);
    // kept here so both ends of the mirror are documented in one place.
    readonly property color iceDeep: "#1C5B7E"
    readonly property color ok: "#7DF781"
    readonly property color danger: "#F76D76"

    // One radius for small elements (fields, chips, dots) and one for cards.
    readonly property int radius: 6
    readonly property int cardRadius: 10

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
