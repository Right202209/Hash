#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <functional>
#include <memory>

namespace hash_core {

// Digester is one incremental hash computation over the bytes of a single
// file. A fresh instance is created per file so digest states never share
// data, mirroring the previous Go implementation.
class Digester {
  public:
    virtual ~Digester() = default;

    virtual void update(const char *data, qint64 length) = 0;
    // digest returns the lower-case, fixed-width hexadecimal digest.
    virtual QByteArray digest() = 0;
    virtual void reset() = 0;
};

using DigesterFactory = std::function<std::unique_ptr<Digester>()>;

std::unique_ptr<Digester> makeQtDigester(QCryptographicHash::Algorithm algorithm);
std::unique_ptr<Digester> makeAdler32();
// makeCrc32 and makeCrc64 take the reflected polynomial; init and final xor
// use all-ones, matching the previous Go hash/crc32 and hash/crc64 output.
std::unique_ptr<Digester> makeCrc32(quint32 reflectedPolynomial);
std::unique_ptr<Digester> makeCrc64(quint64 reflectedPolynomial);
std::unique_ptr<Digester> makeFnv1_32();
std::unique_ptr<Digester> makeFnv1a_32();
std::unique_ptr<Digester> makeFnv1_64();
std::unique_ptr<Digester> makeFnv1a_64();
std::unique_ptr<Digester> makeFnv1_128();
std::unique_ptr<Digester> makeFnv1a_128();
std::unique_ptr<Digester> makeSha512_224();
std::unique_ptr<Digester> makeSha512_256();

} // namespace hash_core
