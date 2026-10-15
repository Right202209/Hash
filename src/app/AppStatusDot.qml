import QtQuick

// AppStatusDot is the colored half of the status double channel: it always
// renders next to (or inside a row that also shows) status text, never alone.
Rectangle {
    id: root

    // Queue status code, resolved through Theme.statusColor so the dot matches
    // the status label on every page.
    property int status: 0

    implicitWidth: Theme.statusDot
    implicitHeight: Theme.statusDot
    radius: width / 2
    color: Theme.statusColor(root.status)
}
