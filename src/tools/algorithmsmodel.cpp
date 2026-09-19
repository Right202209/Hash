#include "tools/algorithmsmodel.h"

#include "core/registry.h"

namespace hash_core {

AlgorithmsModel::AlgorithmsModel(QObject *parent) : QAbstractListModel(parent) {
    const QVector<AlgorithmSpec> specs = algorithms();
    mNames.reserve(specs.size());
    mLabels.reserve(specs.size());
    mCategories.reserve(specs.size());
    for (const AlgorithmSpec &spec : specs) {
        mNames.append(spec.name);
        mLabels.append(spec.label);
        mCategories.append(spec.category);
    }
}

int AlgorithmsModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : mNames.size();
}

QVariant AlgorithmsModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= mNames.size()) {
        return QVariant();
    }
    switch (role) {
    case NameRole:
        return mNames.at(index.row());
    case LabelRole:
        return mLabels.at(index.row());
    case CategoryRole:
        return mCategories.at(index.row());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AlgorithmsModel::roleNames() const {
    return {
        {NameRole, "name"},
        {LabelRole, "label"},
        {CategoryRole, "category"},
    };
}

} // namespace hash_core
