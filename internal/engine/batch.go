package engine

import (
	"context"
	"os"
	"runtime"
	"sync"
)

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
		workers = minInt(4, runtime.GOMAXPROCS(0))
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
	var stateMu sync.Mutex
	completedFiles := 0
	var completedBytes int64
	for worker := 0; worker < workers; worker++ {
		go func() {
			defer group.Done()
			for index := range jobs {
				request := requests[index]
				if err := ctx.Err(); err != nil {
					results[index] = FileResult{Request: request, Err: err}
					continue
				}
				stateMu.Lock()
				emitProgress(options.Progress, Progress{TotalFiles: len(requests), TotalBytes: totalBytes, CurrentPath: request.Path})
				stateMu.Unlock()

				hashOptions := options.HashOptions
				hashOptions.Progress = func(bytesRead int64) {
					stateMu.Lock()
					emitProgress(options.Progress, Progress{
						CompletedFiles: completedFiles,
						TotalFiles:     len(requests),
						CompletedBytes: completedBytes,
						TotalBytes:     totalBytes,
						CurrentPath:    request.Path,
						CurrentBytes:   bytesRead,
						CurrentSize:    fileSizes[index],
					})
					stateMu.Unlock()
				}
				result, err := HashFileWithOptions(ctx, request.Path, request.Algorithms, hashOptions)
				results[index] = FileResult{Request: request, Result: result, Err: err}
				stateMu.Lock()
				completedFiles++
				completedBytes += result.BytesRead
				emitProgress(options.Progress, Progress{
					CompletedFiles: completedFiles,
					TotalFiles:     len(requests),
					CompletedBytes: completedBytes,
					TotalBytes:     totalBytes,
					CurrentPath:    request.Path,
					CurrentSize:    result.Size,
				})
				stateMu.Unlock()
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

func minInt(left, right int) int {
	if left < right {
		return left
	}
	return right
}

func emitProgress(callback func(Progress), progress Progress) {
	if callback != nil {
		callback(progress)
	}
}
