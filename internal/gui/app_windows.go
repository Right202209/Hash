//go:build windows

package gui

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"sync/atomic"
	"syscall"
	"unsafe"

	"hash/internal/cli"
	"hash/internal/compare"
	"hash/internal/engine"
	"hash/internal/registry"
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
	wmNotify         = 0x004E
	wmDropFiles      = 0x0233
	wmClose          = 0x0010
	wmDrawItem       = 0x002B
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

	lvmFirst               = 0x1000
	lvmDeleteAllItems      = lvmFirst + 9
	lvmSetItemText         = lvmFirst + 46
	lvmInsertItem          = lvmFirst + 77
	lvmSetExtendedStyle    = lvmFirst + 54
	lvmInsertColumn        = lvmFirst + 97
	lvmSetColumnWidth      = lvmFirst + 30
	lvifText               = 0x0001
	lvcfWidth              = 0x0002
	lvcfText               = 0x0004
	lvscwAutosizeUseHeader = -2
	lvsReport              = 0x0001
	lvsShowSelAlways       = 0x0008
	lvsExFullRowSelect     = 0x00000020
	lvsExDoubleBuffer      = 0x00010000
	lvmSetBkColor          = lvmFirst + 1
	lvmSetTextBkColor      = lvmFirst + 38
	lvmSetTextColor        = lvmFirst + 37

	pbmSetRange32 = 0x0401
	pbmSetPos     = 0x0402
	pbmSetState   = 0x0410
	pbstNormal    = 0x0001
	pbstError     = 0x0002
	pbstPaused    = 0x0003
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
type minMaxInfo struct {
	reserved     point
	maxSize      point
	maxPosition  point
	minTrackSize point
	maxTrackSize point
}
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
type nmhdr struct {
	hwndFrom uintptr
	idFrom   uintptr
	code     uint32
}
type nmCustomDraw struct {
	hdr                nmhdr
	drawStage          uint32
	hdc                syscall.Handle
	rc                 rect
	itemSpec           uintptr
	itemState          uint32
	itemParam          uintptr
	clrText, clrTextBk uint32
	lItemlParam        int64
}
type nmListViewCustomDraw struct {
	custom                                 nmCustomDraw
	clrText, clrTextBk                     uint32
	subItem                                int32
	itemType                               uint32
	face                                   uint32
	iconEffect, iconPhase, partID, stateID int32
	rcText                                 rect
	align                                  uint32
}

type completedRun struct {
	results         []engine.FileResult
	rawOutput       string
	expectedRecords []compare.Record
	failures        int
	changed         int
	canceled        bool
	formatErr       error
}

type App struct {
	window                                                                                        uintptr
	output, status, queueInfo, algorithmSummary, resultSummary, compareStatus, queue, progressBar uintptr
	paths                                                                                         []string
	rowStates                                                                                     []string
	fileSizes                                                                                     []int64
	rowIndexByPath                                                                                map[string]int
	progressDirty                                                                                 atomic.Bool
	rawOutput                                                                                     string
	comparisonText                                                                                string
	algorithms                                                                                    []registry.Spec
	theme                                                                                         theme
	layout                                                                                        uiLayout
	dpi                                                                                           int32
	phase                                                                                         uiPhase
	cancel                                                                                        context.CancelFunc
	resultsMu                                                                                     sync.RWMutex
	run                                                                                           completedRun
	progressMu                                                                                    sync.RWMutex
	progress                                                                                      engine.Progress
	algorithmsInUse                                                                               []string
	workerDone                                                                                    chan struct{}
	closing                                                                                       atomic.Bool
}

var activeApp *App

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
	getWindowText          = user32.NewProc("GetWindowTextW")
	getWindowTextLength    = user32.NewProc("GetWindowTextLengthW")
	getClientRect          = user32.NewProc("GetClientRect")
	moveWindow             = user32.NewProc("MoveWindow")
	enableWindow           = user32.NewProc("EnableWindow")
	getDlgItem             = user32.NewProc("GetDlgItem")
	checkButton            = user32.NewProc("CheckDlgButton")
	isChecked              = user32.NewProc("IsDlgButtonChecked")
	postMessage            = user32.NewProc("PostMessageW")
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

func Run() {
	setDPIAwarenessContext.Call(^uintptr(3))
	commonControls := initCommonControlsEx{size: uint32(unsafe.Sizeof(initCommonControlsEx{})), classes: 0x00000008 | 0x00000040}
	initCommonControls.Call(uintptr(unsafe.Pointer(&commonControls)))
	app := &App{algorithms: registry.Algorithms(), dpi: 96, phase: phaseIdle}
	activeApp = app
	instance, _, _ := kernel32.NewProc("GetModuleHandleW").Call(0)
	classPtr, _ := syscall.UTF16PtrFromString(className)
	windowProc := syscall.NewCallback(windowProcedure)
	cursor, _, _ := loadCursor.Call(0, idcArrow)
	icon, _, _ := loadIcon.Call(0, idiApplication)
	class := wndClassEx{
		cbSize:     uint32(unsafe.Sizeof(wndClassEx{})),
		style:      0,
		windowProc: windowProc,
		instance:   syscall.Handle(instance),
		icon:       syscall.Handle(icon),
		cursor:     syscall.Handle(cursor),
		background: syscall.Handle(colorWindow + 1),
		className:  classPtr,
		smallIcon:  syscall.Handle(icon),
	}
	if result, _, err := registerClass.Call(uintptr(unsafe.Pointer(&class))); result == 0 && err != syscall.Errno(0) {
		panic(fmt.Errorf("register window class: %w", err))
	}
	titlePtr, _ := syscall.UTF16PtrFromString(windowTitle)
	hwnd, _, err := createWindow.Call(0, uintptr(unsafe.Pointer(classPtr)), uintptr(unsafe.Pointer(titlePtr)), wsOverlapped|wsVisible|wsClipChildren|wsClipSiblings, 0x80000000, 0x80000000, 1080, 760, 0, 0, instance, 0)
	if hwnd == 0 {
		panic(fmt.Errorf("create window: %w", err))
	}
	app.window = hwnd
	applyDarkTitleBar(hwnd)
	showWindow.Call(hwnd, swShow)
	updateWindow.Call(hwnd)
	var message msg
	for {
		result, _, _ := getMessage.Call(uintptr(unsafe.Pointer(&message)), 0, 0, 0)
		if result <= 0 {
			break
		}
		translateMsg.Call(uintptr(unsafe.Pointer(&message)))
		dispatchMsg.Call(uintptr(unsafe.Pointer(&message)))
	}
}

func applyDarkTitleBar(hwnd uintptr) {
	value := uint32(1)
	dwmSetWindowAttribute.Call(hwnd, 20, uintptr(unsafe.Pointer(&value)), unsafe.Sizeof(value))
}

func windowProcedure(hwnd uintptr, message uint32, wParam, lParam uintptr) uintptr {
	if activeApp == nil {
		result, _, _ := defWindowProc.Call(hwnd, uintptr(message), wParam, lParam)
		return result
	}
	return activeApp.handleMessage(hwnd, message, wParam, lParam)
}

func (app *App) handleMessage(hwnd uintptr, message uint32, wParam, lParam uintptr) uintptr {
	switch message {
	case wmCreate:
		app.window = hwnd
		if dpi, _, _ := getDPIForWindow.Call(hwnd); dpi != 0 {
			app.dpi = int32(dpi)
		}
		app.theme = newTheme(createSolidBrush, createFont)
		app.createControls(hwnd)
		dragAccept.Call(hwnd, 1)
		return 0
	case wmSize:
		app.layoutControls()
		return 0
	case wmTimer:
		if wParam == progressTimerID {
			app.processTimer()
		}
		return 0
	case wmGetMinMaxInfo:
		return 0
	case wmDPIChanged:
		if dpi := int32(wParam & 0xffff); dpi > 0 {
			app.dpi = dpi
		}
		app.layoutControls()
		invalidateRect.Call(hwnd, 0, 1)
		return 0
	case wmPaint:
		app.paint()
		return 0
	case wmEraseBkgnd:
		return 1
	case wmCommand:
		id := uint32(wParam & 0xffff)
		app.handleCommand(id)
		return 0
	case wmNotify:
		return app.handleNotify(lParam)
	case wmDrawItem:
		app.drawItem(lParam)
		return 1
	case wmCtlColorStatic, wmCtlColorEdit, wmCtlColorButton:
		return app.colorControl(wParam, lParam)
	case wmDropFiles:
		app.addDroppedFiles(wParam)
		return 0
	case wmClose:
		if app.closing.CompareAndSwap(false, true) && app.cancel != nil {
			app.phase = phaseStopping
			if app.workerDone == nil {
				destroyWindow.Call(hwnd)
				return 0
			}
			app.cancel()
			return 0
		}
		if app.workerDone == nil {
			destroyWindow.Call(hwnd)
		}
		return 0
	case wmDestroy:
		killTimer.Call(app.window, progressTimerID)
		destroyTheme(app.theme, deleteObject)
		postQuit.Call(0)
		return 0
	}
	result, _, _ := defWindowProc.Call(hwnd, uintptr(message), wParam, lParam)
	return result
}

func (app *App) createControls(hwnd uintptr) {
	app.createButton("添加文件", idAdd)
	app.createButton("清空", idClear)
	app.createButton("开始计算", idStart)
	app.createButton("复制", idCopy)
	app.createButton("保存", idSave)
	app.createButton("粘贴对比", idPasteCompare)
	app.createButton("清除对比", idClearCompare)
	app.createButton("取消", idCancel)
	for index, algorithm := range app.algorithms {
		app.createCheckbox(algorithm.Label, idAlgorithmStart+index)
	}
	app.queue = app.createControl("SysListView32", "", lvsReport|lvsShowSelAlways|wsChild|wsVisible|wsBorder|wsTabStop, idQueue)
	app.output = app.createEdit(idOutput)
	app.progressBar = app.createControl(progressClass, "", wsChild|wsVisible, idProgress)
	app.status = app.createControl("STATIC", "就绪 · 尚未添加文件", wsChild|wsVisible|ssLeft, idStatus)
	app.queueInfo = app.createControl("STATIC", "拖放文件到这里，或点击添加文件", wsChild|wsVisible|ssLeft, idQueueInfo)
	app.algorithmSummary = app.createControl("STATIC", "已选择 0 个算法", wsChild|wsVisible|ssLeft, idAlgorithmSummary)
	app.resultSummary = app.createControl("STATIC", "结果摘要：尚未计算", wsChild|wsVisible|ssLeft, idResultSummary)
	app.compareStatus = app.createControl("STATIC", "剪贴板比较：未比较", wsChild|wsVisible|ssLeft, idCompareStatus)
	app.configureListView()
	app.setDefaultFonts()
	checkButton.Call(hwnd, idAlgorithmStart+1, 1)
	app.setOutput("结果会显示在这里。\r\n选择文件和算法后，点击“开始计算”。")
	app.setBusy(false)
	app.rebuildQueue()
	app.updateSelectionSummary()
}

func (app *App) createButton(text string, id int) uintptr {
	return app.createControl("BUTTON", text, wsChild|wsVisible|wsTabStop|bsPushbutton, id)
}
func (app *App) createCheckbox(text string, id int) uintptr {
	return app.createControl("BUTTON", text, wsChild|wsVisible|wsTabStop|bsAutocheckbox, id)
}
func (app *App) createEdit(id int) uintptr {
	return app.createControlEx("EDIT", "", wsVScroll|esMultiline|esAutovscroll|esReadonly|wsChild|wsVisible|wsTabStop, wsExClientEdge, id)
}

func (app *App) createControl(class, text string, style uint32, id int) uintptr {
	return app.createControlEx(class, text, style, 0, id)
}
func (app *App) createControlEx(class, text string, style, exStyle uint32, id int) uintptr {
	classPtr, _ := syscall.UTF16PtrFromString(class)
	textPtr, _ := syscall.UTF16PtrFromString(text)
	hwnd, _, _ := createWindow.Call(uintptr(exStyle), uintptr(unsafe.Pointer(classPtr)), uintptr(unsafe.Pointer(textPtr)), uintptr(style), 0, 0, 0, 0, app.window, uintptr(id), 0, 0)
	if hwnd != 0 && app.theme.font != 0 {
		sendMessage.Call(hwnd, wmSetFont, uintptr(app.theme.font), 1)
	}
	return hwnd
}

func (app *App) setDefaultFonts() {
	if app.output != 0 && app.theme.monoFont != 0 {
		sendMessage.Call(app.output, wmSetFont, uintptr(app.theme.monoFont), 1)
	}
	if app.queue != 0 {
		setWindowTheme.Call(app.queue, 0, 0)
		sendMessage.Call(app.queue, lvmSetExtendedStyle, lvsExFullRowSelect|lvsExDoubleBuffer, lvsExFullRowSelect|lvsExDoubleBuffer)
		sendMessage.Call(app.queue, lvmSetBkColor, uintptr(canvasColor), 0)
		sendMessage.Call(app.queue, lvmSetTextBkColor, uintptr(insetColor), 0)
		sendMessage.Call(app.queue, lvmSetTextColor, uintptr(textColor), 0)
	}
	if app.output != 0 {
		sendMessage.Call(app.output, emSetLimitText, 0x7ffffffe, 0)
	}
}

func (app *App) configureListView() {
	columns := []struct {
		title string
		width int32
	}{{"文件", 0}, {"大小", 92}, {"状态", 92}}
	available := int32(680)
	if app.layout.queue.right > app.layout.queue.left {
		available = app.layout.queue.right - app.layout.queue.left - 8
	}
	columns[0].width = available - columns[1].width - columns[2].width
	if columns[0].width < 220 {
		columns[0].width = 220
	}
	for index, column := range columns {
		title, _ := syscall.UTF16PtrFromString(column.title)
		value := lvColumn{mask: lvcfText | lvcfWidth, cx: column.width, text: title}
		sendMessage.Call(app.queue, lvmInsertColumn, uintptr(index), uintptr(unsafe.Pointer(&value)))
	}
}

func (app *App) layoutControls() {
	if app.window == 0 {
		return
	}
	var client rect
	getClientRect.Call(app.window, uintptr(unsafe.Pointer(&client)))
	app.layout = calculateLayout(client.right-client.left, client.bottom-client.top, app.dpi, len(app.algorithms))
	positions := []struct {
		hwnd uintptr
		r    rect
	}{{getDlgItemValue(app.window, idAdd), app.layout.addButton}, {getDlgItemValue(app.window, idClear), app.layout.clearButton}, {getDlgItemValue(app.window, idStart), app.layout.startButton}, {getDlgItemValue(app.window, idCopy), app.layout.copyButton}, {getDlgItemValue(app.window, idSave), app.layout.saveButton}, {getDlgItemValue(app.window, idPasteCompare), app.layout.pasteCompareButton}, {getDlgItemValue(app.window, idClearCompare), app.layout.clearCompareButton}, {getDlgItemValue(app.window, idCancel), app.layout.cancelButton}, {app.queue, app.layout.queue}, {app.output, app.layout.result}, {app.progressBar, app.layout.progress}, {app.status, app.layout.status}, {app.queueInfo, app.layout.queueInfo}, {app.algorithmSummary, app.layout.algorithmSummary}, {app.resultSummary, app.layout.resultSummary}, {app.compareStatus, app.layout.compareStatus}}
	for index := range app.algorithms {
		positions = append(positions, struct {
			hwnd uintptr
			r    rect
		}{getDlgItemValue(app.window, idAlgorithmStart+index), app.algorithmRect(index)})
	}
	for _, position := range positions {
		if position.hwnd != 0 {
			moveWindow.Call(position.hwnd, uintptr(position.r.left), uintptr(position.r.top), uintptr(position.r.right-position.r.left), uintptr(position.r.bottom-position.r.top), 1)
		}
	}
	invalidateRect.Call(app.window, 0, 1)
}

func getDlgItemValue(hwnd uintptr, id int) uintptr {
	value, _, _ := getDlgItem.Call(hwnd, uintptr(id))
	return value
}
func (app *App) algorithmRect(index int) rect {
	inner := scale(16, app.dpi)
	column := int32(index % 2)
	row := int32(index / 2)
	top := app.layout.algorithmPanel.top + scale(58, app.dpi) + row*scale(28, app.dpi)
	left := app.layout.algorithmPanel.left + inner + column*(app.layout.algorithmPanel.right-app.layout.algorithmPanel.left-inner*2)/2
	return rect{left, top, left + (app.layout.algorithmPanel.right-app.layout.algorithmPanel.left-inner*2)/2 - scale(8, app.dpi), top + scale(24, app.dpi)}
}

func (app *App) paint() {
	var ps paintStruct
	hdc, _, _ := beginPaint.Call(app.window, uintptr(unsafe.Pointer(&ps)))
	if hdc == 0 {
		return
	}
	var client rect
	getClientRect.Call(app.window, uintptr(unsafe.Pointer(&client)))
	fillRect.Call(hdc, uintptr(unsafe.Pointer(&client)), uintptr(app.theme.canvasBrush))
	for _, panel := range []rect{app.layout.algorithmPanel, app.layout.queuePanel, app.layout.resultPanel} {
		fillRect.Call(hdc, uintptr(unsafe.Pointer(&panel)), uintptr(app.theme.surfaceBrush))
		frameRect.Call(hdc, uintptr(unsafe.Pointer(&panel)), uintptr(app.theme.lineBrush))
	}
	accent := app.theme.raisedBrush
	if app.phase == phaseRunning || app.phase == phaseStopping {
		accent = app.theme.canvasBrush
	}
	track := app.layout.algorithmPanel
	track.right = track.left + scale(3, app.dpi)
	fillRect.Call(hdc, uintptr(unsafe.Pointer(&track)), uintptr(accent))
	track = app.layout.resultPanel
	track.right = track.left + scale(3, app.dpi)
	fillRect.Call(hdc, uintptr(unsafe.Pointer(&track)), uintptr(accent))
	drawLabel(hdc, app.layout.title, "HASH / 文件完整性", textColor, app.theme.font, 0x00000000)
	drawLabel(hdc, app.layout.subtitle, "拖放文件到工作区，选择算法后开始校验", mutedColor, app.theme.font, 0x00000000)
	drawLabel(hdc, app.layout.algorithmTitle, "ALGORITHMS", iceColor, app.theme.monoFont, 0x00000000)
	drawLabel(hdc, app.layout.queueTitle, "文件队列", textColor, app.theme.font, 0x00000000)
	drawLabel(hdc, app.layout.resultTitle, "结果 / HASH OUTPUT", iceColor, app.theme.monoFont, 0x00000000)
	endPaint.Call(app.window, uintptr(unsafe.Pointer(&ps)))
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

func (app *App) colorControl(wParam, lParam uintptr) uintptr {
	setTextColor.Call(wParam, uintptr(textColor))
	setBkColor.Call(wParam, uintptr(insetColor))
	setBkMode.Call(wParam, 1)
	return uintptr(app.theme.insetBrush)
}

func (app *App) drawItem(lParam uintptr) {
	return
}

func (app *App) controlText(hwnd uintptr) string {
	buffer := make([]uint16, 512)
	length, _, _ := getWindowText.Call(hwnd, uintptr(unsafe.Pointer(&buffer[0])), uintptr(len(buffer)))
	return syscall.UTF16ToString(buffer[:length])
}

func (app *App) handleNotify(lParam uintptr) uintptr {
	if lParam == 0 {
		return 0
	}
	return 0
}

func (app *App) handleCommand(id uint32) {
	switch {
	case id == idAdd:
		app.addFilesFromDialog()
	case id == idClear:
		app.clearFiles()
	case id == idStart:
		app.startHashing()
	case id == idCancel:
		if app.cancel != nil {
			app.phase = phaseStopping
			app.updateStatus("正在停止……")
			app.cancel()
		}
	case id == idCopy:
		app.copyOutput()
	case id == idSave:
		app.saveOutput()
	case id == idPasteCompare:
		app.pasteCompare()
	case id == idClearCompare:
		app.clearComparison()
	case id >= uint32(idAlgorithmStart) && id < uint32(idAlgorithmStart+len(app.algorithms)):
		app.updateSelectionSummary()
	}
}

func (app *App) clearFiles() {
	if app.busy() {
		return
	}
	app.paths = nil
	app.rowStates = nil
	app.fileSizes = nil
	app.rowIndexByPath = nil
	app.resultsMu.Lock()
	app.run = completedRun{}
	app.resultsMu.Unlock()
	app.clearComparison()
	app.phase = phaseIdle
	app.setOutput("结果会显示在这里。\r\n选择文件和算法后，点击“开始计算”。")
	app.setResultSummary("结果摘要：尚未计算")
	app.rebuildQueue()
	app.updateStatus("就绪 · 尚未添加文件")
	app.updateStartState()
}
func (app *App) addFilesFromDialog() {
	paths, ok := selectFiles(app.window)
	if ok {
		app.addPaths(paths)
	}
}

func (app *App) addDroppedFiles(handle uintptr) {
	count, _, _ := dragQueryFile.Call(handle, 0xffffffff, 0, 0)
	paths := make([]string, 0, count)
	for index := uint32(0); index < uint32(count); index++ {
		length, _, _ := dragQueryFile.Call(handle, uintptr(index), 0, 0)
		buffer := make([]uint16, length+1)
		dragQueryFile.Call(handle, uintptr(index), uintptr(unsafe.Pointer(&buffer[0])), uintptr(length+1))
		paths = append(paths, syscall.UTF16ToString(buffer))
	}
	dragFinish.Call(handle)
	app.addPaths(paths)
}

func (app *App) addPaths(paths []string) {
	if app.busy() {
		app.updateStatus("计算中 · 请等待当前任务结束")
		return
	}
	seen := make(map[string]struct{}, len(app.paths))
	for _, path := range app.paths {
		seen[strings.ToLower(filepath.Clean(path))] = struct{}{}
	}
	rejected := 0
	added := 0
	for _, path := range paths {
		info, err := os.Stat(path)
		if err != nil || info.IsDir() || !info.Mode().IsRegular() {
			rejected++
			continue
		}
		cleaned := filepath.Clean(path)
		key := strings.ToLower(cleaned)
		if _, exists := seen[key]; exists {
			continue
		}
		seen[key] = struct{}{}
		app.paths = append(app.paths, cleaned)
		app.rowStates = append(app.rowStates, "待计算")
		app.fileSizes = append(app.fileSizes, info.Size())
		added++
	}
	if added > 0 {
		app.resultsMu.Lock()
		app.run = completedRun{}
		app.resultsMu.Unlock()
		app.phase = phaseReady
		app.setOutput("结果会显示在这里。\r\n选择文件和算法后，点击“开始计算”。")
		app.clearComparison()
		app.setResultSummary("结果摘要：尚未计算")
	}
	app.rebuildQueue()
	app.updateStartState()
	if rejected > 0 {
		app.updateStatus(fmt.Sprintf("已添加 %d 个文件 · 忽略 %d 项", len(app.paths), rejected))
	} else {
		app.updateStatus(fmt.Sprintf("就绪 · %d 个文件", len(app.paths)))
	}
}

func (app *App) startHashing() {
	if app.busy() || len(app.paths) == 0 {
		app.updateStartState()
		return
	}
	algorithms := app.selectedAlgorithms()
	if len(algorithms) == 0 {
		app.updateStatus("请至少选择一个算法")
		return
	}
	requests := make([]engine.FileRequest, len(app.paths))
	for index, path := range app.paths {
		requests[index] = engine.FileRequest{Path: path, Algorithms: append([]string(nil), algorithms...)}
		app.rowStates[index] = "待计算"
		app.updateQueueRow(index)
	}
	ctx, cancel := context.WithCancel(context.Background())
	done := make(chan struct{})
	app.cancel = cancel
	app.algorithmsInUse = append([]string(nil), algorithms...)
	app.workerDone = done
	app.closing.Store(false)
	app.progressMu.Lock()
	app.progress = engine.Progress{}
	app.progressDirty.Store(false)
	app.progressMu.Unlock()
	app.clearComparison()
	app.phase = phaseRunning
	app.setBusy(true)
	app.setOutput("")
	app.setResultSummary("结果摘要：正在计算……")
	app.updateStatus(fmt.Sprintf("计算中 · %d 个文件", len(app.paths)))
	app.setProgress(0)
	if timer, _, _ := setTimer.Call(app.window, progressTimerID, progressTimerMS, 0); timer == 0 {
		app.workerDone = nil
		app.cancel = nil
		app.phase = phaseError
		app.setBusy(false)
		app.updateStatus("无法启动进度计时器")
		cancel()
		return
	}
	go app.runHashing(ctx, done, requests, algorithms)
}

func (app *App) runHashing(ctx context.Context, done chan struct{}, requests []engine.FileRequest, algorithms []string) {
	defer close(done)
	results := engine.HashFiles(ctx, requests, engine.BatchOptions{Progress: func(progress engine.Progress) {
		app.progressMu.Lock()
		app.progress = progress
		app.progressDirty.Store(true)
		app.progressMu.Unlock()
	}})
	run := buildCompletedRun(results, algorithms)
	app.resultsMu.Lock()
	app.run = run
	app.resultsMu.Unlock()
}

func buildCompletedRun(results []engine.FileResult, algorithms []string) completedRun {
	run := completedRun{results: results, expectedRecords: compare.RecordsFromResults(results)}
	for _, result := range results {
		if result.Err != nil {
			if errors.Is(result.Err, context.Canceled) {
				run.canceled = true
			} else {
				run.failures++
			}
			continue
		}
		if result.Result.Changed {
			run.changed++
		}
	}
	var builder strings.Builder
	if err := cli.WriteResults(&builder, results, algorithms, "text", true, true, false); err != nil {
		run.formatErr = fmt.Errorf("format results: %w", err)
		return run
	}
	run.rawOutput = builder.String()
	return run
}

func (app *App) selectedAlgorithms() []string {
	selected := make([]string, 0, len(app.algorithms))
	for index, algorithm := range app.algorithms {
		state, _, _ := isChecked.Call(app.window, uintptr(idAlgorithmStart+index))
		if state == 1 {
			selected = append(selected, algorithm.Name)
		}
	}
	return selected
}
func (app *App) updateSelectionSummary() {
	app.algorithmSummaryText(fmt.Sprintf("已选择 %d 个算法", len(app.selectedAlgorithms())))
	app.updateStartState()
}
func (app *App) algorithmSummaryText(text string) { setWindowTextText(app.algorithmSummary, text) }
func (app *App) updateStartState() {
	start := getDlgItemValue(app.window, idStart)
	enableWindow.Call(start, boolToUintptr(!app.busy() && len(app.paths) > 0 && len(app.selectedAlgorithms()) > 0))
	hasOutput := app.phase == phaseComplete || app.phase == phaseError || app.phase == phaseCanceled
	enableWindow.Call(getDlgItemValue(app.window, idCopy), boolToUintptr(hasOutput && app.rawOutput != ""))
	enableWindow.Call(getDlgItemValue(app.window, idSave), boolToUintptr(hasOutput && app.rawOutput != ""))
	enableWindow.Call(getDlgItemValue(app.window, idPasteCompare), boolToUintptr(hasOutput && len(app.completedRecords()) > 0))
	enableWindow.Call(getDlgItemValue(app.window, idClearCompare), boolToUintptr(hasOutput && app.comparisonText != ""))
}
func (app *App) busy() bool { return app.phase == phaseRunning || app.phase == phaseStopping }
func (app *App) setBusy(busy bool) {
	for _, id := range []int{idAdd, idClear, idStart, idCopy, idSave, idPasteCompare, idClearCompare} {
		enableWindow.Call(getDlgItemValue(app.window, id), boolToUintptr(!busy))
	}
	for index := range app.algorithms {
		enableWindow.Call(getDlgItemValue(app.window, idAlgorithmStart+index), boolToUintptr(!busy))
	}
	enableWindow.Call(getDlgItemValue(app.window, idCancel), boolToUintptr(busy))
	if !busy {
		app.updateStartState()
	}
}

func (app *App) processTimer() {
	app.showProgress()
	done := app.workerDone
	if done == nil {
		return
	}
	select {
	case <-done:
		app.finishHashing(done)
	default:
	}
}

func (app *App) showProgress() {
	app.progressMu.Lock()
	if !app.progressDirty.Swap(false) {
		app.progressMu.Unlock()
		return
	}
	progress := app.progress
	app.progressMu.Unlock()
	percent := progressPercent(progress)
	app.setProgress(percent)
	app.updateQueueProgress(progress)
	app.updateStatus(fmt.Sprintf("计算中 · %d%% · %s", percent, shortenPath(progress.CurrentPath, 62)))
}

func (app *App) finishHashing(done chan struct{}) {
	if done == nil || done != app.workerDone {
		return
	}
	killTimer.Call(app.window, progressTimerID)
	app.workerDone = nil
	app.cancel = nil
	app.resultsMu.RLock()
	run := app.run
	app.resultsMu.RUnlock()
	if run.formatErr != nil {
		app.phase = phaseError
		app.setBusy(false)
		app.setResultSummary("结果摘要：生成结果失败")
		app.updateStatus("结果生成失败：" + run.formatErr.Error())
		if app.closing.Load() {
			destroyWindow.Call(app.window)
		}
		return
	}
	for index, result := range run.results {
		state := "已完成"
		if result.Err != nil {
			if errors.Is(result.Err, context.Canceled) {
				state = "已取消"
			} else {
				state = "错误"
			}
		} else if result.Result.Changed {
			state = "已完成 · 已变化"
		}
		if index < len(app.rowStates) {
			app.rowStates[index] = state
		}
		if index < len(app.fileSizes) && result.Err == nil {
			app.fileSizes[index] = result.Result.Size
		}
		app.updateQueueRow(index)
	}
	app.setOutput(run.rawOutput)
	app.setResultSummary(fmt.Sprintf("结果摘要：%d 个文件 · %d 个算法 · %d 个错误 · %d 个计算期间变化", len(run.results), len(app.algorithmsInUse), run.failures, run.changed))
	if run.canceled {
		app.phase = phaseCanceled
	} else if run.failures > 0 || run.changed > 0 {
		app.phase = phaseError
	} else {
		app.phase = phaseComplete
	}
	app.setBusy(false)
	if run.canceled {
		app.updateStatus("已取消 · 已保留已完成结果")
	} else if run.failures > 0 {
		app.updateStatus(fmt.Sprintf("完成 · %d 个文件失败", run.failures))
	} else if run.changed > 0 {
		app.updateStatus(fmt.Sprintf("完成 · %d 个文件在计算期间发生变化", run.changed))
	} else {
		app.updateStatus(fmt.Sprintf("已完成 · %d 个文件", len(run.results)))
	}
	if !run.canceled {
		app.setProgress(100)
	}
	app.updateStartState()
	if app.closing.Load() {
		destroyWindow.Call(app.window)
	}
}

func (app *App) updateQueueProgress(progress engine.Progress) {
	index, ok := app.rowIndexByPath[queuePathKey(progress.CurrentPath)]
	if !ok || index >= len(app.rowStates) || app.rowStates[index] == "计算中" {
		return
	}
	app.rowStates[index] = "计算中"
	app.updateQueueRow(index)
}
func (app *App) setProgress(percent int) {
	if percent < 0 {
		percent = 0
	}
	if percent > 100 {
		percent = 100
	}
	sendMessage.Call(app.progressBar, pbmSetPos, uintptr(percent), 0)
}
func (app *App) updateStatus(text string) {
	setWindowTextText(app.status, text)
	invalidateRect.Call(app.window, 0, 1)
}
func setWindowTextText(hwnd uintptr, text string) {
	if hwnd == 0 {
		return
	}
	ptr, _ := syscall.UTF16PtrFromString(text)
	setWindowText.Call(hwnd, uintptr(unsafe.Pointer(ptr)))
}

func (app *App) rebuildQueue() {
	if app.queue == 0 {
		return
	}
	app.rowIndexByPath = make(map[string]int, len(app.paths))
	sendMessage.Call(app.queue, lvmDeleteAllItems, 0, 0)
	for index, path := range app.paths {
		app.rowIndexByPath[queuePathKey(path)] = index
		filename, _ := syscall.UTF16PtrFromString(path)
		item := lvItem{mask: lvifText, item: int32(index), text: filename}
		sendMessage.Call(app.queue, lvmInsertItem, 0, uintptr(unsafe.Pointer(&item)))
		app.updateQueueRow(index)
	}
	if len(app.paths) == 0 {
		setWindowTextText(app.queueInfo, "拖放文件到这里，或点击添加文件")
	} else {
		setWindowTextText(app.queueInfo, fmt.Sprintf("%d 个文件 · 单次读取，多算法并行", len(app.paths)))
	}
}

func (app *App) updateQueueRow(index int) {
	if app.queue == 0 || index < 0 || index >= len(app.paths) {
		return
	}
	size := int64(0)
	if index < len(app.fileSizes) {
		size = app.fileSizes[index]
	}
	state := "待计算"
	if index < len(app.rowStates) {
		state = app.rowStates[index]
	}
	app.setListText(index, 1, formatSize(size))
	app.setListText(index, 2, state)
}

func queuePathKey(path string) string {
	return strings.ToLower(filepath.Clean(path))
}

func (app *App) setListText(item, subItem int, text string) {
	ptr, _ := syscall.UTF16PtrFromString(text)
	value := lvItem{mask: lvifText, item: int32(item), subItem: int32(subItem), text: ptr}
	sendMessage.Call(app.queue, lvmSetItemText, uintptr(item), uintptr(unsafe.Pointer(&value)))
}
func formatSize(value int64) string {
	units := []string{"B", "KB", "MB", "GB", "TB"}
	size := float64(value)
	unit := 0
	for size >= 1024 && unit < len(units)-1 {
		size /= 1024
		unit++
	}
	if unit == 0 {
		return fmt.Sprintf("%d %s", value, units[unit])
	}
	return fmt.Sprintf("%.1f %s", size, units[unit])
}
func shortenPath(path string, limit int) string {
	if len(path) <= limit {
		return path
	}
	if limit < 8 {
		return path[:limit]
	}
	return path[:limit/2-2] + "…" + path[len(path)-limit/2+1:]
}

func (app *App) setOutput(value string) {
	app.rawOutput = value
	setWindowTextText(app.output, value)
}

func (app *App) setResultSummary(text string) {
	setWindowTextText(app.resultSummary, text)
}

func (app *App) completedRecords() []compare.Record {
	app.resultsMu.RLock()
	defer app.resultsMu.RUnlock()
	return append([]compare.Record(nil), app.run.expectedRecords...)
}

func (app *App) clearComparison() {
	app.comparisonText = ""
	setWindowTextText(app.compareStatus, "剪贴板比较：未比较")
	app.updateStartState()
}

func (app *App) pasteCompare() {
	expected := app.completedRecords()
	if len(expected) == 0 {
		app.updateStatus("没有可用于比较的完成结果")
		return
	}
	text, err := readClipboardText(app.window)
	if err != nil {
		app.comparisonText = "比较失败：" + err.Error()
		setWindowTextText(app.compareStatus, "剪贴板比较："+app.comparisonText)
		app.updateStartState()
		return
	}
	actual, err := compare.ParseText(text)
	if err != nil {
		app.comparisonText = "比较失败：" + err.Error()
		setWindowTextText(app.compareStatus, "剪贴板比较："+app.comparisonText)
		app.updateStartState()
		return
	}
	summary := compare.Compare(expected, actual)
	app.comparisonText = formatComparisonSummary(summary)
	setWindowTextText(app.compareStatus, "剪贴板比较："+app.comparisonText)
	app.updateStatus("已完成剪贴板比较")
	app.updateStartState()
}

func formatComparisonSummary(summary compare.Summary) string {
	return fmt.Sprintf("匹配 %d · 不匹配 %d · 缺少 %d · 多余 %d · 重复 %d", summary.Matches, summary.Mismatches, summary.Missing, summary.Unexpected, summary.Duplicates)
}

func readClipboardText(owner uintptr) (string, error) {
	if result, _, _ := openClipboard.Call(owner); result == 0 {
		return "", fmt.Errorf("无法打开剪贴板")
	}
	defer closeClipboard.Call()
	memory, _, _ := getClipboardData.Call(cfUnicodeText)
	if memory == 0 {
		return "", fmt.Errorf("剪贴板不包含 Unicode 文本")
	}
	size, _, _ := globalSize.Call(memory)
	if size == 0 || size%2 != 0 {
		return "", fmt.Errorf("剪贴板文本格式无效")
	}
	if size > maxClipboardSize {
		return "", fmt.Errorf("剪贴板文本超过 %d MiB 限制", maxClipboardSize>>20)
	}
	locked, _, _ := globalLock.Call(memory)
	if locked == 0 {
		return "", fmt.Errorf("无法读取剪贴板文本")
	}
	units := int(size / 2)
	if units == 0 {
		return "", fmt.Errorf("剪贴板文本为空")
	}
	copied := make([]uint16, units)
	rtlMoveMemory.Call(uintptr(unsafe.Pointer(&copied[0])), locked, size)
	globalUnlock.Call(memory)
	for index, unit := range copied {
		if unit == 0 {
			copied = copied[:index]
			break
		}
	}
	return syscall.UTF16ToString(copied), nil
}

func (app *App) copyOutput() {
	text := app.rawOutput
	if text == "" {
		return
	}
	data := append([]uint16(nil), syscall.StringToUTF16(text)...)
	if len(data) == 0 || uintptr(len(data)) > maxClipboardSize/2 || uintptr(len(data)) > ^uintptr(0)/2 {
		app.updateStatus(fmt.Sprintf("结果超过 %d MiB 限制，无法复制", maxClipboardSize>>20))
		return
	}
	dataSize := uintptr(len(data)) * 2
	memory, _, _ := globalAlloc.Call(gmemMoveable|gmemZeroInit, dataSize)
	if memory == 0 {
		app.updateStatus("无法分配剪贴板内存")
		return
	}
	locked, _, _ := globalLock.Call(memory)
	if locked == 0 {
		globalFree.Call(memory)
		app.updateStatus("无法锁定剪贴板内存")
		return
	}
	rtlMoveMemory.Call(locked, uintptr(unsafe.Pointer(&data[0])), dataSize)
	globalUnlock.Call(memory)
	if result, _, _ := openClipboard.Call(app.window); result == 0 {
		globalFree.Call(memory)
		app.updateStatus("无法打开剪贴板")
		return
	}
	defer closeClipboard.Call()
	if result, _, _ := emptyClipboard.Call(); result == 0 {
		globalFree.Call(memory)
		app.updateStatus("无法清空剪贴板")
		return
	}
	if result, _, _ := setClipboard.Call(cfUnicodeText, memory); result == 0 {
		globalFree.Call(memory)
		app.updateStatus("无法写入剪贴板")
		return
	}
	app.updateStatus("结果已复制")
}

func (app *App) saveOutput() {
	text := app.rawOutput
	if text == "" {
		return
	}
	path, ok := saveFile(app.window)
	if !ok {
		return
	}
	if err := cli.WriteFileAtomic(path, app.paths, []byte(strings.ReplaceAll(text, "\r\n", "\n"))); err != nil {
		app.updateStatus("保存失败：" + err.Error())
		return
	}
	app.updateStatus("结果已保存")
}
func boolToUintptr(value bool) uintptr {
	if value {
		return 1
	}
	return 0
}

func selectFiles(owner uintptr) ([]string, bool) {
	buffer := make([]uint16, 32768)
	filter, _ := syscall.UTF16PtrFromString("所有文件\x00*.*\x00\x00")
	dialog := openFileName{structSize: uint32(unsafe.Sizeof(openFileName{})), owner: owner, filter: filter, file: &buffer[0], maxFile: uint32(len(buffer)), flags: ofNExplorer | ofNAllowMulti | ofNFileMustExist | ofNPathMustExist}
	if result, _, _ := getOpenFile.Call(uintptr(unsafe.Pointer(&dialog))); result == 0 {
		return nil, false
	}
	return parseMultiSelect(buffer), true
}
func parseMultiSelect(buffer []uint16) []string {
	values := make([]string, 0)
	for start := 0; start < len(buffer); {
		end := start
		for end < len(buffer) && buffer[end] != 0 {
			end++
		}
		if end == start {
			break
		}
		values = append(values, syscall.UTF16ToString(buffer[start:end]))
		start = end + 1
	}
	if len(values) <= 1 {
		return values
	}
	directory := values[0]
	paths := make([]string, 0, len(values)-1)
	for _, value := range values[1:] {
		paths = append(paths, filepath.Join(directory, value))
	}
	return paths
}
func saveFile(owner uintptr) (string, bool) {
	buffer := make([]uint16, 4096)
	filter, _ := syscall.UTF16PtrFromString("文本文件\x00*.txt\x00所有文件\x00*.*\x00\x00")
	title, _ := syscall.UTF16PtrFromString("保存哈希结果")
	dialog := openFileName{structSize: uint32(unsafe.Sizeof(openFileName{})), owner: owner, filter: filter, file: &buffer[0], maxFile: uint32(len(buffer)), title: title, flags: ofNExplorer | ofNOverwritePrompt | ofNPathMustExist}
	if result, _, _ := getSaveFile.Call(uintptr(unsafe.Pointer(&dialog))); result == 0 {
		return "", false
	}
	return syscall.UTF16ToString(buffer), true
}
