#include "common/compare.h"

#include "common/pathkey.h"
#include "common/quoting.h"

#include <QHash>
#include <QSet>

namespace hash_core {
namespace {

constexpr int kMaxRecords = 100000;
constexpr qsizetype kMaxLineLength = 16 * 1024 * 1024;

// removeFormulaPrefix strips the spreadsheet formula-injection guard added by
// the TSV encoder: a leading apostrophe before a formula-looking value.
QString removeFormulaPrefix(const QString &value) {
    if (value.size() > 1 && value.at(0) == u'\'') {
        const QString tail = value.mid(1);
        int start = 0;
        while (start < tail.size() && (tail.at(start) == u' ' || tail.at(start) == u'\t' ||
                                       tail.at(start) == u'\r' || tail.at(start) == u'\n')) {
            ++start;
        }
        if (start < tail.size()) {
            const char16_t first = tail.at(start).unicode();
            if (first == u'=' || first == u'+' || first == u'-' || first == u'@') {
                return tail;
            }
        }
    }
    return value;
}

// unescapeTsv decodes the escapes written by the TSV encoder.
bool unescapeTsv(const QString &value, QString *out, QString *error) {
    QString decoded;
    decoded.reserve(value.size());
    for (int index = 0; index < value.size(); ++index) {
        const QChar character = value.at(index);
        if (character != u'\\') {
            decoded += character;
            continue;
        }
        if (++index >= value.size()) {
            *error = QStringLiteral("has an incomplete escape");
            return false;
        }
        switch (value.at(index).unicode()) {
        case u'\\':
            decoded += u'\\';
            break;
        case u't':
            decoded += u'\t';
            break;
        case u'r':
            decoded += u'\r';
            break;
        case u'n':
            decoded += u'\n';
            break;
        default:
            *error = QStringLiteral("has unsupported escape \\") + QString(value.at(index));
            return false;
        }
    }
    *out = decoded;
    return true;
}

bool parsePath(const QString &value, QString *out, QString *error) {
    const QString trimmed = value.trimmed();
    QString path;
    if (trimmed.startsWith(u'"')) {
        if (!goUnquote(trimmed, &path, error)) {
            return false;
        }
    } else {
        QString decoded;
        if (!unescapeTsv(trimmed, &decoded, error)) {
            return false;
        }
        path = removeFormulaPrefix(decoded);
    }
    if (path.isEmpty()) {
        *error = QStringLiteral("has empty path");
        return false;
    }
    *out = path;
    return true;
}

QString indexKey(const CompareRecord &record) {
    return record.algorithm.trimmed().toLower() + QChar(u'\0') + pathKey(record.path.trimmed());
}

} // namespace

QVector<CompareRecord> recordsFromResults(const QVector<FileResult> &results) {
    QVector<CompareRecord> records;
    for (const FileResult &result : results) {
        if (!result.error.isEmpty()) {
            continue;
        }
        for (const QString &algorithm : result.result.order) {
            const QString digest = result.result.digestFor(algorithm);
            if (digest.isEmpty()) {
                continue;
            }
            records.append(CompareRecord{algorithm, digest, result.result.path});
        }
    }
    return records;
}

bool parseDigestText(const QString &text, QVector<CompareRecord> *records, QString *error) {
    QString body = text;
    if (body.startsWith(QChar(0xFEFF))) {
        body.remove(0, 1);
    }
    if (body.trimmed().isEmpty()) {
        *error = QStringLiteral("clipboard is empty");
        return false;
    }

    QVector<CompareRecord> parsed;
    const QStringList lines = body.split(u'\n');
    for (int index = 0; index < lines.size(); ++index) {
        const QString line = lines.at(index).trimmed();
        const int lineNumber = index + 1;
        if (line.isEmpty() || line.startsWith(QStringLiteral("WARNING:")) ||
            line.startsWith(QStringLiteral("algorithm\tdigest\tpath"))) {
            continue;
        }
        if (line.size() > kMaxLineLength) {
            *error = QStringLiteral("scan clipboard: token too long");
            return false;
        }
        const QStringList fields = line.split(u'\t');
        if (fields.size() < 3) {
            *error = QStringLiteral("line %1 has too few fields").arg(lineNumber);
            return false;
        }
        const QString algorithm = fields.at(0).trimmed().toLower();
        if (algorithm.compare(QStringLiteral("error"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        const QString digest = fields.at(1).trimmed().toLower();
        if (algorithm.isEmpty() || digest.isEmpty()) {
            *error = QStringLiteral("line %1 is not a digest record").arg(lineNumber);
            return false;
        }
        QString path;
        if (!parsePath(fields.at(2), &path, error)) {
            *error = QStringLiteral("line %1 path: %2").arg(lineNumber).arg(*error);
            return false;
        }
        parsed.append(CompareRecord{algorithm, digest, path});
        if (parsed.size() > kMaxRecords) {
            *error =
                QStringLiteral("clipboard contains more than %1 digest records").arg(kMaxRecords);
            return false;
        }
    }
    if (parsed.isEmpty()) {
        *error = QStringLiteral("clipboard contains no digest records");
        return false;
    }
    *records = parsed;
    return true;
}

CompareSummary compareRecords(const QVector<CompareRecord> &expected,
                              const QVector<CompareRecord> &actual) {
    auto index = [](const QVector<CompareRecord> &records, int *duplicates) {
        QHash<QString, QString> indexed;
        *duplicates = 0;
        for (const CompareRecord &record : records) {
            const QString key = indexKey(record);
            if (indexed.contains(key)) {
                *duplicates += 1;
                continue;
            }
            indexed.insert(key, record.digest.trimmed().toLower());
        }
        return indexed;
    };

    int expectedDuplicates = 0;
    int actualDuplicates = 0;
    const QHash<QString, QString> expectedMap = index(expected, &expectedDuplicates);
    const QHash<QString, QString> actualMap = index(actual, &actualDuplicates);

    CompareSummary summary;
    summary.duplicates = expectedDuplicates + actualDuplicates;
    for (auto it = expectedMap.constBegin(); it != expectedMap.constEnd(); ++it) {
        const auto found = actualMap.constFind(it.key());
        if (found == actualMap.constEnd()) {
            summary.missing += 1;
            continue;
        }
        if (found.value() == it.value()) {
            summary.matches += 1;
        } else {
            summary.mismatches += 1;
        }
    }
    for (auto it = actualMap.constBegin(); it != actualMap.constEnd(); ++it) {
        if (!expectedMap.contains(it.key())) {
            summary.unexpected += 1;
        }
    }
    summary.exact = summary.mismatches == 0 && summary.missing == 0 && summary.unexpected == 0 &&
                    summary.duplicates == 0;
    return summary;
}

} // namespace hash_core
