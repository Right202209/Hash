#include "tools/passwordtool.h"

#include <QRandomGenerator>
#include <QStringList>
#include <QVector>

namespace hash_core {
namespace {

// Characters commonly confused when reading passwords back.
const char kAmbiguous[] = "0Oo1lI|`'\"";

QString filterAmbiguous(const char *characters) {
    QString set = QString::fromLatin1(characters);
    const QString ambiguous = QString::fromLatin1(kAmbiguous);
    for (const QChar &character : ambiguous) {
        set.remove(character);
    }
    return set;
}

} // namespace

PasswordTool::PasswordTool(QObject *parent) : QObject(parent) {}

QVariantMap PasswordTool::generate(int length, int count, bool upper, bool lower, bool digits,
                                   bool symbols, bool excludeAmbiguous) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    if (length < 4 || length > 256) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("长度需在 4-256 之间"));
        return outcome;
    }
    if (count < 1 || count > 100) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("数量需在 1-100 之间"));
        return outcome;
    }
    QVector<QString> sets;
    QString all;
    auto addSet = [&sets, &all, excludeAmbiguous](const char *characters) {
        const QString set =
            excludeAmbiguous ? filterAmbiguous(characters) : QString::fromLatin1(characters);
        if (!set.isEmpty()) {
            sets.append(set);
            all += set;
        }
    };
    if (upper) {
        addSet("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    }
    if (lower) {
        addSet("abcdefghijklmnopqrstuvwxyz");
    }
    if (digits) {
        addSet("0123456789");
    }
    if (symbols) {
        addSet("!@#$%^&*()-_=+[]{};:,.?");
    }
    if (sets.isEmpty()) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("请至少选择一类字符"));
        return outcome;
    }

    QRandomGenerator *random = QRandomGenerator::system();
    QStringList passwords;
    passwords.reserve(count);
    for (int index = 0; index < count; ++index) {
        QString password;
        password.reserve(length);
        for (int position = 0; position < length; ++position) {
            password.append(all.at(random->bounded(all.size())));
        }
        // Guarantee one character from every selected set, then shuffle so
        // the guaranteed positions are not predictable.
        int position = 0;
        for (const QString &set : sets) {
            if (position >= length) {
                break;
            }
            password[position] = set.at(random->bounded(set.size()));
            ++position;
        }
        for (int tail = length - 1; tail > 0; --tail) {
            const int swap = random->bounded(tail + 1);
            password.swapItemsAt(tail, swap);
        }
        passwords.append(password);
    }
    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("list"), passwords);
    return outcome;
}

} // namespace hash_core
