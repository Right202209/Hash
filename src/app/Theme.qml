pragma Singleton
import QtQuick

// Theme carries the toolbox palette and small view helpers. Control colors
// themselves come from the dark application palette set in main.cpp; these
// values style the custom surfaces.
QtObject {
    readonly property color canvas: "#081421"
    readonly property color surface: "#0E2031"
    readonly property color raised: "#142E43"
    readonly property color inset: "#1A2B38"
    readonly property color line: "#28485D"
    readonly property color text: "#E8F5FB"
    readonly property color muted: "#8EA8BB"
    readonly property color ice: "#76D8F7"
    readonly property color ok: "#7DF781"
    readonly property color danger: "#F76D76"

    readonly property var monoFamilies: ["Cascadia Mono", "Consolas", "DejaVu Sans Mono"]

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
}
