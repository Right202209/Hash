package engine_test

import (
	"bytes"
	"context"
	"os"
	"path/filepath"
	"sync"
	"testing"

	"hash/internal/engine"
)

func TestHashFiles_PreservesInputOrderAndReportsProgress(t *testing.T) {
	dir := t.TempDir()
	paths := []string{filepath.Join(dir, "one.bin"), filepath.Join(dir, "two.bin")}
	for index, path := range paths {
		if err := os.WriteFile(path, []byte{byte(index), 1, 2, 3}, 0o600); err != nil {
			t.Fatal(err)
		}
	}
	var mu sync.Mutex
	var progress []engine.Progress
	results := engine.HashFiles(context.Background(), []engine.FileRequest{
		{Path: paths[0], Algorithms: []string{"sha256", "md5"}},
		{Path: paths[1], Algorithms: []string{"sha256"}},
	}, engine.BatchOptions{Workers: 2, Progress: func(value engine.Progress) {
		mu.Lock()
		progress = append(progress, value)
		mu.Unlock()
	}})
	if len(results) != len(paths) {
		t.Fatalf("got %d results, want %d", len(results), len(paths))
	}
	for index, result := range results {
		if result.Err != nil {
			t.Fatalf("result %d error = %v", index, result.Err)
		}
		if result.Result.Path != paths[index] {
			t.Errorf("result %d path = %q, want %q", index, result.Result.Path, paths[index])
		}
	}
	if len(progress) < len(paths) {
		t.Fatalf("got %d progress events, want at least %d", len(progress), len(paths))
	}
}

func TestHashFiles_ContinuesAfterMissingFile(t *testing.T) {
	path := filepath.Join(t.TempDir(), "present.txt")
	if err := os.WriteFile(path, []byte("present"), 0o600); err != nil {
		t.Fatal(err)
	}
	results := engine.HashFiles(context.Background(), []engine.FileRequest{
		{Path: filepath.Join(t.TempDir(), "missing.txt"), Algorithms: []string{"sha256"}},
		{Path: path, Algorithms: []string{"sha256"}},
	}, engine.BatchOptions{Workers: 1})
	if results[0].Err == nil {
		t.Fatal("missing file error = nil")
	}
	if results[1].Err != nil {
		t.Fatalf("present file error = %v", results[1].Err)
	}
}

func TestHashFiles_FailFastCancelsRemainingWork(t *testing.T) {
	missing := filepath.Join(t.TempDir(), "missing.txt")
	present := filepath.Join(t.TempDir(), "present.txt")
	if err := os.WriteFile(present, []byte("present"), 0o600); err != nil {
		t.Fatal(err)
	}
	results := engine.HashFiles(context.Background(), []engine.FileRequest{
		{Path: missing, Algorithms: []string{"sha256"}},
		{Path: present, Algorithms: []string{"sha256"}},
	}, engine.BatchOptions{Workers: 1, FailFast: true})
	if results[0].Err == nil {
		t.Fatal("missing file error = nil")
	}
	if results[1].Err == nil {
		t.Fatal("remaining file should be cancelled in fail-fast mode")
	}
}

func TestHashFiles_ProgressAccountsCompletedAndCurrentBytes(t *testing.T) {
	t.Parallel()

	dir := t.TempDir()
	paths := []string{filepath.Join(dir, "one.bin"), filepath.Join(dir, "two.bin")}
	contents := [][]byte{bytes.Repeat([]byte{0x01}, 4096), bytes.Repeat([]byte{0x02}, 8192)}
	for index, path := range paths {
		if err := os.WriteFile(path, contents[index], 0o600); err != nil {
			t.Fatal(err)
		}
	}
	totalBytes := int64(len(contents[0]) + len(contents[1]))

	var mu sync.Mutex
	var snapshots []engine.Progress
	results := engine.HashFiles(context.Background(), []engine.FileRequest{
		{Path: paths[0], Algorithms: []string{"sha256"}},
		{Path: paths[1], Algorithms: []string{"sha256"}},
	}, engine.BatchOptions{Workers: 1, Progress: func(value engine.Progress) {
		mu.Lock()
		snapshots = append(snapshots, value)
		mu.Unlock()
	}})
	for index, result := range results {
		if result.Err != nil {
			t.Fatalf("result %d error = %v", index, result.Err)
		}
	}

	mu.Lock()
	defer mu.Unlock()
	if len(snapshots) == 0 {
		t.Fatal("no progress snapshots were reported")
	}
	for _, snapshot := range snapshots {
		if snapshot.CompletedBytes > totalBytes {
			t.Fatalf("CompletedBytes = %d exceeds TotalBytes = %d", snapshot.CompletedBytes, totalBytes)
		}
		if snapshot.CompletedBytes+snapshot.CurrentBytes > totalBytes {
			t.Fatalf("CompletedBytes+CurrentBytes = %d exceeds TotalBytes = %d", snapshot.CompletedBytes+snapshot.CurrentBytes, totalBytes)
		}
	}
	last := snapshots[len(snapshots)-1]
	if last.CompletedFiles != len(paths) {
		t.Fatalf("CompletedFiles = %d, want %d", last.CompletedFiles, len(paths))
	}
	if last.CompletedBytes != totalBytes {
		t.Fatalf("CompletedBytes = %d, want %d", last.CompletedBytes, totalBytes)
	}
	if last.CurrentBytes != 0 {
		t.Fatalf("CurrentBytes after completion = %d, want 0", last.CurrentBytes)
	}
	for _, snapshot := range snapshots {
		if snapshot.CompletedFiles == 1 && snapshot.CompletedBytes == int64(len(contents[0])) && snapshot.CurrentBytes == 0 {
			return
		}
	}
	t.Fatalf("no snapshot reported the first file as completed with CurrentBytes reset: %+v", snapshots)
}
