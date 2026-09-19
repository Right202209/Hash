#pragma once

#include <QObject>
#include <QVariantMap>

namespace hash_core {

// PasswordTool generates random passwords from selected character sets using
// the system entropy source. Passwords always contain at least one character
// from every selected set.
class PasswordTool : public QObject {
    Q_OBJECT

  public:
    explicit PasswordTool(QObject *parent = nullptr);

    // generate returns {ok, list: [password, ...]} or {ok: false, error}.
    // excludeAmbiguous removes look-alike characters (0/O/o/1/l/I and quote,
    // backtick, double quote, pipe) from every set.
    Q_INVOKABLE QVariantMap generate(int length, int count, bool upper, bool lower, bool digits,
                                     bool symbols, bool excludeAmbiguous);
};

} // namespace hash_core
