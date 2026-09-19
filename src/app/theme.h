#pragma once

#include <QColor>

class QApplication;
class QWidget;

namespace hash_ui {

// The dark palette carried over from the native Win32 theme. The one
// inconsistency in the old COLORREF packing (the inset control background)
// is resolved to the navy shade its name and the rest of the palette imply.
QColor canvasColor();
QColor surfaceColor();
QColor raisedColor();
QColor insetColor();
QColor lineColor();
QColor textColor();
QColor mutedColor();
QColor iceColor();

// applyDarkTheme installs the palette and the base UI font on the
// application.
void applyDarkTheme(QApplication &application);

// panelStyleSheet returns the widget stylesheet for the panels, controls and
// accent tracks.
QString panelStyleSheet();

// markRunning switches the accent tracks between their raised and dimmed
// colors while a batch is running.
void markRunning(QWidget *panelRoot, bool running);

} // namespace hash_ui
