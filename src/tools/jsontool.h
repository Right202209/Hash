#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// JsonTool validates, pretty-prints and minifies JSON text.
class JsonTool : public QObject {
    Q_OBJECT

  public:
    explicit JsonTool(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap formatJson(const QString &text);
    Q_INVOKABLE QVariantMap minifyJson(const QString &text);

  private:
    static QVariantMap render(const QString &text, QJsonDocument::JsonFormat style);
};

} // namespace hash_core
