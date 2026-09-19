#include "tools/colortool.h"

#include <QChar>
#include <QStringList>

namespace hash_core {
namespace {

struct Rgba {
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 255;
};

int hexDigit(QChar character, bool *ok) {
    const char16_t code = character.toLower().unicode();
    *ok = true;
    if (code >= u'0' && code <= u'9') {
        return code - u'0';
    }
    if (code >= u'a' && code <= u'f') {
        return code - u'a' + 10;
    }
    *ok = false;
    return 0;
}

bool parseHexColor(const QString &body, Rgba *color) {
    if (body.size() != 3 && body.size() != 4 && body.size() != 6 && body.size() != 8) {
        return false;
    }
    int digits[8] = {};
    for (int index = 0; index < body.size(); ++index) {
        bool ok = false;
        digits[index] = hexDigit(body.at(index), &ok);
        if (!ok) {
            return false;
        }
    }
    auto pair = [&digits](int index) { return digits[index] * 16 + digits[index + 1]; };
    if (body.size() <= 4) {
        color->red = digits[0] * 17;
        color->green = digits[1] * 17;
        color->blue = digits[2] * 17;
        color->alpha = body.size() == 4 ? digits[3] * 17 : 255;
        return true;
    }
    // CSS order: RRGGBBAA.
    color->red = pair(0);
    color->green = pair(2);
    color->blue = pair(4);
    color->alpha = body.size() == 8 ? pair(6) : 255;
    return true;
}

// splitChannels takes the text between the parentheses, drops whitespace and
// returns the comma-separated pieces.
QStringList splitChannels(const QString &body) {
    QStringList parts = body.split(u',');
    for (QString &part : parts) {
        part = part.trimmed();
        if (part.endsWith(u'%')) {
            part.chop(1);
        }
    }
    return parts;
}

bool parseChannel(const QString &text, int *out) {
    bool ok = false;
    const int value = text.toInt(&ok);
    *out = ok ? value : -1;
    return ok && value >= 0 && value <= 255;
}

bool parseAlpha(const QString &text, int *alpha) {
    if (text.contains(u'.')) {
        bool ok = false;
        const double value = text.toDouble(&ok);
        if (!ok || value < 0.0 || value > 1.0) {
            return false;
        }
        *alpha = int(value * 255.0 + 0.5);
        return true;
    }
    return parseChannel(text, alpha);
}

bool parseFunctional(const QString &input, Rgba *color) {
    const int open = input.indexOf(u'(');
    if (open < 0 || !input.endsWith(u')')) {
        return false;
    }
    const QString name = input.left(open).trimmed();
    const bool hsl = name == QStringLiteral("hsl") || name == QStringLiteral("hsla");
    const bool rgb = name == QStringLiteral("rgb") || name == QStringLiteral("rgba");
    if (!hsl && !rgb) {
        return false;
    }
    const QStringList parts = splitChannels(input.mid(open + 1, input.size() - open - 2));
    if (parts.size() != 3 && parts.size() != 4) {
        return false;
    }
    bool ok = false;
    if (hsl) {
        const int hue = parts.at(0).toInt(&ok);
        if (!ok) {
            return false;
        }
        const int saturation = parts.at(1).toInt(&ok);
        if (!ok || saturation < 0 || saturation > 100) {
            return false;
        }
        const int lightness = parts.at(2).toInt(&ok);
        if (!ok || lightness < 0 || lightness > 100) {
            return false;
        }
        const double normalizedHue = (hue % 360 + 360) % 360 / 360.0;
        const double s = saturation / 100.0;
        const double l = lightness / 100.0;
        double red = 0;
        double green = 0;
        double blue = 0;
        if (s == 0.0) {
            red = green = blue = l;
        } else {
            const double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
            const double p = 2.0 * l - q;
            auto channel = [p, q](double t) {
                if (t < 0.0) {
                    t += 1.0;
                }
                if (t > 1.0) {
                    t -= 1.0;
                }
                if (t < 1.0 / 6.0) {
                    return p + (q - p) * 6.0 * t;
                }
                if (t < 0.5) {
                    return q;
                }
                if (t < 2.0 / 3.0) {
                    return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
                }
                return p;
            };
            red = channel(normalizedHue + 1.0 / 3.0);
            green = channel(normalizedHue);
            blue = channel(normalizedHue - 1.0 / 3.0);
        }
        color->red = int(red * 255.0 + 0.5);
        color->green = int(green * 255.0 + 0.5);
        color->blue = int(blue * 255.0 + 0.5);
    } else {
        if (!parseChannel(parts.at(0), &color->red) || !parseChannel(parts.at(1), &color->green) ||
            !parseChannel(parts.at(2), &color->blue)) {
            return false;
        }
    }
    color->alpha = 255;
    if (parts.size() == 4 && !parseAlpha(parts.at(3), &color->alpha)) {
        return false;
    }
    return true;
}

// rgbToHsl converts to integer degrees and percentages, as CSS displays them.
void rgbToHsl(const Rgba &color, int *hue, int *saturation, int *lightness) {
    const double red = color.red / 255.0;
    const double green = color.green / 255.0;
    const double blue = color.blue / 255.0;
    const double max = qMax(red, qMax(green, blue));
    const double min = qMin(red, qMin(green, blue));
    const double l = (max + min) / 2.0;
    double h = 0.0;
    double s = 0.0;
    if (max != min) {
        const double d = max - min;
        s = l > 0.5 ? d / (2.0 - max - min) : d / (max + min);
        if (max == red) {
            h = (green - blue) / d + (green < blue ? 6.0 : 0.0);
        } else if (max == green) {
            h = (blue - red) / d + 2.0;
        } else {
            h = (red - green) / d + 4.0;
        }
        h /= 6.0;
    }
    *hue = int(h * 360.0 + 0.5);
    *saturation = int(s * 100.0 + 0.5);
    *lightness = int(l * 100.0 + 0.5);
}

} // namespace

ColorTool::ColorTool(QObject *parent) : QObject(parent) {}

QVariantMap ColorTool::convert(const QString &text) {
    QVariantMap outcome;
    outcome.insert(QStringLiteral("ok"), false);
    const QString input = text.trimmed().toLower();
    if (input.isEmpty()) {
        outcome.insert(QStringLiteral("error"), QStringLiteral("请输入颜色值"));
        return outcome;
    }
    Rgba color;
    const bool parsed = input.startsWith(u'#') ? parseHexColor(input.mid(1), &color)
                                               : parseFunctional(input, &color);
    if (!parsed) {
        outcome.insert(
            QStringLiteral("error"),
            QStringLiteral("支持 #RGB、#RRGGBB、#RRGGBBAA、rgb()、rgba()、hsl()、hsla()"));
        return outcome;
    }

    const QString hex = QStringLiteral("#%1%2%3")
                            .arg(color.red, 2, 16, QChar(u'0'))
                            .arg(color.green, 2, 16, QChar(u'0'))
                            .arg(color.blue, 2, 16, QChar(u'0'))
                            .toUpper();
    QString rgba =
        QStringLiteral("rgb(%1, %2, %3)").arg(color.red).arg(color.green).arg(color.blue);
    if (color.alpha != 255) {
        rgba = QStringLiteral("rgba(%1, %2, %3, %4)")
                   .arg(color.red)
                   .arg(color.green)
                   .arg(color.blue)
                   .arg(color.alpha / 255.0, 0, 'f', 2);
    }
    int hue = 0;
    int saturation = 0;
    int lightness = 0;
    rgbToHsl(color, &hue, &saturation, &lightness);
    const QString hsl = QStringLiteral("hsl(%1, %2%, %3%)").arg(hue).arg(saturation).arg(lightness);

    outcome.insert(QStringLiteral("ok"), true);
    outcome.insert(QStringLiteral("hex"), hex);
    outcome.insert(QStringLiteral("rgba"), rgba);
    outcome.insert(QStringLiteral("hsl"), hsl);
    outcome.insert(QStringLiteral("alpha"), color.alpha);
    return outcome;
}

} // namespace hash_core
