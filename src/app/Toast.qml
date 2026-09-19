pragma Singleton
import QtQuick

// Toast is the cross-page notification channel: any page emits show(message)
// and the main window surfaces it in the status bar.
QtObject {
    signal show(string message)
}
