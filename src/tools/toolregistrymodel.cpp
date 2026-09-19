#include "tools/toolregistrymodel.h"

#include <QHash>

namespace hash_core {

ToolRegistryModel::ToolRegistryModel(QObject *parent) : QAbstractListModel(parent) {
    mTools = {
        {QStringLiteral("filehash"), QStringLiteral("文件哈希"),
         QStringLiteral("批量计算文件摘要，支持拖拽与剪贴板比对"), QStringLiteral("#"),
         QStringLiteral("hash md5 sha1 sha256 sha512 crc 校验 校验和 完整性 摘要 文件")},
        {QStringLiteral("texthash"), QStringLiteral("文本哈希"),
         QStringLiteral("计算一段文本的 MD5、SHA 等摘要"), QStringLiteral("≡"),
         QStringLiteral("hash md5 sha 摘要 文本 字符串")},
        {QStringLiteral("base64"), QStringLiteral("Base64 编解码"),
         QStringLiteral("Base64 编码与解码，支持换行容错"), QStringLiteral("64"),
         QStringLiteral("base64 编码 解码 编解码")},
        {QStringLiteral("url"), QStringLiteral("URL 编解码"),
         QStringLiteral("URL 百分号编码与解码"), QStringLiteral("%"),
         QStringLiteral("url encode decode 百分号 编码 解码 转义")},
        {QStringLiteral("json"), QStringLiteral("JSON 格式化"),
         QStringLiteral("格式化、压缩并校验 JSON 文本"), QStringLiteral("{}"),
         QStringLiteral("json 格式化 压缩 校验 美化")},
        {QStringLiteral("timestamp"), QStringLiteral("时间戳"),
         QStringLiteral("Unix 时间戳与日期时间互相转换"), QStringLiteral("⏱"),
         QStringLiteral("timestamp unix 时间戳 日期 时间 转换 epoch")},
        {QStringLiteral("uuid"), QStringLiteral("UUID 生成"),
         QStringLiteral("批量生成 UUID v4，可控制大小写与连字符"), QStringLiteral("⚿"),
         QStringLiteral("uuid guid 生成 随机 标识")},
        {QStringLiteral("radix"), QStringLiteral("进制转换"),
         QStringLiteral("在 2-36 进制之间转换整数值"), QStringLiteral("01"),
         QStringLiteral("radix 进制 二进制 八进制 十六进制 hex binary 转换")},
        {QStringLiteral("color"), QStringLiteral("颜色转换"),
         QStringLiteral("HEX、RGB 与 HSL 颜色格式互相转换"), QStringLiteral("◐"),
         QStringLiteral("color 颜色 hex rgb hsl 转换 调色")},
        {QStringLiteral("password"), QStringLiteral("密码生成"),
         QStringLiteral("按长度与字符集批量生成随机密码"), QStringLiteral("⚿"),
         QStringLiteral("password 密码 随机 生成 安全 strong")},
    };
    rebuildVisible();
}

int ToolRegistryModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : mVisible.size();
}

QVariant ToolRegistryModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= mVisible.size()) {
        return QVariant();
    }
    const ToolSpec &spec = mTools.at(mVisible.at(index.row()));
    switch (role) {
    case ToolIdRole:
        return spec.id;
    case NameRole:
        return spec.name;
    case DescriptionRole:
        return spec.description;
    case IconRole:
        return spec.icon;
    case KeywordsRole:
        return spec.keywords;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ToolRegistryModel::roleNames() const {
    return {
        {ToolIdRole, "toolId"}, {NameRole, "name"},         {DescriptionRole, "description"},
        {IconRole, "icon"},     {KeywordsRole, "keywords"},
    };
}

QString ToolRegistryModel::filter() const { return mFilter; }

void ToolRegistryModel::setFilter(const QString &filter) {
    if (mFilter == filter) {
        return;
    }
    mFilter = filter;
    rebuildVisible();
    emit filterChanged();
}

QString ToolRegistryModel::firstToolId() const {
    return mVisible.isEmpty() ? QString() : mTools.at(mVisible.first()).id;
}

void ToolRegistryModel::rebuildVisible() {
    beginResetModel();
    mVisible.clear();
    const QString needle = mFilter.trimmed().toLower();
    for (int index = 0; index < mTools.size(); ++index) {
        if (needle.isEmpty()) {
            mVisible.append(index);
            continue;
        }
        const ToolSpec &spec = mTools.at(index);
        const QString haystack = spec.name.toLower() + QChar(u' ') + spec.description.toLower() +
                                 QChar(u' ') + spec.keywords.toLower() + QChar(u' ') +
                                 spec.id.toLower();
        if (haystack.contains(needle)) {
            mVisible.append(index);
        }
    }
    endResetModel();
}

} // namespace hash_core
