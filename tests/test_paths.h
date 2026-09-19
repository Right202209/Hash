#pragma once

#include "cli/paths.h"

#include <QTemporaryDir>
#include <QtTest>

#include <memory>

class TestPaths : public QObject {
    Q_OBJECT

  private slots:
    void globAndDeduplicates() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        for (const char *name : {"a.txt", "b.txt", "skip.bin"}) {
            QFile file(directory.path() + QStringLiteral("/") + name);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            file.write(name);
        }
        QStringList paths;
        QString error;
        QVERIFY2(hash_core::expandPaths(QStringList{directory.path() + QStringLiteral("/*.txt"),
                                                    directory.path() + QStringLiteral("/a.txt")},
                                        {}, &paths, &error),
                 qPrintable(error));
        QCOMPARE(paths.size(), 2);
        QVERIFY(paths.at(0).endsWith(QStringLiteral("a.txt")));
        QVERIFY(paths.at(1).endsWith(QStringLiteral("b.txt")));
    }

    void noInputReturnsSentinel() {
        QStringList paths;
        QString error;
        QVERIFY(!hash_core::expandPaths({}, {}, &paths, &error));
        QCOMPARE(error, QStringLiteral("at least one file path or pattern is required"));
    }

    void unmatchedPatternIsRejected() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QStringList paths;
        QString error;
        QVERIFY(!hash_core::expandPaths({directory.path() + QStringLiteral("/*.nothing")}, {},
                                        &paths, &error));
        QVERIFY2(error.contains(QStringLiteral("matched no files")), qPrintable(error));
    }

    void directoryRequiresRecursive() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QStringList paths;
        QString error;
        QVERIFY(!hash_core::expandPaths({directory.path()}, {}, &paths, &error));
        QVERIFY2(error.contains(QStringLiteral("--recursive")), qPrintable(error));
    }

    void recursiveWalksSortedRegularFiles() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QDir dir(directory.path());
        QVERIFY(dir.mkpath(QStringLiteral("sub")));
        for (const char *name : {"z.txt", "a.txt"}) {
            QFile file(directory.path() + QStringLiteral("/") + name);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            file.write(name);
        }
        {
            QFile file(directory.path() + QStringLiteral("/sub/m.txt"));
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            file.write("m");
        }
        QStringList paths;
        QString error;
        QVERIFY2(hash_core::expandPaths({directory.path()}, {true, false}, &paths, &error),
                 qPrintable(error));
        QCOMPARE(paths.size(), 3);
        QVERIFY(paths.at(0).endsWith(QStringLiteral("a.txt")));
        QVERIFY(paths.at(1).endsWith(QStringLiteral("sub/m.txt")));
        QVERIFY(paths.at(2).endsWith(QStringLiteral("z.txt")));
    }

    void globMatchBasics() {
        bool badPattern = false;
        QVERIFY(hash_core::globMatch(QStringLiteral("*"), QStringLiteral("a.txt"), &badPattern));
        QVERIFY(
            hash_core::globMatch(QStringLiteral("*.txt"), QStringLiteral("a.txt"), &badPattern));
        QVERIFY(
            !hash_core::globMatch(QStringLiteral("*.txt"), QStringLiteral("a.bin"), &badPattern));
        QVERIFY(hash_core::globMatch(QStringLiteral("a?c"), QStringLiteral("abc"), &badPattern));
        QVERIFY(
            hash_core::globMatch(QStringLiteral("[a-c]*"), QStringLiteral("b.txt"), &badPattern));
        QVERIFY(
            !hash_core::globMatch(QStringLiteral("[^a-c]*"), QStringLiteral("b.txt"), &badPattern));
        QVERIFY(
            hash_core::globMatch(QStringLiteral("[^a-c]*"), QStringLiteral("d.txt"), &badPattern));
        // A star does not cross separators.
        QVERIFY(
            !hash_core::globMatch(QStringLiteral("/*"), QStringLiteral("dir/file"), &badPattern));
        QVERIFY(hash_core::globMatch(QStringLiteral("/*"), QStringLiteral("/file"), &badPattern));
        QVERIFY(!hash_core::globMatch(QStringLiteral("["), QStringLiteral("x"), &badPattern));
        QVERIFY(badPattern);
    }
};
