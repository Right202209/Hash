#pragma once

#include "core/batch.h"
#include "core/types.h"

#include <QMetaType>
#include <QObject>
#include <atomic>

class QThread;

Q_DECLARE_METATYPE(hash_core::Progress)
Q_DECLARE_METATYPE(QVector<hash_core::FileResult>)

// HashWorker owns the batch hashing call. It lives on a dedicated thread;
// progress snapshots and the final results reach the UI thread through
// queued signals, replacing the previous 100 ms Win32 timer polling.
class HashWorker : public QObject {
    Q_OBJECT

  public:
    explicit HashWorker(QObject *parent = nullptr);

    void cancel();

  public slots:
    void run(QVector<hash_core::FileRequest> requests, QStringList algorithms);

  signals:
    void progressChanged(const hash_core::Progress &progress);
    void runFinished(const QVector<hash_core::FileResult> &results, const QStringList &algorithms);

  private:
    std::atomic<bool> mCanceled{false};
};
