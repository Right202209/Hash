// Package atomicfile replaces files by writing a temporary sibling and
// renaming it over the destination, so readers never observe a partial file.
// It refuses to write through a symbolic link or to clobber one of the
// caller's input files.
package atomicfile

import (
	"fmt"
	"os"
	"path/filepath"

	"hash/internal/pathutil"
)

// Write atomically replaces path with content. inputs are paths that must not
// be overwritten: Write rejects a destination that is the same file as, or an
// alternate name for, any input.
func Write(path string, inputs []string, content []byte) error {
	file, tempPath, err := prepare(path, inputs)
	if err != nil {
		return err
	}
	if _, err := file.Write(content); err != nil {
		_ = file.Close()
		_ = os.Remove(tempPath)
		return fmt.Errorf("write temporary output: %w", err)
	}
	if err := file.Close(); err != nil {
		_ = os.Remove(tempPath)
		return fmt.Errorf("close temporary output: %w", err)
	}
	if err := commit(tempPath, path, inputs); err != nil {
		_ = os.Remove(tempPath)
		return err
	}
	return nil
}

func validate(path string, inputs []string) error {
	pathAbs, err := filepath.Abs(path)
	if err != nil {
		return fmt.Errorf("resolve output path: %w", err)
	}
	info, err := os.Lstat(path)
	if err == nil && info.Mode()&os.ModeSymlink != 0 {
		return fmt.Errorf("refusing to write through symbolic link %q", path)
	}
	if err != nil && !os.IsNotExist(err) {
		return fmt.Errorf("inspect output path: %w", err)
	}
	if err == nil {
		outputInfo, statErr := os.Stat(path)
		if statErr != nil {
			return fmt.Errorf("stat output path: %w", statErr)
		}
		for _, input := range inputs {
			inputInfo, inputErr := os.Stat(input)
			if inputErr == nil && os.SameFile(outputInfo, inputInfo) {
				return fmt.Errorf("output path must not refer to input %q", input)
			}
		}
	}
	for _, input := range inputs {
		inputAbs, absErr := filepath.Abs(input)
		if absErr == nil && pathutil.Key(inputAbs) == pathutil.Key(pathAbs) {
			return fmt.Errorf("output path must differ from input %q", input)
		}
	}
	return nil
}

func prepare(path string, inputs []string) (*os.File, string, error) {
	if err := validate(path, inputs); err != nil {
		return nil, "", err
	}
	file, err := os.CreateTemp(filepath.Dir(path), ".hash-output-*")
	if err != nil {
		return nil, "", fmt.Errorf("create temporary output: %w", err)
	}
	return file, file.Name(), nil
}

func commit(tempPath, path string, inputs []string) error {
	if err := validate(path, inputs); err != nil {
		return err
	}
	if err := replacePath(tempPath, path); err != nil {
		return fmt.Errorf("rename temporary output: %w", err)
	}
	return nil
}
