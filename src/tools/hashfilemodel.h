#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace hash_core {

// FileStatus is the lifecycle of one queued file row.
enum class FileStatus { Pending, Hashing, Done, Error, Canceled };

// HashFileModel holds the file queue and, once a run finished, each file's
// digests and outcome. All mutations come from HashController on the GUI
// thread; the model only carries data for the views.
class HashFileModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        SizeRole,
        StatusRole,
        ChangedRole,
        ErrorRole,
        DigestsRole,
    };

    struct Row {
        QString path;
        QString name;
        qint64 size = 0;
        FileStatus status = FileStatus::Pending;
        bool changed = false;
        QString error;
        QVariantList digests;
    };

    explicit HashFileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QVector<Row> &rows() const { return mRows; }

    void resetRows(const QVector<Row> &rows);
    void appendRow(const Row &row);
    void removeRow(int row);
    void markStatus(int row, FileStatus status);
    // applyResult records the outcome of one file after a run.
    void applyResult(int row, FileStatus status, bool changed, const QVariantList &digests,
                     qint64 size, const QString &error);
    void clear();

  private:
    QVector<Row> mRows;
};

} // namespace hash_core
