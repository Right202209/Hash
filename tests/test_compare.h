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
