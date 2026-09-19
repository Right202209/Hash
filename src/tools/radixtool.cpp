#include "tools/radixtool.h"

namespace hash_core {

RadixTool::RadixTool(QObject *parent) : QObject(parent) {}

QVariantMap RadixTool::convert(const QString &value, int fromBase, int toBase) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    if (fromBase < 2 || fromBase > 36 || toBase < 2 || toBase > 36) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("进制需在 2-36 之间"));
        return outcome;
    }
    bool valid = false;
    const qlonglong parsed = value.trimmed().toLongLong(&valid, fromBase);
    if (!valid) {
        outcome.insert(QStringLiteral("error"),
                       QStringLiteral("无法按 %1 进制解析输入").arg(fromBase));
        return outcome;
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("value"), QString::number(parsed, toBase));
    outcome.insert(QStringLiteral("decimal"), parsed);
    return outcome;
}

} // namespace hash_core
