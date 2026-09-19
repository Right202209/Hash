#pragma once

#include "tools/codectool.h"

#include <QtTest>

class TestCodecs : public QObject {
    Q_OBJECT

  private slots:
    void base64RoundTrip() {
        hash_core::CodecTool tool;
        const QVariantMap encoded = tool.base64Encode(QStringLiteral("hello 世界"));
        QCOMPARE(encoded.value("ok").toBool(), true);
        QCOMPARE(encoded.value("text").toString(),
                 QString::fromLatin1(QByteArray("hello 世界").toBase64()));
        const QVariantMap decoded = tool.base64Decode(encoded.value("text").toString());
        QCOMPARE(decoded.value("ok").toBool(), true);
        QCOMPARE(decoded.value("text").toString(), QStringLiteral("hello 世界"));
    }

    void base64DecodeToleratesWhitespace() {
        hash_core::CodecTool tool;
        const QVariantMap decoded = tool.base64Decode(QStringLiteral("aGVs\nbG8=\r\n"));
        QCOMPARE(decoded.value("ok").toBool(), true);
        QCOMPARE(decoded.value("text").toString(), QStringLiteral("hello"));
    }

    void base64DecodeRejectsInvalidInput() {
        hash_core::CodecTool tool;
        const QVariantMap decoded = tool.base64Decode(QStringLiteral("not*valid!"));
        QCOMPARE(decoded.value("ok").toBool(), false);
        QVERIFY(!decoded.value("error").toString().isEmpty());
    }

    void urlEncodeDecode() {
        hash_core::CodecTool tool;
        const QVariantMap encoded = tool.urlEncode(QStringLiteral("a b&c=d/中"));
        QCOMPARE(encoded.value("ok").toBool(), true);
        const QVariantMap decoded = tool.urlDecode(encoded.value("text").toString());
        QCOMPARE(decoded.value("ok").toBool(), true);
        QCOMPARE(decoded.value("text").toString(), QStringLiteral("a b&c=d/中"));
    }
};
