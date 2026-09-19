#pragma once

#include <QString>

namespace hash_core {

// goQuote renders value in double quotes, escaping the characters the
// previous Go implementation escaped through strconv.Quote: printable
// characters stay as-is, control characters become named, hex or unicode
// escapes.
QString goQuote(const QString &value);

// goUnquote decodes a double-quoted Go string literal. It returns false and
// sets error for malformed input.
bool goUnquote(const QString &value, QString *out, QString *error);

} // namespace hash_core
