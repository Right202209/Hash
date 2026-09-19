#include "common/pathkey.h"

#include <QDir>

namespace hash_core {

QString pathKey(const QString &path) {
    const QString cleaned = QDir::cleanPath(path);
#ifdef Q_OS_WIN
    return cleaned.toLower();
#else
    return cleaned;
#endif
}

} // namespace hash_core
