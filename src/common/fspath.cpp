#include "common/fspath.h"

namespace hash_core {

#ifdef _WIN32
std::filesystem::path toFsPath(const QString &path) {
    return std::filesystem::path(path.toStdWString());
}

QString fromFsPath(const std::filesystem::path &value) {
    return QString::fromStdWString(value.wstring());
}
#else
std::filesystem::path toFsPath(const QString &path) {
    return std::filesystem::path(path.toStdString());
}

QString fromFsPath(const std::filesystem::path &value) {
    return QString::fromStdString(value.string());
}
#endif

} // namespace hash_core
