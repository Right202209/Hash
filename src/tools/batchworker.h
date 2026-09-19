#pragma once

#include "core/batch.h"
#include "core/types.h"

#include <QElapsedTimer>
#include <QObject>
#include <atomic>
#include <memory>

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

        // The engine reports progress per read chunk. Emit on every file
        // change and at most every 100 ms otherwise, which keeps the UI
        // responsive without flooding the event loop.
        QElapsedTimer throttle;
        throttle.start();
        qint64 lastEmit = -1000;
        QString lastPath;
        options.progress = [&](const Progress &progress) {
            const bool pathChanged = progress.currentPath != lastPath;
            if (!pathChanged && throttle.elapsed() - lastEmit < 100) {
                return;
            }
            lastPath = progress.currentPath;
            lastEmit = throttle.elapsed();
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
