#pragma once

#include "tools/colortool.h"

#include <QtTest>

class TestColorTool : public QObject {
    Q_OBJECT

  private slots:
    void convertsHexToAllForms() {
        hash_core::ColorTool tool;
        const QVariantMap outcome = tool.convert(QStringLiteral("#FF8000"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("hex").toString(), QStringLiteral("#FF8000"));
        QCOMPARE(outcome.value("rgba").toString(), QStringLiteral("rgb(255, 128, 0)"));
        QCOMPARE(outcome.value("hsl").toString(), QStringLiteral("hsl(30, 100%, 50%)"));
    }

    void expandsShortHex() {
        hash_core::ColorTool tool;
        const QVariantMap outcome = tool.convert(QStringLiteral("#f80"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("hex").toString(), QStringLiteral("#FF8800"));
        QCOMPARE(outcome.value("rgba").toString(), QStringLiteral("rgb(255, 136, 0)"));
    }

    void parsesRgbWithAlpha() {
        hash_core::ColorTool tool;
        const QVariantMap outcome = tool.convert(QStringLiteral("rgba(255, 0, 0, 0.5)"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("hex").toString(), QStringLiteral("#FF0000"));
        QCOMPARE(outcome.value("rgba").toString(), QStringLiteral("rgba(255, 0, 0, 0.50)"));
        QCOMPARE(outcome.value("alpha").toInt(), 128);
    }

    void parsesEightDigitHexAlpha() {
        hash_core::ColorTool tool;
        const QVariantMap outcome = tool.convert(QStringLiteral("#80FFFFFF"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("hex").toString(), QStringLiteral("#FFFFFF"));
        QCOMPARE(outcome.value("rgba").toString(), QStringLiteral("rgba(255, 255, 255, 0.50)"));
    }

    void convertsHslInput() {
        hash_core::ColorTool tool;
        const QVariantMap outcome = tool.convert(QStringLiteral("hsl(120, 100%, 25%)"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("hex").toString(), QStringLiteral("#008000"));
    }

    void rejectsUnknownNotation() {
        hash_core::ColorTool tool;
        QCOMPARE(tool.convert(QStringLiteral("red")).value("ok").toBool(), false);
        QCOMPARE(tool.convert(QStringLiteral("#12")).value("ok").toBool(), false);
        QCOMPARE(tool.convert(QStringLiteral("rgb(300, 0, 0)")).value("ok").toBool(), false);
    }
};
