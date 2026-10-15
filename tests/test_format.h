#pragma once

#include "cli/format.h"
#include "cli/options.h"
#include "cli/paths.h"
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
        QCOMPARE(lines.at(2), QStringLiteral("ERROR\t\tmissing\tfile does not exist\t-\t-"));
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

    void jsonEscapesControlCharactersAsFourHexDigits() {
        hash_core::FileResult failed;
        failed.request.path = QString::fromLatin1("a\x01"
                                                  "b\x1f"
                                                  "c.bin");
        failed.error = QStringLiteral("boom");
        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {failed}, {},
                     hash_core::FormatOptions{QStringLiteral("json"), false, false, false}, &output,
                     &error),
                 qPrintable(error));
        QVERIFY2(output.contains(QStringLiteral("\"path\": \"a\\u0001b\\u001fc.bin\"")),
                 qPrintable(output));
    }

    void unsupportedFormatIsRejected() {
        QString output;
        QString error;
        QVERIFY(!hash_core::writeResults(
            {}, {}, hash_core::FormatOptions{QStringLiteral("yaml"), false, false, false}, &output,
            &error));
        QCOMPARE(error, QStringLiteral("unsupported output format \"yaml\""));
    }

    void textEmitsWarningOnceForChangedFiles() {
        using namespace hash_core;
        FileResult changed;
        changed.request.path = QStringLiteral("a.bin");
        changed.result.path = QStringLiteral("a.bin");
        changed.result.order = {QStringLiteral("md5"), QStringLiteral("sha256")};
        changed.result.digests = {{QStringLiteral("md5"), QStringLiteral("aa")},
                                  {QStringLiteral("sha256"), QStringLiteral("bb")}};
        changed.result.changed = true;
        FileResult stable;
        stable.request.path = QStringLiteral("b.bin");
        stable.result.path = QStringLiteral("b.bin");
        stable.result.order = {QStringLiteral("sha256")};
        stable.result.digests = {{QStringLiteral("sha256"), QStringLiteral("cc")}};

        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {changed, changed, stable}, {QStringLiteral("md5"), QStringLiteral("sha256")},
                     hash_core::FormatOptions{QStringLiteral("text"), false, false, false}, &output,
                     &error),
                 qPrintable(error));
        // One warning per changed file, not per algorithm row.
        QCOMPARE(output.count(QStringLiteral("WARNING: file changed while hashing: \"a.bin\"\n")),
                 1);
        QVERIFY(!output.contains(QStringLiteral("WARNING: file changed while hashing: \"b.bin\"")));
        QVERIFY(output.contains(QStringLiteral("md5\taa\t")));
        QVERIFY(output.contains(QStringLiteral("sha256\tbb\t")));
        QVERIFY(output.contains(QStringLiteral("sha256\tcc\t\"b.bin\"")));
    }

    void jsonEmitsChangedFlagForChangedFiles() {
        hash_core::FileResult good;
        good.request.path = QStringLiteral("a.bin");
        good.result.path = QStringLiteral("a.bin");
        good.result.order = {QStringLiteral("sha256")};
        good.result.digests = {{QStringLiteral("sha256"), QStringLiteral("aa")}};
        good.result.changed = true;

        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {good}, {QStringLiteral("sha256")},
                     hash_core::FormatOptions{QStringLiteral("json"), false, false, false}, &output,
                     &error),
                 qPrintable(error));
        QCOMPARE(output, QStringLiteral("[\n"
                                        "  {\n"
                                        "    \"path\": \"a.bin\",\n"
                                        "    \"changed\": true,\n"
                                        "    \"digests\": {\n"
                                        "      \"sha256\": \"aa\"\n"
                                        "    }\n"
                                        "  }\n"
                                        "]\n"));
    }

    void jsonOmitsChangedFlagForStableFiles() {
        hash_core::FileResult good;
        good.request.path = QStringLiteral("a.bin");
        good.result.path = QStringLiteral("a.bin");
        good.result.order = {QStringLiteral("sha256")};
        good.result.digests = {{QStringLiteral("sha256"), QStringLiteral("aa")}};

        QString output;
        QString error;
        QVERIFY2(hash_core::writeResults(
                     {good}, {QStringLiteral("sha256")},
                     hash_core::FormatOptions{QStringLiteral("json"), false, false, false}, &output,
                     &error),
                 qPrintable(error));
        QVERIFY(!output.contains(QStringLiteral("changed")));
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
