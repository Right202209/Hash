#pragma once

#include "core/types.h"

#include <QString>
#include <QVector>

namespace hash_core {

// CompareRecord is one algorithm, digest and path tuple read from a listing.
struct CompareRecord {
    QString algorithm;
    QString digest;
    QString path;
};

// CompareSummary counts how an actual listing compares with an expected one.
// exact is true only when every expected record matched and neither side had
// extras or duplicates.
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
// header rows and error rows. Returns false and sets error on malformed
// input.
bool parseDigestText(const QString &text, QVector<CompareRecord> *records, QString *error);

// compareRecords indexes both listings by algorithm and normalized path, then
// counts matches, mismatches, missing, unexpected and duplicate records.
CompareSummary compareRecords(const QVector<CompareRecord> &expected,
                              const QVector<CompareRecord> &actual);

} // namespace hash_core
