#pragma once

#include "common/quoting.h"
#include "tools/hashcontroller.h"
#include "tools/hashfilemodel.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

class TestHashController : public QObject {
    Q_OBJECT

  private slots:
    void marksEveryStartedFileAsHashing() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QList<QUrl> urls;
        for (int index = 0; index < 6; ++index) {
            const QByteArray name = QByteArray("file") + QByteArray::number(index) + ".txt";
            urls.append(QUrl::fromLocalFile(writeFile(dir, name.constData(), "payload")));
        }

        hash_core::HashController controller;
        QCOMPARE(controller.addFiles(urls).value("added").toInt(), 6);
        auto *model = qobject_cast<hash_core::HashFileModel *>(controller.files());
        QVERIFY(model);

        QSet<int> seenHashing;
        connect(model, &QAbstractItemModel::dataChanged, this,
                [model, &seenHashing](const QModelIndex &first, const QModelIndex &last) {
                    for (int row = first.row(); row <= last.row(); ++row) {
                        const int status =
                            model->index(row).data(hash_core::HashFileModel::StatusRole).toInt();
                        if (status == int(hash_core::FileStatus::Hashing)) {
                            seenHashing.insert(row);
                        }
                    }
                });

        controller.start({QStringLiteral("sha256")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);
        QCOMPARE(seenHashing.size(), 6);
    }

    void marksEachFileDoneBeforeTheRunEnds() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QList<QUrl> urls;
        for (int index = 0; index < 4; ++index) {
            const QByteArray name = QByteArray("done") + QByteArray::number(index) + ".txt";
            urls.append(QUrl::fromLocalFile(writeFile(dir, name.constData(), "payload")));
        }

        hash_core::HashController controller;
        QCOMPARE(controller.addFiles(urls).value("added").toInt(), 4);
        auto *model = qobject_cast<hash_core::HashFileModel *>(controller.files());
        QVERIFY(model);

        // markStatus reports only StatusRole; the end-of-run applyResult reports
        // several roles. A status-only Done update means the row finished early.
        QSet<int> doneEarly;
        connect(model, &QAbstractItemModel::dataChanged, this,
                [model, &doneEarly](const QModelIndex &first, const QModelIndex &last,
                                    const QList<int> &roles) {
                    if (roles.size() != 1 ||
                        roles.first() != hash_core::HashFileModel::StatusRole) {
                        return;
                    }
                    for (int row = first.row(); row <= last.row(); ++row) {
                        const int status =
                            model->index(row).data(hash_core::HashFileModel::StatusRole).toInt();
                        if (status == int(hash_core::FileStatus::Done)) {
                            doneEarly.insert(row);
                        }
                    }
                });

        controller.start({QStringLiteral("sha256")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);
        QCOMPARE(doneEarly.size(), 4);
    }

    void hashesQueuedFilesAndSavesOutput() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString first = writeFile(dir, "first.txt", "abc");
        const QString second = writeFile(dir, "second.bin", "hello");

        hash_core::HashController controller;
        const QVariantMap added =
            controller.addFiles({QUrl::fromLocalFile(first), QUrl::fromLocalFile(second)});
        QCOMPARE(added.value("added").toInt(), 2);
        QCOMPARE(added.value("rejected").toInt(), 0);

        controller.start({QStringLiteral("sha256")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);

        auto *model = qobject_cast<hash_core::HashFileModel *>(controller.files());
        QVERIFY(model);
        QCOMPARE(model->rowCount(), 2);
        const QModelIndex firstIndex = model->index(0, 0);
        QCOMPARE(firstIndex.data(hash_core::HashFileModel::StatusRole).toInt(),
                 int(hash_core::FileStatus::Done));
        const QVariantList digests =
            firstIndex.data(hash_core::HashFileModel::DigestsRole).toList();
        QCOMPARE(digests.size(), 1);
        QCOMPARE(
            digests.at(0).toMap().value("value").toString(),
            QString::fromLatin1(
                QCryptographicHash::hash(QByteArray("abc"), QCryptographicHash::Sha256).toHex()));
        QVERIFY(!controller.outputText().isEmpty());
        QVERIFY(controller.hasOutput());

        const QString target = dir.filePath("out.txt");
        QCOMPARE(controller.saveOutput(QUrl::fromLocalFile(target)), QString());
        QVERIFY(QFile::exists(target));

        // Queueing the same files again changes nothing.
        const QVariantMap repeat =
            controller.addFiles({QUrl::fromLocalFile(first), QUrl::fromLocalFile(second)});
        QCOMPARE(repeat.value("added").toInt(), 0);
    }

    void outputTextIsCliTextFormat() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // The GUI listing must be byte-for-byte what `hash -size -modified`
        // prints, so it round-trips through the clipboard and parses back
        // (FR-X-3, FR-FHP-9). saveOutput normalizes CRLF, so compare the
        // newline-normalized form.
        const QString path = writeFile(dir, "roundtrip.bin", "hello");
        const QString digest = QString::fromLatin1(
            QCryptographicHash::hash(QByteArray("hello"), QCryptographicHash::Sha256).toHex());

        hash_core::HashController controller;
        QCOMPARE(controller.addFiles({QUrl::fromLocalFile(path)}).value("added").toInt(), 1);
        controller.start({QStringLiteral("sha256")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);

        const QString output =
            QString(controller.outputText()).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
        const QString expected = QStringLiteral("sha256\t%1\t%2\tsize=5\tmodified=%3\n")
                                     .arg(digest, hash_core::goQuote(path),
                                          QFileInfo(path).lastModified().toString(Qt::ISODate));
        QCOMPARE(output, expected);

        // saveOutput writes that same listing, not a separately formatted one.
        const QString target = dir.filePath("gui.txt");
        QCOMPARE(controller.saveOutput(QUrl::fromLocalFile(target)), QString());
        QFile saved(target);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(saved.readAll()), output);
        saved.close();

        // The exported listing feeds the compare pipeline unchanged.
        QVector<hash_core::CompareRecord> records;
        QString error;
        QVERIFY2(hash_core::parseDigestText(output, &records, &error), qPrintable(error));
        QCOMPARE(records.size(), 1);
        QCOMPARE(records.at(0).digest, digest);
    }

    void exportsCliTextForMultipleAlgorithms() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = writeFile(dir, "multi.bin", "abc");

        hash_core::HashController controller;
        QCOMPARE(controller.addFiles({QUrl::fromLocalFile(path)}).value("added").toInt(), 1);
        // Request order is preserved in the listing, matching the CLI which
        // writes one line per algorithm in request order (FR-FMT-1).
        controller.start({QStringLiteral("sha256"), QStringLiteral("md5")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);

        const QStringList lines = controller.outputText().trimmed().split(u'\n');
        QCOMPARE(lines.size(), 2);
        QVERIFY(lines.at(0).startsWith(QStringLiteral("sha256\t")));
        QVERIFY(lines.at(1).startsWith(QStringLiteral("md5\t")));
        QCOMPARE(
            lines.at(0).split(u'\t').at(1),
            QString::fromLatin1(
                QCryptographicHash::hash(QByteArray("abc"), QCryptographicHash::Sha256).toHex()));
        QCOMPARE(
            lines.at(1).split(u'\t').at(1),
            QString::fromLatin1(
                QCryptographicHash::hash(QByteArray("abc"), QCryptographicHash::Md5).toHex()));
    }

    void rejectDirectoriesAndClear() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        hash_core::HashController controller;
        const QVariantMap added = controller.addFiles({QUrl::fromLocalFile(dir.path())});
        QCOMPARE(added.value("added").toInt(), 0);
        QCOMPARE(added.value("rejected").toInt(), 1);

        const QString file = writeFile(dir, "file.txt", "x");
        QCOMPARE(controller.addFiles({QUrl::fromLocalFile(file)}).value("added").toInt(), 1);
        controller.clearFiles();
        auto *model = qobject_cast<hash_core::HashFileModel *>(controller.files());
        QCOMPARE(model->rowCount(), 0);
        QVERIFY(!controller.hasOutput());
    }

    void compareClipboardCountsMismatches() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = writeFile(dir, "file.txt", "abc");

        hash_core::HashController controller;
        QCOMPARE(controller.addFiles({QUrl::fromLocalFile(file)}).value("added").toInt(), 1);
        controller.start({QStringLiteral("md5")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);

        auto *model = qobject_cast<hash_core::HashFileModel *>(controller.files());
        const QVariantList digests =
            model->index(0, 0).data(hash_core::HashFileModel::DigestsRole).toList();
        const QString digest = digests.at(0).toMap().value("value").toString();

        controller.compareClipboard(
            QStringLiteral("md5\t%1\t%2").arg(digest, hash_core::goQuote(file)));
        QVERIFY(controller.compare().value("valid").toBool());
        QCOMPARE(controller.compare().value("matches").toInt(), 1);
        QCOMPARE(controller.compare().value("exact").toBool(), true);

        controller.compareClipboard(
            QStringLiteral("md5\t%1\t%2")
                .arg(QStringLiteral("d41d8cd98f00b204e9800998ecf8427e"), hash_core::goQuote(file)));
        QVERIFY(controller.compare().value("valid").toBool());
        QCOMPARE(controller.compare().value("mismatches").toInt(), 1);
        QCOMPARE(controller.compare().value("exact").toBool(), false);
    }

    void compareClipboardAcceptsPrefixedDigest() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = writeFile(dir, "file.txt", "abc");

        hash_core::HashController controller;
        QCOMPARE(controller.addFiles({QUrl::fromLocalFile(file)}).value("added").toInt(), 1);
        controller.start({QStringLiteral("sha256")});
        QTRY_COMPARE_WITH_TIMEOUT(controller.busy(), false, 30000);

        const QString digest = QString::fromLatin1(
            QCryptographicHash::hash(QByteArray("abc"), QCryptographicHash::Sha256).toHex());
        // A digest pasted with an "sha256:" tag (full-width colon and
        // surrounding whitespace included) verifies without any manual edit.
        controller.compareClipboard(QStringLiteral("  SHA256：%1\n").arg(digest));
        QVERIFY(controller.compare().value("valid").toBool());
        QCOMPARE(controller.compare().value("matches").toInt(), 1);
        QCOMPARE(controller.compare().value("exact").toBool(), true);
    }

    void compareWithoutResultsReportsError() {
        hash_core::HashController controller;
        controller.compareClipboard(QStringLiteral("md5\td41d8cd98f00b204e9800998ecf8427e\tx"));
        QCOMPARE(controller.compare().value("valid").toBool(), false);
        QVERIFY(!controller.compare().value("error").toString().isEmpty());
        controller.resetCompare();
        QCOMPARE(controller.compare().value("valid").toBool(), false);
        QCOMPARE(controller.compare().value("error").toString(), QString());
    }

  private:
    static QString writeFile(const QTemporaryDir &dir, const QString &name,
                             const QByteArray &content) {
        const QString path = dir.filePath(name);
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.write(content);
        file.close();
        return path;
    }
};
