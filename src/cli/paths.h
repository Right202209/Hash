#pragma once

#include <QString>
#include <QStringList>

namespace hash_core {

// Error returned when no file path or pattern was provided.
inline const char kNoInputMessage[] = "at least one file path or pattern is required";

// ExpandOptions controls how expandPaths interprets its inputs. recursive
// descends into directories instead of rejecting them; literal treats inputs
// as literal paths even when they contain glob metacharacters.
struct ExpandOptions {
    bool recursive = false;
    bool literal = false;
};

// expandPaths resolves inputs into a de-duplicated, ordered list of regular
// files, applying globbing, recursion and literal handling as configured by
// options. Returns false and sets error on failure.
bool expandPaths(const QStringList &inputs, const ExpandOptions &options, QStringList *paths,
                 QString *error);

// hasGlobMeta reports whether path contains glob metacharacters. Exposed for
// tests.
bool hasGlobMeta(const QString &path);

// globMatch reports whether name matches the glob pattern, which may contain
// *, ? and [class] with ^ negation. Returns false with badPattern set for a
// malformed pattern. Exposed for tests.
bool globMatch(const QString &pattern, const QString &name, bool *badPattern);

} // namespace hash_core
