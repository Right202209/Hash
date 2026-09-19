#pragma once

#include "core/digesters.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace hash_core {

// AlgorithmSpec describes one algorithm. name is the lower-case registry key;
// create returns a fresh digester for each file so digest states never share
// data.
struct AlgorithmSpec {
    QString name;
    QString label;
    QString category;
    bool secure = false;
    DigesterFactory create;
};

// Categories reported by AlgorithmSpec::category.
inline const char CategoryCryptographic[] = "cryptographic";
inline const char CategoryLegacy[] = "legacy";
inline const char CategoryChecksum[] = "checksum";
inline const char CategoryNonCrypto[] = "non-cryptographic";

// algorithms returns a copy of every registered algorithm, in display order.
QVector<AlgorithmSpec> algorithms();

// lookupAlgorithm returns the algorithm registered under name.
bool lookupAlgorithm(const QString &name, AlgorithmSpec *spec = nullptr);

} // namespace hash_core
