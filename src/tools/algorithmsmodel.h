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
        // SecureRole mirrors AlgorithmSpec::secure: false marks an algorithm
        // that must not be relied on for security (md5, sha1, checksums, FNV).
        // It is the single registry-sourced flag the GUI renders as a warning
        // marker, so the registry stays the only place it is declared.
        SecureRole,
    };

    explicit AlgorithmsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // grouped returns the registry bucketed by category, preserving registry
    // order, so the QML chips can render category headings without a second
    // model or fragile sibling-index lookups.
    Q_INVOKABLE QVariantList grouped() const;

  private:
    QStringList mNames;
    QStringList mLabels;
    QStringList mCategories;
    QVector<bool> mSecures;
};

} // namespace hash_core
