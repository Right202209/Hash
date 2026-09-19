#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace hash_core {

// ToolSpec describes one toolbox entry shown on the home grid.
struct ToolSpec {
    QString id;
    QString name;
    QString description;
    QString icon;
    QString keywords;
};

// ToolRegistryModel lists every toolbox tool with a live case-insensitive
// filter over name, description and keywords. The list is static; only the
// filtered projection changes. Accessed from the GUI thread only.
class ToolRegistryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

  public:
    enum Roles {
        ToolIdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        IconRole,
        KeywordsRole,
    };

    explicit ToolRegistryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const;
    void setFilter(const QString &filter);

    // firstToolId returns the id of the first visible tool, or an empty
    // string when the filter matches nothing. The launcher uses it to open
    // the top hit when the user presses Enter in the command bar.
    Q_INVOKABLE QString firstToolId() const;

  signals:
    void filterChanged();

  private:
    void rebuildVisible();

    QVector<ToolSpec> mTools;
    QVector<int> mVisible;
    QString mFilter;
};

} // namespace hash_core
