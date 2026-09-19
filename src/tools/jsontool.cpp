#include "tools/jsontool.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace hash_core {

JsonTool::JsonTool(QObject *parent) : QObject(parent) {}

QVariantMap JsonTool::formatJson(const QString &text) {
    return render(text, QJsonDocument::Indented);
}

QVariantMap JsonTool::minifyJson(const QString &text) {
    return render(text, QJsonDocument::Compact);
}

QVariantMap JsonTool::render(const QString &text, QJsonDocument::JsonFormat style) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    QJsonParseError parseError;
    const QByteArray bytes = text.toUtf8();
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || document.isNull()) {
        // offset counts bytes in the UTF-8 input; convert to a 1-based line.
        const int line = bytes.left(parseError.offset).count('\n') + 1;
        outcome.insert(QStringLiteral("error"),
                       QStringLiteral("第 %1 行：%2").arg(line).arg(parseError.errorString()));
        return outcome;
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("text"), QString::fromUtf8(document.toJson(style)));
    return outcome;
}

} // namespace hash_core
