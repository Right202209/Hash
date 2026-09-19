#include "tools/uuidtool.h"

#include <QUuid>

namespace hash_core {

UuidTool::UuidTool(QObject *parent) : QObject(parent) {}

QVariantMap UuidTool::generate(int count, bool uppercase, bool braces, bool hyphens) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    if (count < 1 || count > 1000) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("数量需在 1-1000 之间"));
        return outcome;
    }
    QStringList values;
    values.reserve(count);
    for (int index = 0; index < count; ++index) {
        QString value =
            QUuid::createUuid().toString(braces ? QUuid::WithBraces : QUuid::WithoutBraces);
        if (!hyphens) {
            value.remove(u'-');
        }
        if (uppercase) {
            value = value.toUpper();
        }
        values.append(value);
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("list"), values);
    return outcome;
}

} // namespace hash_core
