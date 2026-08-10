package cli_test

import (
	"bytes"
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
	if err := cli.WriteResults(&output, results, []string{"sha256"}, "tsv", true, true, true); err != nil {
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

func TestWriteFileAtomic_RejectsHardLinkToInput(t *testing.T) {
	if err := os.WriteFile(filepath.Join(t.TempDir(), "placeholder"), nil, 0o600); err != nil {
		t.Fatal(err)
	}
	dir := t.TempDir()
	input := filepath.Join(dir, "input.bin")
	output := filepath.Join(dir, "output.txt")
	content := []byte("original input")
	if err := os.WriteFile(input, content, 0o600); err != nil {
		t.Fatal(err)
	}
	if err := os.Link(input, output); err != nil {
		t.Skipf("hard links unavailable: %v", err)
	}
	if err := cli.WriteFileAtomic(output, []string{input}, []byte("replacement")); err == nil {
		t.Fatal("WriteFileAtomic unexpectedly accepted hard link to input")
	}
	got, err := os.ReadFile(input)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != string(content) {
		t.Fatalf("input changed to %q", got)
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
