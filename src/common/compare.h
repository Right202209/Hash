#pragma once

#include "core/types.h"

#include <QString>
#include <QVector>

namespace hash_core {

// CompareRecord is one algorithm, digest and path tuple read from a listing.
// loose marks records parsed from a bare digest line without the full
// algorithm/digest/path TSV structure; they carry no trustworthy path or
// algorithm and are compared by digest value alone.
struct CompareRecord {
    QString algorithm;
    QString digest;
    QString path;
    bool loose = false;
};

// CompareSummary counts how an actual listing compares with an expected one.
// exact is true only when at least one record matched and neither side had
// mismatches, missing, extra or duplicate records; an empty comparison is
// never exact.
struct CompareSummary {
    int matches = 0;
    int mismatches = 0;
    int missing = 0;
    int unexpected = 0;
    int duplicates = 0;
    bool exact = false;
};

// recordsFromResults flattens successful hash results into comparison
// records, skipping files that failed.
QVector<CompareRecord> recordsFromResults(const QVector<FileResult> &results);

// parseDigestText reads a text or TSV digest listing, tolerating a BOM,
// header rows and error rows. Lines with at least three tab-separated fields
// are full records; digest values carrying an algorithm tag such as
// "sha256:", "SHA-256:" or "md5：" (half-width or full-width separators, or
// an equals sign) have the tag stripped automatically. Shorter lines are
// treated as loose digests: an optional algorithm tag followed by the hex
// digest and an optional trailing path. Returns false and sets error on
// malformed input.
bool parseDigestText(const QString &text, QVector<CompareRecord> *records, QString *error);

// compareRecords indexes full listings by algorithm and normalized path, then
// counts matches, mismatches, missing, unexpected and duplicate records.
// Loose records carry no path or algorithm context, so they verify by digest
// value alone: each one counts as a match when any digest on the other side
// is equal, and as a mismatch otherwise; an equal loose digest present on
// both sides counts once. They never produce missing,
// unexpected or duplicate counts, and when the actual side is made up entirely
// of loose records (a bare-digest verification paste), expected records are
// not counted as missing either.
CompareSummary compareRecords(const QVector<CompareRecord> &expected,
                              const QVector<CompareRecord> &actual);

} // namespace hash_core
