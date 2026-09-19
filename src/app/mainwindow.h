#pragma once

#include "common/compare.h"
#include "core/registry.h"
#include "core/types.h"

#include <QHash>
#include <QMainWindow>
#include <QVector>

class QCheckBox;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QThread;
class QTreeWidget;
class QTreeWidgetItem;
class HashWorker;

// MainWindow carries the single-window interface: file queue, algorithm
// selection, hashing progress and the clipboard comparison workflow. All
// window state is owned by the UI thread; the hashing worker communicates
// through signals.
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow();

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void addFilesFromDialog();
    void clearFiles();
    void startHashing();
    void cancelHashing();
    void copyOutput();
    void saveOutput();
    void pasteCompare();
    void clearComparison();
    void onProgressChanged(const hash_core::Progress &progress);
    void onRunFinished(const QVector<hash_core::FileResult> &results,
                       const QStringList &algorithms);

  private:
    enum class Phase { Idle, Ready, Running, Stopping, Complete, Canceled, Error };

    void buildLayout();
    void addPaths(const QStringList &paths);
    void rebuildQueue();
    void updateQueueRow(int index);
    void updateQueueProgress(const hash_core::Progress &progress);
    void updateStatus(const QString &text);
    void setOutput(const QString &text);
    void setResultSummary(const QString &text);
    void setProgress(int percent);
    void updateSelectionSummary();
    void updateStartState();
    void setBusy(bool busy);
    bool busy() const;
    QStringList selectedAlgorithms() const;
    QVector<hash_core::CompareRecord> completedRecords() const;

    QVector<hash_core::AlgorithmSpec> mAlgorithms;
    QVector<QCheckBox *> mAlgorithmChecks;
    QTreeWidget *mQueue = nullptr;
    QVector<QTreeWidgetItem *> mQueueRows;
    QPlainTextEdit *mOutput = nullptr;
    QProgressBar *mProgressBar = nullptr;
    QLabel *mStatus = nullptr;
    QLabel *mQueueInfo = nullptr;
    QLabel *mAlgorithmSummary = nullptr;
    QLabel *mResultSummary = nullptr;
    QLabel *mCompareStatus = nullptr;
    QPushButton *mAddButton = nullptr;
    QPushButton *mClearButton = nullptr;
    QPushButton *mStartButton = nullptr;
    QPushButton *mCopyButton = nullptr;
    QPushButton *mSaveButton = nullptr;
    QPushButton *mPasteCompareButton = nullptr;
    QPushButton *mClearCompareButton = nullptr;
    QPushButton *mCancelButton = nullptr;
    QWidget *mCentral = nullptr;

    QStringList mPaths;
    QStringList mRowStates;
    QVector<qint64> mFileSizes;
    QHash<QString, int> mRowIndexByPath;
    QString mRawOutput;
    QString mComparisonText;
    QStringList mAlgorithmsInUse;
    Phase mPhase = Phase::Idle;
    bool mClosing = false;

    struct CompletedRun {
        QVector<hash_core::FileResult> results;
        QString rawOutput;
        QVector<hash_core::CompareRecord> expectedRecords;
        int failures = 0;
        int changed = 0;
        bool canceled = false;
        bool formatError = false;
    };
    CompletedRun mLastRun;

    QThread *mWorkerThread = nullptr;
    HashWorker *mWorker = nullptr;
};
