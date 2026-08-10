//go:build windows

package gui

import (
	"math/bits"

	"hash/internal/engine"
)

type uiPhase uint8

const (
	phaseIdle uiPhase = iota
	phaseReady
	phaseRunning
	phaseStopping
	phaseComplete
	phaseCanceled
	phaseError
)

func progressPercent(progress engine.Progress) int {
	percent := 0
	if progress.TotalBytes > 0 {
		completed := progress.CompletedBytes + progress.CurrentBytes
		if completed > progress.TotalBytes {
			completed = progress.TotalBytes
		}
		if completed > 0 {
			high, low := bits.Mul64(uint64(completed), 100)
			percent64, _ := bits.Div64(high, low, uint64(progress.TotalBytes))
			percent = int(percent64)
		}
	} else if progress.TotalFiles > 0 {
		percent = progress.CompletedFiles * 100 / progress.TotalFiles
	}
	if percent < 0 {
		return 0
	}
	if percent > 100 {
		return 100
	}
	return percent
}
