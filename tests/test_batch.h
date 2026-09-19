#pragma once

#include "core/batch.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <mutex>

class TestBatch : public QObject {
    Q_OBJECT

    // QVERIFY/QCOMPARE may only run in void functions, so the fixture helper
    // reports failures through QTest::qFail and returns an empty path.
    QString writeFile(const QString &name, const QByteArray &content) {
        const QString path = mDirectory.path() + QStringLiteral("/") + name;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTest::qFail(qPrintable(QStringLiteral("cannot open %1").arg(path)), __FILE__,
                         __LINE__);
            return {};
        }
        if (!content.isEmpty() && file.write(content) != qint64(content.size())) {
            QTest::qFail(qPrintable(QStringLiteral("short write to %1").arg(path)), __FILE__,
                         __LINE__);
            return {};
        }
        return path;
    }

    QTemporaryDir mDirectory;

  private slots:
    void initTestCase() { QVERIFY(mDirectory.isValid()); }

    void preservesInputOrderAndReportsProgress() {
        const QString one = writeFile(QStringLiteral("one.bin"), QByteArray("\x00\x01\x02\x03", 4));
        const QString two = writeFile(QStringLiteral("two.bin"), QByteArray("\x01\x01\x02\x03", 4));
        QVector<hash_core::Progress> progress;
        std::mutex progressMutex;
        const QVector<hash_core::FileResult> results = hash_core::hashFiles(
            {
                hash_core::FileRequest{one, {QStringLiteral("sha256"), QStringLiteral("md5")}},
                hash_core::FileRequest{two, {QStringLiteral("sha256")}},
            },
            hash_core::BatchOptions{{},
                                    2,
                                    [&progress, &progressMutex](const hash_core::Progress &value) {
                                        std::lock_guard<std::mutex> lock(progressMutex);
                                        progress.append(value);
                                    },
                                    {},
                                    false});
        QCOMPARE(results.size(), 2);
        for (int index = 0; index < results.size(); ++index) {
            QVERIFY2(results.at(index).error.isEmpty(),
                     qPrintable(
                         QStringLiteral("result %1: %2").arg(index).arg(results.at(index).error)));
            QCOMPARE(results.at(index).request.path, index == 0 ? one : two);
        }
        QVERIFY(progress.size() >= 2);
    }

    void continuesAfterMissingFile() {
        const QString present = writeFile(QStringLiteral("present.txt"), QByteArray("present"));
        const QString missing = mDirectory.path() + QStringLiteral("/missing.txt");
        const QVector<hash_core::FileResult> results = hash_core::hashFiles(
            {
                hash_core::FileRequest{missing, {QStringLiteral("sha256")}},
                hash_core::FileRequest{present, {QStringLiteral("sha256")}},
            },
            hash_core::BatchOptions{{}, 1, {}, {}, false});
        QVERIFY(!results.at(0).error.isEmpty());
        QVERIFY2(results.at(1).error.isEmpty(), qPrintable(results.at(1).error));
    }

    void failFastCancelsRemainingWork() {
        const QString present = writeFile(QStringLiteral("kept.txt"), QByteArray("present"));
        const QString missing = mDirectory.path() + QStringLiteral("/missing.txt");
        const QVector<hash_core::FileResult> results = hash_core::hashFiles(
            {
                hash_core::FileRequest{missing, {QStringLiteral("sha256")}},
                hash_core::FileRequest{present, {QStringLiteral("sha256")}},
            },
            hash_core::BatchOptions{{}, 1, {}, {}, true});
        QVERIFY(!results.at(0).error.isEmpty());
        QVERIFY2(!results.at(1).error.isEmpty(),
                 "remaining file should be cancelled in fail-fast mode");
        QVERIFY(results.at(1).canceled);
    }

    void progressAccountsCompletedAndCurrentBytes() {
        const QByteArray firstContent(4096, '\x01');
        const QByteArray secondContent(8192, '\x02');
        const QString one = writeFile(QStringLiteral("one.bin"), firstContent);
        const QString two = writeFile(QStringLiteral("two.bin"), secondContent);
        const qint64 totalBytes = firstContent.size() + secondContent.size();

        QVector<hash_core::Progress> snapshots;
        std::mutex snapshotsMutex;
        const QVector<hash_core::FileResult> results = hash_core::hashFiles(
            {
                hash_core::FileRequest{one, {QStringLiteral("sha256")}},
                hash_core::FileRequest{two, {QStringLiteral("sha256")}},
            },
            hash_core::BatchOptions{
                {},
                1,
                [&snapshots, &snapshotsMutex](const hash_core::Progress &value) {
                    std::lock_guard<std::mutex> lock(snapshotsMutex);
                    snapshots.append(value);
                },
                {},
                false});
        for (const hash_core::FileResult &result : results) {
            QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        }

        QVERIFY(!snapshots.isEmpty());
        for (const hash_core::Progress &snapshot : snapshots) {
            QVERIFY2(snapshot.completedBytes <= totalBytes,
                     qPrintable(QStringLiteral("CompletedBytes %1 exceeds TotalBytes %2")
                                    .arg(snapshot.completedBytes)
                                    .arg(totalBytes)));
            QVERIFY2(snapshot.completedBytes + snapshot.currentBytes <= totalBytes,
                     "CompletedBytes+CurrentBytes must not exceed TotalBytes");
        }
        const hash_core::Progress last = snapshots.last();
        QCOMPARE(last.completedFiles, 2);
        QCOMPARE(last.completedBytes, totalBytes);
        QCOMPARE(last.currentBytes, qint64(0));

        bool sawFirstFileCompleted = false;
        for (const hash_core::Progress &snapshot : snapshots) {
            if (snapshot.completedFiles == 1 &&
                snapshot.completedBytes == qint64(firstContent.size()) &&
                snapshot.currentBytes == 0) {
                sawFirstFileCompleted = true;
                break;
            }
        }
        QVERIFY(sawFirstFileCompleted);
    }
};
