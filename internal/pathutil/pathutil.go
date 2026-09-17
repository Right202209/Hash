// Package pathutil provides the platform-aware path normalization shared by
// the CLI and the comparison code so that both agree on when two paths refer
// to the same file.
package pathutil

import (
	"path/filepath"
	"runtime"
	"strings"
)

// Key returns the canonical map key for path. The path is cleaned on every
// platform. Case is folded only on Windows, whose filesystems are
// case-insensitive; on other platforms the case is preserved because two
// distinct files may differ only by case.
func Key(path string) string {
	cleaned := filepath.Clean(path)
	if runtime.GOOS == "windows" {
		return strings.ToLower(cleaned)
	}
	return cleaned
}
