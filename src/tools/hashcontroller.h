#pragma once

#include "core/types.h"
#include "tools/hashfilemodel.h"

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <atomic>
#include <memory>

class QThread;

namespace hash_core {

class BatchWorker;

// HashController drives one batch hashing session from the GUI thread. The
// batch runs on a dedicated worker thread; progress snapshots and the final
// results arrive through queued signals. Queue mutations and start are
// rejected while busy; cancel is asynchronous and the run keeps its completed
// results.
class HashController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY progressChanged)
    Q_PROPERTY(QString outputText READ outputText NOTIFY outputChanged)
    Q_PROPERTY(bool hasOutput READ hasOutput NOTIFY outputChanged)
    Q_PROPERTY(QVariantMap lastSummary READ lastSummary NOTIFY outputChanged)
    Q_PROPERTY(QVariantMap compare READ compare NOTIFY compareChanged)
    Q_PROPERTY(QAbstractListModel *files READ files CONSTANT)

  public:
    explicit HashController(QObject *parent = nullptr);
    ~HashController() override;

    bool busy() const { return mBusy; }
    int progress() const { return mProgress; }
    QString currentFile() const { return mCurrentFile; }
    QString outputText() const { return mOutputText; }
    bool hasOutput() const { return !mOutputText.isEmpty(); }
    QVariantMap lastSummary() const { return mLastSummary; }
    QVariantMap compare() const { return mCompare; }
    QAbstractListModel *files() const { return mFiles; }

    // addFiles queues local files from dropped or picked URLs. Directories
    // and missing paths are rejected; duplicates are ignored. Returns
    // {added, rejected} and, when the controller is busy, {error}.
    Q_INVOKABLE QVariantMap addFiles(const QList<QUrl> &urls);
    Q_INVOKABLE void removeRow(int row);
    Q_INVOKABLE void clearFiles();
    // start hashes every queued file with algorithms (registry names).
    Q_INVOKABLE void start(const QStringList &algorithms);
    Q_INVOKABLE void cancel();
    // saveOutput writes the formatted result listing atomically to target,
    // never overwriting queued input files. Returns an empty string on
    // success, or the error message.
    Q_INVOKABLE QString saveOutput(const QUrl &target);
    // compareClipboard parses a digest listing and compares it with the last
    // run; the result is exposed through the compare property.
    Q_INVOKABLE void compareClipboard(const QString &text);
    Q_INVOKABLE void resetCompare();

  signals:
    void busyChanged();
    void progressChanged();
    void outputChanged();
    void compareChanged();

  private:
    void onProgressChanged(const Progress &progress);
    void onRunFinished(const QVector<FileResult> &results, const QStringList &algorithms);
    void setBusy(bool busy);
    void setCompare(const QVariantMap &compare);
    void rebuildIndex();
    QStringList queuedPaths() const;

    HashFileModel *mFiles = nullptr;
    QThread *mThread = nullptr;
    BatchWorker *mWorker = nullptr;
    std::shared_ptr<std::atomic<bool>> mCancel;
    QHash<QString, int> mRowByKey;
    QVector<FileResult> mLastResults;
    QVariantMap mLastSummary;
    QVariantMap mCompare;
    QString mOutputText;
    QString mCurrentFile;
    int mProgress = 0;
    bool mBusy = false;
};

} // namespace hash_core
