#pragma once

#include "core/types.h"

#include <functional>
#include <memory>

namespace hash_core {

// FileStat is the authoritative snapshot of one file, taken from the open
// handle so it cannot be separated from the read by a path swap. mtimeMs is
// the modification time in milliseconds since the Unix epoch.
struct FileStat {
    bool valid = false;
    bool regular = false;
    qint64 size = 0;
    qint64 mtimeMs = 0;
    quint64 deviceId = 0; // volume serial number / st_dev
    quint64 fileId = 0;   // file index / st_ino
};

// PlatformFile is a read-only handle opened through the platform API.
// Symbolic links are followed. On Windows, opening a directory fails like
// os.Open did; on POSIX, the regular-file check happens on stat.
class PlatformFile {
  public:
    PlatformFile();
    ~PlatformFile();
    PlatformFile(const PlatformFile &) = delete;
    PlatformFile &operator=(const PlatformFile &) = delete;

    bool open(const QString &path, QString *error);
    void close();
    bool stat(FileStat *out, QString *error);
    // read returns the number of bytes read, 0 at end of file, or -1 on
    // error with the system message in *error.
    qint64 read(char *buffer, qint64 maxSize, QString *error);

  private:
    void *m_handle = nullptr; // Win32 HANDLE, or nullptr
    int m_descriptor = -1;    // POSIX file descriptor
};

// statPath stats path through the filesystem, following symbolic links.
bool statPath(const QString &path, FileStat *out, QString *error);

// sameFile reports whether two stats describe the same file.
bool sameFile(const FileStat &left, const FileStat &right);

QDateTime fileStatTime(const FileStat &stat);

// AlgorithmError mirrors the previous sentinel errors.
enum class AlgorithmError {
    None,
    NoAlgorithms,
    EmptyAlgorithmName,
    UnknownAlgorithm,
    DuplicateAlgorithm,
};

// validateAlgorithms reports whether algorithms is a non-empty, duplicate-free
// list of registered algorithm names. Names must already be normalized to the
// lower-case registry form. When the validation fails, message carries the
// human-readable error.
AlgorithmError validateAlgorithms(const QStringList &algorithms, QString *message = nullptr);

// HashOptions tunes a single-file hash operation. canceled is polled before
// every read; when set, work stops with a canceled outcome.
struct HashOptions {
    qint64 bufferSize = 0; // non-positive selects the 1 MiB default
    std::function<void(qint64 bytesRead)> progress;
    std::function<bool()> canceled;
};

// HashOutcome pairs the result of one file hash with its error. error is
// empty on success; canceled marks cancellation, in which case result still
// carries the path and the bytes read so the batch progress stays monotonic.
struct HashOutcome {
    Result result;
    QString error;
    bool canceled = false;
};

// hashFileWithOptions hashes path, applying options. Symbolic links are
// followed and the opened file handle is authoritative, so the regular-file
// check and the read cannot be separated by a path swap. On a failure after
// reading began, the returned result still carries the path and the number of
// bytes read before the failure; digests are absent.
HashOutcome hashFileWithOptions(const QString &path, const QStringList &algorithms,
                                const HashOptions &options);

// hashFile hashes path with the default options.
inline HashOutcome hashFile(const QString &path, const QStringList &algorithms) {
    return hashFileWithOptions(path, algorithms, HashOptions{});
}

} // namespace hash_core
