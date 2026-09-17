//go:build windows

package atomicfile

import (
	"fmt"
	"syscall"
	"unsafe"
)

var moveFileEx = syscall.NewLazyDLL("kernel32.dll").NewProc("MoveFileExW")

const moveFileReplaceExisting = 0x1

func replacePath(source, target string) error {
	sourcePtr, err := syscall.UTF16PtrFromString(source)
	if err != nil {
		return err
	}
	targetPtr, err := syscall.UTF16PtrFromString(target)
	if err != nil {
		return err
	}
	result, _, callErr := moveFileEx.Call(uintptr(unsafe.Pointer(sourcePtr)), uintptr(unsafe.Pointer(targetPtr)), moveFileReplaceExisting)
	if result == 0 {
		return fmt.Errorf("replace path: %w", callErr)
	}
	return nil
}
