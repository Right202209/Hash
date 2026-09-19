#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// TimestampTool converts between Unix timestamps and human-readable dates.
// Display strings come back in both the local zone and UTC so results do not
// depend on where the tool runs.
class TimestampTool : public QObject {
    Q_OBJECT

  public:
    explicit TimestampTool(QObject *parent = nullptr);

    // timestampToDate returns {ok, seconds, milliseconds, local, utc, iso}.
    // value is seconds unless milliseconds is true.
    Q_INVOKABLE QVariantMap timestampToDate(qint64 value, bool milliseconds);
    // dateToTimestamp parses ISO 8601, "yyyy-MM-dd HH:mm:ss",
    // "yyyy/MM/dd HH:mm:ss" or a bare date and returns {ok, seconds,
    // milliseconds}.
    Q_INVOKABLE QVariantMap dateToTimestamp(const QString &text);
    // now returns the same shape as timestampToDate for the current moment.
    Q_INVOKABLE QVariantMap now();
};

} // namespace hash_core
