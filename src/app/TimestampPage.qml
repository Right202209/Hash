import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

Page {
    id: root

    property var stampOutcome: null
    property var dateOutcome: null

    header: PageHeader {
        icon: "⏱"
        title: qsTr("时间戳")
        description: qsTr("Unix 时间戳与日期时间互相转换 · 本地时区与 UTC 双显示")
    }

    function convertStamp() {
        const value = Number(stampInput.text)
        if (stampInput.text.trim() === "" || isNaN(value)) {
            stampError.text = qsTr("请输入数字时间戳")
            root.stampOutcome = null
            return
        }
        const outcome = Timestamp.timestampToDate(Math.trunc(value), msRadio.checked)
        if (!outcome.ok) {
            stampError.text = outcome.error
            root.stampOutcome = null
            return
        }
        stampError.text = ""
        root.stampOutcome = outcome
    }

    function convertDate() {
        const outcome = Timestamp.dateToTimestamp(dateInput.text)
        if (!outcome.ok) {
            dateError.text = outcome.error
            root.dateOutcome = null
            return
        }
        dateError.text = ""
        root.dateOutcome = outcome
    }

    function fillNow() {
        const outcome = Timestamp.now()
        stampInput.text = String(outcome.seconds)
        root.stampOutcome = outcome
        dateInput.text = outcome.local
        dateError.text = ""
        root.dateOutcome = {
            "ok": true,
            "seconds": outcome.seconds,
            "milliseconds": outcome.milliseconds
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("时间戳 → 日期时间")

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: stampInput

                        Layout.preferredWidth: 220
                        placeholderText: qsTr("例如 946684800")
                        selectByMouse: true
                        font.families: Theme.monoFamilies
                        onAccepted: root.convertStamp()
                    }
                    RadioButton {
                        id: secondsRadio

                        text: qsTr("秒")
                        checked: true
                    }
                    RadioButton {
                        id: msRadio

                        text: qsTr("毫秒")
                    }
                    Button {
                        text: qsTr("转换")
                        highlighted: true
                        onClicked: root.convertStamp()
                    }
                    Button {
                        text: qsTr("当前时间")
                        onClicked: root.fillNow()
                    }
                    Label {
                        id: stampError

                        text: ""
                        color: Theme.danger
                        visible: text !== ""
                    }
                }

                Repeater {
                    model: root.stampOutcome ? [{
                            "label": qsTr("本地"),
                            "value": root.stampOutcome.local
                                + qsTr("  （%1 毫秒）").arg(root.stampOutcome.milliseconds)
                        }, {
                            "label": qsTr("UTC"),
                            "value": root.stampOutcome.utc
                        }, {
                            "label": qsTr("ISO"),
                            "value": root.stampOutcome.iso
                        }] : []

                    delegate: RowLayout {
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            Layout.preferredWidth: 44
                            text: modelData.label
                            color: Theme.ice
                        }
                        Label {
                            Layout.fillWidth: true
                            text: modelData.value
                            color: Theme.text
                            font.pixelSize: 13
                            font.families: Theme.monoFamilies
                        }
                        ToolButton {
                            text: qsTr("⧉")
                            font.pixelSize: 12
                            onClicked: Clipboard.copy(modelData.value)
                                       ? Toast.show(qsTr("已复制"))
                                       : Toast.show(qsTr("内容过大，无法复制"))
                        }
                    }
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("日期时间 → 时间戳")

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: dateInput

                        Layout.preferredWidth: 260
                        placeholderText: qsTr("2026-01-02 03:04:05 或 ISO 8601")
                        selectByMouse: true
                        font.families: Theme.monoFamilies
                        onAccepted: root.convertDate()
                    }
                    Button {
                        text: qsTr("转换")
                        highlighted: true
                        onClicked: root.convertDate()
                    }
                    Label {
                        id: dateError

                        text: ""
                        color: Theme.danger
                        visible: text !== ""
                    }
                }

                Repeater {
                    model: root.dateOutcome ? [{
                            "label": qsTr("秒"),
                            "value": String(root.dateOutcome.seconds)
                        }, {
                            "label": qsTr("毫秒"),
                            "value": String(root.dateOutcome.milliseconds)
                        }] : []

                    delegate: RowLayout {
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            Layout.preferredWidth: 44
                            text: modelData.label
                            color: Theme.ice
                        }
                        Label {
                            Layout.fillWidth: true
                            text: modelData.value
                            color: Theme.text
                            font.pixelSize: 13
                            font.families: Theme.monoFamilies
                        }
                        ToolButton {
                            text: qsTr("⧉")
                            font.pixelSize: 12
                            onClicked: Clipboard.copy(modelData.value)
                                       ? Toast.show(qsTr("已复制"))
                                       : Toast.show(qsTr("内容过大，无法复制"))
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
