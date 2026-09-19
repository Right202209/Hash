#include "tools/codectool.h"

#include <QByteArray>
#include <QUrl>

namespace hash_core {

CodecTool::CodecTool(QObject *parent) : QObject(parent) {}

QVariantMap CodecTool::base64Encode(const QString &text) {
    return textOutcome(QString::fromLatin1(text.toUtf8().toBase64()));
}

QVariantMap CodecTool::base64Decode(const QString &text) {
    // Tolerate pasted text wrapped over lines.
    QByteArray cleaned;
    cleaned.reserve(text.size());
    for (const QChar character : text) {
        const char16_t code = character.unicode();
        if (code == u' ' || code == u'\t' || code == u'\r' || code == u'\n') {
            continue;
        }
        cleaned.append(char(code));
    }
    const QByteArray::FromBase64Result result =
        QByteArray::fromBase64Encoding(cleaned, QByteArray::AbortOnBase64DecodingErrors);
    if (result.decodingStatus != QByteArray::Base64DecodingStatus::Ok) {
        return errorOutcome(QStringLiteral("不是有效的 Base64 输入"));
    }
    return textOutcome(QString::fromUtf8(result.decoded));
}

QVariantMap CodecTool::urlEncode(const QString &text) {
    return textOutcome(QString::fromLatin1(QUrl::toPercentEncoding(text)));
}

QVariantMap CodecTool::urlDecode(const QString &text) {
    // fromPercentEncoding already returns decoded text.
    return textOutcome(QUrl::fromPercentEncoding(text.toUtf8()));
}

QVariantMap CodecTool::textOutcome(const QString &text) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("text"), text);
    return outcome;
}

QVariantMap CodecTool::errorOutcome(const QString &error) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    outcome.insert(QStringLiteral("error"), error);
    return outcome;
}

} // namespace hash_core
