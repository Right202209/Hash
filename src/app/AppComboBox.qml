import QtQuick
import QtQuick.Controls.Basic

// AppComboBox themes plain and editable combo boxes (radix tool): inset
// field, shared focus border and a popup whose rows use the toolbox
// surfaces instead of the palette-derived Basic look.
ComboBox {
    id: control

    leftPadding: 10
    rightPadding: 30
    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
        implicitWidth: 110
        implicitHeight: Theme.controlHeight
        radius: Theme.radius
        color: Theme.inset
        border.width: 1
        border.color: control.activeFocus ? Theme.ice : control.hovered ? Theme.muted : Theme.line
        Behavior on border.color {
            ColorAnimation {
                duration: Theme.fadeMs
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: Theme.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.focusRing
            visible: control.visualFocus
        }
    }

    indicator: Label {
        x: control.width - width - 10
        y: (control.height - height) / 2
        text: "▾"
        color: control.hovered ? Theme.text : Theme.muted
        font.pixelSize: Theme.fontSmall
    }

    contentItem: TextInput {
        text: control.editable ? control.editText : control.displayText
        readOnly: !control.editable
        font: control.font
        color: Theme.text
        selectionColor: Theme.iceDeep
        selectedTextColor: Theme.text
        selectByMouse: true
        verticalAlignment: TextInput.AlignVCenter
        validator: control.validator
        cursorVisible: control.editable && control.activeFocus
    }

    delegate: ItemDelegate {
        id: row

        required property var modelData
        required property int index

        width: ListView.view ? ListView.view.width : implicitWidth
        highlighted: control.highlightedIndex === index
        leftPadding: Theme.space2
        rightPadding: Theme.space2
        topPadding: Theme.space1
        bottomPadding: Theme.space1

        contentItem: Label {
            text: row.modelData
            font: control.font
            color: row.highlighted ? Theme.ice : Theme.text
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: Theme.radius
            color: row.highlighted ? Theme.raised : "transparent"
        }
    }

    popup: Popup {
        y: control.height + Theme.space1
        width: control.width
        padding: Theme.space1

        background: Rectangle {
            radius: Theme.cardRadius
            color: Theme.surface
            border.width: 1
            border.color: Theme.line
        }

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            spacing: Theme.space1
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollBar.vertical: AppScrollBar {}
        }
    }
}
