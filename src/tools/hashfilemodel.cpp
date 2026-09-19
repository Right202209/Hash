#include "tools/hashfilemodel.h"

namespace hash_core {

HashFileModel::HashFileModel(QObject *parent) : QAbstractListModel(parent) {}

int HashFileModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : mRows.size();
}

QVariant HashFileModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    if (row < 0 || row >= mRows.size()) {
        return QVariant();
    }
    const Row &entry = mRows.at(row);
    switch (role) {
    case PathRole:
        return entry.path;
    case NameRole:
        return entry.name;
    case SizeRole:
        return entry.size;
    case StatusRole:
        return int(entry.status);
    case ChangedRole:
        return entry.changed;
    case ErrorRole:
        return entry.error;
    case DigestsRole:
        return entry.digests;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> HashFileModel::roleNames() const {
    return {
        {PathRole, "path"},       {NameRole, "name"},       {SizeRole, "size"},
        {StatusRole, "status"},   {ChangedRole, "changed"}, {ErrorRole, "error"},
        {DigestsRole, "digests"},
    };
}

void HashFileModel::resetRows(const QVector<Row> &rows) {
    beginResetModel();
    mRows = rows;
    endResetModel();
}

void HashFileModel::appendRow(const Row &row) {
    beginInsertRows(QModelIndex(), mRows.size(), mRows.size());
    mRows.append(row);
    endInsertRows();
}

void HashFileModel::removeRow(int row) {
    if (row < 0 || row >= mRows.size()) {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    mRows.removeAt(row);
    endRemoveRows();
}

void HashFileModel::markStatus(int row, FileStatus status) {
    if (row < 0 || row >= mRows.size() || mRows.at(row).status == status) {
        return;
    }
    mRows[row].status = status;
    const QModelIndex index = this->index(row);
    emit dataChanged(index, index, {StatusRole});
}

void HashFileModel::applyResult(int row, FileStatus status, bool changed,
                                const QVariantList &digests, qint64 size, const QString &error) {
    if (row < 0 || row >= mRows.size()) {
        return;
    }
    Row &entry = mRows[row];
    entry.status = status;
    entry.changed = changed;
    entry.digests = digests;
    entry.size = size;
    entry.error = error;
    const QModelIndex modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex,
                     {StatusRole, ChangedRole, ErrorRole, SizeRole, DigestsRole});
}

void HashFileModel::clear() {
    beginResetModel();
    mRows.clear();
    endResetModel();
}

} // namespace hash_core
