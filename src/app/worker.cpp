#include "app/worker.h"

#include <QElapsedTimer>

HashWorker::HashWorker(QObject *parent) : QObject(parent) {}

void HashWorker::cancel() { mCanceled.store(true, std::memory_order_relaxed); }

void HashWorker::run(QVector<hash_core::FileRequest> requests, QStringList algorithms) {
    mCanceled.store(false, std::memory_order_relaxed);

    hash_core::BatchOptions options;
    options.canceled = [this]() { return mCanceled.load(std::memory_order_relaxed); };

    // The engine reports progress per read chunk. Emit on every file change
    // and at most every 100 ms otherwise, which reproduces the update rate
    // of the previous timer-driven UI without flooding the event loop.
    QElapsedTimer throttle;
    throttle.start();
    qint64 lastEmit = -1000;
    QString lastPath;
    options.progress = [&](const hash_core::Progress &progress) {
        const bool pathChanged = progress.currentPath != lastPath;
        if (!pathChanged && throttle.elapsed() - lastEmit < 100) {
            return;
        }
        lastPath = progress.currentPath;
        lastEmit = throttle.elapsed();
        emit progressChanged(progress);
    };

    const QVector<hash_core::FileResult> results = hash_core::hashFiles(requests, options);
    emit runFinished(results, algorithms);
}
