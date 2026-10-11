#pragma once

#include "core/hasher.h"

#include <mutex>

namespace hash_core {

// BatchOptions configures hashFiles. workers defaults to
// min(4, ideal thread count), is capped at 64, and progress, when set, may be called
// concurrently. cancel is polled between files and between reads; setting it
// stops the batch.
struct BatchOptions {
    HashOptions hash;
    int workers = 0;
    std::function<void(const Progress &)> progress;
    std::function<bool()> canceled;
    // fileFinished, when set, is called once per request as soon as its result
    // is final, with the request index. It may be called concurrently.
    std::function<void(int, const FileResult &)> fileFinished;
    bool failFast = false;
};

// hashFiles hashes requests concurrently and returns one result per request,
// in request order. failFast cancels the remaining work after the first
// failure.
QVector<FileResult> hashFiles(const QVector<FileRequest> &requests, const BatchOptions &options);

} // namespace hash_core
