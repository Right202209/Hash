#include "common/compare.h"

#include "common/pathkey.h"
#include "common/quoting.h"

#include <QHash>
#include <QSet>

namespace hash_core {
namespace {

constexpr int kMaxRecords = 100000;
constexpr qsizetype kMaxLineLength = 16 * 1024 * 1024;

// Digests are byte-aligned, so the shortest supported value is a 32-bit
// checksum (8 hex digits) and the longest is a 512-bit digest (128 digits).
constexpr int kMinDigestDigits = 8;
constexpr int kMaxDigestDigits = 128;

// looksLikeAlgorithmName reports whether token can serve as the algorithm tag
// of a "sha256:..." prefix: a short run of letters, digits, hyphens and
// underscores containing at least one letter. Bare hex never matches because
// it has no letters.
bool looksLikeAlgorithmName(const QString &token) {
    if (token.size() < 2 || token.size() > 24) {
        return false;
    }
    bool hasLetter = false;
    for (const QChar character : token) {
        if (character.isLetter()) {
            hasLetter = true;
            continue;
        }
        if (!character.isNumber() && character != u'-' && character != u'_') {
            return false;
        }
    }
    return hasLetter;
}

bool isHexChar(QChar character) {
    return (character >= u'0' && character <= u'9') || (character >= u'a' && character <= u'f') ||
           (character >= u'A' && character <= u'F');
}

bool isHexDigest(const QString &digest) {
    if (digest.size() < kMinDigestDigits || digest.size() > kMaxDigestDigits ||
        digest.size() % 2 != 0) {
        return false;
    }
    for (const QChar character : digest) {
        if (!isHexChar(character)) {
            return false;
        }
    }
    return true;
}

// normalizeDigest cleans a pasted digest value: whitespace inside the value is
// dropped, a leading algorithm tag such as "sha256:", "SHA-256：" or "md5 =" is
// stripped (only when the tag looks like an algorithm name), and the rest is
// lowercased. When algorithm is given and the tag is present, the tag is
// stored there; callers pass nullptr to keep their own algorithm field.
QString normalizeDigest(const QString &value, QString *algorithm) {
    QString digest;
    digest.reserve(value.size());
    for (const QChar character : value) {
        if (!character.isSpace()) {
            digest += character;
        }
    }
    int separator = -1;
    for (int index = 0; index < digest.size(); ++index) {
        const char16_t code = digest.at(index).unicode();
        if (code == u':' || code == u'：' || code == u'=') {
            separator = index;
            break;
        }
    }
    if (separator > 0) {
        const QString tag = digest.left(separator);
        if (looksLikeAlgorithmName(tag)) {
            if (algorithm && algorithm->isEmpty()) {
                *algorithm = tag.toLower();
            }
            digest = digest.mid(separator + 1);
        }
    }
    return digest.toLower();
}

// isSeparatorToken reports whether token is a standalone algorithm/digest
// separator, as in "md5 = <digest>".
bool isSeparatorToken(const QString &token) {
    return token == QStringLiteral(u"=") || token == QStringLiteral(u":") ||
           token == QStringLiteral(u"：");
}

// splitLooseTokens breaks a non-TSV line into whitespace-separated tokens so
// "sha256:abc123 file.bin" and "abc123  file.bin" (sha256sum style) parse.
QStringList splitLooseTokens(const QString &line) {
    QStringList tokens;
    QString current;
    for (const QChar character : line) {
        if (character.isSpace()) {
            if (!current.isEmpty()) {
                tokens.append(current);
                current.clear();
            }
            continue;
        }
        current += character;
    }
    if (!current.isEmpty()) {
        tokens.append(current);
    }
    return tokens;
}

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
            // A non-TSV line is a loose digest: "[algorithm:]digest [path]".
            // The algorithm tag ("sha256:" and friends) is stripped
            // automatically and the digest verifies by value.
            const QStringList tokens = splitLooseTokens(line);
            QString algorithm;
            QString digest;
            QString path;
            bool loose = false;
            if (tokens.size() >= 3 && looksLikeAlgorithmName(tokens.first()) &&
                isSeparatorToken(tokens.at(1))) {
                // "md5 = <digest> [path]" with spaced separators.
                algorithm = tokens.first().toLower();
                digest = normalizeDigest(tokens.at(2), nullptr);
                path = tokens.mid(3).join(u' ');
                loose = isHexDigest(digest);
            } else if (!tokens.isEmpty()) {
                digest = normalizeDigest(tokens.first(), &algorithm);
                path = tokens.mid(1).join(u' ');
                loose = isHexDigest(digest);
            }
            if (!loose) {
                *error = QStringLiteral("line %1 is not a digest record").arg(lineNumber);
                return false;
            }
            parsed.append(CompareRecord{algorithm, digest, path, true});
            if (parsed.size() > kMaxRecords) {
                *error = QStringLiteral("clipboard contains more than %1 digest records")
                             .arg(kMaxRecords);
                return false;
            }
            continue;
        }
        const QString algorithm = fields.at(0).trimmed().toLower();
        if (algorithm.compare(QStringLiteral("error"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        const QString digest = normalizeDigest(fields.at(1), nullptr);
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
            if (record.loose) {
                continue;
            }
            const QString key = indexKey(record);
            if (indexed.contains(key)) {
                *duplicates += 1;
                continue;
            }
            indexed.insert(key, record.digest.trimmed().toLower());
        }
        return indexed;
    };
    auto values = [](const QVector<CompareRecord> &records) {
        QSet<QString> digests;
        for (const CompareRecord &record : records) {
            digests.insert(record.digest.trimmed().toLower());
        }
        return digests;
    };

    int expectedDuplicates = 0;
    int actualDuplicates = 0;
    const QHash<QString, QString> expectedMap = index(expected, &expectedDuplicates);
    const QHash<QString, QString> actualMap = index(actual, &actualDuplicates);
    const QSet<QString> expectedValues = values(expected);
    const QSet<QString> actualValues = values(actual);

    // A bare-digest paste verifies by value: when the actual side has no full
    // records at all, expected records must not be counted as missing just
    // because no keyed counterpart exists.
    bool actualHasKeyedRecords = false;
    for (const CompareRecord &record : actual) {
        if (!record.loose) {
            actualHasKeyedRecords = true;
            break;
        }
    }

    CompareSummary summary;
    summary.duplicates = expectedDuplicates + actualDuplicates;
    for (auto it = expectedMap.constBegin(); it != expectedMap.constEnd(); ++it) {
        const auto found = actualMap.constFind(it.key());
        if (found == actualMap.constEnd()) {
            if (actualHasKeyedRecords) {
                summary.missing += 1;
            }
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
    // Loose records verify by digest value alone: one match or mismatch per
    // pasted digest, without touching the keyed counters.
    for (const CompareRecord &record : expected) {
        if (!record.loose) {
            continue;
        }
        if (actualValues.contains(record.digest.trimmed().toLower())) {
            summary.matches += 1;
        } else {
            summary.mismatches += 1;
        }
    }
    for (const CompareRecord &record : actual) {
        if (record.loose) {
            if (expectedValues.contains(record.digest.trimmed().toLower())) {
                summary.matches += 1;
            } else {
                summary.mismatches += 1;
            }
        }
    }
    summary.exact = summary.mismatches == 0 && summary.missing == 0 && summary.unexpected == 0 &&
                    summary.duplicates == 0;
    return summary;
}

} // namespace hash_core
