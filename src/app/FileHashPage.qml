import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import HashTools

Page {
    id: root

    property int algorithmCount: 0

    header: PageHeader {
        icon: "#"
        title: qsTr("文件哈希")
        description: qsTr("批量计算文件摘要 · 单次读取多算法并行 · 支持拖拽与剪贴板比对")
        onBackRequested: {
            if (root.StackView.view) {
                root.StackView.view.pop()
            }
        }
    }

    function collectAlgorithms() {
        var names = []
        for (var i = 0; i < algorithmChips.count; ++i) {
            const chip = algorithmChips.itemAt(i)
            if (chip.checked) {
                names.push(chip.name)
            }
        }
        return names
    }

    function refreshAlgorithmCount() {
        root.algorithmCount = collectAlgorithms().length
    }

    Component.onCompleted: refreshAlgorithmCount()

    function announceAdd(outcome) {
        if (outcome.error !== undefined) {
            Toast.show(outcome.error)
            return
        }
        if (outcome.added === 0 && outcome.rejected === 0) {
            Toast.show(qsTr("没有新增文件"))
        } else if (outcome.rejected > 0) {
            Toast.show(qsTr("已添加 %1 个文件 · 忽略 %2 项").arg(outcome.added).arg(outcome.rejected))
        } else {
            Toast.show(qsTr("已添加 %1 个文件").arg(outcome.added))
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
            Toast.show(error === "" ? qsTr("结果已保存") : qsTr("保存失败：") + error)
        }
    }

    contentItem: ColumnLayout {
        spacing: 8

        Flow {
            Layout.fillWidth: true
            spacing: 4

            Repeater {
                id: algorithmChips

                model: Algorithms

                delegate: CheckBox {
                    required property string name
                    required property string label

                    text: label
                    checked: name === "sha256"
                    onToggled: root.refreshAlgorithmCount()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.inset
            border.color: Theme.line

            ListView {
                id: fileList

                anchors.fill: parent
                clip: true
                model: HashController.files
                boundsBehavior: Flickable.StopAtBounds
                reuseItems: false
                ScrollBar.vertical: ScrollBar {}

                delegate: Rectangle {
                    id: fileRow

                    width: fileList.width
                    height: fileColumn.implicitHeight + 14
                    color: fileRow.index % 2 === 0 ? "transparent" : Theme.canvas

                    required property int index
                    required property string path
                    required property string name
                    required property int size
                    required property int status
                    required property bool changed
                    required property string error
                    required property var digests

                    ColumnLayout {
                        id: fileColumn

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 8
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 8
                                Layout.preferredHeight: 8
                                radius: 4
                                color: fileRow.status === 2 ? Theme.ok
                                       : fileRow.status === 3 ? Theme.danger
                                       : fileRow.status === 1 ? Theme.ice
                                       : fileRow.status === 4 ? Theme.muted : Theme.line
                            }
                            Label {
                                Layout.fillWidth: true
                                text: fileRow.path
                                color: Theme.text
                                elide: Text.ElideMiddle
                            }
                            Label {
                                text: fileRow.changed ? qsTr("已变化") : ""
                                color: Theme.danger
                                visible: fileRow.changed
                            }
                            Label {
                                text: Theme.sizeText(fileRow.size)
                                color: Theme.muted
                            }
                            Label {
                                text: Theme.statusText(fileRow.status)
                                color: fileRow.status === 3 ? Theme.danger : Theme.muted
                            }
                            ToolButton {
                                text: qsTr("✕")
                                enabled: !HashController.busy
                                visible: !HashController.busy
                                onClicked: HashController.removeRow(fileRow.index)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: fileRow.error !== ""
                            text: fileRow.error
                            color: Theme.danger
                            wrapMode: Text.WrapAnywhere
                        }

                        Repeater {
                            model: fileRow.digests

                            delegate: RowLayout {
                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.preferredWidth: 110
                                    text: modelData.algorithm
                                    color: Theme.ice
                                    font.pixelSize: 12
                                    font.families: Theme.monoFamilies
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.value
                                    color: Theme.text
                                    elide: Text.ElideMiddle
                                    font.pixelSize: 12
                                    font.families: Theme.monoFamilies
                                }
                                ToolButton {
                                    text: qsTr("⧉")
                                    font.pixelSize: 12
                                    ToolTip.visible: hovered
                                    ToolTip.text: qsTr("复制摘要")
                                    onClicked: Clipboard.copy(modelData.value)
                                               ? Toast.show(qsTr("摘要已复制"))
                                               : Toast.show(qsTr("内容过大，无法复制"))
                                }
                            }
                        }
                    }
                }
            }

            Column {
                anchors.centerIn: parent
                visible: fileList.count === 0
                spacing: 6

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("把文件拖到这里")
                    color: Theme.ice
                    font.pixelSize: 16
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("或点击“添加文件”选择要计算摘要的文件")
                    color: Theme.muted
                }
            }

            DropArea {
                anchors.fill: parent
                onDropped: function(drop) {
                    if (drop.hasUrls) {
                        root.announceAdd(HashController.addFiles(drop.urls))
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("添加文件")
                enabled: !HashController.busy
                onClicked: addDialog.open()
            }
            Button {
                text: qsTr("清空")
                enabled: !HashController.busy && fileList.count > 0
                onClicked: {
                    HashController.clearFiles()
                    Toast.show(qsTr("已清空文件列表"))
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("%1 个文件 · 已选 %2 个算法").arg(fileList.count).arg(root.algorithmCount)
                color: Theme.muted
            }
            Button {
                text: HashController.busy ? qsTr("取消") : qsTr("开始计算")
                highlighted: !HashController.busy
                enabled: HashController.busy || (fileList.count > 0 && root.algorithmCount > 0)
                onClicked: {
                    if (HashController.busy) {
                        HashController.cancel()
                        Toast.show(qsTr("正在停止……"))
                    } else {
                        HashController.start(root.collectAlgorithms())
                    }
                }
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: HashController.progress
            visible: HashController.busy
        }

        Label {
            Layout.fillWidth: true
            visible: HashController.busy
            text: qsTr("计算中 %1% · %2").arg(HashController.progress).arg(HashController.currentFile)
            color: Theme.muted
            elide: Text.ElideMiddle
        }

        Label {
            Layout.fillWidth: true
            visible: !HashController.busy && HashController.lastSummary.files !== undefined
            text: {
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
            color: Theme.muted
        }

        Label {
            Layout.fillWidth: true
            visible: HashController.compare.valid
            text: HashController.compare.valid
                  ? qsTr("剪贴板比对：匹配 %1 · 不匹配 %2 · 缺少 %3 · 多余 %4 · 重复 %5")
                        .arg(HashController.compare.matches)
                        .arg(HashController.compare.mismatches)
                        .arg(HashController.compare.missing)
                        .arg(HashController.compare.unexpected)
                        .arg(HashController.compare.duplicates)
                  : ""
            color: HashController.compare.exact ? Theme.ok : Theme.danger
        }

        Label {
            Layout.fillWidth: true
            visible: !HashController.compare.valid && HashController.compare.error !== ""
            text: HashController.compare.error
            color: Theme.danger
            wrapMode: Text.WrapAnywhere
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: qsTr("复制结果")
                enabled: HashController.hasOutput
                onClicked: Clipboard.copy(HashController.outputText)
                           ? Toast.show(qsTr("结果已复制"))
                           : Toast.show(qsTr("结果超过 16 MiB，无法复制"))
            }
            Button {
                text: qsTr("保存结果")
                enabled: HashController.hasOutput
                onClicked: saveDialog.open()
            }
            Button {
                text: qsTr("粘贴对比")
                enabled: HashController.hasOutput
                onClicked: HashController.compareClipboard(Clipboard.text())
            }
            Button {
                text: qsTr("清除比对")
                visible: HashController.compare.valid || HashController.compare.error !== ""
                onClicked: HashController.resetCompare()
            }
        }

        TextArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
            readOnly: true
            text: HashController.outputText
            placeholderText: qsTr("结果会显示在这里")
            wrapMode: TextArea.NoWrap
            textFormat: TextArea.PlainText
            font.pixelSize: 13
            font.families: Theme.monoFamilies
        }
    }
}
