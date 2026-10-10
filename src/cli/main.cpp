// Command hash is the command-line entry point for the hashing tool.
#include "cli/format.h"
#include "cli/options.h"
#include "cli/paths.h"
#include "common/atomicfile.h"
#include "common/quoting.h"
#include "core/batch.h"
#include "core/types.h"

#include <QCoreApplication>
#include <QStringList>

#include <atomic>
#include <csignal>
#include <cstdio>

namespace {

std::atomic<bool> gCanceled{false};

void writeStream(std::FILE *stream, const QString &text) {
    const QByteArray encoded = text.toUtf8();
    std::fwrite(encoded.constData(), 1, size_t(encoded.size()), stream);
}

// exitCodeForResults follows the documented contract: 3 only when the user
// interrupted the run (Ctrl+C or SIGTERM), otherwise 1 when any file failed.
// Files that fail-fast skipped count as failures, not cancellations, and the
// answer never depends on which file happened to come first.
int exitCodeForResults(const QVector<hash_core::FileResult> &results, bool interrupted) {
    if (interrupted) {
        return hash_core::ExitCanceled;
    }
    for (const hash_core::FileResult &result : results) {
        if (!result.error.isEmpty()) {
            return hash_core::ExitFailure;
        }
    }
    return hash_core::ExitSuccess;
}

} // namespace

extern "C" void handleSignal(int) { gCanceled.store(true, std::memory_order_relaxed); }

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    QStringList args = app.arguments();
    if (!args.isEmpty()) {
        args.removeFirst();
    }

    hash_core::Config config;
    QString error;
    if (!hash_core::parseArgs(args, &config, &error)) {
        writeStream(stderr, error + u'\n');
        writeStream(stderr, hash_core::usageText());
        return hash_core::ExitUsage;
    }
    if (config.showHelp) {
        writeStream(stdout, hash_core::usageText());
        return hash_core::ExitSuccess;
    }
    if (config.showVersion) {
        writeStream(stdout, QStringLiteral("hash " HASH_VERSION "\n"));
        return hash_core::ExitSuccess;
    }

    QStringList algorithms;
    if (!hash_core::selectAlgorithms(config, &algorithms, &error)) {
        writeStream(stderr, error + u'\n');
        return hash_core::ExitUsage;
    }

    QStringList paths;
    if (!hash_core::expandPaths(config.paths,
                                hash_core::ExpandOptions{config.recursive, config.literal}, &paths,
                                &error)) {
        writeStream(stderr, error + u'\n');
        return hash_core::ExitUsage;
    }

    if (!config.output.isEmpty()) {
        // Reject a bad destination before spending time hashing;
        // writeAtomicFile re-validates before committing, so this early
        // check is purely fail-fast.
        error = hash_core::validateOutputFile(config.output, paths);
        if (!error.isEmpty()) {
            writeStream(stderr, error + u'\n');
            return hash_core::ExitFailure;
        }
    }

    QVector<hash_core::FileRequest> requests;
    requests.reserve(paths.size());
    for (const QString &path : paths) {
        requests.append(hash_core::FileRequest{path, algorithms});
    }

    hash_core::BatchOptions options;
    options.workers = config.workers;
    options.failFast = config.failFast;
    options.canceled = []() { return gCanceled.load(std::memory_order_relaxed); };
    const QVector<hash_core::FileResult> results = hash_core::hashFiles(requests, options);

    QString output;
    if (!hash_core::writeResults(results, algorithms,
                                 hash_core::FormatOptions{config.format, config.showSize,
                                                          config.showModified, config.utc},
                                 &output, &error)) {
        writeStream(stderr, error + u'\n');
        return hash_core::ExitFailure;
    }

    if (config.output.isEmpty()) {
        writeStream(stdout, output);
    } else {
        error = hash_core::writeAtomicFile(config.output, paths, output.toUtf8());
        if (!error.isEmpty()) {
            writeStream(stderr, QStringLiteral("write output: ") + error + u'\n');
            return hash_core::ExitFailure;
        }
    }

    return exitCodeForResults(results, gCanceled.load(std::memory_order_relaxed));
}
