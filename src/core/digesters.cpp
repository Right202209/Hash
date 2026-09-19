#include "core/digesters.h"

#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cstdint>

namespace hash_core {
namespace {

// ---------------------------------------------------------------------------
// SHA-512 constants. Generated with exact integer arithmetic (fractional
// parts of cube roots of the first 80 primes, and of square roots of the
// first 8 primes) and verified against FIPS 180-4 reference digests.
// ---------------------------------------------------------------------------
static const quint64 kSha512K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
};

static const quint64 kSha512Iv[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
};

static const quint64 kSha512_224Iv[8] = {
    0x8c3d37c819544da2ULL, 0x73e1996689dcd4d6ULL, 0x1dfab7ae32ff9c82ULL, 0x679dd514582f9fcfULL,
    0x0f6d2b697bd44da8ULL, 0x77e36f7304c48942ULL, 0x3f9d85a86a1d36c8ULL, 0x1112e6ad91d692a1ULL,
};

static const quint64 kSha512_256Iv[8] = {
    0x22312194fc2bf72cULL, 0x9f555fa3c84c64c2ULL, 0x2393b86b6f53b151ULL, 0x963877195940eabdULL,
    0x96283ee2a88effe3ULL, 0xbe5e1e2553863992ULL, 0x2b0199fc2c85b8aaULL, 0x0eb72ddc81c52ca2ULL,
};

// ---------------------------------------------------------------------------
// Qt-backed digester for the standard algorithms.
// ---------------------------------------------------------------------------
class QtDigester final : public Digester {
  public:
    explicit QtDigester(QCryptographicHash::Algorithm algorithm) : m_hash(algorithm) {}

    void update(const char *data, qint64 length) override {
        m_hash.addData(data, qsizetype(length));
    }

    QByteArray digest() override { return m_hash.result().toHex(); }

    void reset() override { m_hash.reset(); }

  private:
    QCryptographicHash m_hash;
};

// ---------------------------------------------------------------------------
// Adler-32 (mod 65521, processed in chunks that cannot overflow).
// ---------------------------------------------------------------------------
class Adler32Digester final : public Digester {
  public:
    void update(const char *data, qint64 length) override {
        quint32 a = m_a;
        quint32 b = m_b;
        const quint8 *bytes = reinterpret_cast<const quint8 *>(data);
        while (length > 0) {
            // zlib NMAX: the largest chunk that cannot overflow before the
            // modulo is applied.
            qint64 chunk = qMin<qint64>(length, 5552);
            for (qint64 index = 0; index < chunk; ++index) {
                a += bytes[index];
                b += a;
            }
            a %= 65521;
            b %= 65521;
            bytes += chunk;
            length -= chunk;
        }
        m_a = a;
        m_b = b;
    }

    QByteArray digest() override { return fixedHex(8, (quint64(m_b) << 16) | quint64(m_a)); }

    void reset() override {
        m_a = 1;
        m_b = 0;
    }

  private:
    quint32 m_a = 1;
    quint32 m_b = 0;
};

// ---------------------------------------------------------------------------
// Reflected CRC with all-ones init and final xor (Go-compatible output).
// ---------------------------------------------------------------------------
template <typename Word> class CrcDigester final : public Digester {
  public:
    explicit CrcDigester(Word reflectedPolynomial) : m_polynomial(reflectedPolynomial) {
        reset();
        buildTable();
    }

    void update(const char *data, qint64 length) override {
        Word crc = m_crc;
        const quint8 *bytes = reinterpret_cast<const quint8 *>(data);
        for (qint64 index = 0; index < length; ++index) {
            crc = (crc >> 8) ^ m_table[size_t((crc ^ bytes[index]) & 0xFF)];
        }
        m_crc = crc;
    }

    QByteArray digest() override {
        Word value = m_crc ^ ~Word(0);
        return fixedHex(sizeof(Word) * 2, quint64(value));
    }

    void reset() override { m_crc = ~Word(0); }

  private:
    void buildTable() {
        for (size_t index = 0; index < 256; ++index) {
            Word value = Word(index);
            for (int bit = 0; bit < 8; ++bit) {
                value = (value & 1) ? (value >> 1) ^ m_polynomial : value >> 1;
            }
            m_table[index] = value;
        }
    }

    Word m_polynomial;
    std::array<Word, 256> m_table;
    Word m_crc = 0;
};

// ---------------------------------------------------------------------------
// FNV-1 and FNV-1a for 32 and 64 bits.
// ---------------------------------------------------------------------------
template <typename Word, bool OneA> class FnvDigester final : public Digester {
  public:
    void update(const char *data, qint64 length) override {
        const quint8 *bytes = reinterpret_cast<const quint8 *>(data);
        for (qint64 index = 0; index < length; ++index) {
            if constexpr (OneA) {
                m_hash ^= Word(bytes[index]);
                m_hash *= kPrime;
            } else {
                m_hash *= kPrime;
                m_hash ^= Word(bytes[index]);
            }
        }
    }

    QByteArray digest() override { return fixedHex(sizeof(Word) * 2, quint64(m_hash)); }

    void reset() override { m_hash = kOffsetBasis; }

  private:
    static constexpr Word kOffsetBasis = Word(0x811C9DC5ULL);
    static constexpr Word kPrime = Word(0x01000193ULL);
    Word m_hash = kOffsetBasis;
};

template <typename Word, bool OneA> class Fnv64Digester final : public Digester {
  public:
    void update(const char *data, qint64 length) override {
        const quint8 *bytes = reinterpret_cast<const quint8 *>(data);
        for (qint64 index = 0; index < length; ++index) {
            if constexpr (OneA) {
                m_hash ^= Word(bytes[index]);
                m_hash *= kPrime;
            } else {
                m_hash *= kPrime;
                m_hash ^= Word(bytes[index]);
            }
        }
    }

    QByteArray digest() override { return fixedHex(16, quint64(m_hash)); }

    void reset() override { m_hash = kOffsetBasis; }

  private:
    static constexpr Word kOffsetBasis = Word(0xCBF29CE484222325ULL);
    static constexpr Word kPrime = Word(0x100000001B3ULL);
    Word m_hash = kOffsetBasis;
};

// ---------------------------------------------------------------------------
// FNV-1 and FNV-1a for 128 bits. The prime is 2^88 + 2^8 + 0x3b, so the
// 128x128 multiply is done with shifts and one small 64x64 multiply, which
// stays portable to compilers without __int128 (MSVC).
// ---------------------------------------------------------------------------
struct Uint128 {
    quint64 high = 0;
    quint64 low = 0;
};

quint64 mulHigh64(quint64 left, quint64 right) {
    quint64 leftLow = left & 0xFFFFFFFFULL;
    quint64 leftHigh = left >> 32;
    quint64 rightLow = right & 0xFFFFFFFFULL;
    quint64 rightHigh = right >> 32;
    quint64 p0 = leftLow * rightLow;
    quint64 p1 = leftLow * rightHigh;
    quint64 p2 = leftHigh * rightLow;
    quint64 p3 = leftHigh * rightHigh;
    quint64 middle = (p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL);
    return p3 + (p1 >> 32) + (p2 >> 32) + (middle >> 32);
}

Uint128 fnv128MultiplyByPrime(const Uint128 &value) {
    // t1 = value * 2^88 (mod 2^128): only the low word's low 40 bits survive.
    Uint128 t1{value.low << 24, 0};
    // t2 = value * 2^8.
    Uint128 t2{(value.high << 8) | (value.low >> 56), value.low << 8};
    // t3 = value * 0x3b.
    Uint128 t3{value.high * 0x3b + mulHigh64(value.low, 0x3b), value.low * 0x3b};
    Uint128 sum;
    sum.low = t2.low + t3.low;
    quint64 carry = sum.low < t2.low ? 1ULL : 0ULL;
    sum.high = t1.high + t2.high + t3.high + carry;
    return sum;
}

template <bool OneA> class Fnv128Digester final : public Digester {
  public:
    void update(const char *data, qint64 length) override {
        const quint8 *bytes = reinterpret_cast<const quint8 *>(data);
        for (qint64 index = 0; index < length; ++index) {
            if constexpr (OneA) {
                m_hash.low ^= quint64(bytes[index]);
                m_hash = fnv128MultiplyByPrime(m_hash);
            } else {
                m_hash = fnv128MultiplyByPrime(m_hash);
                m_hash.low ^= quint64(bytes[index]);
            }
        }
    }

    QByteArray digest() override { return fixedHex(16, m_hash.high) + fixedHex(16, m_hash.low); }

    void reset() override { m_hash = kOffsetBasis; }

  private:
    static const Uint128 kOffsetBasis;
    Uint128 m_hash = kOffsetBasis;
};

template <bool OneA>
const Uint128 Fnv128Digester<OneA>::kOffsetBasis = {0x6c62272e07bb0142ULL, 0x62b821756295c58dULL};

// ---------------------------------------------------------------------------
// SHA-512 and the SHA-512/224 and SHA-512/256 truncated variants, which use
// the same round function and constants with different initial values.
// ---------------------------------------------------------------------------
class Sha512Digester final : public Digester {
  public:
    Sha512Digester(const quint64 initial[8], int digestBytes) : m_digestBytes(digestBytes) {
        std::copy(initial, initial + 8, m_initial.begin());
        std::copy(initial, initial + 8, m_state.begin());
    }

    void update(const char *data, qint64 length) override {
        const quint8 *bytes = reinterpret_cast<const quint8 *>(data);
        m_byteCount += quint64(length);
        while (length > 0) {
            size_t space = 128 - m_bufferLength;
            size_t take = size_t(qMin<qint64>(qint64(space), length));
            std::copy(bytes, bytes + take, m_buffer.begin() + m_bufferLength);
            m_bufferLength += take;
            bytes += take;
            length -= qint64(take);
            if (m_bufferLength == 128) {
                compress(m_buffer.data());
                m_bufferLength = 0;
            }
        }
    }

    QByteArray digest() override {
        quint64 bitLength = m_byteCount << 3;
        quint64 bitLengthHigh = m_byteCount >> 61;
        quint8 finalBlock[128];
        size_t buffered = m_bufferLength;
        std::copy(m_buffer.begin(), m_buffer.begin() + buffered, finalBlock);
        finalBlock[buffered++] = 0x80;
        size_t padded = (buffered <= 112) ? 128 : 256;
        for (size_t index = buffered; index < padded - 16; ++index) {
            finalBlock[index] = 0;
        }
        storeBigEndian64(finalBlock + padded - 16, bitLengthHigh);
        storeBigEndian64(finalBlock + padded - 8, bitLength);
        compress(finalBlock);
        if (padded == 256) {
            compress(finalBlock + 128);
        }

        QByteArray raw;
        raw.resize(m_digestBytes);
        size_t offset = 0;
        for (int word = 0; word < 8 && offset < size_t(m_digestBytes); ++word) {
            quint8 encoded[8];
            storeBigEndian64(encoded, m_state[size_t(word)]);
            size_t take = qMin(size_t(8), size_t(m_digestBytes) - offset);
            std::copy(encoded, encoded + take, reinterpret_cast<quint8 *>(raw.data()) + offset);
            offset += take;
        }
        return raw.toHex();
    }

    void reset() override {
        std::copy(m_initial.begin(), m_initial.end(), m_state.begin());
        m_bufferLength = 0;
        m_byteCount = 0;
    }

  private:
    static quint64 loadBigEndian64(const quint8 *bytes) {
        quint64 value = 0;
        for (int index = 0; index < 8; ++index) {
            value = (value << 8) | quint64(bytes[index]);
        }
        return value;
    }

    static void storeBigEndian64(quint8 *bytes, quint64 value) {
        for (int index = 7; index >= 0; --index) {
            bytes[index] = quint8(value & 0xFF);
            value >>= 8;
        }
    }

    static quint64 rotateRight(quint64 value, unsigned int bits) {
        return (value >> bits) | (value << (64 - bits));
    }

    void compress(const quint8 *block) {
        std::array<quint64, 80> schedule;
        for (int index = 0; index < 16; ++index) {
            schedule[size_t(index)] = loadBigEndian64(block + index * 8);
        }
        for (int index = 16; index < 80; ++index) {
            quint64 s0 = rotateRight(schedule[size_t(index - 15)], 1) ^
                         rotateRight(schedule[size_t(index - 15)], 8) ^
                         (schedule[size_t(index - 15)] >> 7);
            quint64 s1 = rotateRight(schedule[size_t(index - 2)], 19) ^
                         rotateRight(schedule[size_t(index - 2)], 61) ^
                         (schedule[size_t(index - 2)] >> 6);
            schedule[size_t(index)] =
                schedule[size_t(index - 16)] + s0 + schedule[size_t(index - 7)] + s1;
        }

        quint64 a = m_state[0];
        quint64 b = m_state[1];
        quint64 c = m_state[2];
        quint64 d = m_state[3];
        quint64 e = m_state[4];
        quint64 f = m_state[5];
        quint64 g = m_state[6];
        quint64 h = m_state[7];
        for (int index = 0; index < 80; ++index) {
            quint64 s1 = rotateRight(e, 14) ^ rotateRight(e, 18) ^ rotateRight(e, 41);
            quint64 ch = (e & f) ^ (~e & g);
            quint64 t1 = h + s1 + ch + kSha512K[size_t(index)] + schedule[size_t(index)];
            quint64 s0 = rotateRight(a, 28) ^ rotateRight(a, 34) ^ rotateRight(a, 39);
            quint64 majority = (a & b) ^ (a & c) ^ (b & c);
            quint64 t2 = s0 + majority;
            quint64 newE = d + t1;
            quint64 newA = t1 + t2;
            h = g;
            g = f;
            f = e;
            e = newE;
            d = c;
            c = b;
            b = a;
            a = newA;
        }
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
        m_state[4] += e;
        m_state[5] += f;
        m_state[6] += g;
        m_state[7] += h;
    }

    std::array<quint64, 8> m_initial = {kSha512Iv[0], kSha512Iv[1], kSha512Iv[2], kSha512Iv[3],
                                        kSha512Iv[4], kSha512Iv[5], kSha512Iv[6], kSha512Iv[7]};
    std::array<quint64, 8> m_state;
    std::array<quint8, 128> m_buffer;
    size_t m_bufferLength = 0;
    quint64 m_byteCount = 0;
    int m_digestBytes;
};

QByteArray fixedHex(int width, quint64 value) {
    return QByteArray::number(value, 16).rightJustified(width, '0');
}

} // namespace

std::unique_ptr<Digester> makeQtDigester(QCryptographicHash::Algorithm algorithm) {
    return std::make_unique<QtDigester>(algorithm);
}

std::unique_ptr<Digester> makeAdler32() { return std::make_unique<Adler32Digester>(); }

std::unique_ptr<Digester> makeCrc32(quint32 reflectedPolynomial) {
    return std::make_unique<CrcDigester<quint32>>(reflectedPolynomial);
}

std::unique_ptr<Digester> makeCrc64(quint64 reflectedPolynomial) {
    return std::make_unique<CrcDigester<quint64>>(reflectedPolynomial);
}

std::unique_ptr<Digester> makeFnv1_32() { return std::make_unique<FnvDigester<quint32, false>>(); }

std::unique_ptr<Digester> makeFnv1a_32() { return std::make_unique<FnvDigester<quint32, true>>(); }

std::unique_ptr<Digester> makeFnv1_64() {
    return std::make_unique<Fnv64Digester<quint64, false>>();
}

std::unique_ptr<Digester> makeFnv1a_64() {
    return std::make_unique<Fnv64Digester<quint64, true>>();
}

std::unique_ptr<Digester> makeFnv1_128() { return std::make_unique<Fnv128Digester<false>>(); }

std::unique_ptr<Digester> makeFnv1a_128() { return std::make_unique<Fnv128Digester<true>>(); }

std::unique_ptr<Digester> makeSha512_224() {
    return std::make_unique<Sha512Digester>(kSha512_224Iv, 28);
}

std::unique_ptr<Digester> makeSha512_256() {
    return std::make_unique<Sha512Digester>(kSha512_256Iv, 32);
}

} // namespace hash_core
