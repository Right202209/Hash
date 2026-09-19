#include "common/atomicfile.h"

#include "common/fspath.h"
#include "common/pathkey.h"
#include "common/quoting.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>

#include <filesystem>
#include <system_error>

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
    }
    if (!created) {
        return QStringLiteral("create temporary output: open ") + goQuote(temporaryPath) +
               QStringLiteral(": file exists");
    }

    if (temporary.write(content) != content.size()) {
        error = QStringLiteral("write temporary output: short write");
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
