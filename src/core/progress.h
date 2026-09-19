#pragma once

#include "core/types.h"

#include <QtGlobal>

namespace hash_core {

// mulHigh64 returns the high 64 bits of a 64x64 multiply. Portable to
// compilers without __int128 (MSVC).
inline quint64 mulHigh64(quint64 left, quint64 right) {
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

// mulDiv64 returns floor(left * right / divisor) with 128-bit intermediate
// precision, so byte counts near 2^64 do not overflow.
inline quint64 mulDiv64(quint64 left, quint64 right, quint64 divisor) {
    if (divisor == 0) {
        return 0;
    }
    const quint64 productHigh = mulHigh64(left, right);
    const quint64 productLow = left * right;
    if (productHigh == 0) {
        return productLow / divisor;
    }

    // Bitwise long division over the 128-bit product. The remainder stays
    // below divisor (< 2^64); the shifted value can reach 2^64, tracked in
    // remainderCarry.
    quint64 quotient = 0;
    quint64 remainder = 0;
    bool remainderCarry = false;
    for (int bit = 127; bit >= 0; --bit) {
        const bool next = bit >= 64 ? ((productHigh >> (bit - 64)) & 1ULL) != 0
                                    : ((productLow >> quint64(bit)) & 1ULL) != 0;
        remainderCarry = (remainder >> 63) != 0;
        remainder = (remainder << 1) | (next ? 1ULL : 0ULL);
        const bool subtract = remainderCarry || remainder >= divisor;
        if (subtract) {
            remainder -= divisor;
        }
        quotient = (quotient << 1) | (subtract ? 1ULL : 0ULL);
    }
    return quotient;
}

// progressPercent converts a progress snapshot into a percentage. It uses
// byte-precision math while bytes are known and falls back to file counts for
// zero-byte inputs.
inline int progressPercent(const Progress &progress) {
    int percent = 0;
    if (progress.totalBytes > 0) {
        qint64 completed = progress.completedBytes + progress.currentBytes;
        if (completed > progress.totalBytes) {
            completed = progress.totalBytes;
        }
        if (completed > 0) {
            percent = int(mulDiv64(quint64(completed), 100, quint64(progress.totalBytes)));
        }
    } else if (progress.totalFiles > 0) {
        percent = progress.completedFiles * 100 / progress.totalFiles;
    }
    return qBound(0, percent, 100);
}

} // namespace hash_core
