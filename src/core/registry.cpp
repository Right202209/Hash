#include "core/registry.h"

#include <QHash>

namespace hash_core {
namespace {

QVector<AlgorithmSpec> buildSpecs() {
    return {
        {QStringLiteral("sha224"), QStringLiteral("SHA-224"),
         QString::fromLatin1(CategoryCryptographic), true,
         []() { return makeQtDigester(QCryptographicHash::Sha224); }},
        {QStringLiteral("sha256"), QStringLiteral("SHA-256"),
         QString::fromLatin1(CategoryCryptographic), true,
         []() { return makeQtDigester(QCryptographicHash::Sha256); }},
        {QStringLiteral("sha384"), QStringLiteral("SHA-384"),
         QString::fromLatin1(CategoryCryptographic), true,
         []() { return makeQtDigester(QCryptographicHash::Sha384); }},
        {QStringLiteral("sha512"), QStringLiteral("SHA-512"),
         QString::fromLatin1(CategoryCryptographic), true,
         []() { return makeQtDigester(QCryptographicHash::Sha512); }},
        {QStringLiteral("sha512-224"), QStringLiteral("SHA-512/224"),
         QString::fromLatin1(CategoryCryptographic), true, []() { return makeSha512_224(); }},
        {QStringLiteral("sha512-256"), QStringLiteral("SHA-512/256"),
         QString::fromLatin1(CategoryCryptographic), true, []() { return makeSha512_256(); }},
        {QStringLiteral("md5"), QStringLiteral("MD5"), QString::fromLatin1(CategoryLegacy), false,
         []() { return makeQtDigester(QCryptographicHash::Md5); }},
        {QStringLiteral("sha1"), QStringLiteral("SHA-1"), QString::fromLatin1(CategoryLegacy),
         false, []() { return makeQtDigester(QCryptographicHash::Sha1); }},
        {QStringLiteral("adler32"), QStringLiteral("Adler-32"),
         QString::fromLatin1(CategoryChecksum), false, []() { return makeAdler32(); }},
        {QStringLiteral("crc32-ieee"), QStringLiteral("CRC-32/IEEE"),
         QString::fromLatin1(CategoryChecksum), false, []() { return makeCrc32(0xEDB88320ULL); }},
        {QStringLiteral("crc32-castagnoli"), QStringLiteral("CRC-32C/Castagnoli"),
         QString::fromLatin1(CategoryChecksum), false, []() { return makeCrc32(0x82F63B78ULL); }},
        {QStringLiteral("crc32-koopman"), QStringLiteral("CRC-32/Koopman"),
         QString::fromLatin1(CategoryChecksum), false, []() { return makeCrc32(0xEB31D82EULL); }},
        {QStringLiteral("crc64-ecma"), QStringLiteral("CRC-64/ECMA"),
         QString::fromLatin1(CategoryChecksum), false,
         []() { return makeCrc64(0xC96C5795D7870F42ULL); }},
        {QStringLiteral("crc64-iso"), QStringLiteral("CRC-64/ISO"),
         QString::fromLatin1(CategoryChecksum), false,
         []() { return makeCrc64(0xD800000000000000ULL); }},
        {QStringLiteral("fnv1-32"), QStringLiteral("FNV-1 32"),
         QString::fromLatin1(CategoryNonCrypto), false, []() { return makeFnv1_32(); }},
        {QStringLiteral("fnv1a-32"), QStringLiteral("FNV-1a 32"),
         QString::fromLatin1(CategoryNonCrypto), false, []() { return makeFnv1a_32(); }},
        {QStringLiteral("fnv1-64"), QStringLiteral("FNV-1 64"),
         QString::fromLatin1(CategoryNonCrypto), false, []() { return makeFnv1_64(); }},
        {QStringLiteral("fnv1a-64"), QStringLiteral("FNV-1a 64"),
         QString::fromLatin1(CategoryNonCrypto), false, []() { return makeFnv1a_64(); }},
        {QStringLiteral("fnv1-128"), QStringLiteral("FNV-1 128"),
         QString::fromLatin1(CategoryNonCrypto), false, []() { return makeFnv1_128(); }},
        {QStringLiteral("fnv1a-128"), QStringLiteral("FNV-1a 128"),
         QString::fromLatin1(CategoryNonCrypto), false, []() { return makeFnv1a_128(); }},
    };
}

} // namespace

QVector<AlgorithmSpec> algorithms() {
    static const QVector<AlgorithmSpec> specs = buildSpecs();
    return specs;
}

bool lookupAlgorithm(const QString &name, AlgorithmSpec *spec) {
    static const QHash<QString, AlgorithmSpec> index = []() {
        QHash<QString, AlgorithmSpec> built;
        built.reserve(24);
        for (const AlgorithmSpec &entry : algorithms()) {
            built.insert(entry.name, entry);
        }
        return built;
    }();
    const auto found = index.constFind(name);
    if (found == index.constEnd()) {
        return false;
    }
    if (spec) {
        *spec = found.value();
    }
    return true;
}

} // namespace hash_core
