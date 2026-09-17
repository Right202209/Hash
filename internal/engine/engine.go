package engine

import (
	"context"
	"encoding/hex"
	"errors"
	"fmt"
	"hash"
	"io"
	"os"
	"sort"

	"hash/internal/registry"
)

const defaultBufferSize = 1024 * 1024

func HashFile(ctx context.Context, path string, algorithms []string) (Result, error) {
	return HashFileWithOptions(ctx, path, algorithms, Options{})
}

func HashFileWithOptions(ctx context.Context, path string, algorithms []string, options Options) (Result, error) {
	if err := validateRequest(algorithms); err != nil {
		return Result{}, err
	}
	if err := ctx.Err(); err != nil {
		return Result{}, err
	}
	bufferSize := options.BufferSize
	if bufferSize <= 0 {
		bufferSize = defaultBufferSize
	}

	// Open before inspecting: the returned file descriptor is authoritative,
	// so there is no window between a path-based regular-file check and the
	// read in which the path could be replaced. Symbolic links are followed,
	// matching the CLI's path expansion; non-regular targets are rejected.
	file, err := os.Open(path)
	if err != nil {
		return Result{}, fmt.Errorf("open %q: %w", path, err)
	}
	defer file.Close()

	before, err := file.Stat()
	if err != nil {
		return Result{}, fmt.Errorf("stat %q: %w", path, err)
	}
	if !before.Mode().IsRegular() {
		return Result{}, fmt.Errorf("hash input %q is not a regular file", path)
	}
	digesters := make([]hash.Hash, len(algorithms))
	writers := make([]io.Writer, len(digesters))
	for index, name := range algorithms {
		spec, _ := registry.Lookup(name)
		digesters[index] = spec.New()
		writers[index] = digesters[index]
	}

	buffer := make([]byte, bufferSize)
	var bytesRead int64
	for {
		if err := ctx.Err(); err != nil {
			return Result{}, err
		}
		count, readErr := file.Read(buffer)
		if count > 0 {
			if _, err := io.MultiWriter(writers...).Write(buffer[:count]); err != nil {
				return Result{}, fmt.Errorf("hash %q: %w", path, err)
			}
			bytesRead += int64(count)
			if options.Progress != nil {
				options.Progress(bytesRead)
			}
		}
		if errors.Is(readErr, io.EOF) {
			break
		}
		if readErr != nil {
			return Result{}, fmt.Errorf("read %q: %w", path, readErr)
		}
	}

	after, err := file.Stat()
	if err != nil {
		return Result{}, fmt.Errorf("stat %q after hashing: %w", path, err)
	}
	pathAfter, err := os.Stat(path)
	if err != nil {
		return Result{}, fmt.Errorf("stat %q after hashing: %w", path, err)
	}
	digests := make(map[string]string, len(digesters))
	for index, digester := range digesters {
		digests[algorithms[index]] = hex.EncodeToString(digester.Sum(nil))
	}
	return Result{
		Path:      path,
		Size:      before.Size(),
		Modified:  before.ModTime(),
		Digests:   digests,
		Order:     append([]string(nil), algorithms...),
		BytesRead: bytesRead,
		Reads:     1,
		Changed:   !os.SameFile(after, pathAfter) || before.Size() != after.Size() || !before.ModTime().Equal(after.ModTime()),
	}, nil
}

func validateRequest(algorithms []string) error {
	if len(algorithms) == 0 {
		return errors.New("at least one algorithm is required")
	}
	seen := make(map[string]struct{}, len(algorithms))
	for _, name := range algorithms {
		if name == "" {
			return errors.New("algorithm name cannot be empty")
		}
		if _, ok := registry.Lookup(name); !ok {
			return fmt.Errorf("unknown algorithm %q", name)
		}
		if _, duplicate := seen[name]; duplicate {
			return fmt.Errorf("algorithm %q was selected more than once", name)
		}
		seen[name] = struct{}{}
	}
	return nil
}

func sortedAlgorithmNames() []string {
	algorithms := registry.Algorithms()
	names := make([]string, len(algorithms))
	for index, algorithm := range algorithms {
		names[index] = algorithm.Name
	}
	sort.Strings(names)
	return names
}
