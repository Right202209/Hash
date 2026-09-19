#ifdef _WIN32

#include "core/hasher.h"

#include <QDateTime>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace hash_core {
namespace {

QString systemErrorMessage(DWORD code) {
    LPWSTR buffer = nullptr;
    DWORD size = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                      FORMAT_MESSAGE_IGNORE_INSERTS,
                                  nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    QString message = size && buffer ? QString::fromWCharArray(buffer, int(size)).trimmed()
                                     : QStringLiteral("unknown error");
    if (buffer) {
        ::LocalFree(buffer);
    }
    return message;
}

quint64 fileTimeToMs(const FILETIME &time) {
    const quint64 ticks = (quint64(time.dwHighDateTime) << 32) | quint64(time.dwLowDateTime);
    // 100 ns units since 1601-01-01; convert to Unix epoch milliseconds.
    const quint64 kUnixEpochTicks = 116444736000000000ULL;
    if (ticks < kUnixEpochTicks) {
        return 0;
    }
    return (ticks - kUnixEpochTicks) / 10000ULL;
}

void fillInfo(const BY_HANDLE_FILE_INFORMATION &info, FileStat *out) {
    const DWORD attributes = info.dwFileAttributes;
    out->valid = true;
    out->regular = (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE |
                                  FILE_ATTRIBUTE_REPARSE_POINT)) == 0;
    out->size = (qint64(info.nFileSizeHigh) << 32) | qint64(info.nFileSizeLow);
    out->mtimeMs = qint64(fileTimeToMs(info.ftLastWriteTime));
    out->deviceId = quint64(info.dwVolumeSerialNumber);
    out->fileId = (quint64(info.nFileIndexHigh) << 32) | quint64(info.nFileIndexLow);
}

HANDLE openByPath(const QString &path, DWORD desiredAccess, QString *error) {
    HANDLE handle = ::CreateFileW(
        reinterpret_cast<const WCHAR *>(path.utf16()), desiredAccess,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        desiredAccess == 0 ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE && error) {
        *error = systemErrorMessage(::GetLastError());
    }
    return handle;
}

} // namespace

PlatformFile::PlatformFile() = default;

PlatformFile::~PlatformFile() { close(); }

void PlatformFile::close() {
    if (m_handle) {
        ::CloseHandle(static_cast<HANDLE>(m_handle));
        m_handle = nullptr;
    }
}

bool PlatformFile::open(const QString &path, QString *error) {
    HANDLE handle = openByPath(path, GENERIC_READ, error);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    m_handle = handle;
    return true;
}

bool PlatformFile::stat(FileStat *out, QString *error) {
    BY_HANDLE_FILE_INFORMATION info;
    if (!::GetFileInformationByHandle(static_cast<HANDLE>(m_handle), &info)) {
        if (error) {
            *error = systemErrorMessage(::GetLastError());
        }
        return false;
    }
    fillInfo(info, out);
    return true;
}

qint64 PlatformFile::read(char *buffer, qint64 maxSize, QString *error) {
    DWORD bytesRead = 0;
    if (!::ReadFile(static_cast<HANDLE>(m_handle), buffer, DWORD(maxSize), &bytesRead, nullptr)) {
        const DWORD code = ::GetLastError();
        if (code == ERROR_HANDLE_EOF || code == ERROR_BROKEN_PIPE) {
            return 0;
        }
        if (error) {
            *error = systemErrorMessage(code);
        }
        return -1;
    }
    return qint64(bytesRead);
}

bool statPath(const QString &path, FileStat *out, QString *error) {
    HANDLE handle = openByPath(path, 0, error);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    BY_HANDLE_FILE_INFORMATION info;
    const bool filled = ::GetFileInformationByHandle(handle, &info) != 0;
    if (!filled && error) {
        *error = systemErrorMessage(::GetLastError());
    }
    ::CloseHandle(handle);
    if (!filled) {
        return false;
    }
    fillInfo(info, out);
    return true;
}

bool sameFile(const FileStat &left, const FileStat &right) {
    return left.deviceId == right.deviceId && left.fileId == right.fileId;
}

QDateTime fileStatTime(const FileStat &stat) {
    return QDateTime::fromMSecsSinceEpoch(stat.mtimeMs);
}

} // namespace hash_core

#endif // _WIN32
