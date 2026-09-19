#include "tools/hashcontroller.h"

#include "cli/format.h"
#include "common/atomicfile.h"
#include "common/compare.h"
#include "common/pathkey.h"
#include "core/progress.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSet>
#include <QThread>

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

HashController::HashController(QObject *parent) : QObject(parent), mFiles(new HashFileModel(this)) {
    qRegisterMetaType<Progress>("hash_core::Progress");
    qRegisterMetaType<QVector<FileResult>>("QVector<hash_core::FileResult>");

    mCancel = std::make_shared<std::atomic<bool>>(false);
    mThread = new QThread(this);
    mWorker = new BatchWorker();
    mWorker->setCancelFlag(mCancel);
    mWorker->moveToThread(mThread);
    connect(mThread, &QThread::finished, mWorker, &QObject::deleteLater);
    connect(mWorker, &BatchWorker::progressChanged, this, &HashController::onProgressChanged);
    connect(mWorker, &BatchWorker::runFinished, this, &HashController::onRunFinished);
    mThread->start();
    resetCompare();
}

HashController::~HashController() {
    mCancel->store(true, std::memory_order_relaxed);
    mThread->quit();
    mThread->wait();
}

QVariantMap HashController::addFiles(const QList<QUrl> &urls) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("added"), 0);
    outcome.insert(QStringLiteral("rejected"), 0);
    if (mBusy) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("计算中，请等待当前任务结束"));
        return outcome;
    }
    QSet<QString> seen;
    for (const HashFileModel::Row &row : mFiles->rows()) {
        seen.insert(pathKey(row.path));
    }
    int added = 0;
    int rejected = 0;
    for (const QUrl &url : urls) {
        const QString local = url.isLocalFile() ? url.toLocalFile() : url.toString();
        const QFileInfo info(local);
        if (!info.exists() || !info.isFile()) {
            ++rejected;
            continue;
        }
        const QString cleaned = QDir::cleanPath(local);
        const QString key = pathKey(cleaned);
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        HashFileModel::Row row;
        row.path = cleaned;
        row.name = info.fileName();
        row.size = info.size();
        mFiles->appendRow(row);
        mRowByKey.insert(key, mFiles->rows().size() - 1);
        ++added;
    }
    outcome.insert(QStringLiteral("added"), added);
    outcome.insert(QStringLiteral("rejected"), rejected);
    return outcome;
}

void HashController::removeRow(int row) {
    if (mBusy || row < 0 || row >= mFiles->rows().size()) {
        return;
    }
    mFiles->removeRow(row);
    rebuildIndex();
}

void HashController::clearFiles() {
    if (mBusy) {
        return;
    }
    mFiles->clear();
    mLastResults.clear();
    mOutputText.clear();
    mLastSummary.clear();
    mCurrentFile.clear();
    mProgress = 0;
    rebuildIndex();
    resetCompare();
    emit progressChanged();
    emit outputChanged();
}

void HashController::start(const QStringList &algorithms) {
    if (mBusy || algorithms.isEmpty() || mFiles->rows().isEmpty()) {
        return;
    }
    QVector<HashFileModel::Row> reset;
    reset.reserve(mFiles->rows().size());
    for (const HashFileModel::Row &row : mFiles->rows()) {
        HashFileModel::Row pending = row;
        pending.status = FileStatus::Pending;
        pending.changed = false;
        pending.error.clear();
        pending.digests.clear();
        reset.append(pending);
    }
    mFiles->resetRows(reset);
    rebuildIndex();

    QVector<FileRequest> requests;
    requests.reserve(reset.size());
    for (const HashFileModel::Row &row : reset) {
        requests.append(FileRequest{row.path, algorithms});
    }

    mLastResults.clear();
    mOutputText.clear();
    mCurrentFile.clear();
    mProgress = 0;
    mLastSummary.clear();
    resetCompare();
    setBusy(true);
    emit progressChanged();
    emit outputChanged();

    BatchWorker *worker = mWorker;
    QMetaObject::invokeMethod(
        worker, [worker, requests, algorithms]() { worker->run(requests, algorithms); });
}

void HashController::cancel() {
    if (mBusy) {
        mCancel->store(true, std::memory_order_relaxed);
    }
}

QString HashController::saveOutput(const QUrl &target) {
    if (mOutputText.isEmpty()) {
        return QStringLiteral("没有可保存的结果");
    }
    const QString content =
        QString(mOutputText).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    return writeAtomicFile(target.toLocalFile(), queuedPaths(), content.toUtf8());
}

void HashController::compareClipboard(const QString &text) {
    if (mLastResults.isEmpty()) {
        QVariantMap compare;
        compare.insert(QStringLiteral("valid"), false);
        compare.insert(QStringLiteral("error"), QStringLiteral("没有可用于比较的完成结果"));
        setCompare(compare);
        return;
    }
    QVector<CompareRecord> actual;
    QString error;
    if (!parseDigestText(text, &actual, &error)) {
        QVariantMap compare;
        compare.insert(QStringLiteral("valid"), false);
        compare.insert(QStringLiteral("error"), QStringLiteral("解析失败：") + error);
        setCompare(compare);
        return;
    }
    const CompareSummary summary = compareRecords(recordsFromResults(mLastResults), actual);
    QVariantMap compare;
    compare.insert(QStringLiteral("valid"), true);
    compare.insert(QStringLiteral("matches"), summary.matches);
    compare.insert(QStringLiteral("mismatches"), summary.mismatches);
    compare.insert(QStringLiteral("missing"), summary.missing);
    compare.insert(QStringLiteral("unexpected"), summary.unexpected);
    compare.insert(QStringLiteral("duplicates"), summary.duplicates);
    compare.insert(QStringLiteral("exact"), summary.exact);
    setCompare(compare);
}

void HashController::onProgressChanged(const Progress &progress) {
    mProgress = progressPercent(progress);
    mCurrentFile = progress.currentPath;
    const int row = mRowByKey.value(pathKey(progress.currentPath), -1);
    if (row >= 0) {
        mFiles->markStatus(row, FileStatus::Hashing);
    }
    emit progressChanged();
}

void HashController::onRunFinished(const QVector<FileResult> &results,
                                   const QStringList &algorithms) {
    int failures = 0;
    int changed = 0;
    bool canceled = false;
    for (int index = 0; index < results.size() && index < mFiles->rows().size(); ++index) {
        const FileResult &result = results.at(index);
        if (!result.error.isEmpty()) {
            if (result.canceled) {
                canceled = true;
            } else {
                ++failures;
            }
            mFiles->applyResult(index, result.canceled ? FileStatus::Canceled : FileStatus::Error,
                                false, QVariantList(), 0, result.error);
            continue;
        }
        if (result.result.changed) {
            ++changed;
        }
        QVariantList digests;
        digests.reserve(result.result.digests.size());
        for (const Digest &digest : result.result.digests) {
            QVariantMap entry;
            entry.insert(QStringLiteral("algorithm"), digest.algorithm);
            entry.insert(QStringLiteral("value"), digest.value);
            digests.append(entry);
        }
        mFiles->applyResult(index, FileStatus::Done, result.result.changed, digests,
                            result.result.size, QString());
    }

    QString output;
    QString formatError;
    if (!writeResults(results, algorithms, FormatOptions{QStringLiteral("text"), true, true, false},
                      &output, &formatError)) {
        output = formatError;
    }
    mOutputText = output;
    mLastResults = results;

    QVariantMap summary;
    summary.insert(QStringLiteral("files"), results.size());
    summary.insert(QStringLiteral("algorithms"), algorithms.size());
    summary.insert(QStringLiteral("failures"), failures);
    summary.insert(QStringLiteral("changed"), changed);
    summary.insert(QStringLiteral("canceled"), canceled);
    mLastSummary = summary;
    resetCompare();

    mProgress = canceled ? mProgress : 100;
    setBusy(false);
    emit progressChanged();
    emit outputChanged();
}

void HashController::setBusy(bool busy) {
    if (mBusy == busy) {
        return;
    }
    mBusy = busy;
    emit busyChanged();
}

void HashController::setCompare(const QVariantMap &compare) {
    mCompare = compare;
    emit compareChanged();
}

void HashController::resetCompare() {
    QVariantMap compare;
    compare.insert(QStringLiteral("valid"), false);
    compare.insert(QStringLiteral("error"), QString());
    setCompare(compare);
}

void HashController::rebuildIndex() {
    mRowByKey.clear();
    const QVector<HashFileModel::Row> rows = mFiles->rows();
    mRowByKey.reserve(rows.size());
    for (int index = 0; index < rows.size(); ++index) {
        mRowByKey.insert(pathKey(rows.at(index).path), index);
    }
}

QStringList HashController::queuedPaths() const {
    QStringList paths;
    const QVector<HashFileModel::Row> rows = mFiles->rows();
    paths.reserve(rows.size());
    for (const HashFileModel::Row &row : rows) {
        paths.append(row.path);
    }
    return paths;
}

} // namespace hash_core
