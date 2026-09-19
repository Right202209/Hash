#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// ColorTool converts color notations: #RGB, #RGBA, #RRGGBB, #RRGGBBAA and
// the rgb()/rgba()/hsl()/hsla() functional forms (alpha in 0-1 or 0-255).
class ColorTool : public QObject {
    Q_OBJECT

  public:
    explicit ColorTool(QObject *parent = nullptr);

    // convert returns {ok, hex, rgba, hsl} or {ok: false, error}. hex keeps
    // six digits; rgba includes the alpha channel only when it is below 255.
    Q_INVOKABLE QVariantMap convert(const QString &text);
};

} // namespace hash_core
