#include "app/theme.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QWidget>

namespace hash_ui {
namespace {

QColor color(int red, int green, int blue) { return QColor(red, green, blue); }

} // namespace

QColor canvasColor() { return color(0x08, 0x14, 0x21); }

QColor surfaceColor() { return color(0x0E, 0x20, 0x31); }

QColor raisedColor() { return color(0x14, 0x2E, 0x43); }

QColor insetColor() { return color(0x1A, 0x2B, 0x38); }

QColor lineColor() { return color(0x28, 0x48, 0x5D); }

QColor textColor() { return color(0xE8, 0xF5, 0xFB); }

QColor mutedColor() { return color(0x8E, 0xA8, 0xBB); }

QColor iceColor() { return color(0x76, 0xD8, 0xF7); }

void applyDarkTheme(QApplication &application) {
    QPalette palette;
    palette.setColor(QPalette::Window, canvasColor());
    palette.setColor(QPalette::WindowText, textColor());
    palette.setColor(QPalette::Base, insetColor());
    palette.setColor(QPalette::AlternateBase, surfaceColor());
    palette.setColor(QPalette::Text, textColor());
    palette.setColor(QPalette::Button, surfaceColor());
    palette.setColor(QPalette::ButtonText, textColor());
    palette.setColor(QPalette::ToolTipBase, surfaceColor());
    palette.setColor(QPalette::ToolTipText, textColor());
    palette.setColor(QPalette::Highlight, raisedColor());
    palette.setColor(QPalette::HighlightedText, textColor());
    palette.setColor(QPalette::PlaceholderText, mutedColor());
    palette.setColor(QPalette::Disabled, QPalette::WindowText, mutedColor());
    palette.setColor(QPalette::Disabled, QPalette::Text, mutedColor());
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, mutedColor());
    palette.setColor(QPalette::Disabled, QPalette::Button, canvasColor());
    application.setPalette(palette);

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(16);
    application.setFont(font);
}

QString panelStyleSheet() {
    return QStringLiteral(
               "QFrame#accentPanel { background: #%1; border: 1px solid #%2; border-left: 3px "
               "solid "
               "#%3; border-radius: 0px; }"
               "QFrame#accentPanel[running=\"true\"] { border-left: 3px solid #%1; }"
               "QPushButton { background: #%4; color: #%5; border: 1px solid #%2; padding: 3px "
               "14px; }"
               "QPushButton:hover:enabled { background: #%6; }"
               "QPushButton:pressed:enabled { background: #%2; }"
               "QPushButton:disabled { color: #%7; border-color: #%2; }"
               "QCheckBox { color: #%5; spacing: 6px; background: transparent; }"
               "QCheckBox:disabled { color: #%7; }"
               "QLabel { color: #%5; background: transparent; }"
               "QLabel[class=\"muted\"] { color: #%7; }"
               "QLabel[class=\"ice\"] { color: #%8; font-family: \"Cascadia Mono\"; }"
               "QTreeWidget { background: #%1; color: #%5; border: 1px solid #%2; }"
               "QTreeWidget::item:selected { background: #%3; }"
               "QHeaderView::section { background: #%4; color: #%7; border: none; border-right: "
               "1px "
               "solid #%2; padding: 2px 6px; }"
               "QPlainTextEdit { background: #%9; color: #%5; border: 1px solid #%2; "
               "font-family: \"Cascadia Mono\"; font-size: 15px; }"
               "QProgressBar { background: #%4; border: 1px solid #%2; color: #%5; text-align: "
               "center; }"
               "QProgressBar::chunk { background: #%6; }"
               "QScrollBar:vertical { background: #%1; width: 10px; border: none; }"
               "QScrollBar::handle:vertical { background: #%2; min-height: 24px; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }")
        .arg(canvasColor().name(), lineColor().name(), raisedColor().name(), surfaceColor().name(),
             textColor().name(), insetColor().name(), mutedColor().name(), iceColor().name(),
             insetColor().name());
}

void markRunning(QWidget *panelRoot, bool running) {
    const QList<QWidget *> panels = panelRoot->findChildren<QWidget *>(
        QStringLiteral("accentPanel"), Qt::FindDirectChildrenOnly);
    for (QWidget *panel : panels) {
        panel->setProperty("running", running);
        panel->style()->unpolish(panel);
        panel->style()->polish(panel);
    }
}

} // namespace hash_ui
