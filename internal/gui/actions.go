//go:build windows

package gui

import (
	"context"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"unsafe"

	"hash/internal/atomicfile"
	"hash/internal/cli"
	"hash/internal/compare"
	"hash/internal/engine"
	"hash/internal/pathutil"
)

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
		seen[pathutil.Key(path)] = struct{}{}
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
		key := pathutil.Key(cleaned)
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
	if err := cli.WriteResults(&builder, results, algorithms, cli.FormatOptions{Format: "text", ShowSize: true, ShowModified: true}); err != nil {
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
	index, ok := app.rowIndexByPath[pathutil.Key(progress.CurrentPath)]
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

func (app *App) rebuildQueue() {
	if app.queue == 0 {
		return
	}
	app.rowIndexByPath = make(map[string]int, len(app.paths))
	sendMessage.Call(app.queue, lvmDeleteAllItems, 0, 0)
	for index, path := range app.paths {
		app.rowIndexByPath[pathutil.Key(path)] = index
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
	if err := atomicfile.Write(path, app.paths, []byte(strings.ReplaceAll(text, "\r\n", "\n"))); err != nil {
		app.updateStatus("保存失败：" + err.Error())
		return
	}
	app.updateStatus("结果已保存")
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
