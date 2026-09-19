#pragma once

#include "common/quoting.h"
#include "tools/hashcontroller.h"
#include "tools/hashfilemodel.h"

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

class TestHashController : public QObject {
    Q_OBJECT

  private slots:
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
