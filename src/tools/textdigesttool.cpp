#include "tools/textdigesttool.h"

#include "core/registry.h"

#include <memory>

namespace hash_core {

TextDigestTool::TextDigestTool(QObject *parent) : QObject(parent) {}

QVariantMap TextDigestTool::hashText(const QString &text, const QStringList &algorithms) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    if (algorithms.isEmpty()) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("请至少选择一个算法"));
        return outcome;
    }
    const QByteArray bytes = text.toUtf8();
    QVariantList digests;
    digests.reserve(algorithms.size());
    for (const QString &name : algorithms) {
        AlgorithmSpec spec;
        if (!lookupAlgorithm(name, &spec)) {
            outcome.insert(QStringLiteral("error"), QStringLiteral("未知算法：") + name);
            return outcome;
        }
        std::unique_ptr<Digester> digester = spec.create();
        digester->update(bytes.constData(), bytes.size());
        QVariantMap entry;
        entry.insert(QStringLiteral("algorithm"), spec.name);
        entry.insert(QStringLiteral("label"), spec.label);
        entry.insert(QStringLiteral("value"), QString::fromLatin1(digester->digest()));
        digests.append(entry);
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("digests"), digests);
    return outcome;
}

} // namespace hash_core
