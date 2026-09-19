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
// control without shipping a styled QML module.
void applyDarkPalette(QGuiApplication &application) {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x08, 0x14, 0x21));
    palette.setColor(QPalette::WindowText, QColor(0xE8, 0xF5, 0xFB));
    palette.setColor(QPalette::Base, QColor(0x1A, 0x2B, 0x38));
    palette.setColor(QPalette::AlternateBase, QColor(0x0E, 0x20, 0x31));
    palette.setColor(QPalette::Text, QColor(0xE8, 0xF5, 0xFB));
    palette.setColor(QPalette::Button, QColor(0x0E, 0x20, 0x31));
    palette.setColor(QPalette::ButtonText, QColor(0xE8, 0xF5, 0xFB));
    palette.setColor(QPalette::ToolTipBase, QColor(0x0E, 0x20, 0x31));
    palette.setColor(QPalette::ToolTipText, QColor(0xE8, 0xF5, 0xFB));
    palette.setColor(QPalette::Highlight, QColor(0x14, 0x2E, 0x43));
    palette.setColor(QPalette::HighlightedText, QColor(0xE8, 0xF5, 0xFB));
    palette.setColor(QPalette::PlaceholderText, QColor(0x8E, 0xA8, 0xBB));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x8E, 0xA8, 0xBB));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0x8E, 0xA8, 0xBB));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x8E, 0xA8, 0xBB));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(0x08, 0x14, 0x21));
    application.setPalette(palette);
}

} // namespace

int main(int argc, char *argv[]) {
    QGuiApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Hash"));
    QCoreApplication::setApplicationName(QStringLiteral("hash-gui"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.1.0"));
    qInstallMessageHandler(guiMessageHandler);
    applyDarkPalette(application);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(15);
    application.setFont(font);

    qRegisterMetaType<hash_core::Progress>("hash_core::Progress");
    qRegisterMetaType<QVector<hash_core::FileResult>>("QVector<hash_core::FileResult>");

    hash_core::HashController hashController;
    hash_core::AlgorithmsModel algorithmsModel;
    hash_core::ToolRegistryModel toolRegistryModel;
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
