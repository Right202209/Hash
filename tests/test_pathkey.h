#pragma once

#include "common/pathkey.h"

#include <QtTest>

class TestPathKey : public QObject {
    Q_OBJECT

  private slots:
    void cleansAndFoldsCaseOnlyOnWindows() {
        QCOMPARE(hash_core::pathKey(QStringLiteral("a/../b")), QStringLiteral("b"));
#ifdef Q_OS_WIN
        // Qt-normalized paths keep the forward-slash separator; Windows keys
        // fold case.
        QCOMPARE(hash_core::pathKey(QStringLiteral("Dir/File.bin")),
                 QStringLiteral("dir/file.bin"));
#else
        QCOMPARE(hash_core::pathKey(QStringLiteral("Dir/File.bin")),
                 QStringLiteral("Dir/File.bin"));
#endif
    }

    void trailingDotAndSlashAreCleaned() {
        QCOMPARE(hash_core::pathKey(QStringLiteral("a/b/")), QStringLiteral("a/b"));
        QCOMPARE(hash_core::pathKey(QStringLiteral("./a")), QStringLiteral("a"));
    }
};
