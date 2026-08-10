//go:build !windows

package cli

import "os"

func replacePath(source, target string) error {
	return os.Rename(source, target)
}
