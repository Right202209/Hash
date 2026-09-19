#pragma once

#include "core/registry.h"

#include <QtTest>

#include <memory>

class TestRegistry : public QObject {
    Q_OBJECT

  private slots:
    void standardDigestVectors() {
        struct Vector {
            const char *algorithm;
            QByteArray digest;
        };
        const QString empty = QString();
        const QString abc = QStringLiteral("abc");
        const QString fox = QStringLiteral("The quick brown fox jumps over the lazy dog");
        const QString digits = QStringLiteral("123456789");
        struct Expectation {
            const char *algorithm;
            const char *emptyDigest;
            const char *abcDigest;
            const char *foxDigest;
            const char *digitsDigest;
        };
        const QVector<Expectation> expectations = {
            {"sha224", "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f",
             "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7",
             "730e109bd7a8a32b1cb9d9a09aa2325d2430587ddbc0c38bad911525", nullptr},
            {"sha256", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
             "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592", nullptr},
            {"sha384",
             "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f6"
             "5fbd51ad2f14898b95b",
             "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7c"
             "c2358baeca134c825a7",
             "ca737f1014a48f4c0b6dd43cb177b0afd9e5169367544c494011e3317dbf9a509cb1e5dc1e85a"
             "941bbee3d7f2afbc9b1",
             nullptr},
            {"sha512",
             "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f"
             "2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
             "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc"
             "1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
             "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f"
             "23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6",
             nullptr},
            {"sha512-224", "6ed0dd02806fa89e25de060c19d3ac86cabb87d6a0ddd05c333b84f4",
             "4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa",
             "944cd2847fb54558d4775db0485a50003111c8e5daa63fe722c6aa37",
             "f2a68a474bcbea375e9fc62eaab7b81fefbda64bb1c72d72e7c27314"},
            {"sha512-256", "c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a",
             "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23",
             "dd9d67b371519c339ed8dbd25af90e976a1eeefd4ad3d889005e532fc5bef04d",
             "1877345237853a31ad79e14c1fcb0ddcd3df9973b61af7f906e4b4d052cc9416"},
            {"md5", "d41d8cd98f00b204e9800998ecf8427e", "900150983cd24fb0d6963f7d28e17f72",
             "9e107d9d372bb6826bd81d3542a419d6", nullptr},
            {"sha1", "da39a3ee5e6b4b0d3255bfef95601890afd80709",
             "a9993e364706816aba3e25717850c26c9cd0d89d", "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12",
             nullptr},
            {"adler32", "00000001", "024d0127", "5bdc0fda", "091e01de"},
            {"crc32-ieee", "00000000", "352441c2", "414fa339", "cbf43926"},
            {"crc32-castagnoli", "00000000", "364b3fb7", "22620404", "e3069283"},
            {"crc32-koopman", "00000000", "ba2322ac", "e021db90", "2d3dd0ae"},
            {"crc64-ecma", "0000000000000000", "2cd8094a1a277627", "5b5eb8c2e54aa1c4",
             "995dc9bbdf1939fa"},
            {"crc64-iso", "0000000000000000", "3776c42000000000", "4ef14e19f4c6e28e",
             "b90956c775a41001"},
            {"fnv1-32", "811c9dc5", "439c2f4b", "e9c86c6e", "24148816"},
            {"fnv1a-32", "811c9dc5", "1a47e90b", "048fff90", "bb86b11c"},
            {"fnv1-64", "cbf29ce484222325", "d8dcca186bafadcb", "a8b2f3117de37ace",
             "a72ffc362bf916d6"},
            {"fnv1a-64", "cbf29ce484222325", "e71fa2190541574b", "f3f9b7f5e7e47110",
             "06d5573923c6cdfc"},
            {"fnv1-128", "6c62272e07bb014262b821756295c58d", "a68bb2a4348b5822836dbc78c6aee73b",
             "185adb693e7c97844ecfa9497cb529b6", "8bea2c73be03b30fd4142fb1ec2c2066"},
            {"fnv1a-128", "6c62272e07bb014262b821756295c58d", "a68d622cec8b5822836dbc7977af7f3b",
             "68cce4cd885ea04239f02af30e297870", "da2d42a08d04e4585dd325117f71d504"},
        };

        for (const Expectation &expectation : expectations) {
            hash_core::AlgorithmSpec spec;
            QVERIFY2(hash_core::lookupAlgorithm(QString::fromLatin1(expectation.algorithm), &spec),
                     qPrintable(QStringLiteral("algorithm %1 should be registered")
                                    .arg(QString::fromLatin1(expectation.algorithm))));
            const QVector<QPair<QString, const char *>> cases = {
                {empty, expectation.emptyDigest},
                {abc, expectation.abcDigest},
                {fox, expectation.foxDigest},
                {digits, expectation.digitsDigest}};
            for (const auto &testCase : cases) {
                if (!testCase.second) {
                    continue;
                }
                const QByteArray input = testCase.first.toUtf8();
                const std::unique_ptr<hash_core::Digester> digester = spec.create();
                digester->update(input.constData(), input.size());
                QCOMPARE(digester->digest(), QByteArray(testCase.second));
            }
        }
    }

    void classificationAndSecurityMarkers() {
        const QHash<QString, QPair<QString, bool>> want = {
            {QStringLiteral("md5"), {QStringLiteral("legacy"), false}},
            {QStringLiteral("sha1"), {QStringLiteral("legacy"), false}},
            {QStringLiteral("sha224"), {QStringLiteral("cryptographic"), true}},
            {QStringLiteral("sha256"), {QStringLiteral("cryptographic"), true}},
            {QStringLiteral("sha384"), {QStringLiteral("cryptographic"), true}},
            {QStringLiteral("sha512"), {QStringLiteral("cryptographic"), true}},
        };
        const QVector<hash_core::AlgorithmSpec> algorithms = hash_core::algorithms();
        QVERIFY(algorithms.size() >= want.size());
        QCOMPARE(algorithms.size(), 20);
        QSet<QString> seen;
        for (const hash_core::AlgorithmSpec &algorithm : algorithms) {
            QVERIFY2(!seen.contains(algorithm.name),
                     qPrintable(QStringLiteral("duplicate algorithm %1").arg(algorithm.name)));
            seen.insert(algorithm.name);
            const auto found = want.constFind(algorithm.name);
            if (found == want.constEnd()) {
                continue;
            }
            QCOMPARE(algorithm.category, found->first);
            QCOMPARE(algorithm.secure, found->second);
            QVERIFY(bool(algorithm.create));
        }
    }

    void lookupUnknownAlgorithm() {
        QVERIFY(!hash_core::lookupAlgorithm(QStringLiteral("sha3-256")));
    }
};
