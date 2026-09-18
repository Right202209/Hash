//go:build windows

package gui

import (
	"syscall"
	"unsafe"
)

const (
	className        = "HashToolWindow"
	windowTitle      = "Hash / 文件完整性"
	wmCreate         = 0x0001
	wmDestroy        = 0x0002
	wmSize           = 0x0005
	wmPaint          = 0x000F
	wmEraseBkgnd     = 0x0014
	wmCommand        = 0x0111
	wmDropFiles      = 0x0233
	wmClose          = 0x0010
	wmCtlColorStatic = 0x0138
	wmCtlColorEdit   = 0x0133
	wmCtlColorButton = 0x0135
	wmDPIChanged     = 0x02E0
	wmGetMinMaxInfo  = 0x0024
	wmTimer          = 0x0113
	wmSetFont        = 0x0030

	wsOverlapped   = 0x00CF0000
	wsChild        = 0x40000000
	wsVisible      = 0x10000000
	wsBorder       = 0x00800000
	wsVScroll      = 0x00200000
	wsTabStop      = 0x00010000
	wsClipChildren = 0x02000000
	wsClipSiblings = 0x04000000
	wsExClientEdge = 0x00000200

	esMultiline    = 0x0004
	esAutovscroll  = 0x0040
	esReadonly     = 0x0800
	emSetLimitText = 0x00C5
	ssLeft         = 0x00000000
	bsPushbutton   = 0x00000000
	bsAutocheckbox = 0x00000003

	cfUnicodeText  = 13
	gmemMoveable   = 0x0002
	gmemZeroInit   = 0x0040
	swShow         = 5
	idiApplication = 32512
	idcArrow       = 32512
	colorWindow    = 5

	ofNExplorer        = 0x00080000
	ofNAllowMulti      = 0x00000200
	ofNFileMustExist   = 0x00001000
	ofNPathMustExist   = 0x00000800
	ofNOverwritePrompt = 0x00000002

	idAdd              = 1001
	idClear            = 1002
	idStart            = 1003
	idCancel           = 1004
	idCopy             = 1005
	idSave             = 1006
	idOutput           = 1100
	idStatus           = 1101
	idQueueInfo        = 1102
	idAlgorithmSummary = 1103
	idQueue            = 1104
	idProgress         = 1105
	idPasteCompare     = 1007
	idClearCompare     = 1008
	idCompareStatus    = 1106
	idResultSummary    = 1107
	idAlgorithmStart   = 1200
	progressTimerID    = 1
	progressTimerMS    = 100
	maxClipboardSize   = 16 << 20

	lvmFirst            = 0x1000
	lvmDeleteAllItems   = lvmFirst + 9
	lvmSetItemText      = lvmFirst + 46
	lvmInsertItem       = lvmFirst + 77
	lvmSetExtendedStyle = lvmFirst + 54
	lvmInsertColumn     = lvmFirst + 97
	lvifText            = 0x0001
	lvcfWidth           = 0x0002
	lvcfText            = 0x0004
	lvsReport           = 0x0001
	lvsShowSelAlways    = 0x0008
	lvsExFullRowSelect  = 0x00000020
	lvsExDoubleBuffer   = 0x00010000
	lvmSetBkColor       = lvmFirst + 1
	lvmSetTextBkColor   = lvmFirst + 38
	lvmSetTextColor     = lvmFirst + 37

	pbmSetPos     = 0x0402
	progressClass = "msctls_progress32"
	listViewClass = "SysListView32"
)

type point struct{ x, y int32 }
type msg struct {
	hwnd           uintptr
	message        uint32
	wParam, lParam uintptr
	time           uint32
	pt             point
}
type rect struct{ left, top, right, bottom int32 }

type wndClassEx struct {
	cbSize      uint32
	style       uint32
	windowProc  uintptr
	classExtra  int32
	windowExtra int32
	instance    syscall.Handle
	icon        syscall.Handle
	cursor      syscall.Handle
	background  syscall.Handle
	menuName    *uint16
	className   *uint16
	smallIcon   syscall.Handle
}
type openFileName struct {
	structSize                uint32
	owner, instance           uintptr
	filter                    *uint16
	customFilter              *uint16
	maxCustomFilter           uint32
	filterIndex               uint32
	file                      *uint16
	maxFile                   uint32
	fileTitle                 *uint16
	maxFileTitle              uint32
	initialDir                *uint16
	title                     *uint16
	flags                     uint32
	fileOffset, fileExtension uint16
	defaultExtension          *uint16
	customData, hook          uintptr
	templateName              *uint16
}
type initCommonControlsEx struct{ size, classes uint32 }
type lvColumn struct {
	mask         uint32
	fmt, cx      int32
	text         *uint16
	textMax      int32
	subItem      int32
	image, order int32
}
type lvItem struct {
	mask             uint32
	item, subItem    int32
	state, stateMask uint32
	text             *uint16
	textMax          int32
	image            int32
	param            uintptr
	indent           int32
	groupID          int32
	columns          uint32
	columnsPtr       *uint32
	columnFormats    *int32
	group            int32
}

var (
	user32   = syscall.NewLazyDLL("user32.dll")
	kernel32 = syscall.NewLazyDLL("kernel32.dll")
	shell32  = syscall.NewLazyDLL("shell32.dll")
	comdlg32 = syscall.NewLazyDLL("comdlg32.dll")
	comctl32 = syscall.NewLazyDLL("comctl32.dll")
	gdi32    = syscall.NewLazyDLL("gdi32.dll")
	dwmapi   = syscall.NewLazyDLL("dwmapi.dll")
	uxtheme  = syscall.NewLazyDLL("uxtheme.dll")

	registerClass          = user32.NewProc("RegisterClassExW")
	createWindow           = user32.NewProc("CreateWindowExW")
	defWindowProc          = user32.NewProc("DefWindowProcW")
	destroyWindow          = user32.NewProc("DestroyWindow")
	showWindow             = user32.NewProc("ShowWindow")
	updateWindow           = user32.NewProc("UpdateWindow")
	getMessage             = user32.NewProc("GetMessageW")
	translateMsg           = user32.NewProc("TranslateMessage")
	dispatchMsg            = user32.NewProc("DispatchMessageW")
	postQuit               = user32.NewProc("PostQuitMessage")
	loadCursor             = user32.NewProc("LoadCursorW")
	loadIcon               = user32.NewProc("LoadIconW")
	setWindowText          = user32.NewProc("SetWindowTextW")
	getClientRect          = user32.NewProc("GetClientRect")
	moveWindow             = user32.NewProc("MoveWindow")
	enableWindow           = user32.NewProc("EnableWindow")
	getDlgItem             = user32.NewProc("GetDlgItem")
	checkButton            = user32.NewProc("CheckDlgButton")
	isChecked              = user32.NewProc("IsDlgButtonChecked")
	sendMessage            = user32.NewProc("SendMessageW")
	openClipboard          = user32.NewProc("OpenClipboard")
	emptyClipboard         = user32.NewProc("EmptyClipboard")
	setClipboard           = user32.NewProc("SetClipboardData")
	getClipboardData       = user32.NewProc("GetClipboardData")
	closeClipboard         = user32.NewProc("CloseClipboard")
	globalAlloc            = kernel32.NewProc("GlobalAlloc")
	globalSize             = kernel32.NewProc("GlobalSize")
	globalLock             = kernel32.NewProc("GlobalLock")
	globalUnlock           = kernel32.NewProc("GlobalUnlock")
	globalFree             = kernel32.NewProc("GlobalFree")
	rtlMoveMemory          = kernel32.NewProc("RtlMoveMemory")
	dragAccept             = shell32.NewProc("DragAcceptFiles")
	dragQueryFile          = shell32.NewProc("DragQueryFileW")
	dragFinish             = shell32.NewProc("DragFinish")
	getOpenFile            = comdlg32.NewProc("GetOpenFileNameW")
	getSaveFile            = comdlg32.NewProc("GetSaveFileNameW")
	initCommonControls     = comctl32.NewProc("InitCommonControlsEx")
	createSolidBrush       = gdi32.NewProc("CreateSolidBrush")
	createFont             = gdi32.NewProc("CreateFontW")
	deleteObject           = gdi32.NewProc("DeleteObject")
	fillRect               = user32.NewProc("FillRect")
	frameRect              = user32.NewProc("FrameRect")
	beginPaint             = user32.NewProc("BeginPaint")
	endPaint               = user32.NewProc("EndPaint")
	drawText               = user32.NewProc("DrawTextW")
	setTextColor           = gdi32.NewProc("SetTextColor")
	setBkColor             = gdi32.NewProc("SetBkColor")
	setBkMode              = gdi32.NewProc("SetBkMode")
	selectObject           = gdi32.NewProc("SelectObject")
	dwmSetWindowAttribute  = dwmapi.NewProc("DwmSetWindowAttribute")
	setDPIAwarenessContext = user32.NewProc("SetProcessDpiAwarenessContext")
	getDPIForWindow        = user32.NewProc("GetDpiForWindow")
	setTimer               = user32.NewProc("SetTimer")
	killTimer              = user32.NewProc("KillTimer")
	invalidateRect         = user32.NewProc("InvalidateRect")
	setWindowTheme         = uxtheme.NewProc("SetWindowTheme")
)

func applyDarkTitleBar(hwnd uintptr) {
	value := uint32(1)
	dwmSetWindowAttribute.Call(hwnd, 20, uintptr(unsafe.Pointer(&value)), unsafe.Sizeof(value))
}

func getDlgItemValue(hwnd uintptr, id int) uintptr {
	value, _, _ := getDlgItem.Call(hwnd, uintptr(id))
	return value
}

func drawLabel(hdc uintptr, bounds rect, text string, color uint32, font syscall.Handle, background uint32) {
	setTextColor.Call(hdc, uintptr(color))
	setBkColor.Call(hdc, uintptr(background))
	setBkMode.Call(hdc, 1)
	if font != 0 {
		selectObject.Call(hdc, uintptr(font))
	}
	ptr, _ := syscall.UTF16PtrFromString(text)
	drawText.Call(hdc, uintptr(unsafe.Pointer(ptr)), ^uintptr(0), uintptr(unsafe.Pointer(&bounds)), dtLeft|dtVCenter|dtSingleLine)
}

func setWindowTextText(hwnd uintptr, text string) {
	if hwnd == 0 {
		return
	}
	ptr, _ := syscall.UTF16PtrFromString(text)
	setWindowText.Call(hwnd, uintptr(unsafe.Pointer(ptr)))
}

func boolToUintptr(value bool) uintptr {
	if value {
		return 1
	}
	return 0
}
