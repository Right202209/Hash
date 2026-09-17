package pathutil

import (
	"runtime"
	"testing"
)

func TestKey_CleansAndFoldsCaseOnlyOnWindows(t *testing.T) {
	t.Parallel()
	if got := Key("a/../b"); got != "b" {
		t.Fatalf("Key(%q) = %q, want %q", "a/../b", got, "b")
	}
	if runtime.GOOS == "windows" {
		if got := Key("Dir/File.bin"); got != "dir\\file.bin" {
			t.Fatalf("Key(%q) = %q, want %q", "Dir/File.bin", got, "dir\\file.bin")
		}
		return
	}
	if got := Key("Dir/File.bin"); got != "Dir/File.bin" {
		t.Fatalf("Key(%q) = %q, want case preserved", "Dir/File.bin", got)
	}
}
