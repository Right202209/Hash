#pragma once

#include <QObject>

// ClipboardService bridges the system clipboard to QML: tool pages copy
// results out and the file-hash page reads digest listings back in for
// comparison.
class ClipboardService : public QObject {
    Q_OBJECT

  public:
    explicit ClipboardService(QObject *parent = nullptr);

    // copy returns false when the text exceeds the clipboard size guard.
    Q_INVOKABLE bool copy(const QString &text);
    Q_INVOKABLE QString text() const;
};
