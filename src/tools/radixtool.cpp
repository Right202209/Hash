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
    // Parse the sign and the unsigned magnitude separately so the full 64-bit
    // range (e.g. FFFFFFFFFFFFFFFF) converts instead of overflowing qlonglong.
    QString text = value.trimmed();
    bool negative = false;
    if (text.startsWith(u'-') || text.startsWith(u'+')) {
        negative = text.startsWith(u'-');
        text.remove(0, 1);
    }
    bool valid = !text.isEmpty() && !text.startsWith(u'-') && !text.startsWith(u'+') &&
                 !text.at(0).isSpace();
    const qulonglong magnitude = valid ? text.toULongLong(&valid, fromBase) : 0;
    if (!valid) {
        outcome.insert(QStringLiteral("error"),
                       QStringLiteral("无法按 %1 进制解析输入").arg(fromBase));
        return outcome;
    }
    outcome.insert(QStringLiteral("ok"), true);
    const QString sign = negative && magnitude != 0 ? QStringLiteral("-") : QString();
    outcome.insert(QStringLiteral("value"), sign + QString::number(magnitude, toBase));
    // decimal is a string: QML numbers are doubles and lose precision above 2^53.
    outcome.insert(QStringLiteral("decimal"), sign + QString::number(magnitude));
    return outcome;
}

} // namespace hash_core
