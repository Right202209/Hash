package cli

import (
	"errors"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"hash/internal/pathutil"
)

// ErrNoInput is returned when no file path or pattern was provided.
var ErrNoInput = errors.New("at least one file path or pattern is required")

func ExpandPaths(inputs []string, recursive, literal bool) ([]string, error) {
	if len(inputs) == 0 {
		return nil, ErrNoInput
	}
	paths := make([]string, 0, len(inputs))
	seen := make(map[string]struct{})
	for _, input := range inputs {
		matches, err := expandPath(input, recursive, literal)
		if err != nil {
			return nil, err
		}
		for _, match := range matches {
			cleaned := filepath.Clean(match)
			key := pathutil.Key(cleaned)
			if _, exists := seen[key]; exists {
				continue
			}
			seen[key] = struct{}{}
			paths = append(paths, cleaned)
		}
	}
	return paths, nil
}

func expandPath(input string, recursive, literal bool) ([]string, error) {
	if !literal && hasMeta(input) {
		matches, err := filepath.Glob(input)
		if err != nil {
			return nil, fmt.Errorf("expand %q: %w", input, err)
		}
		if len(matches) == 0 {
			return nil, fmt.Errorf("pattern %q matched no files", input)
		}
		return filterFiles(matches)
	}
	info, err := os.Stat(input)
	if err != nil {
		return nil, fmt.Errorf("stat %q: %w", input, err)
	}
	if !info.IsDir() {
		if !info.Mode().IsRegular() {
			return nil, fmt.Errorf("%q is not a regular file", input)
		}
		return []string{input}, nil
	}
	if !recursive {
		return nil, fmt.Errorf("%q is a directory; use --recursive to include its files", input)
	}
	var files []string
	err = filepath.WalkDir(input, func(path string, entry fs.DirEntry, walkErr error) error {
		if walkErr != nil {
			return walkErr
		}
		if entry.IsDir() {
			return nil
		}
		info, statErr := entry.Info()
		if statErr != nil {
			return statErr
		}
		if !info.Mode().IsRegular() {
			return nil
		}
		files = append(files, path)
		return nil
	})
	if err != nil {
		return nil, fmt.Errorf("walk %q: %w", input, err)
	}
	sort.Strings(files)
	return files, nil
}

func filterFiles(paths []string) ([]string, error) {
	files := make([]string, 0, len(paths))
	for _, path := range paths {
		info, err := os.Stat(path)
		if err != nil {
			return nil, fmt.Errorf("stat %q: %w", path, err)
		}
		if !info.Mode().IsRegular() {
			continue
		}
		files = append(files, path)
	}
	if len(files) == 0 {
		return nil, errors.New("pattern matched no files")
	}
	sort.Strings(files)
	return files, nil
}

func hasMeta(path string) bool {
	return strings.ContainsAny(path, "*?[")
}
