import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import HashTools

Page {
    id: root

    property int algorithmCount: 0
    // File count carried by the current drag, shown in the drop overlay.
    property int dropCount: 0

    // The five clipboard-compare counters, rendered as stat pills. toned by
    // pillColor so the eye lands on the failures first.
    readonly property var comparePills: [
        {
            label: qsTr("匹配"),
            value: HashController.compare.matches,
            tone: "ok"
        },
        {
            label: qsTr("不匹配"),
            value: HashController.compare.mismatches,
            tone: "danger"
        },
        {
            label: qsTr("缺少"),
            value: HashController.compare.missing,
            tone: "danger"
        },
        {
            label: qsTr("多余"),
            value: HashController.compare.unexpected,
            tone: "danger"
        },
        {
            label: qsTr("重复"),
            value: HashController.compare.duplicates,
            tone: "muted"
        }
    ]

    // The run outcome is a banner rather than a line of text so success,
    // partial failure and cancellation cannot be mistaken for each other.
    readonly property string summaryKind: {
        const summary = HashController.lastSummary
        if (summary.files === undefined) {
            return "info"
        }
        if (summary.canceled) {
            return "info"
        }
        if (summary.failures > 0) {
            return "danger"
        }
        if (summary.changed > 0) {
            return "info"
        }
        return "ok"
    }

    function summaryTitle() {
        const summary = HashController.lastSummary
        if (summary.files === undefined) {
            return ""
        }
        if (summary.canceled) {
            return qsTr("已取消 · 已保留 %1 个文件的完成结果").arg(summary.files)
        }
        if (summary.failures > 0) {
            return qsTr("完成 · %1 个文件 · %2 个失败").arg(summary.files).arg(summary.failures)
        }
        if (summary.changed > 0) {
            return qsTr("完成 · %1 个文件 · %2 个在计算期间发生变化").arg(summary.files).arg(summary.changed)
        }
        return qsTr("已完成 %1 个文件 · %2 个算法").arg(summary.files).arg(summary.algorithms)
    }

    function pillColor(tone) {
        switch (tone) {
        case "ok":
            return Theme.ok
        case "danger":
            return Theme.danger
        }
        return Theme.muted
    }

    function pillTint(tone) {
        switch (tone) {
        case "ok":
            return Theme.okSoft
        case "danger":
            return Theme.dangerSoft
        }
        return Theme.inset
    }

    header: PageHeader {
        icon: "#"
        title: qsTr("文件哈希")
        description: qsTr("批量计算文件摘要 · 单次读取多算法并行 · 剪贴板比对自动忽略 sha256: 等前缀")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function collectAlgorithms() {
        return algorithmChips.selectedNames()
    }

    function refreshAlgorithmCount() {
        root.algorithmCount = algorithmChips.selectedNames().length
    }

    Component.onCompleted: refreshAlgorithmCount()

    function announceAdd(outcome) {
        if (outcome.error !== undefined) {
            Toast.show(outcome.error, "error")
            return
        }
        if (outcome.added === 0 && outcome.rejected === 0) {
            Toast.show(qsTr("没有新增文件"), "info")
        } else if (outcome.rejected > 0) {
            Toast.show(qsTr("已添加 %1 个文件 · 忽略 %2 项").arg(outcome.added).arg(outcome.rejected), "ok")
        } else {
            Toast.show(qsTr("已添加 %1 个文件").arg(outcome.added), "ok")
        }
    }

    FileDialog {
        id: addDialog

        title: qsTr("添加文件")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("所有文件 (*)")]
        onAccepted: root.announceAdd(HashController.addFiles(selectedFiles))
    }

    FileDialog {
        id: saveDialog

        title: qsTr("保存哈希结果")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("文本文件 (*.txt)"), qsTr("所有文件 (*)")]
        onAccepted: {
            const error = HashController.saveOutput(selectedFile)
            if (error === "") {
                Toast.show(qsTr("结果已保存"), "ok")
            } else {
                Toast.show(qsTr("保存失败：") + error, "error")
            }
        }
    }

    // The page scrolls as one column: at the 840x560 minimum window the queue,
    // the actions and the result text no longer fit, so nothing may be sized by
    // a filled-height fraction of the page.
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

            AlgorithmChips {
                id: algorithmChips

                Layout.fillWidth: true
                defaults: ["sha256"]
                onSelectionChanged: root.refreshAlgorithmCount()
            }

            // Queue well: rows are separated by hairlines, not by a zebra
            // stripe, so the expanded digest rows still read as part of the
            // file they belong to.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(Theme.listRowHeight * 4, Math.min(Theme.listRowHeight * 8, scroller.height * 0.4))
                color: Theme.inset
                border.width: 1
                border.color: Theme.line
                radius: Theme.cardRadius

                ListView {
                    id: fileList

                    anchors.fill: parent
                    anchors.margins: 2
                    clip: true
                    model: HashController.files
                    boundsBehavior: Flickable.StopAtBounds
                    reuseItems: false
                    ScrollBar.vertical: AppScrollBar {}

                    delegate: Rectangle {
                        id: fileRow

                        required property int index
                        required property string path
                        required property string name
                        required property real size
                        required property int status
                        required property bool changed
                        required property string error
                        required property var digests

                        width: fileList.width
                        height: Math.max(Theme.listRowHeight, fileColumn.implicitHeight + 2 * Theme.space1)
                        color: rowHover.containsMouse ? Theme.raised : "transparent"

                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.fadeMs
                            }
                        }

                        // Declared first so it sits under the row content:
                        // the remove button keeps its own hover and clicks.
                        MouseArea {
                            id: rowHover

                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.line
                        }

                        ColumnLayout {
                            id: fileColumn

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: Theme.space3
                            anchors.rightMargin: Theme.space1
                            spacing: Theme.space1

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space2

                                AppStatusDot {
                                    status: fileRow.status
                                    Accessible.name: Theme.statusText(fileRow.status)
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: fileRow.path
                                    color: Theme.text
                                    font.pixelSize: Theme.fontSmall
                                    elide: Text.ElideMiddle

                                    HoverHandler {
                                        id: pathHover
                                    }
                                    ToolTip.visible: pathHover.hovered
                                    ToolTip.delay: 600
                                    ToolTip.text: fileRow.path
                                }

                                // The "changed" marker is a badge, not a word in
                                // the meta run: it invalidates the result and
                                // has to stand out from the status text.
                                Rectangle {
                                    visible: fileRow.changed
                                    implicitWidth: changedLabel.implicitWidth + 2 * Theme.space2
                                    implicitHeight: Theme.space5
                                    radius: Theme.radiusSm
                                    color: Theme.dangerSoft
                                    border.width: 1
                                    border.color: Theme.danger

                                    Label {
                                        id: changedLabel

                                        anchors.centerIn: parent
                                        text: qsTr("已变化")
                                        color: Theme.danger
                                        font.pixelSize: Theme.fontCaption
                                    }
                                }

                                Label {
                                    Layout.preferredWidth: 92
                                    horizontalAlignment: Text.AlignRight
                                    text: Theme.sizeText(fileRow.size)
                                    color: Theme.muted
                                    font.pixelSize: Theme.fontSmall
                                    font.family: Theme.monoFamily
                                }
                                Label {
                                    Layout.preferredWidth: 56
                                    text: Theme.statusText(fileRow.status)
                                    color: fileRow.status === 3 ? Theme.danger : Theme.muted
                                    font.pixelSize: Theme.fontCaption
                                }
                                AppToolButton {
                                    text: qsTr("✕")
                                    tip: qsTr("移除")
                                    enabled: !HashController.busy
                                    visible: !HashController.busy
                                    onClicked: HashController.removeRow(fileRow.index)
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.leftMargin: Theme.space4
                                visible: fileRow.error !== ""
                                text: fileRow.error
                                color: Theme.danger
                                font.pixelSize: Theme.fontSmall
                                wrapMode: Text.WrapAnywhere
                            }

                            // Digests are indented under the path so the
                            // file/digest hierarchy survives expansion.
                            Repeater {
                                model: fileRow.digests

                                delegate: AppKeyValueRow {
                                    required property var modelData

                                    Layout.fillWidth: true
                                    Layout.leftMargin: Theme.space6
                                    Layout.rightMargin: Theme.space2
                                    label: modelData.algorithm
                                    value: modelData.value
                                    onCopyRequested: text => Clipboard.copy(text) ? Toast.show(qsTr("摘要已复制"), "ok")
                                                                                  : Toast.show(qsTr("内容过大，无法复制"), "error")
                                }
                            }
                        }
                    }
                }

                AppEmptyState {
                    anchors.centerIn: parent
                    width: Math.min(implicitWidth, parent.width - 2 * Theme.space4)
                    visible: fileList.count === 0
                    glyph: "#"
                    title: qsTr("把文件拖到这里")
                    hint: qsTr("或点击「添加文件」· 目录暂不支持")
                }

                // Drop feedback: an accent rim plus a translucent wash, drawn
                // over the queue. Fills are more legible than a dashed border
                // at this size.
                Rectangle {
                    anchors.fill: parent
                    radius: Theme.cardRadius
                    color: Theme.iceSoft
                    border.width: 2
                    border.color: Theme.ice
                    visible: dropArea.containsDrag

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("松开以添加 %1 个文件").arg(root.dropCount)
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        font.bold: true
                    }
                }

                DropArea {
                    id: dropArea

                    anchors.fill: parent
                    onEntered: function(drag) {
                        root.dropCount = drag.hasUrls ? drag.urls.length : 0
                    }
                    onDropped: function(drop) {
                        if (drop.hasUrls) {
                            root.announceAdd(HashController.addFiles(drop.urls))
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppButton {
                    text: qsTr("添加文件")
                    variant: "primary"
                    enabled: !HashController.busy
                    onClicked: addDialog.open()
                }
                AppButton {
                    text: qsTr("清空")
                    variant: "ghost"
                    enabled: !HashController.busy && fileList.count > 0
                    onClicked: {
                        HashController.clearFiles()
                        Toast.show(qsTr("已清空文件列表"), "info")
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("%1 个文件 · 已选 %2 个算法").arg(fileList.count).arg(root.algorithmCount)
                    color: Theme.muted
                    font.pixelSize: Theme.fontSmall
                }
                AppButton {
                    // The primary action flips to a danger variant while a run
                    // is in flight so stopping it is never mistaken for starting.
                    text: HashController.busy ? qsTr("取消") : qsTr("开始计算")
                    variant: HashController.busy ? "danger" : "primary"
                    enabled: HashController.busy || (fileList.count > 0 && root.algorithmCount > 0)
                    onClicked: {
                        if (HashController.busy) {
                            HashController.cancel()
                            Toast.show(qsTr("正在停止……"), "info")
                        } else {
                            HashController.start(root.collectAlgorithms())
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2
                visible: HashController.busy

                AppProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: HashController.progress
                }
                Label {
                    Layout.maximumWidth: Math.round(contentColumn.width * 0.4)
                    text: qsTr("计算中 %1% · %2").arg(HashController.progress).arg(HashController.currentFile)
                    color: Theme.muted
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideMiddle

                    HoverHandler {
                        id: progressHover
                    }
                    ToolTip.visible: progressHover.hovered
                    ToolTip.delay: 600
                    ToolTip.text: HashController.currentFile
                }
            }

            AppBanner {
                Layout.fillWidth: true
                visible: !HashController.busy && HashController.lastSummary.files !== undefined
                kind: root.summaryKind
                title: root.summaryTitle()
            }

            // Compare result: one banner plus the counters as pills, so the
            // five numbers are scannable instead of one run-on sentence.
            AppBanner {
                Layout.fillWidth: true
                visible: HashController.compare.valid
                kind: HashController.compare.exact ? "ok" : "danger"
                title: HashController.compare.exact ? qsTr("剪贴板比对完全匹配") : qsTr("剪贴板比对存在差异")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2
                visible: HashController.compare.valid

                Repeater {
                    model: root.comparePills

                    delegate: Rectangle {
                        required property var modelData

                        implicitWidth: pillLabel.implicitWidth + 2 * Theme.space3
                        implicitHeight: Theme.space6
                        radius: Theme.radiusSm
                        color: root.pillTint(modelData.tone)
                        border.width: 1
                        border.color: Theme.line

                        Label {
                            id: pillLabel

                            anchors.centerIn: parent
                            text: modelData.label + " " + (modelData.value === undefined ? "–" : modelData.value)
                            color: root.pillColor(modelData.tone)
                            font.pixelSize: Theme.fontSmall
                            font.family: Theme.monoFamily
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
            }

            AppBanner {
                Layout.fillWidth: true
                visible: !HashController.compare.valid && HashController.compare.error !== ""
                kind: "danger"
                title: HashController.compare.error
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppButton {
                    text: qsTr("复制结果")
                    enabled: HashController.hasOutput
                    onClicked: Clipboard.copy(HashController.outputText) ? Toast.show(qsTr("结果已复制"), "ok")
                                                                        : Toast.show(qsTr("结果超过 16 MiB，无法复制"), "error")
                }
                AppButton {
                    text: qsTr("保存结果")
                    enabled: HashController.hasOutput
                    onClicked: saveDialog.open()
                }
                Item {
                    Layout.fillWidth: true
                }
                AppButton {
                    text: qsTr("粘贴对比")
                    variant: "ghost"
                    enabled: HashController.hasOutput
                    onClicked: HashController.compareClipboard(Clipboard.text())

                    HoverHandler {
                        id: compareHover
                    }
                    ToolTip.visible: compareHover.hovered
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("支持完整清单，或直接粘贴摘要（自动忽略 sha256: 等算法前缀）")
                }
                AppButton {
                    text: qsTr("清除比对")
                    variant: "ghost"
                    visible: HashController.compare.valid || HashController.compare.error !== ""
                    onClicked: HashController.resetCompare()
                }
            }

            AppTextArea {
                id: outputArea

                Layout.fillWidth: true
                Layout.preferredHeight: Theme.listRowHeight * 5
                readOnly: true
                text: HashController.outputText
                placeholderText: qsTr("结果会显示在这里")
                wrapMode: TextArea.NoWrap
                textFormat: TextArea.PlainText
                font.pixelSize: Theme.fontMono
                font.family: Theme.monoFamily
            }
        }
    }
}
