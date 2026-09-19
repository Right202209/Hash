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
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

namespace {

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
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return application.exec();
}
