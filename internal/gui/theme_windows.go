//go:build windows

package gui

import (
	"syscall"
	"unsafe"
)

const (
	canvasColor  uint32 = 0x00211408
	surfaceColor uint32 = 0x0031200E
	raisedColor  uint32 = 0x00432E14
	insetColor   uint32 = 0x001A2B38
	lineColor    uint32 = 0x005D4828
	textColor    uint32 = 0x00FBF5E8
	mutedColor   uint32 = 0x00BBA88E
	iceColor     uint32 = 0x00F7D876
	successColor uint32 = 0x00B0DB68
	warningColor uint32 = 0x007DD3FF
	dangerColor  uint32 = 0x009480FF
)

type theme struct {
	canvasBrush  syscall.Handle
	surfaceBrush syscall.Handle
	raisedBrush  syscall.Handle
	insetBrush   syscall.Handle
	lineBrush    syscall.Handle
	font         syscall.Handle
	monoFont     syscall.Handle
}

type paintStruct struct {
	hdc        syscall.Handle
	fErase     uint32
	rcPaint    rect
	fRestore   uint32
	fIncUpdate uint32
	reserved   [32]byte
}

type drawItemStruct struct {
	ctlType    uint32
	ctlID      uint32
	itemID     uint32
	itemAction uint32
	itemState  uint32
	hwndItem   syscall.Handle
	hdc        syscall.Handle
	rcItem     rect
	itemData   uintptr
}

const (
	odsSelected    = 0x0001
	odsDisabled    = 0x0004
	odsFocus       = 0x0010
	odsNoFocusRect = 0x0200
	dtLeft         = 0x00000000
	dtVCenter      = 0x00000004
	dtSingleLine   = 0x00000020
)

func newTheme(createBrush, createFont *syscall.LazyProc) theme {
	return theme{
		canvasBrush:  syscall.Handle(mustCall(createBrush, uintptr(canvasColor))),
		surfaceBrush: syscall.Handle(mustCall(createBrush, uintptr(surfaceColor))),
		raisedBrush:  syscall.Handle(mustCall(createBrush, uintptr(raisedColor))),
		insetBrush:   syscall.Handle(mustCall(createBrush, uintptr(insetColor))),
		lineBrush:    syscall.Handle(mustCall(createBrush, uintptr(lineColor))),
		font:         syscall.Handle(mustCallFont(createFont, -16, 400, "Microsoft YaHei UI")),
		monoFont:     syscall.Handle(mustCallFont(createFont, -15, 400, "Cascadia Mono")),
	}
}

func mustCall(proc *syscall.LazyProc, args ...uintptr) uintptr {
	value, _, _ := proc.Call(args...)
	return value
}

func mustCallFont(proc *syscall.LazyProc, height, weight int32, face string) uintptr {
	facePtr, _ := syscall.UTF16PtrFromString(face)
	value, _, _ := proc.Call(
		uintptr(height), 0, 0, 0, uintptr(weight), 0, 0, 0,
		0, 0, 0, 0, 0, uintptr(unsafe.Pointer(facePtr)),
	)
	return value
}

func destroyTheme(theme theme, deleteObject *syscall.LazyProc) {
	for _, handle := range []syscall.Handle{theme.canvasBrush, theme.surfaceBrush, theme.raisedBrush, theme.insetBrush, theme.lineBrush, theme.font, theme.monoFont} {
		if handle != 0 {
			deleteObject.Call(uintptr(handle))
		}
	}
}
