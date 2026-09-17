//go:build windows

package gui

import (
	"fmt"
	"syscall"
	"unsafe"

	"hash/internal/registry"
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

func (app *App) colorControl(wParam, lParam uintptr) uintptr {
	setTextColor.Call(wParam, uintptr(textColor))
	setBkColor.Call(wParam, uintptr(insetColor))
	setBkMode.Call(wParam, 1)
	return uintptr(app.theme.insetBrush)
}

func (app *App) drawItem(lParam uintptr) {
	return
}

func (app *App) handleNotify(lParam uintptr) uintptr {
	if lParam == 0 {
		return 0
	}
	return 0
}
