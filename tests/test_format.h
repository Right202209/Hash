#pragma once

#include "cli/format.h"
#include "cli/options.h"
#include "core/types.h"

#include <QtTest>

class TestFormat : public QObject {
    Q_OBJECT

  private slots:
    void tsvHasStableColumnsAndEscapesFormula() {
        hash_core::FileResult good;
        good.request.path = QStringLiteral("=file.txt");
        good.result.path = QStringLiteral("=file.txt");
        good.result.order = {QStringLiteral("sha256")};
        good.result.digests = {{QStringLiteral("sha256"), QStringLiteral("abc")}};
        hash_core::FileResult failed;
        failed.request.path = QStringLiteral("missing");
        failed.error = QStringLiteral("file does not exist");

        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {good, failed}, {QStringLiteral("sha256")},
                     hash_core::FormatOptions{QStringLiteral("tsv"), true, true, true}, &output,
                     &error),
                 qPrintable(error));

        const QStringList lines = output.trimmed().split(u'\n');
        QCOMPARE(lines.size(), 3);
        for (const QString &line : lines) {
            QCOMPARE(line.split(u'\t').size(), 6);
        }
        QVERIFY2(output.contains(QStringLiteral("'=file.txt")),
                 "formula-like path was not escaped");
    }

    void tsvValueEscapesFormulaAfterLeadingWhitespace() {
        const QStringList values = {QStringLiteral("=1+1"), QStringLiteral(" =1+1"),
                                    QStringLiteral("\t@SUM(A:A)")};
        for (const QString &value : values) {
            const QString escaped = hash_core::tsvValue(value);
            QVERIFY2(
                !escaped.isEmpty() && escaped.at(0) == u'\'',
                qPrintable(QStringLiteral("tsvValue(%1) lacks leading apostrophe").arg(value)));
        }
    }

    void textFormatQuotesPathsAndErrors() {
        hash_core::FileResult failed;
        failed.request.path = QStringLiteral("missing.bin");
        failed.error = QStringLiteral("open \"missing.bin\": no such file");
        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {failed}, {},
                     hash_core::FormatOptions{QStringLiteral("text"), false, false, false}, &output,
                     &error),
                 qPrintable(error));
        QCOMPARE(
            output,
            QStringLiteral("ERROR\t\"missing.bin\"\t\"open \\\"missing.bin\\\": no such file\"\n"));
    }

    void jsonFormatEmitsOneObjectPerFile() {
        hash_core::FileResult good;
        good.request.path = QStringLiteral("a.bin");
        good.result.path = QStringLiteral("a.bin");
        good.result.order = {QStringLiteral("md5"), QStringLiteral("sha256")};
        good.result.digests = {{QStringLiteral("sha256"), QStringLiteral("bb")},
                               {QStringLiteral("md5"), QStringLiteral("aa")}};

        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {good}, {QStringLiteral("md5"), QStringLiteral("sha256")},
                     hash_core::FormatOptions{QStringLiteral("json"), true, false, true}, &output,
                     &error),
                 qPrintable(error));
        // Digest keys are sorted like encoding/json sorts map keys.
        QCOMPARE(output, QStringLiteral("[\n"
                                        "  {\n"
                                        "    \"path\": \"a.bin\",\n"
                                        "    \"size\": 0,\n"
                                        "    \"digests\": {\n"
                                        "      \"md5\": \"aa\",\n"
                                        "      \"sha256\": \"bb\"\n"
                                        "    }\n"
                                        "  }\n"
                                        "]\n"));
    }

    void unsupportedFormatIsRejected() {
        QString output;
        QString error;
        QVERIFY(!hash_core::writeResults(
            {}, {}, hash_core::FormatOptions{QStringLiteral("yaml"), false, false, false}, &output,
            &error));
        QCOMPARE(error, QStringLiteral("unsupported output format \"yaml\""));
    }

    void parseAndSelectAlgorithms() {
        hash_core::Config config;
        QString error;
        QVERIFY2(hash_core::parseArgs(
                     QStringList{QStringLiteral("--algorithm"), QStringLiteral("SHA256, md5"),
                                 QStringLiteral("--format"), QStringLiteral("json"),
                                 QStringLiteral("file.bin")},
                     &config, &error),
                 qPrintable(error));
        QStringList selected;
        QVERIFY2(hash_core::selectAlgorithms(config, &selected, &error), qPrintable(error));
        QCOMPARE(selected.join(u','), QStringLiteral("sha256,md5"));
    }

    void algorithmDefaultsToSha256() {
        hash_core::Config config;
        QString error;
        QVERIFY2(hash_core::parseArgs(QStringList{QStringLiteral("file.bin")}, &config, &error),
                 qPrintable(error));
        QStringList selected;
        QVERIFY2(hash_core::selectAlgorithms(config, &selected, &error), qPrintable(error));
        QCOMPARE(selected.join(u','), QStringLiteral("sha256"));
    }

    void noInputReturnsSentinelMessage() {
        QStringList paths;
        QString error;
        QVERIFY(!hash_core::expandPaths({}, {}, &paths, &error));
        QCOMPARE(error, QStringLiteral("at least one file path or pattern is required"));
    }

    void unsupportedFormatReturnsSentinelMessage() {
        hash_core::Config config;
        QString error;
        QVERIFY(
            !hash_core::parseArgs(QStringList{QStringLiteral("--format"), QStringLiteral("yaml"),
                                              QStringLiteral("file.bin")},
                                  &config, &error));
        QCOMPARE(error, QStringLiteral("unsupported output format \"yaml\""));
    }
};
