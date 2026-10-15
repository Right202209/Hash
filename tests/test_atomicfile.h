#pragma once

#include "common/atomicfile.h"
#include "common/fspath.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include <filesystem>
#include <system_error>

class TestAtomicFile : public QObject {
    Q_OBJECT

  private slots:
    void rejectsHardLinkToInput() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString input = directory.path() + QStringLiteral("/input.bin");
        const QString output = directory.path() + QStringLiteral("/output.txt");
        const QByteArray content("original input");
        QFile file(input);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(content), qint64(content.size()));
        file.close();

        std::error_code code;
        std::filesystem::create_hard_link(hash_core::toFsPath(input), hash_core::toFsPath(output),
                                          code);
        if (code) {
            QSKIP("hard links unavailable");
        }
        QVERIFY(QFile::exists(output));
        const QString error =
            hash_core::writeAtomicFile(output, {input}, QByteArray("replacement"));
        QVERIFY2(!error.isEmpty(), "Write unexpectedly accepted hard link to input");

        QFile verify(input);
        QVERIFY(verify.open(QIODevice::ReadOnly));
        QCOMPARE(verify.readAll(), content);
    }

    void refusesSymbolicLinkDestination() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString target = directory.path() + QStringLiteral("/target.txt");
        QFile file(target);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("target");
        file.close();

        const QString link = directory.path() + QStringLiteral("/link.txt");
        std::error_code code;
        std::filesystem::create_symlink(hash_core::toFsPath(target), hash_core::toFsPath(link),
                                        code);
        if (code) {
            QSKIP("symbolic links unavailable");
        }
        const QString error = hash_core::writeAtomicFile(link, {target}, QByteArray("replacement"));
        QVERIFY2(!error.isEmpty(), "write through a symbolic link was accepted");
        QVERIFY(!error.contains(QStringLiteral("rename")));
    }

    void rejectsPathKeyCollision() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString input = directory.path() + QStringLiteral("/same.txt");
        const QString output = directory.path() + QStringLiteral("/same.txt");
        const QString error = hash_core::writeAtomicFile(output, {input}, QByteArray("x"));
        QVERIFY2(!error.isEmpty(), "output equal to an input path was accepted");
    }

    void writeReplacesDestination() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString input = directory.path() + QStringLiteral("/input.bin");
        const QString output = directory.path() + QStringLiteral("/output.txt");
        QFile file(input);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("input");
        file.close();
        const QString error = hash_core::writeAtomicFile(output, {input}, QByteArray("replaced"));
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QFile verify(output);
        QVERIFY(verify.open(QIODevice::ReadOnly));
        QCOMPARE(verify.readAll(), QByteArray("replaced"));
    }

    void reportsRealReasonWhenTemporaryFileCannotBeCreated() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        // A missing parent directory makes creating the temporary sibling
        // fail; the message must name the failing step and the real temporary
        // path rather than a generic failure (FR-ATOMIC-5).
        const QString output = directory.path() + QStringLiteral("/missing/sub/out.txt");
        const QString error = hash_core::writeAtomicFile(output, {}, QByteArray("x"));
        QVERIFY2(!error.isEmpty(), "a missing output directory was not reported");
        QVERIFY2(error.startsWith(QStringLiteral("create temporary output: open ")),
                 qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral(".hash-output-")), qPrintable(error));
    }

    void reportsRealReasonWhenRenameFails() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        // An existing directory as the destination lets the temporary file be
        // written but makes the final rename fail; the real reason survives
        // (FR-ATOMIC-5).
        const QString output = directory.path() + QStringLiteral("/occupied");
        QVERIFY(QDir().mkpath(output));
        const QString error = hash_core::writeAtomicFile(output, {}, QByteArray("x"));
        QVERIFY2(!error.isEmpty(), "renaming onto a directory was accepted");
        QVERIFY2(error.startsWith(QStringLiteral("rename temporary output: ")), qPrintable(error));
    }
};
