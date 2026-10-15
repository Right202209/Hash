pragma Singleton
import QtQuick

// Theme is the single source of truth for every custom surface color, radius,
// spacing, size, font size and animation duration in the toolbox. Pages must
// reference these tokens (never hard-coded hex values or bare metric numbers)
// so all tools share one look; the colors that map onto a QPalette role are
// mirrored on the ApplicationWindow palette (Main.qml) and the application
// palette (main.cpp) for the Basic style and platform frames.
QtObject {
    // ---- Surfaces, darkest to lightest -----------------------------------
    // canvas: window, surface: sidebar/cards, raised: hover/expanded rows,
    // inset: input wells and text areas. line is the shared hairline.
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

    // ---- Semantic colors ------------------------------------------------
    // muted is only 2.9:1 on iceDeep, so secondary text on a selected row must
    // use mutedOnAccent. The *Soft tokens are translucent fills (alpha is the
    // first hex pair) used by banners, the drag overlay and the changed badge.
    readonly property color mutedOnAccent: "#B8C0CF"
    readonly property color focusRing: ice
    readonly property color okSoft: "#245FD38D"
    readonly property color dangerSoft: "#24F2777A"
    readonly property color iceSoft: "#337AA2FF"
    readonly property color scrim: "#99000000"
    readonly property color edgeHighlight: "#0FFFFFFF"

    // ---- Spacing (4 px grid) --------------------------------------------
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 20
    readonly property int space6: 24
    readonly property int space8: 32

    // ---- Sizes ----------------------------------------------------------
    readonly property int controlHeight: 32
    // fieldWidth is the shared implicit width of the small numeric controls
    // (combo boxes, spin boxes) so a column of them lines up.
    readonly property int fieldWidth: 110
    readonly property int rowHeight: 36
    readonly property int listRowHeight: 40
    readonly property int statusDot: 8
    readonly property int kvLabelWidth: 96
    // Square glyph blocks: command-palette rows, overview cards, page headers.
    readonly property int iconBlockSm: 24
    readonly property int iconBlock: 36
    readonly property int iconBlockLg: 38
    // swatchSize is the colour-page preview square: big enough for its 8 px
    // alpha checkerboard to be legible.
    readonly property int swatchSize: 48

    // ---- Type scale -----------------------------------------------------
    readonly property int fontCaption: 11
    readonly property int fontSmall: 12
    readonly property int fontMono: 13
    readonly property int fontBody: 15
    readonly property int fontTitle: 18
    readonly property int fontDisplay: 24
    // fontGlyph is the oversized monospace mark in empty states; it is a token
    // rather than a literal so the placeholder scales with the rest of the
    // type scale.
    readonly property int fontGlyph: 40

    // ---- Radii, layers and motion ---------------------------------------
    readonly property int radiusSm: 4
    // One radius for small elements (fields, chips, rows) and one for cards.
    readonly property int radius: 8
    readonly property int cardRadius: 12
    readonly property int radiusPill: 999
    // fadeMs: hover/press color transitions, overlayMs: popup and scrim fades.
    readonly property int fadeMs: 120
    readonly property int overlayMs: 160

    // Shell metrics.
    readonly property int sidebarWidth: 224
    readonly property int pagePadding: 20

    // The QML font grouped property exposes a single family; Qt substitutes
    // automatically when the name is unavailable (e.g. non-Windows CI). The
    // monospace chain prefers Cascadia Mono, falls back to Consolas (present
    // on every Windows 10/11 install) and finally to the generic alias.
    readonly property string monoFamily: {
        const families = Qt.fontFamilies()
        if (families.indexOf("Cascadia Mono") >= 0) {
            return "Cascadia Mono"
        }
        if (families.indexOf("Consolas") >= 0) {
            return "Consolas"
        }
        return "monospace"
    }

    // sizeText renders a byte count with binary units (B / KiB / MiB / GiB),
    // keeping two decimals above the byte range. This is display-only: the CLI
    // still prints raw byte counts for size=.
    function sizeText(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        }
        var value = bytes
        var units = ["KiB", "MiB", "GiB", "TiB"]
        var unit = -1
        while (value >= 1024 && unit < 3) {
            value /= 1024
            unit++
        }
        return value.toFixed(2) + " " + units[unit]
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
