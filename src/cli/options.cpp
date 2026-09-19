#include "cli/options.h"

#include "common/quoting.h"
#include "core/hasher.h"
#include "core/registry.h"

#include <algorithm>

namespace hash_core {
namespace {

constexpr const char *kUsageFlags =
    "  -a string\n"
    "    \tcomma-separated algorithms (default \"sha256\")\n"
    "  -all\n"
    "    \tselect every available algorithm\n"
    "  -fail-fast\n"
    "    \tstop scheduling after cancellation or first failed result\n"
    "  -format string\n"
    "    \toutput format: text, tsv, or json (default \"text\")\n"
    "  -literal\n"
    "    \ttreat wildcard characters as literal\n"
    "  -modified\n"
    "    \tshow modification time\n"
    "  -o string\n"
    "    \twrite output to a file\n"
    "  -output string\n"
    "    \twrite output to a file\n"
    "  -recursive\n"
    "    \thash files under directory paths recursively\n"
    "  -size\n"
    "    \tshow file size in bytes\n"
    "  -utc\n"
    "    \tshow times in UTC\n"
    "  -workers int\n"
    "    \tnumber of files processed concurrently";

// splitAlgorithmList normalizes a comma-separated --algorithm value the way
// the previous Go implementation did: each part is trimmed and lower-cased.
QStringList splitAlgorithmList(const QString &value) {
    QStringList parts = value.split(u',');
    for (QString &part : parts) {
        part = part.trimmed().toLower();
    }
    return parts;
}

bool parseBool(const QString &value, bool *out) {
    if (value == QStringLiteral("1") || value == QStringLiteral("t") ||
        value == QStringLiteral("T") || value == QStringLiteral("true") ||
        value == QStringLiteral("TRUE") || value == QStringLiteral("True")) {
        *out = true;
        return true;
    }
    if (value == QStringLiteral("0") || value == QStringLiteral("f") ||
        value == QStringLiteral("F") || value == QStringLiteral("false") ||
        value == QStringLiteral("FALSE") || value == QStringLiteral("False")) {
        *out = false;
        return true;
    }
    return false;
}

bool parseInt(const QString &value, int *out) {
    bool ok = false;
    const qlonglong parsed = value.toLongLong(&ok);
    if (!ok || parsed < -0x80000000ll || parsed > 0x7fffffffll) {
        return false;
    }
    *out = int(parsed);
    return true;
}

} // namespace

QString usageText() {
    return QStringLiteral("Usage of hash:\n") + QString::fromLatin1(kUsageFlags) + u'\n';
}

bool parseArgs(const QStringList &args, Config *config, QString *error) {
    QStringList paths;
    bool sawAlgorithm = false;
    int index = 0;
    while (index < args.size()) {
        const QString arg = args.at(index);
        if (arg == QStringLiteral("--")) {
            ++index;
            break;
        }
        if (!arg.startsWith(u'-') || arg == QStringLiteral("-")) {
            break;
        }
        QString name = arg.mid(1);
        if (name.startsWith(u'-')) {
            name.remove(0, 1);
        }
        QString value;
        bool hasValue = false;
        const int equals = name.indexOf(u'=');
        if (equals >= 0) {
            value = name.mid(equals + 1);
            name.truncate(equals);
            hasValue = true;
        }
        if (name == QStringLiteral("h") || name == QStringLiteral("help")) {
            *error = QStringLiteral("flag: help requested");
            return false;
        }

        // fetch reads the following argument for flags that need a value.
        auto fetch = [&index, &args, &value, &error, hasValue](const QString &flagName) {
            if (hasValue) {
                return true;
            }
            ++index;
            if (index >= args.size()) {
                *error = QStringLiteral("flag needs an argument: -") + flagName;
                return false;
            }
            value = args.at(index);
            return true;
        };
        auto assignBool = [&value, &error, hasValue](bool *target, const QString &flagName) {
            const QString raw = hasValue ? value : QStringLiteral("true");
            if (!parseBool(raw, target)) {
                *error = QStringLiteral("invalid boolean value ") + goQuote(raw) +
                         QStringLiteral(" for flag -") + flagName;
                return false;
            }
            return true;
        };

        if (name == QStringLiteral("algorithm") || name == QStringLiteral("a")) {
            if (!fetch(name)) {
                return false;
            }
            config->algorithms = splitAlgorithmList(value);
            sawAlgorithm = true;
        } else if (name == QStringLiteral("all")) {
            if (!assignBool(&config->all, name)) {
                return false;
            }
        } else if (name == QStringLiteral("size")) {
            if (!assignBool(&config->showSize, name)) {
                return false;
            }
        } else if (name == QStringLiteral("modified")) {
            if (!assignBool(&config->showModified, name)) {
                return false;
            }
        } else if (name == QStringLiteral("utc")) {
            if (!assignBool(&config->utc, name)) {
                return false;
            }
        } else if (name == QStringLiteral("workers")) {
            if (!fetch(name)) {
                return false;
            }
            if (!parseInt(value, &config->workers)) {
                *error = QStringLiteral("invalid value ") + goQuote(value) +
                         QStringLiteral(" for flag -") + name + QStringLiteral(": parse error");
                return false;
            }
        } else if (name == QStringLiteral("format")) {
            if (!fetch(name)) {
                return false;
            }
            config->format = value;
        } else if (name == QStringLiteral("output") || name == QStringLiteral("o")) {
            if (!fetch(name)) {
                return false;
            }
            config->output = value;
        } else if (name == QStringLiteral("recursive")) {
            if (!assignBool(&config->recursive, name)) {
                return false;
            }
        } else if (name == QStringLiteral("literal")) {
            if (!assignBool(&config->literal, name)) {
                return false;
            }
        } else if (name == QStringLiteral("fail-fast")) {
            if (!assignBool(&config->failFast, name)) {
                return false;
            }
        } else {
            *error = QStringLiteral("flag provided but not defined: -") + name;
            return false;
        }
        ++index;
    }
    while (index < args.size()) {
        paths.append(args.at(index));
        ++index;
    }
    config->paths = paths;
    // The -a/--algorithm flag defaults to sha256 when it is not given.
    if (!sawAlgorithm) {
        config->algorithms = splitAlgorithmList(QStringLiteral("sha256"));
    }
    if (config->workers < 0) {
        *error = QStringLiteral("workers cannot be negative");
        return false;
    }
    const QString format = config->format.toLower();
    if (format != QStringLiteral("text") && format != QStringLiteral("tsv") &&
        format != QStringLiteral("json")) {
        *error = QString::fromLatin1(kUnsupportedFormatMessage) + u' ' + goQuote(config->format);
        return false;
    }
    return true;
}

bool selectAlgorithms(const Config &config, QStringList *selected, QString *error) {
    if (config.all) {
        const QVector<AlgorithmSpec> registered = algorithms();
        QStringList names;
        names.reserve(registered.size());
        for (const AlgorithmSpec &algorithm : registered) {
            names.append(algorithm.name);
        }
        *selected = names;
        return true;
    }
    QStringList normalized;
    normalized.reserve(config.algorithms.size());
    for (const QString &name : config.algorithms) {
        normalized.append(name.trimmed().toLower());
    }
    QString validationError;
    if (validateAlgorithms(normalized, &validationError) != AlgorithmError::None) {
        *error = validationError;
        return false;
    }
    *selected = normalized;
    return true;
}

} // namespace hash_core
