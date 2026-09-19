#include "core/hasher.h"

#include "core/registry.h"

#include <QSet>
#include <vector>

namespace hash_core {
namespace {

constexpr qint64 kDefaultBufferSize = 1024 * 1024;

QString quotePath(const QString &path) {
    // Engine messages quote paths like the previous Go implementation did
    // with %q.
    QString quoted;
    quoted += u'"';
    for (const QChar character : path) {
        char16_t code = character.unicode();
        switch (code) {
        case u'"':
            quoted += QStringLiteral("\\\"");
            break;
        case u'\\':
            quoted += QStringLiteral("\\\\");
            break;
        case u'\n':
            quoted += QStringLiteral("\\n");
            break;
        case u'\r':
            quoted += QStringLiteral("\\r");
            break;
        case u'\t':
            quoted += QStringLiteral("\\t");
            break;
        default:
            quoted += character;
            break;
        }
    }
    quoted += u'"';
    return quoted;
}

} // namespace

AlgorithmError validateAlgorithms(const QStringList &algorithms, QString *message) {
    if (algorithms.isEmpty()) {
        if (message) {
            *message = QStringLiteral("at least one algorithm is required");
        }
        return AlgorithmError::NoAlgorithms;
    }
    QSet<QString> seen;
    seen.reserve(algorithms.size());
    for (const QString &name : algorithms) {
        if (name.isEmpty()) {
            if (message) {
                *message = QStringLiteral("algorithm name cannot be empty");
            }
            return AlgorithmError::EmptyAlgorithmName;
        }
        if (!lookupAlgorithm(name)) {
            if (message) {
                *message = QStringLiteral("unknown algorithm ") + quotePath(name);
            }
            return AlgorithmError::UnknownAlgorithm;
        }
        if (seen.contains(name)) {
            if (message) {
                *message = QStringLiteral("duplicate algorithm ") + quotePath(name);
            }
            return AlgorithmError::DuplicateAlgorithm;
        }
        seen.insert(name);
    }
    if (message) {
        message->clear();
    }
    return AlgorithmError::None;
}

HashOutcome hashFileWithOptions(const QString &path, const QStringList &algorithms,
                                const HashOptions &options) {
    QString error;
    if (validateAlgorithms(algorithms, &error) != AlgorithmError::None) {
        return HashOutcome{Result{}, error, false};
    }
    if (options.canceled && options.canceled()) {
        return HashOutcome{Result{}, QStringLiteral("context canceled"), true};
    }
    const qint64 bufferSize = options.bufferSize > 0 ? options.bufferSize : kDefaultBufferSize;

    // Open before inspecting: the returned handle is authoritative, so there
    // is no window between a path-based regular-file check and the read in
    // which the path could be replaced. Symbolic links are followed, matching
    // the CLI's path expansion; non-regular targets are rejected.
    PlatformFile file;
    if (!file.open(path, &error)) {
        return HashOutcome{Result{}, QStringLiteral("open ") + quotePath(path) + ": " + error,
                           false};
    }
    FileStat before;
    if (!file.stat(&before, &error)) {
        return HashOutcome{Result{}, QStringLiteral("stat ") + quotePath(path) + ": " + error,
                           false};
    }
    if (!before.regular) {
        return HashOutcome{Result{},
                           QStringLiteral("hash input ") + quotePath(path) +
                               QStringLiteral(" is not a regular file"),
                           false};
    }

    std::vector<std::unique_ptr<Digester>> digesters;
    digesters.reserve(algorithms.size());
    for (const QString &name : algorithms) {
        AlgorithmSpec spec;
        lookupAlgorithm(name, &spec);
        digesters.push_back(spec.create());
    }

    std::vector<char> buffer(size_t(bufferSize));
    qint64 bytesRead = 0;
    auto partial = [&path, &bytesRead]() {
        Result result;
        result.path = path;
        result.bytesRead = bytesRead;
        return result;
    };
    for (;;) {
        if (options.canceled && options.canceled()) {
            return HashOutcome{partial(), QStringLiteral("context canceled"), true};
        }
        qint64 count = file.read(buffer.data(), bufferSize, &error);
        if (count < 0) {
            return HashOutcome{partial(), QStringLiteral("read ") + quotePath(path) + ": " + error,
                               false};
        }
        if (count > 0) {
            for (std::unique_ptr<Digester> &digester : digesters) {
                digester->update(buffer.data(), count);
            }
            bytesRead += count;
            if (options.progress) {
                options.progress(bytesRead);
            }
        }
        if (count == 0) {
            break;
        }
    }

    FileStat after;
    if (!file.stat(&after, &error)) {
        return HashOutcome{partial(),
                           QStringLiteral("stat ") + quotePath(path) +
                               QStringLiteral(" after hashing: ") + error,
                           false};
    }
    FileStat pathAfter;
    if (!statPath(path, &pathAfter, &error)) {
        return HashOutcome{partial(),
                           QStringLiteral("stat ") + quotePath(path) +
                               QStringLiteral(" after hashing: ") + error,
                           false};
    }

    Result result;
    result.path = path;
    result.size = before.size;
    result.modified = fileStatTime(before);
    result.digests.reserve(algorithms.size());
    for (int index = 0; index < algorithms.size(); ++index) {
        result.digests.append(
            Digest{algorithms.at(index), QString::fromLatin1(digesters[size_t(index)]->digest())});
    }
    result.order = algorithms;
    result.bytesRead = bytesRead;
    result.changed =
        !sameFile(after, pathAfter) || before.size != after.size || before.mtimeMs != after.mtimeMs;
    return HashOutcome{result, QString(), false};
}

} // namespace hash_core
