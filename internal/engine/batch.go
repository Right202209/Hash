package engine

import (
	"context"
	"os"
	"runtime"
	"sync"
)

// Progress is an immutable snapshot of batch hashing progress. Snapshots are
// delivered to BatchOptions.Progress from worker goroutines; the snapshot
// never aliases internal state and may be retained by the receiver.
//
// Progress follows a small state machine:
//
//   - Starting a file sets CurrentPath and CurrentSize and resets CurrentBytes
//     to zero.
//   - While the file is hashed, CurrentBytes grows monotonically up to
//     CurrentSize. CompletedBytes deliberately excludes the in-flight file, so
//     CompletedBytes+CurrentBytes is the number of bytes hashed so far.
//   - Completing a file, successfully or not, increments CompletedFiles, adds
//     the file's read bytes to CompletedBytes and resets CurrentBytes to zero.
//     CurrentPath keeps naming the file that just finished until another file
//     starts.
//
// TotalBytes is the sum of the sizes known when the batch started; files whose
// size could not be determined contribute zero.
type Progress struct {
	CompletedFiles int
	TotalFiles     int
	CompletedBytes int64
	TotalBytes     int64
	CurrentPath    string
	CurrentBytes   int64
	CurrentSize    int64
}

type BatchOptions struct {
	HashOptions Options
	Workers     int
	Progress    func(Progress)
	FailFast    bool
}

type FileRequest struct {
	Path       string
	Algorithms []string
}

type FileResult struct {
	Request FileRequest
	Result  Result
	Err     error
}

func HashFiles(ctx context.Context, requests []FileRequest, options BatchOptions) []FileResult {
	results := make([]FileResult, len(requests))
	if len(requests) == 0 {
		return results
	}
	workers := options.Workers
	if workers <= 0 {
		workers = min(4, runtime.GOMAXPROCS(0))
	}
	if workers > len(requests) {
		workers = len(requests)
	}

	var totalBytes int64
	fileSizes := make([]int64, len(requests))
	for index, request := range requests {
		if info, err := os.Stat(request.Path); err == nil {
			fileSizes[index] = info.Size()
			totalBytes += info.Size()
		}
	}
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()
	jobs := make(chan int)
	var group sync.WaitGroup
	group.Add(workers)
	tracker := &progressTracker{progress: Progress{TotalFiles: len(requests), TotalBytes: totalBytes}}
	for worker := 0; worker < workers; worker++ {
		go func() {
			defer group.Done()
			for index := range jobs {
				request := requests[index]
				if err := ctx.Err(); err != nil {
					results[index] = FileResult{Request: request, Err: err}
					continue
				}
				emitProgress(options.Progress, tracker.update(func(progress *Progress) {
					progress.CurrentPath = request.Path
					progress.CurrentBytes = 0
					progress.CurrentSize = fileSizes[index]
				}))

				hashOptions := options.HashOptions
				hashOptions.Progress = func(bytesRead int64) {
					emitProgress(options.Progress, tracker.update(func(progress *Progress) {
						progress.CurrentBytes = bytesRead
					}))
				}
				result, err := HashFileWithOptions(ctx, request.Path, request.Algorithms, hashOptions)
				results[index] = FileResult{Request: request, Result: result, Err: err}
				emitProgress(options.Progress, tracker.update(func(progress *Progress) {
					progress.CompletedFiles++
					progress.CompletedBytes += result.BytesRead
					progress.CurrentBytes = 0
				}))
				if err != nil && options.FailFast {
					cancel()
				}
			}
		}()
	}
	for index := range requests {
		if err := ctx.Err(); err != nil {
			results[index] = FileResult{Request: requests[index], Err: err}
			continue
		}
		select {
		case jobs <- index:
		case <-ctx.Done():
			results[index] = FileResult{Request: requests[index], Err: ctx.Err()}
		}
	}
	close(jobs)
	group.Wait()
	return results
}

// progressTracker serializes progress updates so that the shared counters stay
// consistent. The user callback is always invoked after the lock is released:
// a slow or re-entrant callback therefore cannot stall the workers or deadlock
// the batch. Callbacks may run concurrently with one another, so receivers
// must be safe for concurrent use.
type progressTracker struct {
	mu       sync.Mutex
	progress Progress
}

func (tracker *progressTracker) update(mutate func(*Progress)) Progress {
	tracker.mu.Lock()
	mutate(&tracker.progress)
	snapshot := tracker.progress
	tracker.mu.Unlock()
	return snapshot
}

func emitProgress(callback func(Progress), progress Progress) {
	if callback != nil {
		callback(progress)
	}
}
