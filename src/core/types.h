#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

namespace hash_core {

// Digest is one algorithm/result pair. Results keep the caller's requested
// algorithm order, mirroring the registry display order.
struct Digest {
    QString algorithm;
    QString value;
};

// Result is the outcome of hashing one file. Digests is ordered by the
// requested algorithms; digestFor returns the empty string when a digest is
// absent.
struct Result {
    QString path;
    qint64 size = 0;
    QDateTime modified;
    QVector<Digest> digests;
    QStringList order;
    qint64 bytesRead = 0;
    bool changed = false;

    QString digestFor(const QString &algorithm) const {
        for (const Digest &digest : digests) {
            if (digest.algorithm == algorithm) {
                return digest.value;
            }
        }
        return QString();
    }
};

// Progress is an immutable snapshot of batch hashing progress. Snapshots are
// delivered to BatchOptions::progress from worker threads; the snapshot never
// aliases internal state and may be retained by the receiver.
//
// Progress follows a small state machine:
//
//   - Starting a file sets currentPath and currentSize and resets currentBytes
//     to zero.
//   - While the file is hashed, currentBytes advances as bytes are read.
//     completedBytes deliberately excludes the in-flight file, so
//     completedBytes + currentBytes is the number of bytes hashed so far.
//   - Completing a file, successfully or not, increments completedFiles, adds
//     the bytes the attempt read to completedBytes and resets currentBytes to
//     zero. currentPath and currentSize keep describing the file that just
//     finished until another file starts.
//
// Files skipped because the batch was cancelled before they started emit no
// events, so completedFiles ends below totalFiles when work is cancelled.
struct Progress {
    int completedFiles = 0;
    int totalFiles = 0;
    qint64 completedBytes = 0;
    qint64 totalBytes = 0;
    QString currentPath;
    qint64 currentBytes = 0;
    qint64 currentSize = 0;
};

// FileRequest names one file and the algorithms to compute for it.
struct FileRequest {
    QString path;
    QStringList algorithms;
};

// FileResult pairs a FileRequest with its Result or the error that prevented
// it. error is empty on success; canceled marks results cancelled before or
// during hashing.
struct FileResult {
    FileRequest request;
    Result result;
    QString error;
    bool canceled = false;
};

} // namespace hash_core
