#pragma once

#include "tools/jsontool.h"

#include <QtTest>

class TestJsonTool : public QObject {
    Q_OBJECT

  private slots:
    void formatSortsKeysAndIndents() {
        hash_core::JsonTool tool;
        const QVariantMap outcome = tool.formatJson(QStringLiteral("{\"b\":1,\"a\":2}"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("text").toString().trimmed(),
                 QStringLiteral("{\n    \"a\": 2,\n    \"b\": 1\n}"));
    }

    void minifyRemovesWhitespace() {
        hash_core::JsonTool tool;
        const QVariantMap outcome =
            tool.minifyJson(QStringLiteral("{ \"b\" : 1 , \"a\" : [1, 2] }"));
        QCOMPARE(outcome.value("ok").toBool(), true);
        QCOMPARE(outcome.value("text").toString(), QStringLiteral("{\"a\":[1,2],\"b\":1}"));
    }

    void invalidInputReportsLine() {
        hash_core::JsonTool tool;
        const QVariantMap outcome = tool.formatJson(QStringLiteral("{\n  \"a\": ,\n}"));
        QCOMPARE(outcome.value("ok").toBool(), false);
        QVERIFY(outcome.value("error").toString().contains(QStringLiteral("第 2 行")));
    }

    void nonObjectTopLevelIsRejected() {
        hash_core::JsonTool tool;
        const QVariantMap outcome = tool.formatJson(QStringLiteral("42"));
        QCOMPARE(outcome.value("ok").toBool(), false);
    }
};
