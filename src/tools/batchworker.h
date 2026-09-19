#pragma once

#include "core/batch.h"
#include "core/types.h"

#include <QElapsedTimer>
#include <QObject>
#include <atomic>
#include <memory>
#include <mutex>

namespace hash_core {

// BatchWorker owns the hashFiles call. It lives on a dedicated thread; the
// controller only touches it through queued invocations. cancelFlag is shared
// with the controller so cancel() never has to touch the worker object.
class BatchWorker : public QObject {
    Q_OBJECT

  public:
    void setCancelFlag(const std::shared_ptr<std::atomic<bool>> &cancel) { mCancel = cancel; }

  public slots:
    void run(QVector<FileRequest> requests, QStringList algorithms) {
        mCancel->store(false, std::memory_order_relaxed);

        BatchOptions options;
        options.canceled = [this]() { return mCancel->load(std::memory_order_relaxed); };

        // Worker threads report file changes and chunk progress. Forward at
        // most one snapshot per 100 ms to the GUI thread: the engine callback
        // may fire from every worker concurrently (hence the mutex), and an
        // unthrottled burst over a large queue would flood the event loop and
        // burn CPU on QML updates that users cannot perceive.
        QElapsedTimer throttle;
        throttle.start();
        qint64 lastEmit = -1000;
        std::mutex emitMutex;
        options.progress = [&](const Progress &progress) {
            {
                const std::lock_guard<std::mutex> lock(emitMutex);
                const qint64 elapsed = throttle.elapsed();
                if (elapsed - lastEmit < 100) {
                    return;
                }
                lastEmit = elapsed;
            }
            emit progressChanged(progress);
        };

        const QVector<FileResult> results = hashFiles(requests, options);
        emit runFinished(results, algorithms);
    }

  signals:
    void progressChanged(const Progress &progress);
    void runFinished(const QVector<FileResult> &results, const QStringList &algorithms);

  private:
    std::shared_ptr<std::atomic<bool>> mCancel;
};

} // namespace hash_core
