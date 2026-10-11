// Command hash-gui is the Qt Quick entry point for the hashing toolbox.
#include "app/clipservice.h"
#include "core/types.h"
#include "tools/algorithmsmodel.h"
#include "tools/codectool.h"
#include "tools/colortool.h"
#include "tools/hashcontroller.h"
#include "tools/jsontool.h"
#include "tools/passwordtool.h"
#include "tools/radixtool.h"
#include "tools/textdigesttool.h"
#include "tools/timestamptool.h"
#include "tools/toolregistrymodel.h"
#include "tools/uuidtool.h"

#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QMutex>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTextStream>
#include <QtQml>

#include <cstdio>

namespace {

// guiMessageHandler appends Qt and QML diagnostics to a small log file under
// the application data directory and mirrors them to stderr: the windowsgui
// subsystem has no console, so a failed launch would otherwise be completely
// silent, while CI smoke runs need the console output. The file is reset once
// it grows past 512 KiB, keeping the on-disk footprint negligible.
void guiMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message) {
    // The Qt calls below (QStandardPaths, QDir, QFile) may log themselves. A
    // nested call on the same thread would block forever on the non-recursive
    // mutex, so it only reaches stderr.
    thread_local bool inHandler = false;
    if (inHandler) {
        std::fprintf(stderr, "%s\n", qUtf8Printable(message));
        return;
    }
    inHandler = true;
    struct ResetGuard {
        ~ResetGuard() { inHandler = false; }
    } resetGuard;
    static QMutex logMutex;
    QMutexLocker locker(&logMutex);
    std::fprintf(stderr, "%s\n", qUtf8Printable(message));
    std::fflush(stderr);
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (directory.isEmpty()) {
        return;
    }
    QDir().mkpath(directory);
    QFile logFile(directory + QStringLiteral("/gui.log"));
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }
    if (logFile.size() > 512 * 1024) {
        logFile.resize(0);
    }
    const char *level = "info";
    switch (type) {
    case QtDebugMsg:
        level = "debug";
        break;
    case QtInfoMsg:
        level = "info";
        break;
    case QtWarningMsg:
        level = "warning";
        break;
    case QtCriticalMsg:
        level = "critical";
        break;
    case QtFatalMsg:
        level = "fatal";
        break;
    }
    QTextStream stream(&logFile);
    stream << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) << ' '
           << level << ": " << message << '\n';
}

// applyDarkPalette ports the widget theme to Qt Quick: the Basic style
// renders from the application palette, so one dark palette covers every
// control without shipping a styled QML module. The same values are mirrored
// on the ApplicationWindow palette (Main.qml), which is what the QML scene
// actually inherits on every platform: a QGuiApplication palette alone can
// be overridden by the Windows platform theme, which left tool pages on the
// Basic style's light defaults (issue #1). Mid, Light and Dark drive the
// style's derived borders so every control shares the same hairline.
void applyDarkPalette(QGuiApplication &application) {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x0F, 0x11, 0x15));
    palette.setColor(QPalette::WindowText, QColor(0xE6, 0xE9, 0xEF));
    palette.setColor(QPalette::Base, QColor(0x22, 0x28, 0x36));
    palette.setColor(QPalette::AlternateBase, QColor(0x15, 0x19, 0x22));
    palette.setColor(QPalette::Text, QColor(0xE6, 0xE9, 0xEF));
    palette.setColor(QPalette::Button, QColor(0x15, 0x19, 0x22));
    palette.setColor(QPalette::ButtonText, QColor(0xE6, 0xE9, 0xEF));
    palette.setColor(QPalette::ToolTipBase, QColor(0x15, 0x19, 0x22));
    palette.setColor(QPalette::ToolTipText, QColor(0xE6, 0xE9, 0xEF));
    palette.setColor(QPalette::Highlight, QColor(0x2E, 0x47, 0x80));
    palette.setColor(QPalette::HighlightedText, QColor(0xE6, 0xE9, 0xEF));
    palette.setColor(QPalette::Link, QColor(0x7A, 0xA2, 0xFF));
    palette.setColor(QPalette::Light, QColor(0x1C, 0x21, 0x2C));
    palette.setColor(QPalette::Midlight, QColor(0x19, 0x1E, 0x28));
    palette.setColor(QPalette::Mid, QColor(0x2A, 0x31, 0x40));
    palette.setColor(QPalette::Dark, QColor(0x0A, 0x0C, 0x10));
    palette.setColor(QPalette::PlaceholderText, QColor(0x8A, 0x93, 0xA3));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x8A, 0x93, 0xA3));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x8A, 0x93, 0xA3));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x8A, 0x93, 0xA3));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(0x0F, 0x11, 0x15));
    application.setPalette(palette);
}

} // namespace

int main(int argc, char *argv[]) {
    QGuiApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Hash"));
    QCoreApplication::setApplicationName(QStringLiteral("hash-gui"));
    QCoreApplication::setApplicationVersion(QStringLiteral(HASH_VERSION));
    qInstallMessageHandler(guiMessageHandler);
    applyDarkPalette(application);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    // Pin the color scheme so the Basic style resolves any role the mirrored
    // palettes leave unset from its dark defaults, and Windows renders the
    // title bar dark instead of blending a light frame into the shell.
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(15);
    application.setFont(font);

    qRegisterMetaType<hash_core::Progress>("hash_core::Progress");
    qRegisterMetaType<QVector<hash_core::FileResult>>("QVector<hash_core::FileResult>");

    hash_core::HashController hashController;
    hash_core::AlgorithmsModel algorithmsModel;
    hash_core::ToolRegistryModel toolRegistryModel;
    // A second, independently filtered registry backs the Ctrl+K palette so
    // typing a query never hides entries in the always-visible sidebar.
    hash_core::ToolRegistryModel toolSearchModel;
    hash_core::TextDigestTool textDigestTool;
    hash_core::CodecTool codecTool;
    hash_core::JsonTool jsonTool;
    hash_core::TimestampTool timestampTool;
    hash_core::UuidTool uuidTool;
    hash_core::RadixTool radixTool;
    hash_core::ColorTool colorTool;
    hash_core::PasswordTool passwordTool;
    ClipboardService clipboardService;

    qmlRegisterSingletonInstance("HashTools", 1, 0, "Tools", &toolRegistryModel);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "ToolSearch", &toolSearchModel);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Algorithms", &algorithmsModel);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "HashController", &hashController);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "TextDigest", &textDigestTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Codec", &codecTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Json", &jsonTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Timestamp", &timestampTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Uuid", &uuidTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Radix", &radixTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Color", &colorTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Password", &passwordTool);
    qmlRegisterSingletonInstance("HashTools", 1, 0, "Clipboard", &clipboardService);

    QQmlApplicationEngine engine;
    engine.loadFromModule("Hash", "Main");
    qInfo() << "QML load finished with" << engine.rootObjects().size() << "root object(s)";
    if (engine.rootObjects().isEmpty()) {
        qCritical().noquote() << QStringLiteral("QML load failed; the window was not created. "
                                                "Check the QML diagnostics above and gui.log "
                                                "under the application data directory.");
        return 1;
    }
    qInfo() << "entering event loop";
    return application.exec();
}
