#pragma once

#include "core/registry.h"
#include "tools/algorithmsmodel.h"

#include <QtTest>

class TestAlgorithmsModel : public QObject {
    Q_OBJECT

  private slots:
    void mirrorsRegistryInOrder() {
        hash_core::AlgorithmsModel model;
        const QVector<hash_core::AlgorithmSpec> specs = hash_core::algorithms();
        QVERIFY(!specs.isEmpty());
        QCOMPARE(model.rowCount(), int(specs.size()));
        for (int row = 0; row < specs.size(); ++row) {
            const QModelIndex index = model.index(row);
            QCOMPARE(model.data(index, hash_core::AlgorithmsModel::NameRole).toString(),
                     specs.at(row).name);
            QCOMPARE(model.data(index, hash_core::AlgorithmsModel::LabelRole).toString(),
                     specs.at(row).label);
            QCOMPARE(model.data(index, hash_core::AlgorithmsModel::CategoryRole).toString(),
                     specs.at(row).category);
        }
    }

    void exposesRoleNamesToQml() {
        hash_core::AlgorithmsModel model;
        const QHash<int, QByteArray> roles = model.roleNames();
        QCOMPARE(roles.value(hash_core::AlgorithmsModel::NameRole), QByteArray("name"));
        QCOMPARE(roles.value(hash_core::AlgorithmsModel::LabelRole), QByteArray("label"));
        QCOMPARE(roles.value(hash_core::AlgorithmsModel::CategoryRole), QByteArray("category"));
    }

    void rejectsInvalidIndexesAndRoles() {
        hash_core::AlgorithmsModel model;
        QVERIFY(!model.data(QModelIndex(), hash_core::AlgorithmsModel::NameRole).isValid());
        QVERIFY(!model.data(model.index(0), Qt::DisplayRole).isValid());
        QCOMPARE(model.rowCount(model.index(0)), 0);
    }
};
