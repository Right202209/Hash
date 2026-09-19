#include "app/clipservice.h"

#include <QClipboard>
#include <QGuiApplication>

namespace {
constexpr int kMaxClipboardBytes = 16 * 1024 * 1024;
} // namespace

ClipboardService::ClipboardService(QObject *parent) : QObject(parent) {}

bool ClipboardService::copy(const QString &text) {
    if (text.size() * 2 > kMaxClipboardBytes) {
        return false;
    }
    QGuiApplication::clipboard()->setText(text);
    return true;
}

QString ClipboardService::text() const { return QGuiApplication::clipboard()->text(); }
