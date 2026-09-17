//go:build !windows

package atomicfile

import "os"

func replacePath(source, target string) error {
	return os.Rename(source, target)
}
