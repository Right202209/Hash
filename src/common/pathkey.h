#pragma once

#include <QString>

namespace hash_core {

// Key returns the canonical map key for path. The path is cleaned on every
// platform. Case is folded only on Windows, whose filesystems are
// case-insensitive; on other platforms the case is preserved because two
// distinct files may differ only by case. Cleaned paths use the Qt-native
// forward-slash separator on every platform.
QString pathKey(const QString &path);

} // namespace hash_core
