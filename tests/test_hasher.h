#pragma once

#include "core/hasher.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

class TestHasher : public QObject {
    Q_OBJECT

    QString writeFile(const QByteArray &content) {
        const QString path = mDirectory.path() + QStringLiteral("/input.bin");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        if (!content.isEmpty()) {
            QCOMPARE(file.write(content), qint64(content.size()));
        }
        file.close();
        return path;
    }

    QString tempPath(const QString &name) { return mDirectory.path() + QStringLiteral("/") + name; }

    QTemporaryDir mDirectory;

  private slots:
    void initTestCase() { QVERIFY(mDirectory.isValid()); }

    void oneReadComputesEveryRequestedAlgorithm() {
        const QByteArray content =
            QString::fromUtf8(
                "one input stream, several independent digest states: 中文 \xF0\x9F\x98\x80 ' OR "
                "1=1 --\n")
                .toUtf8();
        const QString path = writeFile(content);
        const QStringList requested = {QStringLiteral("sha256"), QStringLiteral("md5"),
                                       QStringLiteral("sha512"), QStringLiteral("sha1")};

        const hash_core::HashOutcome outcome = hash_core::hashFile(path, requested);
        QVERIFY2(outcome.error.isEmpty(), qPrintable(outcome.error));
        QCOMPARE(outcome.result.bytesRead, qint64(content.size()));

        const QHash<QString, const char *> want = {
            {QStringLiteral("md5"), "ed5e0e57a9802d6d92cb1ecd8f2fec5e"},
            {QStringLiteral("sha1"), "b685eba653cc8e3f15520caa9211d6058c10325c"},
            {QStringLiteral("sha256"),
             "100df892b3bd9e2d7e992c9253523f6ec916bd89fd93923a649014ebaca72887"},
            {QStringLiteral("sha512"),
             "18c2d06e0c5487bc9aa10eb37706a4af07c0e7e68e6d45a42e85075cc4f463dac1cffe6f73c29f"
             "f6c4dead53c1c32d6744c1b9849fa3cb0cf1d8b39911a9e5c6"},
        };
        for (auto it = want.constBegin(); it != want.constEnd(); ++it) {
            QCOMPARE(outcome.result.digestFor(it.key()), QString::fromLatin1(it.value()));
        }
    }

    void preservesRequestedResultOrder() {
        const QString path = writeFile(QByteArray("stable ordering"));
        const QStringList requested = {QStringLiteral("sha512"), QStringLiteral("md5"),
                                       QStringLiteral("sha256"), QStringLiteral("sha1")};
        const hash_core::HashOutcome outcome = hash_core::hashFile(path, requested);
        QVERIFY2(outcome.error.isEmpty(), qPrintable(outcome.error));
        QCOMPARE(outcome.result.order, requested);
        QCOMPARE(outcome.result.digests.size(), requested.size());
        for (int index = 0; index < requested.size(); ++index) {
            QCOMPARE(outcome.result.digests.at(index).algorithm, requested.at(index));
        }
    }

    void largeInputAndBufferBoundaries() {
        const int size = 10 * 1024 * 1024 + 17;
        const QByteArray content(size, '\xA5');
        const QString path = writeFile(content);
        const QHash<QString, const char *> want = {
            {QStringLiteral("sha256"),
             "58fb8b6d29dcf43801ac082d8e8bbe3617255697a9606b7039a66709f00ac0fe"},
            {QStringLiteral("sha512"),
             "ed3d52ace6fbea34df859337fa1f6c31cb09e39ca522c794b9c15d22e45e6f604224e989ede63"
             "a9bddbcd89cf24ff9f9431b1d6bb38415a18e7691dcef59c616"},
        };
        const QList<int> bufferSizes = {1, 2, 4095, 4096, 4097, 64 * 1024};
        for (int bufferSize : bufferSizes) {
            hash_core::HashOptions options;
            options.bufferSize = bufferSize;
            const hash_core::HashOutcome outcome = hash_core::hashFileWithOptions(
                path, {QStringLiteral("sha256"), QStringLiteral("sha512")}, options);
            QVERIFY2(
                outcome.error.isEmpty(),
                qPrintable(QStringLiteral("buffer %1: %2").arg(bufferSize).arg(outcome.error)));
            QCOMPARE(outcome.result.bytesRead, qint64(size));
            for (auto it = want.constBegin(); it != want.constEnd(); ++it) {
                QCOMPARE(outcome.result.digestFor(it.key()), QString::fromLatin1(it.value()));
            }
        }
    }

    void cancellationStopsWork() {
        const QString path = writeFile(QByteArray(9 * 1024 * 1024, 'x'));
        hash_core::HashOptions options;
        options.canceled = []() { return true; };
        const hash_core::HashOutcome outcome =
            hash_core::hashFileWithOptions(path, {QStringLiteral("sha256")}, options);
        QVERIFY(outcome.canceled);
        QCOMPARE(outcome.error, QStringLiteral("context canceled"));
    }

    void missingFileReturnsPathError() {
        const hash_core::HashOutcome outcome = hash_core::hashFile(
            tempPath(QStringLiteral("does-not-exist.bin")), {QStringLiteral("sha256")});
        QVERIFY(!outcome.error.isEmpty());
        QVERIFY2(outcome.error.startsWith(QStringLiteral("open ")), qPrintable(outcome.error));
    }

    void invalidRequestIsRejected() {
        const QString path = writeFile(QByteArray("input"));
        const QVector<QStringList> invalid = {
            {},
            {QString()},
            {QStringLiteral("sha256"), QStringLiteral("not-registered")},
            {QStringLiteral("sha256"), QStringLiteral("sha256")},
            {QString()},
        };
        for (const QStringList &requested : invalid) {
            const hash_core::HashOutcome outcome = hash_core::hashFile(path, requested);
            QVERIFY2(
                !outcome.error.isEmpty(),
                qPrintable(QStringLiteral("request %1 should fail").arg(requested.join(u','))));
        }
    }

    void emptyFile() {
        const QString path = writeFile(QByteArray());
        const hash_core::HashOutcome outcome =
            hash_core::hashFile(path, {QStringLiteral("sha256")});
        QVERIFY2(outcome.error.isEmpty(), qPrintable(outcome.error));
        QCOMPARE(outcome.result.bytesRead, qint64(0));
        QCOMPARE(
            outcome.result.digestFor(QStringLiteral("sha256")),
            QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    }

    void validateAlgorithmsReportsSentinelErrors() {
        QString message;
        QCOMPARE(hash_core::validateAlgorithms({}, &message),
                 hash_core::AlgorithmError::NoAlgorithms);
        QCOMPARE(message, QStringLiteral("at least one algorithm is required"));
        QCOMPARE(hash_core::validateAlgorithms({QString()}, &message),
                 hash_core::AlgorithmError::EmptyAlgorithmName);
        QCOMPARE(message, QStringLiteral("algorithm name cannot be empty"));
        QCOMPARE(hash_core::validateAlgorithms({QStringLiteral("sha3-256")}, &message),
                 hash_core::AlgorithmError::UnknownAlgorithm);
        QCOMPARE(message, QStringLiteral("unknown algorithm \"sha3-256\""));
        QCOMPARE(hash_core::validateAlgorithms({QStringLiteral("sha256"), QStringLiteral("sha256")},
                                               &message),
                 hash_core::AlgorithmError::DuplicateAlgorithm);
        QCOMPARE(message, QStringLiteral("duplicate algorithm \"sha256\""));
        QCOMPARE(hash_core::validateAlgorithms({QStringLiteral("sha256"), QStringLiteral("md5")},
                                               &message),
                 hash_core::AlgorithmError::None);
        QVERIFY(message.isEmpty());
    }

    void changedDetectsModificationAfterHashing() {
        const QString path = writeFile(QByteArray("original"));
        const hash_core::HashOutcome first = hash_core::hashFile(path, {QStringLiteral("sha256")});
        QVERIFY2(first.error.isEmpty(), qPrintable(first.error));
        QVERIFY(!first.result.changed);

        // Rewrite the file, then hash again: the size differs, which the
        // changed detection must observe.
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write("original-with-more-bytes"), qint64(24));
        file.close();
        const hash_core::HashOutcome second = hash_core::hashFile(path, {QStringLiteral("sha256")});
        QVERIFY2(second.error.isEmpty(), qPrintable(second.error));
        QVERIFY(second.result.changed);
    }
};
