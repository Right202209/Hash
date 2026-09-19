#pragma once

#include <QString>

#include <filesystem>

namespace hash_core {

// toFsPath converts between QString and std::filesystem::path using the
// wide-character API on Windows so that non-ASCII paths survive.
std::filesystem::path toFsPath(const QString &path);
QString fromFsPath(const std::filesystem::path &value);

} // namespace hash_core
