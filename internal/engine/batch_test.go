package engine_test

import (
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
