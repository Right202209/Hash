#include "core/batch.h"

#include <QFileInfo>
#include <QThread>

#include <atomic>
#include <thread>
#include <vector>

namespace hash_core {
namespace {

// progressTracker serializes progress updates so that the shared counters
// stay consistent. The user callback is always invoked after the lock is
// released: a slow or re-entrant callback therefore cannot stall the workers
// or deadlock the batch. Callbacks may run concurrently with one another, so
// receivers must be safe for concurrent use.
class ProgressTracker {
  public:
    Progress update(const std::function<void(Progress &)> &mutate) {
        std::lock_guard<std::mutex> lock(m_mutex);
        mutate(m_progress);
        return m_progress;
    }

  private:
    std::mutex m_mutex;
    Progress m_progress;
};

void emitProgress(const std::function<void(const Progress &)> &callback, const Progress &progress) {
    if (callback) {
        callback(progress);
    }
}

} // namespace

QVector<FileResult> hashFiles(const QVector<FileRequest> &requests, const BatchOptions &options) {
    QVector<FileResult> results(requests.size());
    if (requests.isEmpty()) {
        return results;
    }

    int workers = options.workers;
    if (workers <= 0) {
        const int ideal = QThread::idealThreadCount();
        workers = qMin(4, ideal > 0 ? ideal : 1);
    }
    workers = int(qMin<qint64>(workers, requests.size()));

    qint64 totalBytes = 0;
    QVector<qint64> fileSizes(requests.size(), 0);
    for (int index = 0; index < requests.size(); ++index) {
        const QFileInfo info(requests.at(index).path);
        if (info.exists()) {
            fileSizes[index] = info.size();
            totalBytes += fileSizes.at(index);
        }
    }

    std::atomic<bool> localCancel{false};
    auto canceled = [&localCancel, &options]() {
        return localCancel.load(std::memory_order_relaxed) ||
               (options.canceled && options.canceled());
    };

    ProgressTracker tracker;
    const int totalFiles = requests.size();
    tracker.update([totalFiles, totalBytes](Progress &progress) {
        progress.totalFiles = totalFiles;
        progress.totalBytes = totalBytes;
    });

    std::atomic<int> nextIndex{0};
    auto workerLoop = [&]() {
        for (;;) {
            const int index = nextIndex.fetch_add(1, std::memory_order_relaxed);
            if (index >= requests.size()) {
                break;
            }
            const FileRequest &request = requests.at(index);
            if (canceled()) {
                FileResult &result = results[index];
                result.request = request;
                result.error = QStringLiteral("context canceled");
                result.canceled = true;
                continue;
            }
            emitProgress(options.progress,
                         tracker.update([&request, &fileSizes, index](Progress &progress) {
                             progress.currentPath = request.path;
                             progress.currentBytes = 0;
                             progress.currentSize = fileSizes.at(index);
                         }));

            HashOptions hashOptions = options.hash;
            hashOptions.canceled = canceled;
            hashOptions.progress = [&tracker, &callback = options.progress](qint64 bytesRead) {
                emitProgress(callback, tracker.update([bytesRead](Progress &progress) {
                    progress.currentBytes = bytesRead;
                }));
            };
            const HashOutcome outcome =
                hashFileWithOptions(request.path, request.algorithms, hashOptions);

            FileResult &result = results[index];
            result.request = request;
            result.result = outcome.result;
            result.error = outcome.error;
            result.canceled = outcome.canceled;
            emitProgress(options.progress, tracker.update([&outcome](Progress &progress) {
                progress.completedFiles += 1;
                progress.completedBytes += outcome.result.bytesRead;
                progress.currentBytes = 0;
            }));
            if (!outcome.error.isEmpty() && options.failFast) {
                localCancel.store(true, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(size_t(workers));
    for (int worker = 0; worker < workers; ++worker) {
        threads.emplace_back(workerLoop);
    }
    for (std::thread &thread : threads) {
        thread.join();
    }
    return results;
}

} // namespace hash_core
