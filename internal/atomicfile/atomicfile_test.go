package atomicfile

import (
	"os"
	"path/filepath"
	"testing"
)

func TestWrite_RejectsHardLinkToInput(t *testing.T) {
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
	if err := Write(output, []string{input}, []byte("replacement")); err == nil {
		t.Fatal("Write unexpectedly accepted hard link to input")
	}
	got, err := os.ReadFile(input)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != string(content) {
		t.Fatalf("input changed to %q", got)
	}
}
