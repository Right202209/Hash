import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// Timestamp page: two conversion directions, each an AppCard with its own
// input row and results. Both directions share the "now" shortcut.
Page {
    id: root

    property var stampOutcome: null
    property var dateOutcome: null
    property string stampError: ""
    property string dateError: ""

    header: PageHeader {
        icon: "⏱"
        title: qsTr("时间戳")
        description: qsTr("Unix 时间戳与日期时间互相转换 · 本地时区与 UTC 双显示")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function convertStamp() {
        const value = Number(stampInput.text)
        if (stampInput.text.trim() === "" || isNaN(value)) {
            root.stampError = qsTr("请输入数字时间戳")
            root.stampOutcome = null
            return
        }
        const outcome = Timestamp.timestampToDate(Math.trunc(value), msRadio.checked)
        if (!outcome.ok) {
            root.stampError = outcome.error
            root.stampOutcome = null
            return
        }
        root.stampError = ""
        root.stampOutcome = outcome
    }

    function convertDate() {
        const outcome = Timestamp.dateToTimestamp(dateInput.text)
        if (!outcome.ok) {
            root.dateError = outcome.error
            root.dateOutcome = null
            return
        }
        root.dateError = ""
        root.dateOutcome = outcome
    }

    function fillNow() {
        const outcome = Timestamp.now()
        stampInput.text = String(outcome.seconds)
        root.stampOutcome = outcome
        dateInput.text = outcome.local
        root.dateError = ""
        root.dateOutcome = {
            "ok": true,
            "seconds": outcome.seconds,
            "milliseconds": outcome.milliseconds
        }
    }

    contentItem: Flickable {
        id: scroller

        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 2 * Theme.space3
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            id: contentColumn

            x: Theme.pagePadding
            y: Theme.space3
            width: scroller.width - 2 * Theme.pagePadding
            spacing: Theme.space3

            AppCard {
                Layout.fillWidth: true
                title: qsTr("时间戳 → 日期时间")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space2

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space2

                        AppTextField {
                            id: stampInput

                            Layout.preferredWidth: 200
                            placeholderText: qsTr("例如 946684800")
                            selectByMouse: true
                            hasError: root.stampError !== ""
                            font.family: Theme.monoFamily
                            onTextChanged: root.stampError = ""
                            onAccepted: root.convertStamp()
                        }
                        AppRadioButton {
                            id: secondsRadio

                            text: qsTr("秒")
                            checked: true
                        }
                        AppRadioButton {
                            id: msRadio

                            text: qsTr("毫秒")
                        }
                        AppButton {
                            text: qsTr("转换")
                            variant: "primary"
                            enabled: stampInput.text !== ""
                            onClicked: root.convertStamp()
                        }
                        AppButton {
                            text: qsTr("当前时间")
                            variant: "ghost"
                            onClicked: root.fillNow()
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    AppBanner {
                        Layout.fillWidth: true
                        kind: "danger"
                        title: root.stampError
                        visible: root.stampError !== ""
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space1
                        visible: root.stampOutcome !== null

                        AppKeyValueRow {
                            Layout.fillWidth: true
                            label: qsTr("本地")
                            value: root.stampOutcome !== null ? root.stampOutcome.local : ""
                            onCopyRequested: text => root.copy(text)
                        }
                        AppKeyValueRow {
                            Layout.fillWidth: true
                            label: qsTr("毫秒")
                            value: root.stampOutcome !== null ? String(root.stampOutcome.milliseconds) : ""
                            onCopyRequested: text => root.copy(text)
                        }
                        AppKeyValueRow {
                            Layout.fillWidth: true
                            label: qsTr("UTC")
                            value: root.stampOutcome !== null ? root.stampOutcome.utc : ""
                            onCopyRequested: text => root.copy(text)
                        }
                        AppKeyValueRow {
                            Layout.fillWidth: true
                            label: qsTr("ISO 8601")
                            value: root.stampOutcome !== null ? root.stampOutcome.iso : ""
                            onCopyRequested: text => root.copy(text)
                        }
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                title: qsTr("日期时间 → 时间戳")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space2

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space2

                        AppTextField {
                            id: dateInput

                            Layout.preferredWidth: 280
                            placeholderText: qsTr("2026-01-02 03:04:05 或 ISO 8601")
                            selectByMouse: true
                            hasError: root.dateError !== ""
                            font.family: Theme.monoFamily
                            onTextChanged: root.dateError = ""
                            onAccepted: root.convertDate()
                        }
                        AppButton {
                            text: qsTr("转换")
                            variant: "primary"
                            enabled: dateInput.text !== ""
                            onClicked: root.convertDate()
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    AppBanner {
                        Layout.fillWidth: true
                        kind: "danger"
                        title: root.dateError
                        visible: root.dateError !== ""
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space1
                        visible: root.dateOutcome !== null

                        AppKeyValueRow {
                            Layout.fillWidth: true
                            label: qsTr("秒")
                            value: root.dateOutcome !== null ? String(root.dateOutcome.seconds) : ""
                            onCopyRequested: text => root.copy(text)
                        }
                        AppKeyValueRow {
                            Layout.fillWidth: true
                            label: qsTr("毫秒")
                            value: root.dateOutcome !== null ? String(root.dateOutcome.milliseconds) : ""
                            onCopyRequested: text => root.copy(text)
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.minimumHeight: 0
            }
        }
    }

    function copy(text) {
        if (text === "") {
            return
        }
        if (Clipboard.copy(text)) {
            Toast.show(qsTr("已复制"), "ok")
        } else {
            Toast.show(qsTr("内容过大，无法复制"), "error")
        }
    }
}
