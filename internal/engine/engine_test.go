package engine_test

import (
	"bytes"
	"context"
	"crypto/md5"
	"crypto/sha1"
	"crypto/sha256"
	"crypto/sha512"
	"encoding/hex"
	"errors"
	"hash"
	"os"
	"strconv"
	"strings"
	"testing"

	"hash/internal/engine"
)

func TestHashFile_OneReadComputesEveryRequestedAlgorithm(t *testing.T) {
	t.Parallel()

	content := []byte("one input stream, several independent digest states: 中文 😀 ' OR 1=1 --\n")
	path := writeFile(t, content)
	requested := []string{"sha256", "md5", "sha512", "sha1"}

	got, err := engine.HashFile(context.Background(), path, requested)
	if err != nil {
		t.Fatalf("HashFile() error = %v", err)
	}
	if got.Reads != 1 {
		t.Fatalf("HashFile() performed %d reads, want exactly one logical input pass", got.Reads)
	}
	if got.BytesRead != int64(len(content)) {
		t.Fatalf("BytesRead = %d, want %d", got.BytesRead, len(content))
	}

	for _, algorithm := range requested {
		want := digest(t, algorithm, content)
		if got.Digests[algorithm] != want {
			t.Errorf("digest[%q] = %q, want %q", algorithm, got.Digests[algorithm], want)
		}
	}
}

func TestHashFile_PreservesRequestedResultOrder(t *testing.T) {
	t.Parallel()

	path := writeFile(t, []byte("stable ordering"))
	requested := []string{"sha512", "md5", "sha256", "sha1"}

	got, err := engine.HashFile(context.Background(), path, requested)
	if err != nil {
		t.Fatalf("HashFile() error = %v", err)
	}
	if !equalStrings(got.Order, requested) {
		t.Fatalf("result order = %v, want %v", got.Order, requested)
	}
}

func TestHashFile_LargeInputAndBufferBoundaries(t *testing.T) {
	t.Parallel()

	const size = 10*1024*1024 + 17
	content := bytes.Repeat([]byte{0xA5}, size)
	path := writeFile(t, content)

	for _, bufferSize := range []int{1, 2, 4095, 4096, 4097, 64 * 1024} {
		t.Run("buffer="+itoa(bufferSize), func(t *testing.T) {
			got, err := engine.HashFileWithOptions(context.Background(), path, []string{"sha256", "sha512"}, engine.Options{BufferSize: bufferSize})
			if err != nil {
				t.Fatalf("HashFileWithOptions() error = %v", err)
			}
			if got.BytesRead != int64(size) {
				t.Fatalf("BytesRead = %d, want %d", got.BytesRead, size)
			}
			for _, algorithm := range []string{"sha256", "sha512"} {
				if got.Digests[algorithm] != digest(t, algorithm, content) {
					t.Errorf("digest[%q] does not match standard library", algorithm)
				}
			}
		})
	}
}

func TestHashFile_CancellationStopsWork(t *testing.T) {
	t.Parallel()

	path := writeFile(t, bytes.Repeat([]byte("cancel me"), 1024*1024))
	ctx, cancel := context.WithCancel(context.Background())
	cancel()

	_, err := engine.HashFile(ctx, path, []string{"sha256"})
	if !errors.Is(err, context.Canceled) {
		t.Fatalf("HashFile() error = %v, want context.Canceled", err)
	}
}

func TestHashFile_MissingFileReturnsPathError(t *testing.T) {
	t.Parallel()

	_, err := engine.HashFile(context.Background(), t.TempDir()+"/does-not-exist.bin", []string{"sha256"})
	if err == nil {
		t.Fatal("HashFile() error = nil, want an error")
	}
	var pathError *os.PathError
	if !errors.As(err, &pathError) {
		t.Fatalf("HashFile() error = %T %v, want wrapped *os.PathError", err, err)
	}
}

func TestHashFile_InvalidRequestIsRejected(t *testing.T) {
	t.Parallel()

	path := writeFile(t, []byte("input"))
	for name, requested := range map[string][]string{
		"nil algorithms":      nil,
		"empty algorithms":    {},
		"unknown algorithm":   {"sha256", "not-registered"},
		"duplicate algorithm": {"sha256", "sha256"},
		"empty name":          {""},
	} {
		t.Run(name, func(t *testing.T) {
			_, err := engine.HashFile(context.Background(), path, requested)
			if err == nil {
				t.Fatalf("HashFile(%v) error = nil, want validation error", requested)
			}
		})
	}
}

func TestHashFile_EmptyFile(t *testing.T) {
	t.Parallel()

	path := writeFile(t, nil)
	got, err := engine.HashFile(context.Background(), path, []string{"sha256"})
	if err != nil {
		t.Fatalf("HashFile() error = %v", err)
	}
	if got.BytesRead != 0 {
		t.Fatalf("BytesRead = %d, want 0", got.BytesRead)
	}
	if got.Digests["sha256"] != digest(t, "sha256", nil) {
		t.Fatalf("empty-file digest = %q, want empty SHA-256 digest", got.Digests["sha256"])
	}
}

func writeFile(t *testing.T, content []byte) string {
	t.Helper()
	path := t.TempDir() + "/input.bin"
	if err := os.WriteFile(path, content, 0o600); err != nil {
		t.Fatalf("WriteFile() error = %v", err)
	}
	return path
}

func digest(t *testing.T, algorithm string, content []byte) string {
	t.Helper()
	var newHash func() hash.Hash
	switch algorithm {
	case "md5":
		newHash = md5.New
	case "sha1":
		newHash = sha1.New
	case "sha256":
		newHash = sha256.New
	case "sha512":
		newHash = sha512.New
	default:
		t.Fatalf("digest helper does not support %q", algorithm)
	}

	h := newHash()
	if _, err := h.Write(content); err != nil {
		t.Fatalf("hash.Write() error = %v", err)
	}
	return hex.EncodeToString(h.Sum(nil))
}

func equalStrings(left, right []string) bool {
	return strings.Join(left, "\x00") == strings.Join(right, "\x00")
}

func itoa(value int) string {
	return strconv.Itoa(value)
}
