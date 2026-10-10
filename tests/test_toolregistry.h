#pragma once

#include "tools/toolregistrymodel.h"

#include <QSet>
#include <QStringList>
#include <QtTest>

class TestToolRegistry : public QObject {
    Q_OBJECT

  private:
    static QString valueAt(const hash_core::ToolRegistryModel &model, int row, int role) {
        return model.data(model.index(row), role).toString();
    }

    static QStringList visibleIds(const hash_core::ToolRegistryModel &model) {
        QStringList ids;
        for (int row = 0; row < model.rowCount(); ++row) {
            ids.append(valueAt(model, row, hash_core::ToolRegistryModel::ToolIdRole));
        }
        return ids;
    }

  private slots:
    void exposesCategoryRoleToQml() {
        hash_core::ToolRegistryModel model;
        const QHash<int, QByteArray> roles = model.roleNames();
        QCOMPARE(roles.value(hash_core::ToolRegistryModel::ToolIdRole), QByteArray("toolId"));
        QCOMPARE(roles.value(hash_core::ToolRegistryModel::CategoryRole), QByteArray("category"));
    }

    void everyToolHasContiguousCategory() {
        hash_core::ToolRegistryModel model;
        QCOMPARE(model.rowCount(), 10);
        QSet<QString> closed;
        QString current;
        for (int row = 0; row < model.rowCount(); ++row) {
            const QString category =
                valueAt(model, row, hash_core::ToolRegistryModel::CategoryRole);
            QVERIFY2(!category.isEmpty(),
                     qPrintable(QStringLiteral("row %1 has no category").arg(row)));
            if (category == current) {
                continue;
            }
            QVERIFY2(!closed.contains(category),
                     qPrintable(QStringLiteral("category %1 is split").arg(category)));
            if (!current.isEmpty()) {
                closed.insert(current);
            }
            current = category;
        }
    }

    void filterMatchesCategory() {
        hash_core::ToolRegistryModel model;
        model.setFilter(QStringLiteral("编解码"));
        QCOMPARE(visibleIds(model), QStringList({QStringLiteral("base64"), QStringLiteral("url"),
                                                 QStringLiteral("json")}));
        QCOMPARE(model.firstToolId(), QStringLiteral("base64"));
    }

    void filterWithoutMatchIsEmpty() {
        hash_core::ToolRegistryModel model;
        model.setFilter(QStringLiteral("no-such-tool"));
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.firstToolId().isEmpty());
        model.setFilter(QString());
        QCOMPARE(model.rowCount(), 10);
    }
};
