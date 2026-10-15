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
        mSecures.append(spec.secure);
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
    case SecureRole:
        return mSecures.at(index.row());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AlgorithmsModel::roleNames() const {
    return {
        {NameRole, "name"},
        {LabelRole, "label"},
        {CategoryRole, "category"},
        {SecureRole, "secure"},
    };
}

QVariantList AlgorithmsModel::grouped() const {
    QStringList order;
    QHash<QString, QVariantList> buckets;
    for (int row = 0; row < mNames.size(); ++row) {
        const QString &category = mCategories.at(row);
        if (!buckets.contains(category)) {
            order.append(category);
            buckets.insert(category, QVariantList());
        }
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), mNames.at(row));
        entry.insert(QStringLiteral("label"), mLabels.at(row));
        entry.insert(QStringLiteral("category"), category);
        entry.insert(QStringLiteral("secure"), mSecures.at(row));
        buckets[category].append(entry);
    }

    QVariantList groups;
    for (const QString &category : order) {
        QVariantMap group;
        group.insert(QStringLiteral("category"), category);
        group.insert(QStringLiteral("algorithms"), buckets.value(category));
        groups.append(group);
    }
    return groups;
}

} // namespace hash_core
