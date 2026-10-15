import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HashTools

// AlgorithmChips renders the registered digest algorithms as toggle chips,
// grouped by the registry's category. Both hash pages share this component so
// the chips look and behave identically everywhere.
//
// Checked state lives in a name-keyed map on the root rather than on the chips
// themselves: the chips are created by a Repeater inside a Repeater, so they
// have no stable identity to reach across groups, and 全选/清空 have to write
// every chip at once.
ColumnLayout {
    id: root

    // Registry names checked when the chips are created; toggling afterwards
    // is free.
    property var defaults: ["sha256"]

    property var checkedMap: ({})

    readonly property var allNames: {
        const names = []
        const groups = Algorithms.grouped()
        for (let g = 0; g < groups.length; ++g) {
            const items = groups[g].algorithms
            for (let i = 0; i < items.length; ++i) {
                names.push(items[i].name)
            }
        }
        return names
    }

    signal selectionChanged()

    spacing: Theme.space2

    // Registry names, not display labels: this is what the engines accept.
    function selectedNames() {
        const names = []
        for (let i = 0; i < root.allNames.length; ++i) {
            const name = root.allNames[i]
            if (root.checkedMap[name] === true) {
                names.push(name)
            }
        }
        return names
    }

    function setChecked(name, value) {
        const next = Object.assign({}, root.checkedMap)
        next[name] = value === true
        root.checkedMap = next
        root.selectionChanged()
    }

    function setAll(value) {
        const next = {}
        for (let i = 0; i < root.allNames.length; ++i) {
            next[root.allNames[i]] = value
        }
        root.checkedMap = next
        root.selectionChanged()
    }

    // The registry's category keys are the engine's vocabulary, not something
    // to show a user; these are the display names for them.
    function categoryLabel(category) {
        switch (category) {
        case "cryptographic":
            return qsTr("密码学哈希")
        case "legacy":
            return qsTr("过时算法")
        case "checksum":
            return qsTr("校验和")
        case "non-cryptographic":
            return qsTr("非加密哈希")
        }
        return category
    }

    Component.onCompleted: {
        const next = {}
        for (let i = 0; i < root.allNames.length; ++i) {
            next[root.allNames[i]] = root.defaults.indexOf(root.allNames[i]) !== -1
        }
        root.checkedMap = next
    }

    // Selection actions, kept on their own row so they do not read as a chip.
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.space2

        Label {
            text: qsTr("算法")
            color: Theme.muted
            font.pixelSize: Theme.fontSmall
        }
        Item {
            Layout.fillWidth: true
        }
        AppButton {
            text: qsTr("全选")
            variant: "ghost"
            onClicked: root.setAll(true)
        }
        AppButton {
            text: qsTr("清空")
            variant: "ghost"
            onClicked: root.setAll(false)
        }
    }

    Repeater {
        model: Algorithms.grouped()

        delegate: ColumnLayout {
            required property var modelData

            Layout.fillWidth: true
            spacing: Theme.space1

            Label {
                text: root.categoryLabel(modelData.category)
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
                font.letterSpacing: 1
            }

            Flow {
                Layout.fillWidth: true
                spacing: Theme.space2

                Repeater {
                    model: modelData.algorithms

                    delegate: AbstractButton {
                        id: chip

                        required property var modelData

                        readonly property string algoName: modelData.name

                        // checked is driven purely by the map: the inner
                        // Repeater has no access to sibling chips, so 全选/清空
                        // must be able to rewrite every chip at once.
                        checkable: false
                        checked: root.checkedMap[chip.algoName] === true
                        hoverEnabled: true
                        leftPadding: Theme.space3
                        rightPadding: Theme.space3
                        topPadding: Theme.space1
                        bottomPadding: Theme.space1
                        implicitHeight: 28

                        onClicked: root.setChecked(chip.algoName, !root.checkedMap[chip.algoName])

                        // HoverHandler is the portable way to give a control a
                        // pointer cursor; Control itself has no cursor property
                        // in QML.
                        HoverHandler {
                            cursorShape: Qt.PointingHandCursor
                        }

                        contentItem: Row {
                            spacing: Theme.space1

                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                width: Theme.space3
                                text: chip.checked ? "✓" : ""
                                color: Theme.ice
                                font.pixelSize: Theme.fontSmall
                                font.family: Theme.monoFamily
                            }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: chip.modelData.label
                                color: chip.checked ? Theme.text : Theme.muted
                                font.pixelSize: Theme.fontSmall
                            }

                            // md5, sha1, checksums and the FNV family are fine
                            // for checksums but not for tamper protection; the
                            // registry is the only source of this flag
                            // (FR-ENG-7, Q-14).
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: chip.modelData.secure === false
                                text: "⚠"
                                color: Theme.danger
                                font.pixelSize: Theme.fontCaption
                                ToolTip.visible: chip.hovered
                                ToolTip.text: qsTr("%1 不适用于安全用途（仅可校验完整性）").arg(chip.modelData.label)
                            }
                        }

                        background: Rectangle {
                            radius: Theme.radiusSm
                            color: chip.checked ? Theme.raised : chip.hovered ? Theme.surface : Theme.inset
                            border.width: 1
                            border.color: chip.checked ? Theme.ice : chip.hovered ? Theme.muted : Theme.line

                            Behavior on color {
                                ColorAnimation {
                                    duration: Theme.fadeMs
                                }
                            }
                            Behavior on border.color {
                                ColorAnimation {
                                    duration: Theme.fadeMs
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: -3
                                radius: Theme.radiusSm + 3
                                color: "transparent"
                                border.width: 2
                                border.color: Theme.focusRing
                                visible: chip.visualFocus
                            }
                        }
                    }
                }
            }
        }
    }
}
