#pragma once

#include "tools/timestamptool.h"

#include <QDateTime>
#include <QtTest>

class TestTimestampTool : public QObject {
    Q_OBJECT

  private slots:
    void convertsKnownEpoch() {
        hash_core::TimestampTool tool;
        const QVariantMap outcome = tool.timestampToDate(946684800, false);
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("utc").toString(), QStringLiteral("2000-01-01 00:00:00"));
        QCOMPARE(outcome.value("iso").toString(), QStringLiteral("2000-01-01T00:00:00Z"));
        QCOMPARE(outcome.value("seconds").toLongLong(), qint64(946684800));
        QCOMPARE(outcome.value("milliseconds").toLongLong(), qint64(946684800000));
    }

    void convertsMilliseconds() {
        hash_core::TimestampTool tool;
        const QVariantMap outcome = tool.timestampToDate(946684800123, true);
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("milliseconds").toLongLong(), qint64(946684800123));
    }

    void parsesIsoUtcDate() {
        hash_core::TimestampTool tool;
        const QVariantMap outcome = tool.dateToTimestamp(QStringLiteral("2000-01-01T00:00:00Z"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("seconds").toLongLong(), qint64(946684800));
    }

    void parsesLocalDateTimeLikeTheSameMoment() {
        hash_core::TimestampTool tool;
        const QDateTime moment(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::LocalTime);
        const QVariantMap outcome = tool.dateToTimestamp(QStringLiteral("2000-01-01 00:00:00"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("seconds").toLongLong(), moment.toSecsSinceEpoch());
    }

    void rejectsUnparseableDate() {
        hash_core::TimestampTool tool;
        const QVariantMap outcome = tool.dateToTimestamp(QStringLiteral("not a date"));
        QCOMPARE(outcome.value("ok").toBool(), false);
        QVERIFY(!outcome.value("error").toString().isEmpty());
    }

    void nowRoundTrips() {
        hash_core::TimestampTool tool;
        const QVariantMap outcome = tool.now();
        QCOMPARE(outcome.value("ok").toBool(), true);
        QVERIFY(outcome.value("seconds").toLongLong() >= qint64(946684800));
    }
};
