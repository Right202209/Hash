#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>

namespace hash_core {

// TextDigestTool computes registry digests over an in-memory UTF-8 payload,
// reusing the same digester implementations as file hashing.
class TextDigestTool : public QObject {
    Q_OBJECT

  public:
    explicit TextDigestTool(QObject *parent = nullptr);

    // hashText returns {ok, digests: [{algorithm, label, value}]} or
    // {ok: false, error} when a name is unknown or the list is empty.
    Q_INVOKABLE QVariantMap hashText(const QString &text, const QStringList &algorithms);
};

} // namespace hash_core
