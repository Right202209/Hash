package cli_test

import (
	"bytes"
	"errors"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"hash/internal/cli"
	"hash/internal/engine"
)

func TestExpandPaths_GlobAndDeduplicates(t *testing.T) {
	dir := t.TempDir()
	for _, name := range []string{"a.txt", "b.txt", "skip.bin"} {
		if err := os.WriteFile(filepath.Join(dir, name), []byte(name), 0o600); err != nil {
			t.Fatal(err)
		}
	}
	paths, err := cli.ExpandPaths([]string{filepath.Join(dir, "*.txt"), filepath.Join(dir, "a.txt")}, false, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(paths) != 2 || !strings.HasSuffix(paths[0], "a.txt") || !strings.HasSuffix(paths[1], "b.txt") {
		t.Fatalf("paths = %v", paths)
	}
}

func TestWriteResults_TSVHasStableColumnsAndEscapesFormula(t *testing.T) {
	var output bytes.Buffer
	results := []engine.FileResult{
		{Request: engine.FileRequest{Path: "=file.txt"}, Result: engine.Result{Path: "=file.txt", Digests: map[string]string{"sha256": "abc"}}},
		{Request: engine.FileRequest{Path: "missing"}, Err: os.ErrNotExist},
	}
	if err := cli.WriteResults(&output, results, []string{"sha256"}, cli.FormatOptions{Format: "tsv", ShowSize: true, ShowModified: true, UTC: true}); err != nil {
		t.Fatal(err)
	}
	lines := strings.Split(strings.TrimSpace(output.String()), "\n")
	if len(lines) != 3 {
		t.Fatalf("lines = %q", lines)
	}
	for _, line := range lines {
		if got := len(strings.Split(line, "\t")); got != 6 {
			t.Fatalf("line %q has %d columns, want 6", line, got)
		}
	}
	if !strings.Contains(output.String(), "'=file.txt") {
		t.Fatalf("formula-like path was not escaped: %q", output.String())
	}
}

func TestParseAndSelectAlgorithms(t *testing.T) {
	config, err := cli.Parse([]string{"--algorithm", "SHA256, md5", "--format", "json", "file.bin"}, &bytes.Buffer{})
	if err != nil {
		t.Fatal(err)
	}
	algorithms, err := cli.SelectAlgorithms(config)
	if err != nil {
		t.Fatal(err)
	}
	if strings.Join(algorithms, ",") != "sha256,md5" {
		t.Fatalf("algorithms = %v", algorithms)
	}
}

func TestExpandPaths_NoInputReturnsSentinel(t *testing.T) {
	t.Parallel()
	if _, err := cli.ExpandPaths(nil, false, false); !errors.Is(err, cli.ErrNoInput) {
		t.Fatalf("ExpandPaths(nil) error = %v, want ErrNoInput", err)
	}
}

func TestParse_UnsupportedFormatReturnsSentinel(t *testing.T) {
	t.Parallel()
	if _, err := cli.Parse([]string{"--format", "yaml", "file.bin"}, &bytes.Buffer{}); !errors.Is(err, cli.ErrUnsupportedFormat) {
		t.Fatalf("Parse(--format yaml) error = %v, want ErrUnsupportedFormat", err)
	}
}
