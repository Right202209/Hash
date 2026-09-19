#pragma once

#include "tools/passwordtool.h"
#include "tools/radixtool.h"
#include "tools/uuidtool.h"

#include <QRegularExpression>
#include <QSet>
#include <QtTest>

class TestGeneratorTools : public QObject {
    Q_OBJECT

  private slots:
    void uuidFormatAndUniqueness() {
        hash_core::UuidTool tool;
        const QVariantMap outcome = tool.generate(50, false, false, true);
        QCOMPARE(outcome.value("ok").toBool(), true);
        const QStringList values = outcome.value("list").toStringList();
        QCOMPARE(values.size(), 50);
        const QRegularExpression pattern(
            QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-"
                           "[0-9a-f]{12}$"));
        QSet<QString> unique;
        for (const QString &value : values) {
            QVERIFY(pattern.match(value).hasMatch());
            unique.insert(value);
        }
        QCOMPARE(unique.size(), 50);
    }

    void uuidOptionsApply() {
        hash_core::UuidTool tool;
        const QVariantMap outcome = tool.generate(1, true, true, false);
        QCOMPARE(outcome.value("ok").toBool(), true);
        const QString value = outcome.value("list").toStringList().first();
        QVERIFY(value.startsWith(u'{'));
        QVERIFY(value.endsWith(u'}'));
        QVERIFY(!value.contains(u'-'));
        const QString hex = value.mid(1, value.size() - 2);
        QCOMPARE(hex, hex.toUpper());
        QCOMPARE(hex.size(), 32);
    }

    void uuidRejectsOutOfRangeCount() {
        hash_core::UuidTool tool;
        QCOMPARE(tool.generate(0, false, false, true).value("ok").toBool(), false);
        QCOMPARE(tool.generate(1001, false, false, true).value("ok").toBool(), false);
    }

    void radixKnownConversions() {
        hash_core::RadixTool tool;
        QCOMPARE(tool.convert(QStringLiteral("255"), 10, 16).value("value").toString(),
                 QStringLiteral("ff"));
        QCOMPARE(tool.convert(QStringLiteral("ff"), 16, 10).value("value").toString(),
                 QStringLiteral("255"));
        QCOMPARE(tool.convert(QStringLiteral("1010"), 2, 10).value("value").toString(),
                 QStringLiteral("10"));
        QCOMPARE(tool.convert(QStringLiteral("-16"), 10, 16).value("value").toString(),
                 QStringLiteral("-10"));
    }

    void radixRejectsBadInputAndBases() {
        hash_core::RadixTool tool;
        QCOMPARE(tool.convert(QStringLiteral("zz"), 16, 10).value("ok").toBool(), false);
        QCOMPARE(tool.convert(QStringLiteral("10"), 1, 10).value("ok").toBool(), false);
        QCOMPARE(tool.convert(QStringLiteral("10"), 10, 37).value("ok").toBool(), false);
    }

    void passwordHonorsLengthAndSets() {
        hash_core::PasswordTool tool;
        const QVariantMap outcome = tool.generate(16, 5, true, true, false, true, false);
        QCOMPARE(outcome.value("ok").toBool(), true);
        const QStringList passwords = outcome.value("list").toStringList();
        QCOMPARE(passwords.size(), 5);
        const QString symbols = QStringLiteral("!@#$%^&*()-_=+[]{};:,.?");
        for (const QString &password : passwords) {
            QCOMPARE(password.size(), 16);
            bool hasUpper = false;
            bool hasLower = false;
            bool hasSymbol = false;
            for (const QChar &character : password) {
                hasUpper = hasUpper || character.isUpper();
                hasLower = hasLower || character.isLower();
                hasSymbol = hasSymbol || symbols.contains(character);
            }
            QVERIFY(hasUpper);
            QVERIFY(hasLower);
            QVERIFY(hasSymbol);
        }
    }

    void passwordExcludesAmbiguousCharacters() {
        hash_core::PasswordTool tool;
        const QVariantMap outcome = tool.generate(64, 20, true, true, true, true, true);
        QCOMPARE(outcome.value("ok").toBool(), true);
        const QString ambiguous = QStringLiteral("0Oo1lI|`'\"");
        for (const QString &password : outcome.value("list").toStringList()) {
            for (const QChar &character : ambiguous) {
                QVERIFY2(
                    !password.contains(character),
                    qPrintable(QStringLiteral("unexpected %1 in %2").arg(character).arg(password)));
            }
        }
    }

    void passwordRejectsBadOptions() {
        hash_core::PasswordTool tool;
        QCOMPARE(tool.generate(3, 1, true, false, false, false, false).value("ok").toBool(), false);
        QCOMPARE(tool.generate(16, 0, true, false, false, false, false).value("ok").toBool(),
                 false);
        QCOMPARE(tool.generate(16, 1, false, false, false, false, false).value("ok").toBool(),
                 false);
    }
};
