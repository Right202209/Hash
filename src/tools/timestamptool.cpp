#include "tools/timestamptool.h"

#include <QDateTime>

namespace hash_core {

TimestampTool::TimestampTool(QObject *parent) : QObject(parent) {}

QVariantMap TimestampTool::timestampToDate(qint64 value, bool milliseconds) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    const QDateTime moment =
        milliseconds ? QDateTime::fromMSecsSinceEpoch(value) : QDateTime::fromSecsSinceEpoch(value);
    if (!moment.isValid()) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("时间戳超出可表示范围"));
        return outcome;
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("seconds"), moment.toSecsSinceEpoch());
    outcome.insert(QStringLiteral("milliseconds"), moment.toMSecsSinceEpoch());
    outcome.insert(QStringLiteral("local"), moment.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    outcome.insert(QStringLiteral("utc"),
                   moment.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    outcome.insert(QStringLiteral("iso"), moment.toUTC().toString(Qt::ISODate));
    return outcome;
}

QVariantMap TimestampTool::dateToTimestamp(const QString &text) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    const QString trimmed = text.trimmed();
    QDateTime moment = QDateTime::fromString(trimmed, Qt::ISODate);
    if (!moment.isValid()) {
        moment = QDateTime::fromString(trimmed, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    if (!moment.isValid()) {
        moment = QDateTime::fromString(trimmed, QStringLiteral("yyyy/MM/dd HH:mm:ss"));
    }
    if (!moment.isValid()) {
        moment = QDateTime::fromString(trimmed, QStringLiteral("yyyy-MM-dd"));
    }
    if (!moment.isValid()) {
        outcome.insert(QStringLiteral("error"),
                       QStringLiteral("无法解析日期时间，支持 ISO 8601 或 yyyy-MM-dd HH:mm:ss"));
        return outcome;
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("seconds"), moment.toSecsSinceEpoch());
    outcome.insert(QStringLiteral("milliseconds"), moment.toMSecsSinceEpoch());
    return outcome;
}

QVariantMap TimestampTool::now() {
    return timestampToDate(QDateTime::currentSecsSinceEpoch(), false);
}

} // namespace hash_core
