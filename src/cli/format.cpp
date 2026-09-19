#include "cli/format.h"

#include "common/quoting.h"

#include <algorithm>

namespace hash_core {
namespace {

QString formatTime(const QDateTime &value, bool utc) {
    const QDateTime adjusted = utc ? value.toUTC() : value;
    return adjusted.toString(Qt::ISODate);
}

QString jsonEscape(const QString &value) {
    QString escaped;
    escaped.reserve(value.size() + 2);
    for (const QChar character : value) {
        const char16_t code = character.unicode();
        switch (code) {
        case u'"':
            escaped += QStringLiteral("\\\"");
            break;
        case u'\\':
            escaped += QStringLiteral("\\\\");
            break;
        case u'\n':
            escaped += QStringLiteral("\\n");
            break;
        case u'\r':
            escaped += QStringLiteral("\\r");
            break;
        case u'\t':
            escaped += QStringLiteral("\\t");
            break;
        case u'<':
            escaped += QStringLiteral("\\u003c");
            break;
        case u'>':
            escaped += QStringLiteral("\\u003e");
            break;
        case u'&':
            escaped += QStringLiteral("\\u0026");
            break;
        case u'\u2028':
            escaped += QStringLiteral("\\u2028");
            break;
        case u'\u2029':
            escaped += QStringLiteral("\\u2029");
            break;
        default:
            if (code < 0x20) {
                escaped +=
                    QStringLiteral("\\u00") + QString::number(code, 16).rightJustified(4, u'0');
            } else {
                escaped += character;
            }
            break;
        }
    }
    return escaped;
}

// row is the flattened, per-algorithm view used by the text and TSV
// encoders. A failed file produces a single row with a non-empty error.
struct Row {
    QString algorithm;
    QString digest;
    QString path;
    QString error;
    qint64 size = 0;
    QDateTime modified;
};

QVector<Row> rowsFromResults(const QVector<FileResult> &results, const QStringList &algorithms) {
    QVector<Row> rows;
    for (const FileResult &item : results) {
        if (!item.error.isEmpty()) {
            Row row;
            row.path = item.request.path;
            row.error = item.error;
            rows.append(row);
            continue;
        }
        for (const QString &algorithm : algorithms) {
            Row row;
            row.algorithm = algorithm;
            row.digest = item.result.digestFor(algorithm);
            row.path = item.result.path;
            row.size = item.result.size;
            row.modified = item.result.modified;
            rows.append(row);
        }
    }
    return rows;
}

void writeText(QString *output, const QVector<Row> &rows, const FormatOptions &options) {
    for (const Row &item : rows) {
        if (!item.error.isEmpty()) {
            *output += QStringLiteral("ERROR\t") + goQuote(item.path) + u'\t' +
                       goQuote(item.error) + u'\n';
            continue;
        }
        *output += item.algorithm + u'\t' + item.digest + u'\t' + goQuote(item.path);
        if (options.showSize) {
            *output += QStringLiteral("\tsize=") + QString::number(item.size);
        }
        if (options.showModified) {
            *output += QStringLiteral("\tmodified=") + formatTime(item.modified, options.utc);
        }
        *output += u'\n';
    }
}

void writeTsv(QString *output, const QVector<Row> &rows, const FormatOptions &options) {
    QStringList columns{QStringLiteral("algorithm"), QStringLiteral("digest"),
                        QStringLiteral("path"), QStringLiteral("error")};
    if (options.showSize) {
        columns.append(QStringLiteral("size"));
    }
    if (options.showModified) {
        columns.append(QStringLiteral("modified"));
    }
    *output += columns.join(u'\t') + u'\n';
    for (const Row &item : rows) {
        QStringList values;
        if (!item.error.isEmpty()) {
            values << QStringLiteral("ERROR") << QString() << item.path << item.error;
            if (options.showSize) {
                values << QStringLiteral("-");
            }
            if (options.showModified) {
                values << QStringLiteral("-");
            }
        } else {
            values << item.algorithm << item.digest << item.path << QString();
            if (options.showSize) {
                values << QString::number(item.size);
            }
            if (options.showModified) {
                values << formatTime(item.modified, options.utc);
            }
        }
        for (QString &value : values) {
            value = tsvValue(value);
        }
        *output += values.join(u'\t') + u'\n';
    }
}

void writeJson(QString *output, const QVector<FileResult> &results, const FormatOptions &options) {
    if (results.isEmpty()) {
        *output += QStringLiteral("[]\n");
        return;
    }
    *output += QStringLiteral("[\n");
    for (int index = 0; index < results.size(); ++index) {
        const FileResult &item = results.at(index);
        const bool failed = !item.error.isEmpty();
        QStringList fields;
        fields.append(QStringLiteral("    \"path\": \"") + jsonEscape(item.request.path) + u'"');
        if (options.showSize && !failed) {
            fields.append(QStringLiteral("    \"size\": ") + QString::number(item.result.size));
        }
        if (options.showModified && !failed) {
            const QDateTime modified =
                options.utc ? item.result.modified.toUTC() : item.result.modified;
            fields.append(QStringLiteral("    \"modified\": \"") +
                          jsonEscape(formatTime(modified, false)) + u'"');
        }
        if (!failed) {
            // encoding/json sorts map keys, so the digests object is sorted
            // by algorithm name rather than request order.
            QStringList names;
            names.reserve(item.result.digests.size());
            for (const Digest &digest : item.result.digests) {
                names.append(digest.algorithm);
            }
            std::sort(names.begin(), names.end());
            QStringList entries;
            for (const QString &name : names) {
                entries.append(QStringLiteral("      \"") + jsonEscape(name) +
                               QStringLiteral("\": \"") + jsonEscape(item.result.digestFor(name)) +
                               u'"');
            }
            fields.append(QStringLiteral("    \"digests\": {\n") +
                          entries.join(QStringLiteral(",\n")) + QStringLiteral("\n    }"));
        }
        if (!item.error.isEmpty()) {
            fields.append(QStringLiteral("    \"error\": \"") + jsonEscape(item.error) + u'"');
        }
        *output += QStringLiteral("  {\n") + fields.join(QStringLiteral(",\n"));
        *output +=
            index + 1 < results.size() ? QStringLiteral("\n  },\n") : QStringLiteral("\n  }\n");
    }
    *output += QStringLiteral("]\n");
}

} // namespace

bool writeResults(const QVector<FileResult> &results, const QStringList &algorithms,
                  const FormatOptions &options, QString *output, QString *error) {
    const QString format = options.format.toLower();
    if (format == QStringLiteral("text")) {
        writeText(output, rowsFromResults(results, algorithms), options);
        return true;
    }
    if (format == QStringLiteral("tsv")) {
        writeTsv(output, rowsFromResults(results, algorithms), options);
        return true;
    }
    if (format == QStringLiteral("json")) {
        writeJson(output, results, options);
        return true;
    }
    *error = QStringLiteral("unsupported output format ") + goQuote(options.format);
    return false;
}

QString tsvValue(const QString &value) {
    int start = 0;
    while (start < value.size() && (value.at(start) == u' ' || value.at(start) == u'\t' ||
                                    value.at(start) == u'\r' || value.at(start) == u'\n')) {
        ++start;
    }
    QString replaced = value;
    replaced.replace(u'\\', QStringLiteral("\\\\"));
    replaced.replace(u'\t', QStringLiteral("\\t"));
    replaced.replace(u'\r', QStringLiteral("\\r"));
    replaced.replace(u'\n', QStringLiteral("\\n"));
    if (start < value.size()) {
        const char16_t first = value.at(start).unicode();
        if (first == u'=' || first == u'+' || first == u'-' || first == u'@') {
            return u'\'' + replaced;
        }
    }
    return replaced;
}

} // namespace hash_core
