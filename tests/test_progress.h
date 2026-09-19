#pragma once

#include "core/progress.h"

#include <QtTest>

class TestProgress : public QObject {
    Q_OBJECT

  private slots:
    void handlesZeroByteFiles() {
        hash_core::Progress progress;
        progress.totalFiles = 3;
        QCOMPARE(hash_core::progressPercent(progress), 0);
        progress.completedFiles = 3;
        QCOMPARE(hash_core::progressPercent(progress), 100);
    }

    void computesByteProgress() {
        hash_core::Progress progress;
        progress.totalBytes = 100;
        progress.completedBytes = 60;
        progress.currentBytes = 20;
        QCOMPARE(hash_core::progressPercent(progress), 80);
    }

    void clampsAboveTotal() {
        hash_core::Progress progress;
        progress.totalBytes = 100;
        progress.completedBytes = 120;
        QCOMPARE(hash_core::progressPercent(progress), 100);
    }

    void handlesLargeByteCounts() {
        hash_core::Progress progress;
        const qint64 total = (qint64(1) << 62) + 1;
        progress.totalBytes = total;
        progress.completedBytes = qint64(1) << 62;
        QCOMPARE(hash_core::progressPercent(progress), 99);
    }

    void mulDiv64MatchesReferenceDivision() {
        QCOMPARE(hash_core::mulDiv64(0, 100, 7), quint64(0));
        QCOMPARE(hash_core::mulDiv64(6, 100, 7), quint64(85));
        QCOMPARE(hash_core::mulDiv64(7, 100, 7), quint64(100));
        QCOMPARE(hash_core::mulDiv64(quint64(1) << 62, 100, (quint64(1) << 62) + 1), quint64(99));
        // 2^63 * 3 / 2^63 = 3 exercises the 128-bit path.
        QCOMPARE(hash_core::mulDiv64(quint64(1) << 63, 3, quint64(1) << 63), quint64(3));
    }
};
