#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// UuidTool generates random UUID v4 values with formatting options.
class UuidTool : public QObject {
    Q_OBJECT

  public:
    explicit UuidTool(QObject *parent = nullptr);

    // generate returns {ok, list: [uuid, ...]} or {ok: false, error}.
    // uppercase uppercases the hex digits, braces wraps the value in {} and
    // hyphens=false strips the dashes.
    Q_INVOKABLE QVariantMap generate(int count, bool uppercase, bool braces, bool hyphens);
};

} // namespace hash_core
