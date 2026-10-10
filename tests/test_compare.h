#pragma once

#include "common/compare.h"

#include <QtTest>

class TestCompare : public QObject {
    Q_OBJECT

  private slots:
    void parseTextAndCompare() {
        const QString text = QStringLiteral("sha256\tABC123\t\"C:\\\\Data\\\\a.bin\"\tsize=4\n");
        QVector<hash_core::CompareRecord> actual;
        QString error;
        QVERIFY2(hash_core::parseDigestText(text, &actual, &error), qPrintable(error));
        const QVector<hash_core::CompareRecord> expected = {{QStringLiteral("SHA256"),
                                                             QStringLiteral("abc123"),
                                                             QStringLiteral("C:\\Data\\a.bin")}};
        const hash_core::CompareSummary summary = hash_core::compareRecords(expected, actual);
        QVERIFY(summary.exact);
        QCOMPARE(summary.matches, 1);
    }

    void parseTextSupportsTsvEscapesAndUnicode() {
        const QString text = QString::fromUtf8(
            "algorithm\tdigest\tpath\terror\nSHA256\tABC123\tC:\\\\资料\\\\文件\\\\a.bin\t\n");
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(text, &records, &error), qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).path, QString::fromUtf8("C:\\资料\\文件\\a.bin"));
    }

    void parseTextSkipsErrorRows() {
        const QString text = QStringLiteral(
            "algorithm\tdigest\tpath\terror\nSHA256\tabc\tC:\\\\ok.bin\t\nERROR\t\tmissing.bin\t"
            "file missing\n");
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(text, &records, &error), qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).path, QStringLiteral("C:\\ok.bin"));
    }

    void parseTextRemovesTsvFormulaSafetyPrefix() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(QStringLiteral("sha256\tabc\t'=report.bin\n"), &records,
                                            &error),
                 qPrintable(error));
        QCOMPARE(records.at(0).path, QStringLiteral("=report.bin"));
    }

    void parseTextAcceptsByteOrderMark() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(QStringLiteral("\uFEFFsha256\tabc\t\"C:\\\\a.bin\"\n"),
                                            &records, &error),
                 qPrintable(error));
        QCOMPARE(records.size(), 1);
    }

    void parseQuotedPathDecodesOctalEscapes() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(QStringLiteral("sha256\tabc\t\"\\101\\x42.bin\"\n"),
                                            &records, &error),
                 qPrintable(error));
        QCOMPARE(records.at(0).path, QStringLiteral("AB.bin"));
        QVERIFY(!hash_core::parseDigestText(QStringLiteral("sha256\tabc\t\"\\108\"\n"),
                                            &records, &error));
    }

    void compareMissingUnexpectedMismatchAndDuplicate() {
        using hash_core::CompareRecord;
        const QVector<CompareRecord> expected = {
            {QStringLiteral("sha256"), QStringLiteral("aaa"), QStringLiteral("a")},
            {QStringLiteral("md5"), QStringLiteral("bbb"), QStringLiteral("b")},
            {QStringLiteral("sha1"), QStringLiteral("ccc"), QStringLiteral("c")}};
        const QVector<CompareRecord> actual = {
            {QStringLiteral("sha256"), QStringLiteral("aaa"), QStringLiteral("a")},
            {QStringLiteral("md5"), QStringLiteral("wrong"), QStringLiteral("b")},
            {QStringLiteral("sha512"), QStringLiteral("ddd"), QStringLiteral("d")},
            {QStringLiteral("sha512"), QStringLiteral("eee"), QStringLiteral("d")}};
        const hash_core::CompareSummary summary = hash_core::compareRecords(expected, actual);
        QCOMPARE(summary.matches, 1);
        QCOMPARE(summary.mismatches, 1);
        QCOMPARE(summary.missing, 1);
        QCOMPARE(summary.unexpected, 1);
        QCOMPARE(summary.duplicates, 1);
        QVERIFY(!summary.exact);
    }

    void recordsFromResultsSkipsErrors() {
        hash_core::FileResult good;
        good.result.path = QStringLiteral("a");
        good.result.order = {QStringLiteral("sha256")};
        good.result.digests = {{QStringLiteral("sha256"), QStringLiteral("abc")}};
        hash_core::FileResult failed;
        failed.error = QStringLiteral("error");
        const QVector<hash_core::CompareRecord> records =
            hash_core::recordsFromResults({good, failed});
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).digest, QStringLiteral("abc"));
    }

    void parseTextRejectsInvalidLines() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY(!hash_core::parseDigestText(QStringLiteral("not-a-record"), &records, &error));
    }

    void parseLooseDigestStripsAlgorithmPrefix() {
        const QString digest =
            QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        QVector<hash_core::CompareRecord> records;
        QString error;
        // Full-width colon, uppercase digest and the tag all get normalized.
        QVERIFY2(hash_core::parseDigestText(QStringLiteral("SHA256：") + digest.toUpper() +
                                                QStringLiteral("\n"),
                                            &records, &error),
                 qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).algorithm, QStringLiteral("sha256"));
        QCOMPARE(records.at(0).digest, digest);
        QVERIFY(records.at(0).loose);

        const QVector<hash_core::CompareRecord> expected = {
            {QStringLiteral("sha256"), digest, QStringLiteral("empty.bin")},
            {QStringLiteral("md5"), QStringLiteral("d41d8cd98f00b204e9800998ecf8427e"),
             QStringLiteral("other.bin")}};
        const hash_core::CompareSummary summary = hash_core::compareRecords(expected, records);
        QCOMPARE(summary.matches, 1);
        QCOMPARE(summary.mismatches, 0);
        // The paste is a bare-digest verification, so the expected record not
        // covered by it must not be counted as missing.
        QCOMPARE(summary.missing, 0);
        QCOMPARE(summary.unexpected, 0);
        QVERIFY(summary.exact);
    }

    void parseLooseDigestIgnoresAlgorithmWhenMatchingByValue() {
        const QString digest = QStringLiteral("d41d8cd98f00b204e9800998ecf8427e");
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(
            hash_core::parseDigestText(QStringLiteral("md5 = %1").arg(digest), &records, &error),
            qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).digest, digest);
        // The expected side uses a different algorithm name; the loose digest
        // still verifies by value.
        const QVector<hash_core::CompareRecord> expected = {
            {QStringLiteral("sha256"), digest, QStringLiteral("a")}};
        const hash_core::CompareSummary summary = hash_core::compareRecords(expected, records);
        QCOMPARE(summary.matches, 1);
        QVERIFY(summary.exact);
    }

    void parseLooseDigestToleratesSpacesAndTrailingPath() {
        const QString digest = QStringLiteral("d41d8cd98f00b204e9800998ecf8427e");
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(QStringLiteral("%1  report.pdf\n").arg(digest),
                                            &records, &error),
                 qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).digest, digest);
        QCOMPARE(records.at(0).path, QStringLiteral("report.pdf"));
        QVERIFY(records.at(0).loose);
    }

    void looseDigestMismatchCountsAsMismatchOnly() {
        const QString unknown =
            QStringLiteral("0000000000000000000000000000000000000000000000000000000000000000");
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(unknown, &records, &error), qPrintable(error));
        const QVector<hash_core::CompareRecord> expected = {
            {QStringLiteral("sha256"),
             QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
             QStringLiteral("empty.bin")}};
        const hash_core::CompareSummary summary = hash_core::compareRecords(expected, records);
        QCOMPARE(summary.matches, 0);
        QCOMPARE(summary.mismatches, 1);
        QCOMPARE(summary.missing, 0);
        QCOMPARE(summary.unexpected, 0);
        QVERIFY(!summary.exact);
    }

    void parseDigestStripsPrefixInsideTsvDigestField() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(QStringLiteral("sha256\tSHA-256:ABC123\tdata.bin\n"),
                                            &records, &error),
                 qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).digest, QStringLiteral("abc123"));
        const QVector<hash_core::CompareRecord> expected = {
            {QStringLiteral("sha256"), QStringLiteral("abc123"), QStringLiteral("data.bin")}};
        QVERIFY(hash_core::compareRecords(expected, records).exact);
    }

    void parseLooseDigestRejectsNonHexTokens() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY(
            !hash_core::parseDigestText(QStringLiteral("sha256:not-a-digest"), &records, &error));
        QVERIFY(!hash_core::parseDigestText(QStringLiteral("abc"), &records, &error));
    }

    void comparePathCaseFoldingMatchesPlatform() {
        using hash_core::CompareRecord;
        const QVector<CompareRecord> expected = {
            {QStringLiteral("sha256"), QStringLiteral("abc"), QStringLiteral("Dir/File.bin")}};
        const QVector<CompareRecord> actual = {
            {QStringLiteral("sha256"), QStringLiteral("abc"), QStringLiteral("dir/file.bin")}};
        const hash_core::CompareSummary summary = hash_core::compareRecords(expected, actual);
#ifdef Q_OS_WIN
        QCOMPARE(summary.matches, 1);
        QVERIFY(summary.exact);
#else
        QCOMPARE(summary.matches, 0);
#endif
    }

    void parseEmptyClipboardIsRejected() {
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY(!hash_core::parseDigestText(QStringLiteral("   \n"), &records, &error));
        QCOMPARE(error, QStringLiteral("clipboard is empty"));
    }
};
