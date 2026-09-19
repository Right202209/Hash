#ifndef _WIN32

#include "core/hasher.h"

#include <QDateTime>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace hash_core {
namespace {

QString systemErrorMessage(int code) { return QString::fromLocal8Bit(std::strerror(code)); }

void fillStat(const struct ::stat &st, FileStat *out) {
    out->valid = true;
    out->regular = S_ISREG(st.st_mode);
    out->size = qint64(st.st_size);
#if defined(__APPLE__)
    out->mtimeMs = qint64(st.st_mtimespec.tv_sec) * 1000 + qint64(st.st_mtimespec.tv_nsec) / 1000;
#else
    out->mtimeMs = qint64(st.st_mtim.tv_sec) * 1000 + qint64(st.st_mtim.tv_nsec) / 1000;
#endif
    out->deviceId = quint64(st.st_dev);
    out->fileId = quint64(st.st_ino);
}

} // namespace

PlatformFile::PlatformFile() = default;

PlatformFile::~PlatformFile() { close(); }

void PlatformFile::close() {
    if (m_descriptor >= 0) {
        ::close(m_descriptor);
        m_descriptor = -1;
    }
}

bool PlatformFile::open(const QString &path, QString *error) {
    const QByteArray encoded = path.toLocal8Bit();
    const int descriptor = ::open(encoded.constData(), O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) {
        if (error) {
            *error = systemErrorMessage(errno);
        }
        return false;
    }
    m_descriptor = descriptor;
    return true;
}

bool PlatformFile::stat(FileStat *out, QString *error) {
    struct ::stat st;
    if (::fstat(m_descriptor, &st) != 0) {
        if (error) {
            *error = systemErrorMessage(errno);
        }
        return false;
    }
    fillStat(st, out);
    return true;
}

qint64 PlatformFile::read(char *buffer, qint64 maxSize, QString *error) {
    for (;;) {
        const ssize_t count = ::read(m_descriptor, buffer, size_t(maxSize));
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count < 0) {
            if (error) {
                *error = systemErrorMessage(errno);
            }
            return -1;
        }
        return qint64(count);
    }
}

bool statPath(const QString &path, FileStat *out, QString *error) {
    struct ::stat st;
    if (::stat(path.toLocal8Bit().constData(), &st) != 0) {
        if (error) {
            *error = systemErrorMessage(errno);
        }
        return false;
    }
    fillStat(st, out);
    return true;
}

bool sameFile(const FileStat &left, const FileStat &right) {
    return left.deviceId == right.deviceId && left.fileId == right.fileId;
}

QDateTime fileStatTime(const FileStat &stat) {
    return QDateTime::fromMSecsSinceEpoch(stat.mtimeMs);
}

} // namespace hash_core

#endif // !_WIN32
