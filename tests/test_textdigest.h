#pragma once

#include "tools/textdigesttool.h"

#include <QtTest>

class TestTextDigest : public QObject {
    Q_OBJECT

  private slots:
    void computesKnownVectors() {
        hash_core::TextDigestTool tool;
        const QVariantMap outcome =
            tool.hashText(QStringLiteral("abc"), {QStringLiteral("sha256"), QStringLiteral("md5"),
                                                  QStringLiteral("crc32-ieee")});
        QCOMPARE(outcome.value("ok").toBool(), true);
        const QVariantList digests = outcome.value("digests").toList();
        QCOMPARE(digests.size(), 3);
        QCOMPARE(digests.at(0).toMap().value("algorithm").toString(), QStringLiteral("sha256"));
        QCOMPARE(
            digests.at(0).toMap().value("value").toString(),
            QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        QCOMPARE(digests.at(1).toMap().value("value").toString(),
                 QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
        // CRC-32/IEEE of "abc" is 0x352441C2, matching the file hashing path.
        QCOMPARE(digests.at(2).toMap().value("value").toString(), QStringLiteral("352441c2"));
    }

    void hashesEmptyText() {
        hash_core::TextDigestTool tool;
        const QVariantMap outcome = tool.hashText(QString(), {QStringLiteral("md5")});
        QCOMPARE(outcome.value("ok").toBool(), true);
        const QVariantList digests = outcome.value("digests").toList();
        QCOMPARE(digests.at(0).toMap().value("value").toString(),
                 QStringLiteral("d41d8cd98f00b204e9800998ecf8427e"));
    }

    void rejectsEmptyAlgorithmList() {
        hash_core::TextDigestTool tool;
        const QVariantMap outcome = tool.hashText(QStringLiteral("abc"), {});
        QCOMPARE(outcome.value("ok").toBool(), false);
        QVERIFY(!outcome.value("error").toString().isEmpty());
    }

    void rejectsUnknownAlgorithm() {
        hash_core::TextDigestTool tool;
        const QVariantMap outcome = tool.hashText(
            QStringLiteral("abc"), {QStringLiteral("sha256"), QStringLiteral("nope")});
        QCOMPARE(outcome.value("ok").toBool(), false);
        QVERIFY(outcome.value("error").toString().contains(QStringLiteral("nope")));
    }
};
