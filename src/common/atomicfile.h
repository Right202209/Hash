#pragma once

#include <QByteArray>
#include <QStringList>

namespace hash_core {

// writeAtomicFile atomically replaces path with content. inputs are paths
// that must not be overwritten: the write rejects a destination that is the
// same file as, or an alternate name for, any input. It refuses to write
// through a symbolic link. Returns an empty string on success, or the error
// message.
QString writeAtomicFile(const QString &path, const QStringList &inputs, const QByteArray &content);

// validateOutputFile reports whether writeAtomicFile would currently accept
// path, without writing anything. Callers use it to reject a bad destination
// before doing expensive work; writeAtomicFile re-validates before
// committing, so a check that has gone stale in the meantime is still caught.
QString validateOutputFile(const QString &path, const QStringList &inputs);

} // namespace hash_core
