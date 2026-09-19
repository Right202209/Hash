#pragma once

#include <QAbstractListModel>

namespace hash_core {

// AlgorithmsModel exposes the hash algorithm registry to the GUI, one row per
// registered algorithm in display order. Read-only; selection state lives in
// the views.
class AlgorithmsModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        CategoryRole,
    };

    explicit AlgorithmsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    QStringList mNames;
    QStringList mLabels;
    QStringList mCategories;
};

} // namespace hash_core
