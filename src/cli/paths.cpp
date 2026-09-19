#include "cli/paths.h"

#include "common/fspath.h"
#include "common/pathkey.h"
#include "common/quoting.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace hash_core {
namespace {

constexpr char kBadPatternMessage[] = "syntax error in pattern";

bool isSeparator(QChar character) {
#ifdef Q_OS_WIN
    return character == u'/' || character == u'\\';
#else
    return character == u'/';
#endif
}

// globMatchHere matches name at offset n against pattern at offset p,
// treating '*' as matching any sequence of non-separator characters.
bool globMatchHere(const QString &pattern, int p, const QString &name, int n, bool *badPattern) {
    while (p < pattern.size()) {
        const QChar pc = pattern.at(p);
        if (pc == u'*') {
            while (p < pattern.size() && pattern.at(p) == u'*') {
                ++p;
            }
            if (p == pattern.size()) {
                // Trailing * matches the rest unless it contains a separator.
                for (int index = n; index < name.size(); ++index) {
                    if (isSeparator(name.at(index))) {
                        return false;
                    }
                }
                return true;
            }
            for (int index = n; index < name.size(); ++index) {
                if (isSeparator(name.at(index))) {
                    break;
                }
                if (globMatchHere(pattern, p, name, index, badPattern)) {
                    return true;
                }
                if (*badPattern) {
                    return false;
                }
            }
            return false;
        }
        if (pc == u'?') {
            if (n >= name.size() || isSeparator(name.at(n))) {
                return false;
            }
            ++p;
            ++n;
            continue;
        }
        if (pc == u'[') {
            int cursor = p + 1;
            bool negated = false;
            if (cursor < pattern.size() && pattern.at(cursor) == u'^') {
                negated = true;
                ++cursor;
            }
            const QChar target = n < name.size() ? name.at(n) : QChar();
            bool matched = false;
            int ranges = 0;
            bool closed = false;
            while (cursor < pattern.size()) {
                if (pattern.at(cursor) == u']' && ranges > 0) {
                    closed = true;
                    ++cursor;
                    break;
                }
                const QChar low = pattern.at(cursor);
                QChar high = low;
                ++cursor;
                if (cursor < pattern.size() && pattern.at(cursor) == u'-') {
                    ++cursor;
                    if (cursor >= pattern.size()) {
                        *badPattern = true;
                        return false;
                    }
                    high = pattern.at(cursor);
                    ++cursor;
                }
                if (n < name.size() && low <= target && target <= high) {
                    matched = true;
                }
                ++ranges;
            }
            if (!closed) {
                *badPattern = true;
                return false;
            }
            if (n >= name.size() || matched == negated) {
                return false;
            }
            ++n;
            p = cursor;
            continue;
        }
        if (pc == u'\\') {
#ifdef Q_OS_WIN
            // On Windows the backslash is a path separator, matching the
            // previous Go implementation on that platform.
            if (n >= name.size() || !(name.at(n) == u'/' || name.at(n) == u'\\')) {
                return false;
            }
            ++p;
            ++n;
            continue;
#else
            if (p + 1 >= pattern.size()) {
                *badPattern = true;
                return false;
            }
            if (n >= name.size() || name.at(n) != pattern.at(p + 1)) {
                return false;
            }
            p += 2;
            ++n;
            continue;
#endif
        }
        if (n >= name.size() || name.at(n) != pc) {
            return false;
        }
        ++p;
        ++n;
    }
    return n == name.size();
}

// cleanGlobPath trims the trailing separator that splitAtLastSeparator keeps,
// mirroring the previous Go implementation.
QString cleanGlobPath(const QString &path) {
    if (path.isEmpty()) {
        return QStringLiteral(".");
    }
#ifdef Q_OS_WIN
    if (path.size() <= 3 && path.endsWith(u':')) {
        return path;
    }
#endif
    if (path.size() == 1 && isSeparator(path.at(0))) {
        return path;
    }
    int end = path.size();
    while (end > 1 && isSeparator(path.at(end - 1))) {
        --end;
    }
    return path.left(end);
}

QString joinDirEntry(const QString &dir, const QString &name) {
    if (dir == QStringLiteral(".")) {
        return name;
    }
    return QDir::cleanPath(dir + u'/' + name);
}

// globInDir appends the entries of dir matching pattern, sorted by name.
// Directory read errors are ignored, like the previous Go implementation.
bool globInDir(const QString &dir, const QString &pattern, QStringList *matches, QString *error) {
    std::error_code code;
    std::filesystem::directory_iterator iterator(
        toFsPath(dir), std::filesystem::directory_options::skip_permission_denied, code);
    if (code) {
        return true;
    }
    QStringList names;
    for (std::error_code endCode; iterator != std::filesystem::directory_iterator();
         iterator.increment(endCode)) {
        if (endCode) {
            break;
        }
        names.append(fromFsPath(iterator->path().filename()));
    }
    std::sort(names.begin(), names.end());
    for (const QString &name : names) {
        bool badPattern = false;
        if (!globMatch(pattern, name, &badPattern)) {
            if (badPattern) {
                *error = QString::fromLatin1(kBadPatternMessage);
                return false;
            }
            continue;
        }
        matches->append(joinDirEntry(dir, name));
    }
    return true;
}

bool globPattern(const QString &pattern, QStringList *matches, QString *error) {
    if (!hasGlobMeta(pattern)) {
        // Lstat semantics: a dangling symbolic link still counts as a match.
        std::error_code code;
        const std::filesystem::file_status status =
            std::filesystem::symlink_status(toFsPath(pattern), code);
        if (!code && std::filesystem::exists(status)) {
            matches->append(pattern);
        }
        return true;
    }
    int split = -1;
    for (int index = pattern.size() - 1; index >= 0; --index) {
        if (isSeparator(pattern.at(index))) {
            split = index;
            break;
        }
    }
#ifdef Q_OS_WIN
    // Drive-relative patterns like "C:*.txt" split into the volume prefix
    // and the rest, like the previous Go implementation.
    if (split < 0 && pattern.size() > 2 && pattern.at(1) == u':') {
        split = 1;
    }
#endif
    const QString dir = cleanGlobPath(split >= 0 ? pattern.left(split + 1) : QString());
    const QString file = split >= 0 ? pattern.mid(split + 1) : pattern;
    if (!hasGlobMeta(dir)) {
        return globInDir(dir, file, matches, error);
    }
    QStringList directories;
    if (!globPattern(dir, &directories, error)) {
        return false;
    }
    for (const QString &directory : directories) {
        if (!globInDir(directory, file, matches, error)) {
            return false;
        }
    }
    return true;
}

// walkDir collects regular files under dir, following the previous Go
// walk semantics: symbolic links are never followed and never collected.
bool walkDir(const QString &dir, QStringList *files, QString *error) {
    std::error_code code;
    std::filesystem::directory_iterator iterator(
        toFsPath(dir), std::filesystem::directory_options::skip_permission_denied, code);
    if (code) {
        *error = QString::fromStdString(code.message());
        return false;
    }
    for (std::error_code endCode; iterator != std::filesystem::directory_iterator();
         iterator.increment(endCode)) {
        if (endCode) {
            *error = QString::fromStdString(endCode.message());
            return false;
        }
        const std::filesystem::directory_entry entry = *iterator;
        std::error_code entryCode;
        if (entry.is_symlink(entryCode)) {
            continue;
        }
        if (entry.is_directory(entryCode)) {
            if (!walkDir(fromFsPath(entry.path()), files, error)) {
                return false;
            }
            continue;
        }
        if (entry.is_regular_file(entryCode)) {
            files->append(fromFsPath(entry.path()));
        } else if (entryCode) {
            *error = QString::fromStdString(entryCode.message());
            return false;
        }
    }
    return true;
}

bool filterFiles(const QStringList &paths, QStringList *out, QString *error) {
    QStringList files;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists()) {
            *error = QStringLiteral("stat ") + goQuote(path) +
                     QStringLiteral(": no such file or directory");
            return false;
        }
        if (!info.isFile()) {
            continue;
        }
        files.append(path);
    }
    if (files.isEmpty()) {
        *error = QStringLiteral("pattern matched no files");
        return false;
    }
    std::sort(files.begin(), files.end());
    *out += files;
    return true;
}

bool expandPath(const QString &input, const ExpandOptions &options, QStringList *out,
                QString *error) {
    if (!options.literal && hasGlobMeta(input)) {
        QStringList matches;
        if (!globPattern(input, &matches, error)) {
            *error = QStringLiteral("expand ") + goQuote(input) + ": " + *error;
            return false;
        }
        if (matches.isEmpty()) {
            *error =
                QStringLiteral("pattern ") + goQuote(input) + QStringLiteral(" matched no files");
            return false;
        }
        return filterFiles(matches, out, error);
    }
    const QFileInfo info(input);
    if (!info.exists()) {
        *error = QStringLiteral("stat ") + goQuote(input) +
                 QStringLiteral(": no such file or directory");
        return false;
    }
    if (!info.isDir()) {
        if (!info.isFile()) {
            *error = goQuote(input) + QStringLiteral(" is not a regular file");
            return false;
        }
        out->append(input);
        return true;
    }
    if (!options.recursive) {
        *error = goQuote(input) +
                 QStringLiteral(" is a directory; use --recursive to include its files");
        return false;
    }
    QStringList files;
    if (!walkDir(input, &files, error)) {
        *error = QStringLiteral("walk ") + goQuote(input) + ": " + *error;
        return false;
    }
    std::sort(files.begin(), files.end());
    *out += files;
    return true;
}

} // namespace

bool hasGlobMeta(const QString &path) {
    for (const QChar character : path) {
        if (character == u'*' || character == u'?' || character == u'[') {
            return true;
        }
    }
    return false;
}

bool globMatch(const QString &pattern, const QString &name, bool *badPattern) {
    *badPattern = false;
    return globMatchHere(pattern, 0, name, 0, badPattern);
}

bool expandPaths(const QStringList &inputs, const ExpandOptions &options, QStringList *paths,
                 QString *error) {
    if (inputs.isEmpty()) {
        *error = QString::fromLatin1(kNoInputMessage);
        return false;
    }
    QSet<QString> seen;
    seen.reserve(inputs.size());
    for (const QString &input : inputs) {
        QStringList matches;
        if (!expandPath(input, options, &matches, error)) {
            return false;
        }
        for (const QString &match : matches) {
            const QString cleaned = QDir::cleanPath(match);
            const QString key = pathKey(cleaned);
            if (seen.contains(key)) {
                continue;
            }
            seen.insert(key);
            paths->append(cleaned);
        }
    }
    return true;
}

} // namespace hash_core
