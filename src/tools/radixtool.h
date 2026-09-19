#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// RadixTool converts integer values between bases 2-36.
class RadixTool : public QObject {
    Q_OBJECT

  public:
    explicit RadixTool(QObject *parent = nullptr);

    // convert returns {ok, value, decimal} or {ok: false, error}.
    Q_INVOKABLE QVariantMap convert(const QString &value, int fromBase, int toBase);
};

} // namespace hash_core
