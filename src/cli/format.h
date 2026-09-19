#pragma once

#include "core/types.h"

#include <QString>
#include <QStringList>

namespace hash_core {

// FormatOptions selects the output encoding and which optional columns are
// included.
struct FormatOptions {
    QString format; // "text", "tsv", or "json"
    bool showSize = false;
    bool showModified = false;
    bool utc = false;
};

// writeResults renders results using options. The text and TSV encoders emit
// one row per algorithm; JSON emits one object per file. Returns false and
// sets error for an unsupported format.
bool writeResults(const QVector<FileResult> &results, const QStringList &algorithms,
                  const FormatOptions &options, QString *output, QString *error);

// tsvValue escapes one TSV cell: backslash, tab, carriage return and newline
// become escape sequences, and a leading formula-like character gains a
// spreadsheet-safe apostrophe prefix.
QString tsvValue(const QString &value);

} // namespace hash_core
