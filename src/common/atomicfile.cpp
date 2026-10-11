#include "common/atomicfile.h"

#include "common/fspath.h"
#include "common/pathkey.h"
#include "common/quoting.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <system_error>

#ifdef Q_OS_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

namespace hash_core {
namespace {

QString validate(const QString &path, const QStringList &inputs) {
    const QFileInfo info(path);
    const QString absolute = info.absoluteFilePath();
    if (info.isSymLink()) {
        return QStringLiteral("refusing to write through symbolic link ") + goQuote(path);
    }
    if (info.exists()) {
        for (const QString &input : inputs) {
            const QFileInfo inputInfo(input);
            if (!inputInfo.exists()) {
                continue;
            }
            std::error_code code;
            if (std::filesystem::equivalent(toFsPath(path), toFsPath(input), code) && !code) {
                return QStringLiteral("output path must not refer to input ") + goQuote(input);
            }
        }
    }
    for (const QString &input : inputs) {
        const QString inputAbsolute = QFileInfo(input).absoluteFilePath();
        if (pathKey(inputAbsolute) == pathKey(absolute)) {
            return QStringLiteral("output path must differ from input ") + goQuote(input);
        }
    }
    return QString();
}

// syncToDisk flushes Qt's buffer and asks the OS to persist the file's data,
// so a crash after the rename cannot leave an empty or truncated output.
// Returns 0 on success, otherwise the errno value.
int syncToDisk(QFile &file) {
    if (!file.flush()) {
        return EIO;
    }
#ifdef Q_OS_WIN
    const int result = _commit(file.handle());
#else
    const int result = ::fsync(file.handle());
#endif
    return result == 0 ? 0 : errno;
}

} // namespace

QString writeAtomicFile(const QString &path, const QStringList &inputs, const QByteArray &content) {
    QString error = validate(path, inputs);
    if (!error.isEmpty()) {
        return error;
    }

    // Create the temporary sibling in the destination directory so the
    // final rename stays on one filesystem.
    const QString directory = QFileInfo(path).absolutePath();
    QFile temporary;
    QString temporaryPath;
    bool created = false;
    for (int attempt = 0; attempt < 100 && !created; ++attempt) {
        temporaryPath = directory + QStringLiteral("/.hash-output-") +
                        QString::number(QRandomGenerator::global()->generate(), 16);
        temporary.setFileName(temporaryPath);
        created = temporary.open(QIODevice::WriteOnly | QIODevice::NewOnly);
        // Only a name collision is worth another attempt; a missing
        // directory or a permission error fails the same way every time.
        if (!created && !QFileInfo::exists(temporaryPath)) {
            break;
        }
    }
    if (!created) {
        return QStringLiteral("create temporary output: open ") + goQuote(temporaryPath) +
               QStringLiteral(": ") + temporary.errorString();
    }

    if (temporary.write(content) != content.size()) {
        error = QStringLiteral("write temporary output: ") +
                (temporary.error() != QFileDevice::NoError ? temporary.errorString()
                                                           : QStringLiteral("short write"));
        temporary.close();
        temporary.remove();
        return error;
    }
    if (const int code = syncToDisk(temporary); code != 0) {
        error = QStringLiteral("sync temporary output: ") +
                QString::fromLocal8Bit(std::strerror(code));
        temporary.close();
        temporary.remove();
        return error;
    }
    temporary.close();
    if (temporary.error() != QFileDevice::NoError) {
        error = QStringLiteral("close temporary output: ") + temporary.errorString();
        temporary.remove();
        return error;
    }

    error = validate(path, inputs);
    if (!error.isEmpty()) {
        temporary.remove();
        return error;
    }

    std::error_code renameCode;
    std::filesystem::rename(toFsPath(temporaryPath), toFsPath(path), renameCode);
    if (renameCode) {
        temporary.remove();
        return QStringLiteral("rename temporary output: ") +
               QString::fromStdString(renameCode.message());
    }
    return QString();
}

QString validateOutputFile(const QString &path, const QStringList &inputs) {
    return validate(path, inputs);
}

} // namespace hash_core
