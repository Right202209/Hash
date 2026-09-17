//go:build windows

package gui

import (
	"context"
	"sync"
	"sync/atomic"

	"hash/internal/compare"
	"hash/internal/engine"
	"hash/internal/registry"
)

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
