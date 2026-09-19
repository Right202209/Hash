#include "app/mainwindow.h"

#include "app/theme.h"
#include "app/worker.h"
#include "cli/format.h"
#include "common/atomicfile.h"
#include "common/pathkey.h"
#include "core/progress.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QThread>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kMaxClipboardBytes = 16 * 1024 * 1024;

// formatSize renders byte counts as B through TB with one decimal above B.
QString formatSize(qint64 value) {
    static const char *kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double size = double(value);
    int unit = 0;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    if (unit == 0) {
        return QString::number(value) + QStringLiteral(" B");
    }
    return QString::number(size, 'f', 1) + QStringLiteral(" ") + QString::fromLatin1(kUnits[unit]);
}

// shortenPath collapses long paths into head + ellipsis + tail.
QString shortenPath(const QString &path, int limit) {
    if (path.size() <= limit) {
        return path;
    }
    if (limit < 8) {
        return path.left(limit);
    }
    return path.left(limit / 2 - 2) + QStringLiteral("…") + path.right(limit / 2 - 1);
}

QString formatComparisonSummary(const hash_core::CompareSummary &summary) {
    return QStringLiteral("匹配 %1 · 不匹配 %2 · 缺少 %3 · 多余 %4 · 重复 %5")
        .arg(summary.matches)
        .arg(summary.mismatches)
        .arg(summary.missing)
        .arg(summary.unexpected)
        .arg(summary.duplicates);
}

} // namespace

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("Hash / 文件完整性"));
    resize(1080, 760);
    setAcceptDrops(true);

    mAlgorithms = hash_core::algorithms();

    buildLayout();

    // sha256 is the second entry in display order, matching the previous
    // default selection.
    if (mAlgorithmChecks.size() > 1) {
        mAlgorithmChecks.at(1)->setChecked(true);
    }

    mWorkerThread = new QThread(this);
    mWorker = new HashWorker();
    mWorker->moveToThread(mWorkerThread);
    connect(mWorkerThread, &QThread::finished, mWorker, &QObject::deleteLater);
    connect(mWorker, &HashWorker::progressChanged, this, &MainWindow::onProgressChanged);
    connect(mWorker, &HashWorker::runFinished, this, &MainWindow::onRunFinished);
    mWorkerThread->start();

    setBusy(false);
    rebuildQueue();
    updateSelectionSummary();
}

void MainWindow::buildLayout() {
    mCentral = new QWidget(this);
    setCentralWidget(mCentral);

    auto *title = new QLabel(QStringLiteral("HASH / 文件完整性"), mCentral);
    QFont titleFont = title->font();
    titleFont.setPixelSize(22);
    title->setFont(titleFont);
    auto *subtitle = new QLabel(QStringLiteral("拖放文件到工作区，选择算法后开始校验"), mCentral);
    subtitle->setProperty("class", QStringLiteral("muted"));

    mAddButton = new QPushButton(QStringLiteral("添加文件"), mCentral);
    mClearButton = new QPushButton(QStringLiteral("清空"), mCentral);
    mStartButton = new QPushButton(QStringLiteral("开始计算"), mCentral);
    mCopyButton = new QPushButton(QStringLiteral("复制"), mCentral);
    mSaveButton = new QPushButton(QStringLiteral("保存"), mCentral);
    mPasteCompareButton = new QPushButton(QStringLiteral("粘贴对比"), mCentral);
    mClearCompareButton = new QPushButton(QStringLiteral("清除对比"), mCentral);
    mCancelButton = new QPushButton(QStringLiteral("取消"), mCentral);

    connect(mAddButton, &QPushButton::clicked, this, &MainWindow::addFilesFromDialog);
    connect(mClearButton, &QPushButton::clicked, this, &MainWindow::clearFiles);
    connect(mStartButton, &QPushButton::clicked, this, &MainWindow::startHashing);
    connect(mCopyButton, &QPushButton::clicked, this, &MainWindow::copyOutput);
    connect(mSaveButton, &QPushButton::clicked, this, &MainWindow::saveOutput);
    connect(mPasteCompareButton, &QPushButton::clicked, this, &MainWindow::pasteCompare);
    connect(mClearCompareButton, &QPushButton::clicked, this, &MainWindow::clearComparison);
    connect(mCancelButton, &QPushButton::clicked, this, &MainWindow::cancelHashing);

    auto *header = new QHBoxLayout();
    auto *headerText = new QVBoxLayout();
    headerText->addWidget(title);
    headerText->addWidget(subtitle);
    header->addLayout(headerText);
    header->addStretch(1);
    header->addWidget(mAddButton);
    header->addWidget(mClearButton);
    header->addWidget(mStartButton);

    // Left panel: the algorithm selection grid.
    auto *algorithmPanel = new QFrame(mCentral);
    algorithmPanel->setObjectName(QStringLiteral("accentPanel"));
    auto *algorithmLayout = new QVBoxLayout(algorithmPanel);
    algorithmLayout->setContentsMargins(16, 16, 16, 16);
    auto *algorithmTitle = new QLabel(QStringLiteral("ALGORITHMS"), algorithmPanel);
    algorithmTitle->setProperty("class", QStringLiteral("ice"));
    algorithmLayout->addWidget(algorithmTitle);
    auto *algorithmGrid = new QGridLayout();
    algorithmGrid->setHorizontalSpacing(12);
    algorithmGrid->setVerticalSpacing(4);
    mAlgorithmChecks.reserve(mAlgorithms.size());
    for (int index = 0; index < mAlgorithms.size(); ++index) {
        auto *check = new QCheckBox(mAlgorithms.at(index).label, algorithmPanel);
        connect(check, &QCheckBox::toggled, this, &MainWindow::updateSelectionSummary);
        mAlgorithmChecks.append(check);
        algorithmGrid->addWidget(check, index / 2, index % 2);
    }
    algorithmLayout->addLayout(algorithmGrid);
    algorithmLayout->addStretch(1);
    mAlgorithmSummary = new QLabel(QString(), algorithmPanel);

    algorithmLayout->addWidget(mAlgorithmSummary);

    // Right side: the file queue and the result output.
    auto *queuePanel = new QFrame(mCentral);
    queuePanel->setObjectName(QStringLiteral("accentPanel"));
    auto *queueLayout = new QVBoxLayout(queuePanel);
    queueLayout->setContentsMargins(16, 16, 16, 16);
    auto *queueTitle = new QLabel(QStringLiteral("文件队列"), queuePanel);
    mQueueInfo = new QLabel(QString(), queuePanel);
    mQueueInfo->setProperty("class", QStringLiteral("muted"));
    mQueue = new QTreeWidget(queuePanel);
    mQueue->setColumnCount(3);
    mQueue->setHeaderLabels(
        {QStringLiteral("文件"), QStringLiteral("大小"), QStringLiteral("状态")});
    mQueue->setRootIsDecorated(false);
    mQueue->setAllColumnsShowFocus(true);
    mQueue->setSelectionMode(QAbstractItemView::SingleSelection);
    mQueue->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    mQueue->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    mQueue->header()->setSectionResizeMode(2, QHeaderView::Fixed);
    mQueue->setColumnWidth(1, 92);
    mQueue->setColumnWidth(2, 120);
    queueLayout->addWidget(queueTitle);
    queueLayout->addWidget(mQueueInfo);
    queueLayout->addWidget(mQueue, 1);

    auto *resultPanel = new QFrame(mCentral);
    resultPanel->setObjectName(QStringLiteral("accentPanel"));
    auto *resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(16, 16, 16, 16);
    auto *resultHeader = new QHBoxLayout();
    auto *resultTitle = new QLabel(QStringLiteral("结果 / HASH OUTPUT"), resultPanel);
    resultTitle->setProperty("class", QStringLiteral("ice"));
    resultHeader->addWidget(resultTitle);
    resultHeader->addStretch(1);
    resultHeader->addWidget(mCopyButton);
    resultHeader->addWidget(mSaveButton);
    resultHeader->addWidget(mPasteCompareButton);
    resultHeader->addWidget(mClearCompareButton);
    mResultSummary = new QLabel(QString(), resultPanel);
    mCompareStatus = new QLabel(QString(), resultPanel);
    mCompareStatus->setProperty("class", QStringLiteral("muted"));
    mOutput = new QPlainTextEdit(resultPanel);
    mOutput->setReadOnly(true);
    resultLayout->addLayout(resultHeader);
    resultLayout->addWidget(mResultSummary);
    resultLayout->addWidget(mCompareStatus);
    resultLayout->addWidget(mOutput, 1);

    auto *right = new QVBoxLayout();
    right->addWidget(queuePanel, 2);
    right->addWidget(resultPanel, 3);
    auto *body = new QHBoxLayout();
    body->addWidget(algorithmPanel, 0);
    body->addLayout(right, 1);

    auto *bottom = new QHBoxLayout();
    mStatus = new QLabel(QString(), mCentral);
    mProgressBar = new QProgressBar(mCentral);
    mProgressBar->setRange(0, 100);
    mProgressBar->setTextVisible(false);
    mProgressBar->setFixedWidth(360);
    bottom->addWidget(mStatus);
    bottom->addWidget(mProgressBar, 1);
    bottom->addWidget(mCancelButton);

    auto *root = new QVBoxLayout(mCentral);
    root->setContentsMargins(24, 18, 24, 18);
    root->setSpacing(16);
    root->addLayout(header);
    root->addLayout(body, 1);
    root->addLayout(bottom);

    mCentral->setStyleSheet(hash_ui::panelStyleSheet());
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QStringList paths;
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
        }
    }
    if (!paths.isEmpty()) {
        addPaths(paths);
    }
    event->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (busy()) {
        if (!mClosing) {
            mClosing = true;
            mPhase = Phase::Stopping;
            updateStatus(QStringLiteral("正在停止……"));
            mWorker->cancel();
        }
        event->ignore();
        return;
    }
    mWorkerThread->quit();
    mWorkerThread->wait();
    event->accept();
}

void MainWindow::addFilesFromDialog() {
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, QStringLiteral("添加文件"), QString(), QStringLiteral("所有文件 (*.*)"));
    if (!paths.isEmpty()) {
        addPaths(paths);
    }
}

void MainWindow::clearFiles() {
    if (busy()) {
        return;
    }
    mPaths.clear();
    mRowStates.clear();
    mFileSizes.clear();
    mRowIndexByPath.clear();
    mLastRun = CompletedRun{};
    clearComparison();
    mPhase = Phase::Idle;
    setOutput(QStringLiteral("结果会显示在这里。\n选择文件和算法后，点击“开始计算”。"));
    setResultSummary(QStringLiteral("结果摘要：尚未计算"));
    rebuildQueue();
    updateStatus(QStringLiteral("就绪 · 尚未添加文件"));
    updateStartState();
}

void MainWindow::addPaths(const QStringList &paths) {
    if (busy()) {
        updateStatus(QStringLiteral("计算中 · 请等待当前任务结束"));
        return;
    }
    QSet<QString> seen;
    for (const QString &path : mPaths) {
        seen.insert(hash_core::pathKey(path));
    }
    int rejected = 0;
    int added = 0;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists() || info.isDir() || !info.isFile()) {
            ++rejected;
            continue;
        }
        const QString cleaned = QDir::cleanPath(path);
        const QString key = hash_core::pathKey(cleaned);
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        mPaths.append(cleaned);
        mRowStates.append(QStringLiteral("待计算"));
        mFileSizes.append(info.size());
        ++added;
    }
    if (added > 0) {
        mLastRun = CompletedRun{};
        mPhase = Phase::Ready;
        setOutput(QStringLiteral("结果会显示在这里。\n选择文件和算法后，点击“开始计算”。"));
        clearComparison();
        setResultSummary(QStringLiteral("结果摘要：尚未计算"));
    }
    rebuildQueue();
    updateStartState();
    if (rejected > 0) {
        updateStatus(
            QStringLiteral("已添加 %1 个文件 · 忽略 %2 项").arg(mPaths.size()).arg(rejected));
    } else {
        updateStatus(QStringLiteral("就绪 · %1 个文件").arg(mPaths.size()));
    }
}

void MainWindow::startHashing() {
    if (busy() || mPaths.isEmpty()) {
        updateStartState();
        return;
    }
    const QStringList selected = selectedAlgorithms();
    if (selected.isEmpty()) {
        updateStatus(QStringLiteral("请至少选择一个算法"));
        return;
    }
    QVector<hash_core::FileRequest> requests;
    requests.reserve(mPaths.size());
    for (int index = 0; index < mPaths.size(); ++index) {
        requests.append(hash_core::FileRequest{mPaths.at(index), selected});
        mRowStates[index] = QStringLiteral("待计算");
        updateQueueRow(index);
    }
    mAlgorithmsInUse = selected;
    mLastRun = CompletedRun{};
    mClosing = false;
    clearComparison();
    mPhase = Phase::Running;
    setBusy(true);
    setOutput(QString());
    setResultSummary(QStringLiteral("结果摘要：正在计算……"));
    updateStatus(QStringLiteral("计算中 · %1 个文件").arg(mPaths.size()));
    setProgress(0);
    hash_ui::markRunning(mCentral, true);
    HashWorker *worker = mWorker;
    QMetaObject::invokeMethod(mWorker,
                              [worker, requests, selected]() { worker->run(requests, selected); });
}

void MainWindow::cancelHashing() {
    if (mPhase == Phase::Running || mPhase == Phase::Stopping) {
        mPhase = Phase::Stopping;
        updateStatus(QStringLiteral("正在停止……"));
        mWorker->cancel();
    }
}

void MainWindow::onProgressChanged(const hash_core::Progress &progress) {
    const int percent = hash_core::progressPercent(progress);
    setProgress(percent);
    updateQueueProgress(progress);
    updateStatus(QStringLiteral("计算中 · %1% · %2")
                     .arg(percent)
                     .arg(shortenPath(progress.currentPath, 62)));
}

void MainWindow::onRunFinished(const QVector<hash_core::FileResult> &results,
                               const QStringList &algorithms) {
    CompletedRun run;
    run.results = results;
    run.expectedRecords = hash_core::recordsFromResults(results);
    for (const hash_core::FileResult &result : results) {
        if (!result.error.isEmpty()) {
            if (result.canceled) {
                run.canceled = true;
            } else {
                run.failures += 1;
            }
            continue;
        }
        if (result.result.changed) {
            run.changed += 1;
        }
    }
    QString formatted;
    QString formatError;
    if (!hash_core::writeResults(
            results, algorithms,
            hash_core::FormatOptions{QStringLiteral("text"), true, true, false}, &formatted,
            &formatError)) {
        run.formatError = true;
        run.rawOutput = formatError;
    } else {
        run.rawOutput = formatted;
    }
    mLastRun = run;
    hash_ui::markRunning(mCentral, false);

    if (run.formatError) {
        mPhase = Phase::Error;
        setBusy(false);
        setResultSummary(QStringLiteral("结果摘要：生成结果失败"));
        updateStatus(QStringLiteral("结果生成失败：") + run.rawOutput);
        if (mClosing) {
            close();
        }
        return;
    }
    for (int index = 0; index < run.results.size(); ++index) {
        const hash_core::FileResult &result = run.results.at(index);
        QString state = QStringLiteral("已完成");
        if (!result.error.isEmpty()) {
            state = result.canceled ? QStringLiteral("已取消") : QStringLiteral("错误");
        } else if (result.result.changed) {
            state = QStringLiteral("已完成 · 已变化");
        }
        if (index < mRowStates.size()) {
            mRowStates[index] = state;
        }
        if (index < mFileSizes.size() && result.error.isEmpty()) {
            mFileSizes[index] = result.result.size;
        }
        updateQueueRow(index);
    }
    setOutput(run.rawOutput);
    setResultSummary(
        QStringLiteral("结果摘要：%1 个文件 · %2 个算法 · %3 个错误 · %4 个计算期间变化")
            .arg(run.results.size())
            .arg(mAlgorithmsInUse.size())
            .arg(run.failures)
            .arg(run.changed));
    if (run.canceled) {
        mPhase = Phase::Canceled;
    } else if (run.failures > 0 || run.changed > 0) {
        mPhase = Phase::Error;
    } else {
        mPhase = Phase::Complete;
    }
    setBusy(false);
    if (run.canceled) {
        updateStatus(QStringLiteral("已取消 · 已保留已完成结果"));
    } else if (run.failures > 0) {
        updateStatus(QStringLiteral("完成 · %1 个文件失败").arg(run.failures));
    } else if (run.changed > 0) {
        updateStatus(QStringLiteral("完成 · %1 个文件在计算期间发生变化").arg(run.changed));
    } else {
        updateStatus(QStringLiteral("已完成 · %1 个文件").arg(run.results.size()));
    }
    if (!run.canceled) {
        setProgress(100);
    }
    updateStartState();
    if (mClosing) {
        close();
    }
}

void MainWindow::updateQueueProgress(const hash_core::Progress &progress) {
    const auto found = mRowIndexByPath.constFind(hash_core::pathKey(progress.currentPath));
    if (found == mRowIndexByPath.constEnd()) {
        return;
    }
    const int index = found.value();
    if (index >= mRowStates.size() || mRowStates.at(index) == QStringLiteral("计算中")) {
        return;
    }
    mRowStates[index] = QStringLiteral("计算中");
    updateQueueRow(index);
}

void MainWindow::rebuildQueue() {
    mQueueRows.clear();
    mQueue->clear();
    mRowIndexByPath.clear();
    for (int index = 0; index < mPaths.size(); ++index) {
        auto *item = new QTreeWidgetItem(mQueue);
        item->setText(0, mPaths.at(index));
        mQueue->addTopLevelItem(item);
        mQueueRows.append(item);
        mRowIndexByPath.insert(hash_core::pathKey(mPaths.at(index)), index);
        updateQueueRow(index);
    }
    if (mPaths.isEmpty()) {
        mQueueInfo->setText(QStringLiteral("拖放文件到这里，或点击添加文件"));
    } else {
        mQueueInfo->setText(QStringLiteral("%1 个文件 · 单次读取，多算法并行").arg(mPaths.size()));
    }
}

void MainWindow::updateQueueRow(int index) {
    if (index < 0 || index >= mPaths.size() || index >= mQueueRows.size()) {
        return;
    }
    const qint64 size = index < mFileSizes.size() ? mFileSizes.at(index) : 0;
    const QString state =
        index < mRowStates.size() ? mRowStates.at(index) : QStringLiteral("待计算");
    mQueueRows.at(index)->setText(1, formatSize(size));
    mQueueRows.at(index)->setText(2, state);
}

void MainWindow::updateStatus(const QString &text) { mStatus->setText(text); }

void MainWindow::setOutput(const QString &text) {
    mRawOutput = text;
    mOutput->setPlainText(text);
}

void MainWindow::setResultSummary(const QString &text) { mResultSummary->setText(text); }

void MainWindow::setProgress(int percent) { mProgressBar->setValue(qBound(0, percent, 100)); }

void MainWindow::updateSelectionSummary() {
    mAlgorithmSummary->setText(QStringLiteral("已选择 %1 个算法").arg(selectedAlgorithms().size()));
    updateStartState();
}

QStringList MainWindow::selectedAlgorithms() const {
    QStringList selected;
    selected.reserve(mAlgorithmChecks.size());
    for (int index = 0; index < mAlgorithmChecks.size(); ++index) {
        if (mAlgorithmChecks.at(index)->isChecked()) {
            selected.append(mAlgorithms.at(index).name);
        }
    }
    return selected;
}

bool MainWindow::busy() const { return mPhase == Phase::Running || mPhase == Phase::Stopping; }

void MainWindow::setBusy(bool busyState) {
    for (QPushButton *button : {mAddButton, mClearButton, mStartButton, mCopyButton, mSaveButton,
                                mPasteCompareButton, mClearCompareButton}) {
        button->setEnabled(!busyState);
    }
    for (QCheckBox *check : mAlgorithmChecks) {
        check->setEnabled(!busyState);
    }
    mCancelButton->setEnabled(busyState);
    if (!busyState) {
        updateStartState();
    }
}

void MainWindow::updateStartState() {
    const bool hasOutput =
        mPhase == Phase::Complete || mPhase == Phase::Error || mPhase == Phase::Canceled;
    mStartButton->setEnabled(!busy() && !mPaths.isEmpty() && !selectedAlgorithms().isEmpty());
    mCopyButton->setEnabled(hasOutput && !mRawOutput.isEmpty());
    mSaveButton->setEnabled(hasOutput && !mRawOutput.isEmpty());
    mPasteCompareButton->setEnabled(hasOutput && !completedRecords().isEmpty());
    mClearCompareButton->setEnabled(hasOutput && !mComparisonText.isEmpty());
}

QVector<hash_core::CompareRecord> MainWindow::completedRecords() const {
    return mLastRun.expectedRecords;
}

void MainWindow::copyOutput() {
    if (mRawOutput.isEmpty()) {
        return;
    }
    if (mRawOutput.size() * 2 > kMaxClipboardBytes) {
        updateStatus(
            QStringLiteral("结果超过 %1 MiB 限制，无法复制").arg(kMaxClipboardBytes >> 20));
        return;
    }
    QApplication::clipboard()->setText(mRawOutput);
    updateStatus(QStringLiteral("结果已复制"));
}

void MainWindow::saveOutput() {
    if (mRawOutput.isEmpty()) {
        return;
    }
    const QString path =
        QFileDialog::getSaveFileName(this, QStringLiteral("保存哈希结果"), QString(),
                                     QStringLiteral("文本文件 (*.txt);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        return;
    }
    const QString content =
        QString(mRawOutput).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    const QString error = hash_core::writeAtomicFile(path, mPaths, content.toUtf8());
    if (!error.isEmpty()) {
        updateStatus(QStringLiteral("保存失败：") + error);
        return;
    }
    updateStatus(QStringLiteral("结果已保存"));
}

void MainWindow::pasteCompare() {
    const QVector<hash_core::CompareRecord> expected = completedRecords();
    if (expected.isEmpty()) {
        updateStatus(QStringLiteral("没有可用于比较的完成结果"));
        return;
    }
    const QString text = QApplication::clipboard()->text();
    QVector<hash_core::CompareRecord> actual;
    QString error;
    if (!hash_core::parseDigestText(text, &actual, &error)) {
        mComparisonText = QStringLiteral("比较失败：") + error;
        mCompareStatus->setText(QStringLiteral("剪贴板比较：") + mComparisonText);
        updateStartState();
        return;
    }
    const hash_core::CompareSummary summary = hash_core::compareRecords(expected, actual);
    mComparisonText = formatComparisonSummary(summary);
    mCompareStatus->setText(QStringLiteral("剪贴板比较：") + mComparisonText);
    updateStatus(QStringLiteral("已完成剪贴板比较"));
    updateStartState();
}

void MainWindow::clearComparison() {
    mComparisonText.clear();
    mCompareStatus->setText(QStringLiteral("剪贴板比较：未比较"));
    updateStartState();
}
