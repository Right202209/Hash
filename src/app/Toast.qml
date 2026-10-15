pragma Singleton
import QtQuick

// Toast is the cross-page notification channel: any page emits
// show(message[, kind]) and the main window surfaces it in the status bar.
// kind is "info" (default, ice), "ok" or "error"; both parameters default so
// the pre-v1.2 one-argument calls keep behaving exactly as before.
QtObject {
    signal show(string message, string kind)
}
