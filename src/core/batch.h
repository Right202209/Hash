#pragma once

#include "core/hasher.h"

#include <mutex>

namespace hash_core {

// BatchOptions configures hashFiles. workers defaults to
// min(4, ideal thread count) and progress, when set, may be called
// concurrently. cancel is polled between files and between reads; setting it
// stops the batch.
struct BatchOptions {
    HashOptions hash;
    int workers = 0;
    std::function<void(const Progress &)> progress;
    std::function<bool()> canceled;
    bool failFast = false;
};

// hashFiles hashes requests concurrently and returns one result per request,
// in request order. failFast cancels the remaining work after the first
// failure.
QVector<FileResult> hashFiles(const QVector<FileRequest> &requests, const BatchOptions &options);

} // namespace hash_core
