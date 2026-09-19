#include "common/quoting.h"

namespace hash_core {

QString goQuote(const QString &value) {
    QString quoted;
    quoted.reserve(value.size() + 2);
    quoted += u'"';
    for (const QChar character : value) {
        const char16_t code = character.unicode();
        switch (code) {
        case u'"':
            quoted += QStringLiteral("\\\"");
            break;
        case u'\\':
            quoted += QStringLiteral("\\\\");
            break;
        case u'\a':
            quoted += QStringLiteral("\\a");
            break;
        case u'\b':
            quoted += QStringLiteral("\\b");
            break;
        case u'\f':
            quoted += QStringLiteral("\\f");
            break;
        case u'\n':
            quoted += QStringLiteral("\\n");
            break;
        case u'\r':
            quoted += QStringLiteral("\\r");
            break;
        case u'\t':
            quoted += QStringLiteral("\\t");
            break;
        case u'\v':
            quoted += QStringLiteral("\\v");
            break;
        default:
            if (code >= 0x20 && code != 0x7F) {
                quoted += character;
            } else if (code < 0x100) {
                quoted += QStringLiteral("\\x") + QString::number(code, 16).rightJustified(2, u'0');
            } else {
                quoted += QStringLiteral("\\u") + QString::number(code, 16).rightJustified(4, u'0');
            }
            break;
        }
    }
    quoted += u'"';
    return quoted;
}

namespace {

bool hexValue(const QString &text, int digits, quint32 *out) {
    if (text.size() < digits) {
        return false;
    }
    quint32 value = 0;
    for (int index = 0; index < digits; ++index) {
        const char16_t code = text.at(index).unicode();
        quint32 digit;
        if (code >= u'0' && code <= u'9') {
            digit = quint32(code - u'0');
        } else if (code >= u'a' && code <= u'f') {
            digit = quint32(code - u'a') + 10;
        } else if (code >= u'A' && code <= u'F') {
            digit = quint32(code - u'A') + 10;
        } else {
            return false;
        }
        value = (value << 4) | digit;
    }
    *out = value;
    return true;
}

} // namespace

bool goUnquote(const QString &value, QString *out, QString *error) {
    const auto fail = [error]() {
        if (error) {
            *error = QStringLiteral("invalid syntax");
        }
        return false;
    };
    if (value.size() < 2 || !value.startsWith(u'"') || !value.endsWith(u'"')) {
        return fail();
    }
    const QString body = value.mid(1, value.size() - 2);
    QString decoded;
    decoded.reserve(body.size());
    for (int index = 0; index < body.size(); ++index) {
        const QChar character = body.at(index);
        if (character != u'\\') {
            decoded += character;
            continue;
        }
        if (++index >= body.size()) {
            return fail();
        }
        const char16_t escape = body.at(index).unicode();
        switch (escape) {
        case u'a':
            decoded += QChar(u'\a');
            break;
        case u'b':
            decoded += QChar(u'\b');
            break;
        case u'f':
            decoded += QChar(u'\f');
            break;
        case u'n':
            decoded += QChar(u'\n');
            break;
        case u'r':
            decoded += QChar(u'\r');
            break;
        case u't':
            decoded += QChar(u'\t');
            break;
        case u'v':
            decoded += QChar(u'\v');
            break;
        case u'\\':
            decoded += u'\\';
            break;
        case u'"':
            decoded += u'"';
            break;
        case u'x': {
            quint32 code = 0;
            if (!hexValue(body.mid(index + 1), 2, &code)) {
                return fail();
            }
            decoded += QChar(char16_t(code));
            index += 2;
            break;
        }
        case u'u': {
            quint32 code = 0;
            if (!hexValue(body.mid(index + 1), 4, &code)) {
                return fail();
            }
            decoded += QChar(char16_t(code));
            index += 4;
            break;
        }
        case u'U': {
            quint32 code = 0;
            if (!hexValue(body.mid(index + 1), 8, &code)) {
                return fail();
            }
            if (code > 0x10FFFF) {
                return fail();
            }
            if (code > 0xFFFF) {
                const char32_t scalar = char32_t(code);
                decoded += QString::fromUcs4(&scalar, 1);
            } else {
                decoded += QChar(char16_t(code));
            }
            index += 8;
            break;
        }
        default:
            if (escape >= u'0' && escape <= u'7') {
                quint32 code = 0;
                if (!hexValue(body.mid(index), 3, &code) || code > 0xFF) {
                    return fail();
                }
                decoded += QChar(char16_t(code));
                index += 2;
                break;
            }
            return fail();
        }
    }
    *out = decoded;
    return true;
}

} // namespace hash_core
