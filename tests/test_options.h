#pragma once

#include "cli/options.h"
#include "core/registry.h"

#include <QtTest>

class TestOptions : public QObject {
    Q_OBJECT

  private:
    static bool parse(const QStringList &args, hash_core::Config *config, QString *error) {
        return hash_core::parseArgs(args, config, error);
    }

  private slots:
    void exitCodesAreStable() {
        QCOMPARE(hash_core::ExitSuccess, 0);
        QCOMPARE(hash_core::ExitFailure, 1);
        QCOMPARE(hash_core::ExitUsage, 2);
        QCOMPARE(hash_core::ExitCanceled, 3);
    }

    void defaultsToSha256Text() {
        hash_core::Config config;
        QString error;
        QVERIFY(parse({QStringLiteral("a.txt")}, &config, &error));
        QCOMPARE(config.algorithms, QStringList({QStringLiteral("sha256")}));
        QCOMPARE(config.format, QStringLiteral("text"));
        QCOMPARE(config.paths, QStringList({QStringLiteral("a.txt")}));
        QVERIFY(!config.showVersion);
    }

    void acceptsSingleAndDoubleDashForms() {
        hash_core::Config config;
        QString error;
        QVERIFY(parse({QStringLiteral("-a"), QStringLiteral(" SHA256 , MD5"),
                       QStringLiteral("--format=json"), QStringLiteral("--workers"),
                       QStringLiteral("4"), QStringLiteral("-o=out.txt"),
                       QStringLiteral("file")},
                      &config, &error));
        QCOMPARE(config.algorithms, QStringList({QStringLiteral("sha256"), QStringLiteral("md5")}));
        QCOMPARE(config.format, QStringLiteral("json"));
        QCOMPARE(config.workers, 4);
        QCOMPARE(config.output, QStringLiteral("out.txt"));
        QCOMPARE(config.paths, QStringList({QStringLiteral("file")}));
    }

    void parsesBooleanFlags() {
        hash_core::Config config;
        QString error;
        QVERIFY(parse({QStringLiteral("-size"), QStringLiteral("-utc=true"),
                       QStringLiteral("-recursive=0"), QStringLiteral("--fail-fast")},
                      &config, &error));
        QVERIFY(config.showSize);
        QVERIFY(config.utc);
        QVERIFY(!config.recursive);
        QVERIFY(config.failFast);

        hash_core::Config bad;
        QVERIFY(!parse({QStringLiteral("-size=maybe")}, &bad, &error));
        QCOMPARE(error, QStringLiteral("invalid boolean value \"maybe\" for flag -size"));
    }

    void stopsAtDoubleDashAndFirstPositional() {
        hash_core::Config config;
        QString error;
        QVERIFY(parse({QStringLiteral("--"), QStringLiteral("-size")}, &config, &error));
        QVERIFY(!config.showSize);
        QCOMPARE(config.paths, QStringList({QStringLiteral("-size")}));

        hash_core::Config positional;
        QVERIFY(parse({QStringLiteral("a"), QStringLiteral("-size")}, &positional, &error));
        QVERIFY(!positional.showSize);
        QCOMPARE(positional.paths, QStringList({QStringLiteral("a"), QStringLiteral("-size")}));
    }

    void reportsUsageErrors() {
        QString error;
        hash_core::Config missing;
        QVERIFY(!parse({QStringLiteral("-a")}, &missing, &error));
        QCOMPARE(error, QStringLiteral("flag needs an argument: -a"));

        hash_core::Config unknown;
        QVERIFY(!parse({QStringLiteral("-bogus")}, &unknown, &error));
        QCOMPARE(error, QStringLiteral("flag provided but not defined: -bogus"));

        hash_core::Config help;
        QVERIFY(!parse({QStringLiteral("-h")}, &help, &error));
        QCOMPARE(error, QStringLiteral("flag: help requested"));

        hash_core::Config negative;
        QVERIFY(!parse({QStringLiteral("-workers=-1")}, &negative, &error));
        QCOMPARE(error, QStringLiteral("workers cannot be negative"));

        hash_core::Config notNumber;
        QVERIFY(!parse({QStringLiteral("-workers=x")}, &notNumber, &error));
        QVERIFY(error.startsWith(QStringLiteral("invalid value")));

        hash_core::Config format;
        QVERIFY(!parse({QStringLiteral("-format=xml")}, &format, &error));
        QVERIFY(error.startsWith(QString::fromLatin1(hash_core::kUnsupportedFormatMessage)));
    }

    void formatIsCaseInsensitive() {
        hash_core::Config config;
        QString error;
        QVERIFY(parse({QStringLiteral("-format=TSV")}, &config, &error));
    }

    void versionFlagIsRecognized() {
        hash_core::Config config;
        QString error;
        QVERIFY(parse({QStringLiteral("-version")}, &config, &error));
        QVERIFY(config.showVersion);
        QVERIFY(hash_core::usageText().contains(QStringLiteral("-version")));
        QVERIFY(!QStringLiteral(HASH_VERSION).isEmpty());
    }

    void selectsAllOrValidatedAlgorithms() {
        QString error;
        QStringList selected;
        hash_core::Config all;
        all.all = true;
        QVERIFY(hash_core::selectAlgorithms(all, &selected, &error));
        QCOMPARE(selected.size(), hash_core::algorithms().size());

        hash_core::Config unknown;
        unknown.algorithms = {QStringLiteral("no-such-algorithm")};
        QVERIFY(!hash_core::selectAlgorithms(unknown, &selected, &error));
        QVERIFY(!error.isEmpty());
    }
};
