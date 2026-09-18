// Package engine streams file contents through one or more hash algorithms and
// orchestrates concurrent batch hashing.
package engine

import (
	"context"
	"encoding/hex"
	"errors"
	"fmt"
	"hash"
	"io"
	"os"

	"hash/internal/registry"
)

const defaultBufferSize = 1024 * 1024

// ErrNoAlgorithms is returned when a request selects no algorithms.
var ErrNoAlgorithms = errors.New("at least one algorithm is required")

// ErrEmptyAlgorithmName is returned for an empty algorithm name.
var ErrEmptyAlgorithmName = errors.New("algorithm name cannot be empty")

// ErrUnknownAlgorithm is returned for a name that is not registered.
var ErrUnknownAlgorithm = errors.New("unknown algorithm")

// ErrDuplicateAlgorithm is returned when a name is selected more than once.
var ErrDuplicateAlgorithm = errors.New("duplicate algorithm")

// HashFile hashes path with the default options.
func HashFile(ctx context.Context, path string, algorithms []string) (Result, error) {
	return HashFileWithOptions(ctx, path, algorithms, Options{})
}

// HashFileWithOptions hashes path, applying options. Symbolic links are followed
// and the opened file descriptor is authoritative, so the regular-file check and
// the read cannot be separated by a path swap. On a failure after reading began,
// the returned Result still carries the path and the number of bytes read before
// the failure, so progress accounting stays monotonic; digests are absent.
func HashFileWithOptions(ctx context.Context, path string, algorithms []string, options Options) (Result, error) {
	if err := ValidateAlgorithms(algorithms); err != nil {
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

	multiWriter := io.MultiWriter(writers...)
	buffer := make([]byte, bufferSize)
	var bytesRead int64
	for {
		if err := ctx.Err(); err != nil {
			return Result{Path: path, BytesRead: bytesRead}, err
		}
		count, readErr := file.Read(buffer)
		if count > 0 {
			if _, err := multiWriter.Write(buffer[:count]); err != nil {
				return Result{Path: path, BytesRead: bytesRead}, fmt.Errorf("hash %q: %w", path, err)
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
			return Result{Path: path, BytesRead: bytesRead}, fmt.Errorf("read %q: %w", path, readErr)
		}
	}

	after, err := file.Stat()
	if err != nil {
		return Result{Path: path, BytesRead: bytesRead}, fmt.Errorf("stat %q after hashing: %w", path, err)
	}
	pathAfter, err := os.Stat(path)
	if err != nil {
		return Result{Path: path, BytesRead: bytesRead}, fmt.Errorf("stat %q after hashing: %w", path, err)
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
		Changed:   !os.SameFile(after, pathAfter) || before.Size() != after.Size() || !before.ModTime().Equal(after.ModTime()),
	}, nil
}

// ValidateAlgorithms reports whether algorithms is a non-empty, duplicate-free
// list of registered algorithm names. Names must already be normalized to the
// lower-case registry form. The returned errors wrap ErrNoAlgorithms,
// ErrEmptyAlgorithmName, ErrUnknownAlgorithm or ErrDuplicateAlgorithm.
func ValidateAlgorithms(algorithms []string) error {
	if len(algorithms) == 0 {
		return ErrNoAlgorithms
	}
	seen := make(map[string]struct{}, len(algorithms))
	for _, name := range algorithms {
		if name == "" {
			return ErrEmptyAlgorithmName
		}
		if _, ok := registry.Lookup(name); !ok {
			return fmt.Errorf("%w %q", ErrUnknownAlgorithm, name)
		}
		if _, duplicate := seen[name]; duplicate {
			return fmt.Errorf("%w %q", ErrDuplicateAlgorithm, name)
		}
		seen[name] = struct{}{}
	}
	return nil
}
