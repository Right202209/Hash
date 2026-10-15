import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Shell: a permanent sidebar lists every tool by category, the content area
// shows the overview or the current tool, and Ctrl+K opens the command
// palette. Tool pages are created on first open and then kept alive, so
// switching between tools preserves their inputs and results.
ApplicationWindow {
    id: root

    width: 1100
    height: 720
    minimumWidth: 840
    minimumHeight: 560
    visible: true
    title: qsTr("Hash 工具箱")
    color: Theme.canvas

    // The window palette is the mirror of the Theme tokens for controls this
    // shell does not restyle itself (Page and GroupBox backgrounds, Label
    // defaults, ToolTip). It is set here, inside the scene, because a
    // QGuiApplication palette can be overridden by the platform theme on
    // Windows; the same values live in applyDarkPalette (main.cpp).
    palette.window: Theme.canvas
    palette.windowText: Theme.text
    palette.base: Theme.canvas
    palette.alternateBase: Theme.surface
    palette.text: Theme.text
    palette.button: Theme.surface
    palette.buttonText: Theme.text
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text
    palette.highlight: Theme.iceDeep
    palette.highlightedText: Theme.text
    palette.link: Theme.ice
    palette.placeholderText: Theme.muted
    palette.light: Theme.raised
    palette.midlight: Theme.raised
    palette.mid: Theme.line
    palette.dark: Theme.canvas
    palette.disabled.text: Theme.muted
    palette.disabled.windowText: Theme.muted
    palette.disabled.button: Theme.canvas
    palette.disabled.buttonText: Theme.muted

    // Empty means the overview is showing.
    property string currentToolId: ""
    property string statusText: qsTr("就绪")
    // statusKind is the Toast type of the current message: "" for the idle
    // ready state, otherwise "info" | "ok" | "error".
    property string statusKind: ""
    // The status dot reuses the queue status colors so the shell and the file
    // page read states in the same hues.
    readonly property int statusIndicator: root.statusKind === "error" ? 3
                                                                       : root.statusKind === "ok" ? 2
                                                                                                  : root.statusKind === "info" ? 1 : 2

    // toolPages maps registry ids to their page file and the initial
    // properties the page is created with.
    readonly property var toolPages: [
        { id: "filehash", url: "FileHashPage.qml", props: {} },
        { id: "texthash", url: "TextHashPage.qml", props: {} },
        { id: "base64", url: "CodecPage.qml", props: { mode: "base64" } },
        { id: "url", url: "CodecPage.qml", props: { mode: "url" } },
        { id: "json", url: "JsonPage.qml", props: {} },
        { id: "timestamp", url: "TimestampPage.qml", props: {} },
        { id: "radix", url: "RadixPage.qml", props: {} },
        { id: "color", url: "ColorPage.qml", props: {} },
        { id: "uuid", url: "UuidPage.qml", props: {} },
        { id: "password", url: "PasswordPage.qml", props: {} }
    ]

    function pageIndex(toolId) {
        for (let index = 0; index < toolPages.length; ++index) {
            if (toolPages[index].id === toolId) {
                return index
            }
        }
        return -1
    }

    function openTool(toolId) {
        if (toolId === "") {
            currentToolId = ""
            return
        }
        const index = pageIndex(toolId)
        if (index < 0) {
            return
        }
        const loader = loaders.itemAt(index)
        if (loader && loader.status === Loader.Null) {
            const page = toolPages[index]
            loader.setSource(Qt.resolvedUrl(page.url), page.props)
        }
        currentToolId = toolId
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.sidebarWidth
            currentToolId: root.currentToolId
            onToolSelected: toolId => root.openTool(toolId)
            onHomeRequested: root.openTool("")
            onSearchRequested: commandPalette.open()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                HomePage {
                    anchors.fill: parent
                    visible: root.currentToolId === ""
                    onOpenRequested: toolId => root.openTool(toolId)
                }

                Repeater {
                    id: loaders

                    model: root.toolPages

                    delegate: Loader {
                        required property var modelData

                        anchors.fill: parent
                        visible: root.currentToolId === modelData.id
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                color: Theme.surface

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: Theme.line
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: Theme.space2

                    AppStatusDot {
                        status: root.statusIndicator
                        Accessible.name: root.statusText
                    }
                    Label {
                        id: statusLabel

                        Layout.fillWidth: true
                        text: root.statusText
                        color: Theme.muted
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideMiddle

                        HoverHandler {
                            id: statusHover
                        }
                        ToolTip.visible: statusHover.hovered && statusLabel.truncated
                        ToolTip.delay: 600
                        ToolTip.text: root.statusText
                    }

                    // Shortcut legend: the key caps make the two global
                    // shortcuts discoverable without opening the palette.
                    RowLayout {
                        spacing: Theme.space1

                        AppKbd {
                            text: "Ctrl"
                        }
                        AppKbd {
                            text: "K"
                        }
                        Label {
                            text: qsTr("快速切换")
                            color: Theme.muted
                            font.pixelSize: Theme.fontSmall
                        }
                        Label {
                            visible: root.currentToolId !== ""
                            text: "·"
                            color: Theme.muted
                            font.pixelSize: Theme.fontSmall
                        }
                        AppKbd {
                            visible: root.currentToolId !== ""
                            text: "Esc"
                        }
                        Label {
                            visible: root.currentToolId !== ""
                            text: qsTr("返回概览")
                            color: Theme.muted
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                }
            }
        }
    }

    CommandPalette {
        id: commandPalette

        onToolChosen: toolId => root.openTool(toolId)
    }

    Connections {
        target: Toast

        function onShow(message, kind) {
            root.statusText = message
            root.statusKind = kind
            statusTimer.restart()
        }
    }

    Timer {
        id: statusTimer

        interval: 2600
        onTriggered: {
            root.statusText = qsTr("就绪")
            root.statusKind = ""
        }
    }

    Shortcut {
        sequences: ["Ctrl+K", "Ctrl+P"]
        onActivated: commandPalette.opened ? commandPalette.close() : commandPalette.open()
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.currentToolId !== "" && !commandPalette.opened
        onActivated: root.openTool("")
    }
}
