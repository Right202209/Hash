#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// CodecTool implements Base64 and percent (URL) text codecs. Every operation
// returns {ok, text} or {ok: false, error}.
class CodecTool : public QObject {
    Q_OBJECT

  public:
    explicit CodecTool(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap base64Encode(const QString &text);
    Q_INVOKABLE QVariantMap base64Decode(const QString &text);
    Q_INVOKABLE QVariantMap urlEncode(const QString &text);
    Q_INVOKABLE QVariantMap urlDecode(const QString &text);

  private:
    static QVariantMap textOutcome(const QString &text);
    static QVariantMap errorOutcome(const QString &error);
};

} // namespace hash_core
