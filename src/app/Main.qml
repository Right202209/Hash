import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Launcher-style shell in the uTools mould: a command bar on top filters the
// tool grid, Enter opens the first hit, Esc returns from a tool, and the
// status bar mirrors transient feedback.
ApplicationWindow {
    id: root

    width: 980
    height: 680
    minimumWidth: 780
    minimumHeight: 540
    visible: true
    title: qsTr("Hash 工具箱")
    color: Theme.canvas

    property string statusText: qsTr("就绪 · 选择一个工具开始")

    function openTool(toolId) {
        const page = pages[toolId]
        if (page) {
            stack.push(page)
        }
    }

    function openFirstTool() {
        if (stack.depth === 1) {
            const toolId = Tools.firstToolId()
            if (toolId !== "") {
                openTool(toolId)
            }
        }
    }

    property var pages: ({
                             "filehash": fileHashPage,
                             "texthash": textHashPage,
                             "base64": codecBase64Page,
                             "url": codecUrlPage,
                             "json": jsonPage,
                             "timestamp": timestampPage,
                             "uuid": uuidPage,
                             "radix": radixPage,
                             "color": colorPage,
                             "password": passwordPage
                         })

    Component {
        id: homeComponent
        HomePage {
            onOpenRequested: toolId => root.openTool(toolId)
        }
    }
    Component {
        id: fileHashPage
        FileHashPage {}
    }
    Component {
        id: textHashPage
        TextHashPage {}
    }
    Component {
        id: codecBase64Page
        CodecPage {
            mode: "base64"
        }
    }
    Component {
        id: codecUrlPage
        CodecPage {
            mode: "url"
        }
    }
    Component {
        id: jsonPage
        JsonPage {}
    }
    Component {
        id: timestampPage
        TimestampPage {}
    }
    Component {
        id: uuidPage
        UuidPage {}
    }
    Component {
        id: radixPage
        RadixPage {}
    }
    Component {
        id: colorPage
        ColorPage {}
    }
    Component {
        id: passwordPage
        PasswordPage {}
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Theme.surface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 16
                spacing: 10

                Label {
                    text: "#"
                    color: Theme.ice
                    font.pixelSize: 24
                    font.family: Theme.monoFamily
                }
                TextField {
                    id: commandBar

                    Layout.fillWidth: true
                    placeholderText: qsTr("搜索工具，回车打开第一个结果")
                    selectByMouse: true
                    Component.onCompleted: forceActiveFocus()
                    background: Rectangle {
                        color: Theme.inset
                        radius: 4
                        border.color: commandBar.activeFocus ? Theme.ice : Theme.line
                    }
                    Keys.onReturnPressed: function(event) {
                        event.accepted = true
                        root.openFirstTool()
                    }
                    Keys.onEnterPressed: function(event) {
                        event.accepted = true
                        root.openFirstTool()
                    }
                }
                Label {
                    text: stack.depth > 1 ? qsTr("Esc 返回")
                                          : qsTr("%1 个工具").arg(Tools.rowCount())
                    color: Theme.muted
                }
            }
        }

        StackView {
            id: stack

            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: homeComponent
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: Theme.surface

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                text: root.statusText
                color: Theme.muted
            }
        }
    }

    Binding {
        target: Tools
        property: "filter"
        value: commandBar.text
    }

    Connections {
        target: Toast

        function onShow(message) {
            root.statusText = message
            statusTimer.restart()
        }
    }

    Timer {
        id: statusTimer

        interval: 2600
        onTriggered: root.statusText = qsTr("就绪")
    }

    Shortcut {
        sequence: "Esc"
        enabled: stack.depth > 1
        onActivated: stack.pop()
    }
}
